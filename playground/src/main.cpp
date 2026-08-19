#include "HTTP_Server.h"
#include "Log.h"

#include <cstdint>

// Scratch driver for hstl::write_decimal. Put a breakpoint on the do/while inside it
// (HTTP_Server.h) and step through each value.
int main()
{
	const size_t values[] =
	{
		0u,                     // the do/while case - must still write one digit
		1u,
		9u,                     // last single digit before the carry
		10u,                    // first two digit value
		404u,                   // a status code
		1234567890u,
		SIZE_MAX                // 20 digits, fills MAX_DECIMAL_DIGITS exactly
	};

	for (size_t value : values)
	{
		char digits[hstl::MAX_DECIMAL_DIGITS + 1]{};

		const size_t count = hstl::write_decimal(digits, value);
		digits[count] = '\0';

		hstl::log_info("{} -> \"{}\" ({} digits)", value, hstl::Str_View{digits, count}, count);
	}

	return 0;
}
