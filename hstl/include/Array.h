#pragma once

#include <cstring>

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
		bool grow_memory(size_t _capacity)
		{
			if (_capacity <= capacity)
			{
				return false;
			}

			if (_capacity <= size)
			{
				return false;
			}

			auto new_data = new int[_capacity];
			memcpy(new_data, data, sizeof(int) * size /*in bytes*/);
			delete[] data;

			data = new_data;
			capacity = _capacity;

			return true;
		}

		bool shrink_memory_to_fit()
		{
			if (size == capacity)
			{
				return false;
			}

			auto new_data = new int[size];
			memcpy(new_data, data, sizeof(int) * size);
			delete[] data;

			data = new_data;
			capacity = size;

			return true;
		}

	private:
		int* data{nullptr};
		size_t size{0u};
		size_t capacity{0u};
	};
};