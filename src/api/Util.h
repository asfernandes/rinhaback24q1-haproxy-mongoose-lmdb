#pragma once

#include <charconv>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <string_view>


namespace rinhaback::api
{
	static constexpr int HTTP_STATUS_OK = 200;
	static constexpr int HTTP_STATUS_NOT_FOUND = 404;
	static constexpr int HTTP_STATUS_UNPROCESSABLE_CONTENT = 422;
	static constexpr int HTTP_STATUS_INTERNAL_SERVER_ERROR = 500;

	inline std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> getCurrentDateTime()
	{
		return std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
	}

	inline int64_t getCurrentDateTimeAsInt()
	{
		return getCurrentDateTime().time_since_epoch().count();
	}

	inline std::string getCurrentDateTimeAsString()
	{
		return std::format("{:%FT%TZ}", getCurrentDateTime());
	}

	inline std::string intDateTimeToString(int64_t dateTime)
	{
		return std::format("{:%FT%TZ}",
			std::chrono::floor<std::chrono::milliseconds>(
				std::chrono::system_clock::time_point(std::chrono::milliseconds(dateTime))));
	}

	inline std::optional<int> parseInt(std::string_view str)
	{
		int val;
		const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
		return ptr == str.end() && ec == std::errc() ? std::optional{val} : std::nullopt;
	}

	inline void checkMdbError(int rc)
	{
		if (rc != 0)
		{
			std::cerr << "MDB error: " << rc << std::endl;
			throw std::runtime_error("MDB error: " + std::to_string(rc));
		}
	}
}  // namespace rinhaback::api
