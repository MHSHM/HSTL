#include <catch2/catch_test_macros.hpp>

#include <Result.h>

using namespace hstl;

inline static Result<int> divide_even_by_2(int n)
{
	if (n % 2 == 0)
	{
		return static_cast<int>(n / 2);
	}
	else
	{
		return Err{"The provided number is not even\n"};
	}
}

inline static Result<std::string> Hello_Mohamed(const std::string& name)
{
	if (name == "Mohamed")
	{
		return std::string("Hello Mohamed");
	}
	else
	{
		return Err{"The provided name is incorrect\n"};
	}
}

TEST_CASE("Result")
{
	SECTION("Success")
	{
		auto result = divide_even_by_2(10);
		auto value = result.get_value();

		REQUIRE(result);
		REQUIRE(value == 5);
	}

	SECTION("Failure")
	{
		auto result = divide_even_by_2(15);
		auto error = result.get_error();

		REQUIRE(!result);
		REQUIRE(error.size() > 0);
	}

	SECTION("Err String")
	{
		auto result = Hello_Mohamed("Ahmed");
		REQUIRE(result == false);

		auto result2 = Hello_Mohamed("Mohamed");
		REQUIRE(result2 == true);
	}
}