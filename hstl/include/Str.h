#pragma once

#include "Array.h"

namespace hstl
{
	// Non owning wrapper aound a sequence of charachters
	class Str_View
	{
	public:
		Str_View(const char* _data, size_t _count):
			_data{_data},
			_count{_count}
		{

		}

	public:
		size_t count() const
		{
			return _count;
		}

		const char* data() const
		{
			return _data;
		}

		const char& operator[](size_t index) const
		{
			if (index >= _count)
			{
				throw std::out_of_range{"Index is out of range"};
			}

			return _data[index];
		}

		size_t find(const char* substr) const
		{

		}

		// TODO: Implement useful functionality

	private:
		const char* _data;
		size_t _count;
	};

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

			memcpy(data.data, c_str, string_len + 1);
		}

		Str(char ch, size_t count)
		{
			if (count == 0)
			{
				init_empty_string();
				return;
			}

			data.resize(count + 1);

			memset(data.data, ch, sizeof(char) * count);

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

			auto count = data.count;

			std::swap(data[count - 1], data[count - 2]);

			return data[count - 2];
		}

		Str_View push(const char* str)
		{
			size_t count = strlen(str);
			size_t old_count = data.count;
			size_t needed = old_count + count;

			if (needed > data.capacity)
			{
				data.reserve(needed * 3u);
			}

			memcpy(data.data + (old_count - 1), str, sizeof(char) * count);

			data.count += count;

			data[data.count - 1] = '\0';

			return Str_View{data.data + old_count - 1u, count};
		}

		void resize(size_t count, char ch)
		{
			size_t old_count = data.count;
			size_t new_count = count + 1;

			if (count == 0u)
			{
				init_empty_string();
				return;
			}

			data.resize(new_count);

			if (new_count > old_count)
			{
				memset(data.data + (old_count - 1), ch, sizeof(char) * (new_count - old_count));
			}

			data[count] = '\0';
		}

		void reserve(size_t capacity)
		{
			data.reserve(capacity);
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
			return data.count - 1;
		}

		const char* c_str() const
		{
			return data.data;
		}

		void clear()
		{
			data.count = 1u;
			data[0] = '\0';
		}

		// Will make a read-only view of the string, any modifications to the source string
		// can cause the view to be invalid
		Str_View view()
		{
			return Str_View{data.data, data.count - 1u};
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
