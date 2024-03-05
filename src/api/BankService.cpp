#include "./BankService.h"
#include "./Config.h"
#include "./Database.h"
#include "./Util.h"
#include <array>
#include <chrono>
#include <mutex>
#include <string>
#include <string_view>
#include <cassert>
#include <cstring>
#include <boost/interprocess/sync/named_mutex.hpp>


using std::string;
using std::string_view;
namespace boostipc = boost::interprocess;


namespace rinhaback::api
{
	static boostipc::named_mutex mutex(boostipc::open_or_create, "rinhaback24q1-haproxy-mongoose-lmdb-api");
	static Connection connection;

	int BankService::postTransaction(
		PostTransactionResponse* response, int accountId, int value, string_view description)
	{
		TransactionKey key;
		key.accountId = accountId;

		TransactionData data;
		memset(&data, 0, sizeof(data));
		data.dateTime = getCurrentDateTimeAsInt();
		memcpy(data.description.begin(), description.begin(), description.length());
		data.value = value;

		std::unique_lock lock(mutex);

		MDB_txn* txn;
		checkMdbError(mdb_txn_begin(connection.env, nullptr, 0, &txn));

		MDB_cursor* cursor;
		checkMdbError(mdb_cursor_open(txn, connection.dbi, &cursor));

		MDB_val mdbReadKey(sizeof(key), &key);
		MDB_val mdbReadData;

		if (int rc = mdb_cursor_get(cursor, &mdbReadKey, &mdbReadData, MDB_SET_KEY); rc == 0)
		{
			const auto readData = static_cast<const TransactionData*>(mdbReadData.mv_data);

			const int32_t reverseSeq = ((readData->reverseSeq[0] << 24) | (readData->reverseSeq[1] << 16) |
										   (readData->reverseSeq[2] << 8) | readData->reverseSeq[3]) -
				1;

			data.reverseSeq[0] = (reverseSeq >> 24) & 0xFF;
			data.reverseSeq[1] = (reverseSeq >> 16) & 0xFF;
			data.reverseSeq[2] = (reverseSeq >> 8) & 0xFF;
			data.reverseSeq[3] = reverseSeq & 0xFF;

			data.balance = readData->balance;
			data.overdraft = readData->overdraft;
		}
		else
		{
			if (rc != MDB_NOTFOUND)
				checkMdbError(rc);

			lock.unlock();

			mdb_cursor_close(cursor);
			mdb_txn_abort(txn);

			return HTTP_STATUS_NOT_FOUND;
		}

		mdb_cursor_close(cursor);

		data.balance += value;

		if (data.balance < -data.overdraft)
		{
			lock.unlock();
			mdb_txn_abort(txn);
			return HTTP_STATUS_UNPROCESSABLE_CONTENT;
		}

		response->balance = data.balance;
		response->overdraft = data.overdraft;

		MDB_val mdbKey(sizeof(key), &key);
		MDB_val mdbData(sizeof(data), &data);
		checkMdbError(mdb_put(txn, connection.dbi, &mdbKey, &mdbData, 0));

		checkMdbError(mdb_txn_commit(txn));

		lock.unlock();

		return HTTP_STATUS_OK;
	}

	int BankService::getStatement(GetStatementResponse* response, int accountId)
	{
		const unsigned MAX_TRANSACTIONS = 10;

		TransactionKey key;
		key.accountId = accountId;

		MDB_val mdbKey(sizeof(key), &key);
		MDB_val mdbData;

		MDB_txn* txn;
		checkMdbError(mdb_txn_begin(connection.env, nullptr, MDB_RDONLY, &txn));

		MDB_cursor* cursor;
		checkMdbError(mdb_cursor_open(txn, connection.dbi, &cursor));

		if (int rc = mdb_cursor_get(cursor, &mdbKey, &mdbData, MDB_SET_KEY); rc != 0)
		{
			if (rc != MDB_NOTFOUND)
				checkMdbError(rc);

			mdb_cursor_close(cursor);
			mdb_txn_abort(txn);

			return HTTP_STATUS_NOT_FOUND;
		}

		response->balance = static_cast<const TransactionData*>(mdbData.mv_data)->balance;
		response->overdraft = static_cast<const TransactionData*>(mdbData.mv_data)->overdraft;
		response->dateTime = getCurrentDateTime();

		response->lastTransactions.reserve(MAX_TRANSACTIONS);

		do
		{
			const auto data = static_cast<const TransactionData*>(mdbData.mv_data);

			if (data->value == 0)
				break;

			response->lastTransactions.emplace_back(
				data->value, data->description, intDateTimeToChrono(data->dateTime));

			if (int rc = mdb_cursor_get(cursor, &mdbKey, &mdbData, MDB_NEXT_DUP); rc != 0)
			{
				if (rc == MDB_NOTFOUND)
					break;
				else
					checkMdbError(rc);
			}
		} while (response->lastTransactions.size() < MAX_TRANSACTIONS);

		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);

		return HTTP_STATUS_OK;
	}
}  // namespace rinhaback::api
