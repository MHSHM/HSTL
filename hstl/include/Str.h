#pragma once

#include "Array.h"

namespace hstl
{
	// Non owning wrapper aound a sequence of charachters
	class Str_View
	{
	public:
		static constexpr size_t npos = static_cast<size_t>(-1);

		Str_View(const char* _data, size_t _count):
			_data{_data},
			_count{_count}
		{

		}

		Str_View(const char* c_str):
		_data{c_str},
		_count{c_str ? std::strlen(c_str) : 0u}
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

		// assumes that c_str in a null-terminated string
		Str_View& operator=(const char* c_str)
		{
			_data = c_str;
			_count = c_str ? std::strlen(c_str) : 0u;

			return *this;
		}

		bool operator==(const Str_View& view) const
		{
			if (view.count() != _count)
			{
				return false;
			}

			int res = memcmp(_data, view.data(), sizeof(char) * _count);

			if (res == 0)
			{
				return true;
			}

			return false;
		}

		size_t find(const char* substr) const
		{
			if (substr == nullptr)
			{
				return npos;
			}

			size_t to_be_found_count = std::strlen(substr);

			if (to_be_found_count == 0u)
			{
				return 0;
			}

			if (to_be_found_count != _count)
			{
				return npos;
			}

			size_t index = 0;

			while (_count - index >= to_be_found_count)
			{
				int res = memcmp(_data + index, substr, sizeof(char) * to_be_found_count);

				if (res == 0)
				{
					return index;
				}

				++index;
			}

			return npos;
		}

		/*
			TODO:
				substr(size_t pos, size_t len = npos)
				starts_with(Str_View prefix)
				ends_with(Str_View suffix)
		*/

	private:
		const char* _data{nullptr};
		size_t _count{0u};
	};

	class Str
	{
	public:
		static constexpr size_t npos = static_cast<size_t>(-1);

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
		Str_View view() const
		{
			return Str_View{data.data, data.count - 1u};
		}

		/* TODO:
			operator+=(char)
			operator+=(const char*)
			insert(size_t pos, const char*)
			insert(size_t pos, char)
			erase(size_t pos, size_t len = npos)
		*/

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
