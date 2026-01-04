#pragma once

#include "Str.h"

#include <cstring>
#include <assert.h>
#include <source_location>

namespace hstl
{
	// ANSI Color Codes
	static constexpr const char* COLOR_RESET  = "\033[0m";
	static constexpr const char* COLOR_RED    = "\033[31m";
	static constexpr const char* COLOR_GREEN  = "\033[32m";
	static constexpr const char* COLOR_YELLOW = "\033[33m";

	struct Log_Format
	{
		const char* fmt;
		std::source_location loc;

		// Constructor allows implicit conversion from string literals (e.g. "Hello {}")
		// and captures the caller's source location automatically.
		Log_Format(const char* fmt, const std::source_location& loc = std::source_location::current())
			:fmt{fmt}, loc{loc}
		{

		}
	};

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
	void _log_impl(const char* prefix, const char* color, const char* fmt, const std::source_location& loc, Args&&... args)
	{
		// This will invoke a dynamic memory allocation
		// TODO: Use Fixed_Str when implemented instead
		Str buffer;
		buffer.reserve(1024);

		buffer.push(color);
		buffer.push(prefix);
		buffer.push(COLOR_RESET);

		buffer.push("[");
		buffer.push(get_filename(loc.file_name()));
		buffer.push(":");
		Str::append(buffer, loc.line());
		buffer.push("] ");

		Str::format(buffer, fmt, std::forward<Args>(args)...);

		buffer.push("\n");

		fwrite(buffer.c_str(), 1, buffer.count(), stdout);
	}

	template<typename... Args>
	void log_error(Log_Format log_format, Args&&... args)
	{
		_log_impl("[ERROR] ", COLOR_RED, log_format.fmt, log_format.loc, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void log_info(Log_Format log_format, Args&&... args)
	{
		_log_impl("[INFO] ", COLOR_GREEN, log_format.fmt, log_format.loc, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void log_warn(Log_Format log_format, Args&&... args)
	{
		_log_impl("[WARN] ", COLOR_YELLOW, log_format.fmt, log_format.loc, std::forward<Args>(args)...);
	}
};
