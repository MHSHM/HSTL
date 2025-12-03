#pragma once

#include "Array.h"

namespace hstl
{
	class Str
	{
	public:
		Str()
		{
			init_empty_string();
		};

		// Expects a null-terminated string
		Str(const char* c_str)
		{
			if (c_str == nullptr)
			{
				init_empty_string();
				return;
			}

			size_t string_len = strlen(c_str);

			data.resize(string_len + 1);

			memcpy(data.buffer(), c_str, string_len + 1);
		}

		Str(char ch, size_t count)
		{
			if (count == 0)
			{
				init_empty_string();
				return;
			}

			data.resize(count + 1);

			memset(data.buffer(), ch, sizeof(char) * count);

			data[count] = '\0';
		}

		Str(const Str&) = default;
		Str& operator=(const Str&) = default;
		Str(Str&&) noexcept = default;
		Str& operator=(Str&&) noexcept = default;
		~Str() = default;

	public:
		char& push(const char& ch)
		{
			data.push(ch);

			auto count = data.size();

			std::swap(data[count - 1], data[count - 2]);

			return data[count - 2];
		}

		const char* begin() const
		{
			return data.begin();
		}

		char* begin()
		{
			return data.begin();
		}

		const char* end() const
		{
			return data.end() - 1; // execlude null-termination char
		}

		char* end()
		{
			return data.end() - 1; // execlude null-termination char
		}

		size_t count() const
		{
			return data.size() - 1;
		}

	private:
		void init_empty_string()
		{
			data.resize(1u);
			data[0] = '\0';
		}

	private:
		hstl::Array<char> data;
	};
};
