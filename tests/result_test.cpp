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
}