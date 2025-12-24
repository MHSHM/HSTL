#pragma once

#include "Str.h"

#include <cstring>
#include <type_traits>
#include <stdio.h>
#include <assert.h>
#include <source_location>

namespace hstl
{
	// ANSI Color Codes
	static constexpr const char* COLOR_RESET  = "\033[0m";
	static constexpr const char* COLOR_RED    = "\033[31m";
	static constexpr const char* COLOR_GREEN  = "\033[32m";
	static constexpr const char* COLOR_YELLOW = "\033[33m";

	inline static void append(Str& buffer, Str_View view)
	{
		buffer.push_range(view.data(), view.count());
	}

	inline static void append(Str& buffer, const char* cstring)
	{
		if (cstring)
		{
			buffer.push(cstring);
		}
	}

	inline static void append(Str& buffer, const Str& str)
	{
		append(buffer, str.view());
	}

	template<typename T>
	requires std::is_integral_v<T>
	inline static void append(Str& buffer, T value)
	{
		// 1234 / 10 -> 123
		// 1234 % 10 -> 4

		if (value == 0)
		{
			buffer.push('0');
			return;
		}

		static constexpr size_t MAX_SIZE = 24u;
		char temp[MAX_SIZE]{};
		size_t end = MAX_SIZE;

		// Take the abs(value) in a safe way, i.e. don't overflow when
		// value is INT_MIN for example
		using Unsigned_T = std::make_unsigned_t<T>;
		Unsigned_T u_value = static_cast<Unsigned_T>(value);

		if (value < 0)
		{
			u_value = 0 - u_value;
		}

		while (u_value > 0)
		{
			temp[--end] = '0' + (u_value % 10);
			u_value /= 10;
		}

		if (value < 0)
		{
			temp[--end] = '-';
		}

		buffer.push_range(temp + end, MAX_SIZE - end);
	}

	constexpr const char* get_filename(const char* path)
	{
		const char* file_name = path;

		while (*path != '\0')
		{
			if (*path == '/' || *path == '\\')
			{
				file_name = path + 1u;
			}

			++path;
		}

		return file_name;
	}

	template<typename... Args>
	void _log_impl(const char* prefix, const char* color, const std::source_location& loc, const char* fmt, Args&&... args)
	{
		// This will invoke a dynamic memory allocation
		// TODO: Use Fixed_Str when implemented instead
		Str buffer;
		buffer.reserve(1024);

		if (prefix)
		{
			assert(color);

			append(buffer, color);
			append(buffer, prefix);
			append(buffer, COLOR_RESET);
		}

		append(buffer, "[");
		append(buffer, get_filename(loc.file_name()));
		append(buffer, ":");
		append(buffer, loc.line());
		append(buffer, "] ");

		const char* read_ptr = fmt;

		auto process_arg = [&buffer, &read_ptr](const auto& arg)
		{
			using T = std::decay_t<decltype(arg)>;

			static_assert(
				std::is_same_v<T, Str> ||
				std::is_same_v<T, Str_View> ||
				std::is_integral_v<T> ||
				std::is_same_v<T, const char*>,
				"hstl doesn't know how to log your type"
			);

			auto next_place_holder = strstr(read_ptr, "{}");

			if (next_place_holder)
			{
				buffer.push_range(read_ptr, next_place_holder - read_ptr);

				append(buffer, arg);

				read_ptr = next_place_holder + 2;
			}
		};

		// expands to: process_arg(arg1), process_arg(arg2), ...
		(process_arg(args), ...);

		append(buffer, read_ptr);
		append(buffer, "\n");

		fwrite(buffer.c_str(), 1, buffer.count(), stdout);
	}

	template<typename... Args>
	void log_error(const char* fmt, Args&&... args, const std::source_location& loc = std::source_location::current())
	{
		_log_impl("[ERROR] ", COLOR_RED, loc, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void log_info(const char* fmt, Args&&... args, const std::source_location& loc = std::source_location::current())
	{
		_log_impl("[INFO] ", COLOR_GREEN, loc, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void log_warn(const char* fmt, Args&&... args, const std::source_location& loc = std::source_location::current())
	{
		_log_impl("[WARN] ", COLOR_YELLOW, loc, fmt, std::forward<Args>(args)...);
	}
};
