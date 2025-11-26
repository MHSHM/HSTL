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
			grow_memory(_count);

			uninitialized_value_construct_range(data, count);

			count = _count;
		}

		Array(const Array& source):
			data{static_cast<T*>(::operator new(source.capacity * sizeof(T)))},
			count{source.count},
			capacity{source.capacity}
		{
			if (source.count > 0u)
			{
				// FIXME: If this throws we will leak "data"
				uninitialized_copy_range(source.data, source.count, data);
			}
		}

		Array& operator=(const Array& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (capacity < source.count)
			{
				grow_memory(source.count, true);
			}
			else
			{
				if constexpr (std::is_trivially_destructible_v<T> == false)
				{
					std::destroy_n(data, count);
				}

				count = 0;
			}

			if (source.count > 0)
			{
				// FIXME: If this throws we will leak "data"
				uninitialized_copy_range(source.data, source.count, data);
			}

			count = source.count;
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

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				std::destroy_n(data, count);
			}
			::operator delete(data);

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
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				std::destroy_n(data, count);
			}
			::operator delete(data);
		}

	public:
		// Make sure my capacity is at least _capacity
		void reserve(size_t _capacity)
		{
			grow_memory(_capacity);
		}

		void resize(size_t new_count)
		{
			if (new_count == count)
			{
				return;
			}

			if (new_count > capacity)
			{
				grow_memory(new_count);
			}

			if (new_count > count)
			{
				uninitialized_value_construct_range(data + count, new_count - count);
			}
			else
			{
				if constexpr (std::is_trivially_destructible_v<T> == false)
				{
					std::destroy_n(data + new_count, count - new_count);
				}
			}

			count = new_count;
		}

		T& push(const T& element)
		{
			if (count == capacity)
			{
				grow_memory(capacity == 0u ? 10u : capacity * 2u);
			}

			new(&data[count++]) T(element);

			return data[count - 1];
		}

		T& push(T&& element)
		{
			if (count == capacity)
			{
				grow_memory(capacity == 0u ? 10u : capacity * 2u);
			}

			new(&data[count++]) T(std::move(element));

			return data[count - 1];
		}

		template<typename... Args>
		T& emplace(Args&&... args)
		{
			if (count == capacity)
			{
				grow_memory(capacity == 0u ? 10u : capacity * 2u);
			}

			new (&data[count++]) T(std::forward<Args>(args)...);

			return data[count - 1];
		}

		bool shrink_to_fit() { }
		bool remove(size_t index) { }
		bool remove_ordered(size_t index) { }
		void remove_if() { }

		T* begin() const noexcept
		{
			return data;
		}

		T* end() const noexcept
		{
			return data + count;
		}

		void clear() noexcept
		{
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				std::destroy_n(data, count);
			}

			count = 0;
		}

	private:
		void grow_memory(size_t _capacity, bool discard_old_data = false)
		{
			if (_capacity <= capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(::operator new(sizeof(T) * _capacity));

			if (data && count > 0 && discard_old_data == false)
			{
				// FIXME: If this throws we will leak "new_data"
				uninitialized_copy_range(data, count, new_data);
			}

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				std::destroy_n(data, count);
			}
			::operator delete(data);

			data = new_data;
			capacity = _capacity;

			if (discard_old_data == true)
			{
				count = 0u;
			}
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
				// FIXME: If this throws we will leak "new_data"
				uninitialized_copy_range(data, new_count, new_data);
			}

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				std::destroy_n(data, count);
			}

			::operator delete(data);

			data = new_data;
			capacity = _capacity;
			count = new_count;
		}

		void uninitialized_copy_range(T* src, size_t count, T* dst)
		{
			if constexpr (std::is_trivially_copyable_v<T> == true)
			{
				memcpy(dst, src, sizeof(T) * count);
			}
			else
			{
				std::uninitialized_copy_n(src, count, dst);
			}
		}

		void uninitialized_value_construct_range(T* start, size_t count)
		{
			if constexpr (std::is_fundamental_v<T> == true)
			{
				memset(start, 0, sizeof(T) * count);
			}
			else
			{
				std::uninitialized_value_construct_n(start, count);
			}
		}

	private:
		T* data{nullptr};
		size_t count{0u};
		size_t capacity{0u};
	};
};