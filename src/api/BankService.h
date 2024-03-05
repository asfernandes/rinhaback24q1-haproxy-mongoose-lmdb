#pragma once

#include "./Util.h"
#include <array>
#include <string_view>
#include <vector>
#include <cstdint>


namespace rinhaback::api
{
	class BankService final
	{
	public:
		struct Transaction
		{
			int value;
			std::array<char, 11> description;
			DateTimeMillis dateTime;
		};

		struct PostTransactionResponse
		{
			int overdraft;
			int balance;
		};

		struct GetStatementResponse
		{
			DateTimeMillis dateTime;
			int overdraft;
			int balance;
			std::vector<Transaction> lastTransactions;
		};

	public:
		int postTransaction(PostTransactionResponse* response, int accountId, int value, std::string_view description);
		int getStatement(GetStatementResponse* response, int accountId);
	};
}  // namespace rinhaback::api
