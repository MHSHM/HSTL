#pragma once

#include <cstring>
#include <assert.h>

#include "Str.h"

namespace hstl
{
	template<size_t capacity>
	class Fixed_Str
	{
		static_assert(capacity > 0u);

	private:
		char data[capacity];
		size_t _count;

	private:
		void init_empty_string()
		{
			_count = 0u;
			data[0] = '\0';
		}

	public:
		static constexpr size_t npos = static_cast<size_t>(-1);

		Fixed_Str()
		{
			init_empty_string();
		}

		Fixed_Str(const char* cstring)
		{
			if (cstring == nullptr)
			{
				init_empty_string();
				return;
			}

			size_t length = strlen(cstring);

			assert(length < capacity);

			memcpy(data, cstring, length + 1u);

			_count = length;
		}

		Fixed_Str(const Fixed_Str& source):
			_count{source._count}
		{
			memcpy(data, source.data, source._count + 1u);
		}

		Fixed_Str& operator=(const Fixed_Str& source)
		{
			if (this == &source)
			{
				return *this;
			}

			memcpy(data, source.data, source._count + 1u);

			_count = source._count;

			return *this;
		}

	public:
		char& push(char ch)
		{
			assert(_count + 1 < capacity);

			data[_count] = ch;

			_count++;

			data[_count] = '\0';

			return data[_count - 1u];
		}

		Str_View push(const char* cstring)
		{
			if (cstring == nullptr)
			{
				return Str_View{nullptr, 0u};
			}

			size_t count = strlen(cstring);
			size_t required_count = count + _count;

			assert(required_count < capacity);

			memcpy(data + _count, cstring, count);

			data[required_count] = '\0';

			char* start = data + _count;

			_count = required_count;

			return Str_View{start, count};
		}

		// The range shouldn't be null-terminated
		Str_View push_range(const char* start, size_t length)
		{
			assert(start);

			size_t required_count = length + _count;

			assert(required_count < capacity);

			memcpy(data + _count, start, sizeof(char) * length);

			data[required_count] = '\0';

			char* new_segment_start = data + _count;

			_count += length;

			return Str_View{new_segment_start, length};
		}

		Str_View push_n(char ch, size_t n)
		{
			size_t required_count = _count + n;

			assert(required_count < capacity);

			memset(data + _count, ch, n);

			data[required_count] = '\0';

			char* start = data + _count;

			_count = required_count;

			return Str_View{start, n};
		}

		// Will remove the first occurence if "all_occurences" was false
		void remove(char ch, bool all_occurences = false)
		{
			if (all_occurences)
			{
				auto read_ptr = begin();
				auto write_ptr = begin();
				auto e = end();

				while (read_ptr < e)
				{
					if (*read_ptr != ch)
					{
						*write_ptr = *read_ptr;
						++write_ptr;
					}

					++read_ptr;
				}

				*write_ptr = '\0';

				_count = (write_ptr - begin());
			}
			else
			{
				if (auto loc = view().find(ch); loc != npos)
				{
					size_t move_count = _count - loc;
					memmove(data + loc, data + loc + 1, move_count);

					_count--;
				}
			}
		}

		// Will remove the first occurence
		void remove(const char* substr)
		{
			if (substr == nullptr)
			{
				return;
			}

			auto str_view = view();
			size_t length = strlen(substr);

			if (length == 0u)
			{
				return;
			}

			size_t loc = str_view.find(substr);

			if (loc != npos)
			{
				size_t num_bytes_to_move = (_count - (loc + length)) + 1u;
				memmove(data + loc, data + (loc + length), num_bytes_to_move);

				_count -= length;
			}
		}

		Str_View insert(const char* substr, size_t pos)
		{
			assert(substr);
			assert(pos <= _count);

			auto str_length = strlen(substr);
			auto required_size = _count + str_length;

			assert(required_size < capacity);

			// make way
			memmove(data + pos + str_length, data + pos, (_count - pos) + 1u);

			// put the new substr
			memcpy(data + pos, substr, sizeof(char) * str_length);

			_count += str_length;

			return Str_View{data + pos, str_length};
		}

		const char* begin() const
		{
			return data;
		}

		char* begin()
		{
			return data;
		}

		const char* end() const
		{
			return data + _count; // exclude null-termination char
		}

		char* end()
		{
			return data + _count; // exclude null-termination char
		}

		size_t count() const
		{
			return _count;
		}

		const char* c_str() const
		{
			return data;
		}

		void clear()
		{
			init_empty_string();
		}

		bool empty() const
		{
			return _count == 0u;
		}

		// Will make a read-only view of the string execluding the null-terminator
		// any modifications to the source string can cause the view to be invalid
		const Str_View view() const
		{
			return Str_View{data, _count};
		}

		size_t find(const char* substr) const
		{
			return view().find(substr);
		}

		bool starts_with(const char* prefix) const
		{
			return view().starts_with(prefix);
		}

		bool starts_with(const Str_View& prefix) const
		{
			return view().starts_with(prefix);
		}

		bool ends_with(const char* suffix) const
		{
			return view().ends_with(suffix);
		}

		bool ends_with(const Str_View& suffix) const
		{
			return view().ends_with(suffix);
		}

		Fixed_Str& operator+=(char ch)
		{
			push(ch);
			return *this;
		}

		Fixed_Str& operator+=(const char* str)
		{
			push(str);
			return *this;
		}

		char& operator[](size_t index)
		{
			assert(empty() == false);
			assert(index < _count + 1u);

			return data[index];
		}

		const char& operator[](size_t index) const
		{
			assert(empty() == false);
			assert(index < _count + 1u);

			return data[index];
		}
	};
};
