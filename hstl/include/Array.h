#pragma once

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

	private:
		bool grow_memory(size_t size) {}
		bool shrink_memory_to_fit() {}

	private:
		int* data;
		size_t size;
		size_t capacity;
	};
};