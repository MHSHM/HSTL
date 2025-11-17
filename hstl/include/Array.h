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
		Array(size_t capacity) {  };
		Array(size_t size) { };
		Array(size_t size, int value) { }
		Array(const Array& source) { }
		Array& operator=(const Array& source) { }
		Array(Array&& source) { }
		Array& operator=(Array&& source) { }
		~Array() { }

	public:
		bool reserve(size_t capacity) {}
		bool resize(size_t size) { }
		bool resize_with_value(size_t size, int value) { }
		bool push(int value) { }
		bool shrink_to_fit() { }
		bool remove(size_t index) { }
		bool remove_ordered(size_t index) { }
		void remove_if() { }
		int* begin() const { }
		int* end() const { }
		bool clear() { }

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
				memcpy(new_data, data, sizeof(int) * count /*in bytes*/);
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