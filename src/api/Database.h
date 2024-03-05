#pragma once

#include <array>
#include <cstdint>
#include "lmdb.h"


namespace rinhaback::api
{
	struct TransactionKey
	{
		int accountId;
	};

	struct __attribute__((packed)) TransactionData
	{
		uint8_t reverseSeq[4];
		int64_t dateTime;
		std::array<char, 11> description;
		int value;
		int balance;
		int overdraft;
	};

	class Connection final
	{
	public:
		explicit Connection();
		~Connection();

		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

	public:
		MDB_env* env;
		MDB_dbi dbi;
	};
}  // namespace rinhaback::api
