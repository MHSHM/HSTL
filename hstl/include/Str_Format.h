#pragma once

#include "Str.h"
#include "Fixed_Str.h"
#include "Memory.h"

#include <type_traits>

namespace hstl
{
	template<typename Buffer>
	void append(Buffer& buffer, char ch)
	{
		buffer.push(ch);
	}

	template<typename Buffer>
	void append(Buffer& buffer, const char* str)
	{
		if (str)
		{
			buffer.push(str);
		}
	}

	template<typename Buffer>
	void append(Buffer& buffer, Str_View view)
	{
		buffer.push_range(view.data(), view.count());
	}

	template<typename Buffer>
	void append(Buffer& buffer, const Str& str)
	{
		buffer.push_range(str.c_str(), str.count());
	}

	template<typename Buffer, typename T>
	requires std::is_integral_v<T>
	void append(Buffer& buffer, T value)
	{
		if (value == 0)
		{
			buffer.push('0');
			return;
		}

		static constexpr size_t MAX_SIZE = 24u;
		char temp[MAX_SIZE]{};
		size_t end = MAX_SIZE;

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

	template<typename Buffer, typename... Args>
	void fmt(Buffer& buffer, const char* fmt, Args&&... args)
	{
		const char* read_ptr = fmt;

		auto process_arg = [&](const auto& arg)
		{
				using T = std::decay_t<decltype(arg)>;

				static_assert(
					std::is_same_v<T, Str> ||
					std::is_same_v<T, Str_View> ||
					std::is_integral_v<T> ||
					std::is_same_v<T, const char*>,
					"hstl doesn't know how to handle your type"
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

		if (read_ptr)
		{
			buffer.push(read_ptr);
		}
	}

	template<typename... Args>
	static Str fmt_str(Allocator* allocator, const char* fmt, Args&&... args)
	{
		Str buffer(allocator);
		buffer.reserve(1024);

		format(buffer, fmt, std::forward<Args>(args)...);

		return buffer;
	}

	template<typename... Args>
	static Str fmt_str(const char* fmt, Args&&... args)
	{
		Str buffer(Default_Allocator::get());
		buffer.reserve(1024);

		format(buffer, fmt, std::forward<Args>(args)...);

		return buffer;
	}

	template<size_t N, typename... Args>
	static Fixed_Str<N> fmt_fixed_str(const char* fmt, Args&&... args)
	{
		Fixed_Str<N> buffer;

		format(buffer, fmt, std::forward<Args>(args)...);

		return buffer;
	}
}
