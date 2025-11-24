#pragma once

#include <cstring>
#include <type_traits>
#include <exception>
#include <stdexcept>
#include <memory>
#include <algorithm>

namespace hstl
{
	template<typename T>
	class Array
	{
	public:
		Array() = default;

		Array(size_t _count)
		{
			static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");

			grow_memory(_count);

			if constexpr (std::is_trivially_default_constructible_v<T> &&
						  std::is_trivially_copyable_v<T> &&
						  std::is_trivially_destructible_v<T> &&
						  std::is_trivially_move_constructible_v<T>)
			{
				memset(data, 0, sizeof(T) * count);
			}
			else
			{
				std::uninitialized_default_construct_n(data, count);
			}

			count = _count;
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

			// NOTE: Will make a redundunt memcpy
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
		// Make sure my capacity is at least _capacity
		void reserve(size_t _capacity)
		{
			grow_memory(_capacity);
		}

		void resize(size_t new_count)
		{
			if (new_count <= count)
			{
				// TODO: Call destructors for non-primitive types
				count = new_count;
				return;
			}

			if (new_count > capacity)
			{
				grow_memory(new_count);
			}

			// TODO: Call constructors for non-primitive types
			memset(data + count, 0, sizeof(int) * (new_count - count));

			count = new_count;
		}

		void resize_with_value(size_t _count, int value)
		{
			auto old_count = count;

			resize(_count);

			// Grow
			if (old_count < count)
			{
				for (size_t i = old_count; i < count; ++i)
				{
					data[i] = value;
				}
			}
		}

		int* push(int value)
		{
			
		}

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

			T* new_data = static_cast<T*>(::operator new(sizeof(T) * _capacity));

			if (data && count > 0)
			{
				if constexpr (std::is_trivially_copyable_v<T> == true)
				{
					memcpy(new_data, data, sizeof(T) * count);
				}
				else
				{
					// FIXME: If this throws we will leak "new_data"
					std::uninitialized_copy_n(data, count, new_data);
				}
			}

			std::destroy_n(data, count);
			::operator delete(data);

			data = new_data;
			capacity = _capacity;
		}

		void shrink_memory(size_t _capacity)
		{
			if (_capacity >= capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(::operator new(sizeof(T) * _capacity));

			size_t new_count = std::min(count, _capacity);

			if (data && new_count > 0u)
			{
				if constexpr (std::is_trivially_copyable_v<T> == true)
				{
					memcpy(new_data, data, sizeof(T) * new_count);
				}
				else
				{
					// FIXME: If this throws we will leak "new_data"
					std::uninitialized_copy_n(data, new_count, new_data);
				}
			}

			std::destroy_n(data, count);
			::operator delete(data);

			data = new_data;
			capacity = _capacity;
			count = new_count;
		}

	private:
		T* data{nullptr};
		size_t count{0u};
		size_t capacity{0u};
	};
};