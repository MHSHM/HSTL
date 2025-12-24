#pragma once

#include "Array.h"

#include <assert.h>

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
			assert(_data);
		}

		Str_View(const char* c_str):
		_data{c_str},
		_count{0u}
		{
			assert(_data);

			_count = strlen(c_str);
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

		// Will return the first occurence of the provided character
		size_t find(char ch) const
		{
			const void* location = memchr(_data, static_cast<unsigned char>(ch), _count);

			if (location == nullptr)
			{
				return npos;
			}

			return static_cast<size_t>(static_cast<const char*>(location) - _data);
		}

		// Will return the first occurence of the provided substr maybe we should have find_last
		// TODO: Implement a better matching algo Current complexity is O(N * M)
		// where N is the size of the string and M is the size of the view
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

			if (to_be_found_count > _count)
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

		Str_View substr(size_t pos, size_t length) const
		{
			assert(pos <= _count);

			size_t remaining = _count - pos;

			assert(length <= remaining);

			return {_data + pos, length};
		}

		// Expects a null-terminated string
		bool starts_with(const char* prefix) const
		{
			if (prefix == nullptr)
			{
				return false;
			}

			auto length = strlen(prefix);

			if (length > _count)
			{
				return false;
			}

			return memcmp(_data, prefix, sizeof(char) * length) == 0;
		}

		bool starts_with(const Str_View& prefix) const
		{
			if (prefix.data() == nullptr)
			{
				return false;
			}

			auto length = prefix.count();

			if (length > _count)
			{
				return false;
			}

			return memcmp(_data, prefix.data(), sizeof(char) * length) == 0;
		}

		// Expects a null-terminated string
		bool ends_with(const char* suffix) const
		{
			if (suffix == nullptr)
			{
				return false;
			}

			auto length = strlen(suffix);

			if (length > _count)
			{
				return false;
			}

			return memcmp(_data + (_count - length), suffix, sizeof(char) * length) == 0;
		}

		bool ends_with(const Str_View& suffix) const
		{
			if (suffix.data() == nullptr)
			{
				return false;
			}

			auto length = suffix.count();

			if (length > _count)
			{
				return false;
			}

			return memcmp(_data + (_count - length), suffix.data(), sizeof(char) * length) == 0;
		}

		// The splits are alive as long as the view is valid
		Array<Str_View> split(char delimiter) const
		{
			Array<Str_View> splits;

			const char* split_end = data();
			const char* split_start = data();
			const char* end = data() + count();

			while (split_end < end)
			{
				if (*split_end == delimiter) // That's a full split
				{
					splits.push(Str_View{split_start, static_cast<size_t>(split_end - split_start)});

					split_start = split_end + 1u;
				}

				++split_end;
			}

			splits.push(Str_View{split_start, static_cast<size_t>(split_end - split_start)});

			return splits;
		}

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
		char& push(char ch)
		{
			data.push(ch);

			auto count = data.count;

			std::swap(data[count - 1], data[count - 2]);

			return data[count - 2];
		}

		Str_View push(const char* str)
		{
			if (str == nullptr)
			{
				return Str_View{nullptr, 0};
			}

			size_t count = strlen(str);
			size_t old_count = data.count;
			size_t required_count = old_count + count;

			if (required_count > data._capacity)
			{
				data.reserve(required_count * 2u);
			}

			memcpy(data.data + (old_count - 1), str, sizeof(char) * count);

			data.count += count;

			data[data.count - 1] = '\0';

			return Str_View{data.data + old_count - 1u, count};
		}

		Str_View push_n(char ch, size_t n)
		{
			size_t required_count = data.count + n;
			size_t old_count = data.count;

			if (required_count > data._capacity)
			{
				data.reserve(required_count * 2u);
			}

			memset(data.data + old_count - 1u, ch, sizeof(char) * n);

			data.count += n;

			data[data.count - 1] = '\0';

			return Str_View{data.data + old_count - 1u, n};
		}

		// The range shouldn't be null-terminated
		Str_View push_range(const char* start, size_t length)
		{
			assert(start);

			size_t required_count = length + data.count;
			size_t old_count = data.count;

			if (required_count > data._capacity)
			{
				data.reserve(required_count * 2u);
			}

			memcpy(data.data + old_count - 1u, start, sizeof(char) * length);

			data.count += length;

			data[data.count - 1] = '\0';

			return Str_View{data.data + old_count - 1, length};
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

		// Will make a read-only view of the string execluding the null-terminator
		// any modifications to the source string can cause the view to be invalid
		const Str_View view() const
		{
			return Str_View{data.data, data.count - 1u};
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

		Str& operator+=(char ch)
		{
			push(ch);
			return *this;
		}

		Str& operator+=(const char* str)
		{
			push(str);
			return *this;
		}

		char& operator[](size_t index)
		{
			assert(empty() == false);
			assert(index < data.count);

			return data[index];
		}

		const char& operator[](size_t index) const
		{
			assert(empty() == false);
			assert(index < data.count);

			return data[index];
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
				data.count = (write_ptr - begin()) + 1u;
			}
			else
			{
				if (auto loc = view().find(ch); loc != npos)
				{
					data.remove_ordered(loc);
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
				memmove(data.data + loc, data.data + (loc + length), data.count - (loc + length));
				data.count -= length;
			}
		}

		Str_View insert(const char* substr, size_t pos)
		{
			assert(substr);
			assert(pos <= data.count - 1);

			auto str_length = strlen(substr);
			auto required_size = data.count + str_length;

			if (required_size > data._capacity)
			{
				data.reserve(required_size);
			}

			// make way
			memmove(data.data + pos + str_length, data.data + pos, data.count - pos);

			// put the new substr
			memcpy(data.data + pos, substr, sizeof(char) * str_length);

			data.count += str_length;

			return Str_View{data.data + pos, str_length};
		}

		// Splits are valid as long as the string is valid
		Array<Str_View> split(char delimiter)
		{
			return view().split(delimiter);
		}

		bool empty() const
		{
			return data.count == 1u;
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
