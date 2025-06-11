#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>

#include <Json_Value.h>

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

	return 0;
}