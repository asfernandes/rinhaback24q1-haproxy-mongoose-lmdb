#include "./Database.h"
#include "./Config.h"
#include "./Util.h"
#include <filesystem>
#include <thread>
#include <cstring>

namespace stdfs = std::filesystem;


namespace rinhaback::api
{
	Connection::Connection()
	{
		if (Config::databaseInit)
		{
			if (stdfs::exists(Config::database))
			{
				stdfs::remove(stdfs::path(Config::database).append("data.mdb"));
				stdfs::remove(stdfs::path(Config::database).append("lock.mdb"));
			}
			else
				stdfs::create_directory(Config::database);
		}
		else
			std::this_thread::sleep_for(std::chrono::seconds(1));

		checkMdbError(mdb_env_create(&env));
		checkMdbError(mdb_env_set_mapsize(env, Config::databaseSize));
		checkMdbError(mdb_env_open(env, Config::database.c_str(),
			MDB_WRITEMAP | MDB_NOMETASYNC | MDB_NOSYNC | MDB_NOTLS | MDB_NOMEMINIT |
				(Config::databaseInit ? MDB_CREATE : 0),
			0664));

		MDB_txn* txn;
		checkMdbError(mdb_txn_begin(env, nullptr, 0, &txn));
		checkMdbError(mdb_dbi_open(txn, nullptr, MDB_DUPSORT | MDB_DUPFIXED, &dbi));

		if (Config::databaseInit)
		{
			struct InitAccount
			{
				int accountId;
				int overdraft;
			};

			static const InitAccount initAccounts[] = {
				{1, 100000},
				{2, 80000},
				{3, 1000000},
				{4, 10000000},
				{5, 500000},
			};

			TransactionKey key;

			TransactionData data;
			memset(&data, 0, sizeof(data));
			memset(data.reverseSeq, 0xFF, sizeof(data.reverseSeq));

			MDB_val mdbKey(sizeof(key), &key);
			MDB_val mdbData(sizeof(data), &data);

			for (const auto& initAccount : initAccounts)
			{
				key.accountId = initAccount.accountId;
				data.overdraft = initAccount.overdraft;
				checkMdbError(mdb_put(txn, dbi, &mdbKey, &mdbData, 0));
			}
		}

		checkMdbError(mdb_txn_commit(txn));
	}

	Connection::~Connection()
	{
		mdb_dbi_close(env, dbi);
		mdb_env_close(env);
	}
}  // namespace rinhaback::api
