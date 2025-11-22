#pragma once

#include <cstring>
#include <type_traits>
#include <exception>
#include <stdexcept>

namespace hstl
{
	class Array
	{
	public:
		Array() = default;

		Array(size_t _count):
			count{_count}
		{
			grow_memory(_count);
			memset(data, 0, sizeof(int) * count);
		}

		Array(const Array& source):
			data{new int[source.capacity]},
			count{source.count},
			capacity{source.capacity}
		{
			if (source.count > 0)
			{
				memcpy(data, source.data, sizeof(int) * count);
			}
		}

		Array& operator=(const Array& source)
		{
			if (this == &source)
			{
				return *this;
			}

			// FIXME: Will make a redundunt memcpy
			grow_memory(source.capacity);

			if (source.count > 0)
			{
				count = source.count;
				memcpy(data, source.data, sizeof(int) * source.count);
			}

			return *this;
		}

		Array(Array&& source) noexcept:
			data{source.data},
			count{source.count},
			capacity{source.capacity}
		{
			source.data = nullptr;
			source.count = 0u;
			source.capacity = 0u;
		}

		Array& operator=(Array&& source) noexcept
		{
			if (this == &source)
			{
				return *this;
			}

			delete[] data;

			data = source.data;
			count = source.count;
			capacity = source.capacity;

			source.data = nullptr;
			source.count = 0u;
			source.capacity = 0u;

			return *this;
		}

		~Array() noexcept
		{
			delete[] data;
		}

	public:
		bool reserve(size_t capacity) {}
		bool resize(size_t size) { }
		bool resize_with_value(size_t size, int value) { }
		bool push(int value) { }
		bool shrink_to_fit() { }
		bool remove(size_t index) { }
		bool remove_ordered(size_t index) { }
		void remove_if() { }
		int* begin() const noexcept { }
		int* end() const noexcept { }
		bool clear() noexcept { }

	private:
		void grow_memory(size_t _capacity)
		{
			if (_capacity <= capacity)
			{
				return;
			}

			auto new_data = new int[_capacity];
			if (data && count > 0)
			{
				memcpy(new_data, data, sizeof(int) * count);
			}
			delete[] data;

			data = new_data;
			capacity = _capacity;
		}

		void shrink_memory_to_fit()
		{
			if (count == capacity)
			{
				return;
			}

			auto new_data = new int[count];

			if (data && count > 0)
			{
				memcpy(new_data, data, sizeof(int) * count);
			}
			delete[] data;

			data = new_data;
			capacity = count;
		}

	private:
		int* data{nullptr};
		size_t count{0u};
		size_t capacity{0u};
	};
};