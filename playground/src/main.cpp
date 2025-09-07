#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>

#include <Json_Value.h>
#include <Result.h>

hstl::Result<int> calc(int i)
{
	if (i % 2 == 0)
	{
		return i;
	}

	return hstl::Err{"The provided number is not even\n"};
}

int main()
{
	hstl::Json_Value a;
	hstl::Json_Value b("test");
	hstl::Json_Value c(true);
	hstl::Json_Value d(1.0);
	hstl::Json_Value array({1.0, 2.0, "Test"});
	hstl::Json_Value object({
		std::make_pair("Name", "Mohamed"),
		std::make_pair("Age", "25")
	});

	hstl::Result<std::string_view> ress{std::string("hello")};

	auto res = calc(10);
	auto res2 = calc(11);

	if (res)
	{
		std::cout << "Even\n";
	}
	else
	{
	}

	if (res2)
	{
		std::cout << "Odd\n";
	}
	else
	{
	}

	return 0;
}