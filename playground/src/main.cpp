#include <Result.h>
#include <Str.h>
#include <Log.h>

int main()
{
	auto formatted_str = hstl::Str::format("{} {}", "Hello", "World");
	auto formatted_err = hstl::Err("Failed to open {}", "bunny.obj");

	hstl::log_info("{}", formatted_str);
	hstl::log_error("{}", formatted_err.get_message());

	return 0;
}
