#pragma once

#include <type_traits>
#include <exception>
#include <stdexcept>
#include <memory>
#include <algorithm>

namespace hstl
{
	class Str;

	template<typename T>
	class Array
	{
		friend class Str;

	public:
		using iterator = T*;
		using const_iterator = const T*;

		Array() = default;

		Array(size_t _count)
		{
			ensure_type_traits();

			grow_memory(_count);

			uninitialized_value_construct_range(data, _count);

			count = _count;
		}

		Array(const Array& source):
			data{static_cast<T*>(::operator new(source.capacity * sizeof(T)))},
			count{source.count},
			capacity{source.capacity}
		{
			ensure_type_traits();

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

		void shrink_to_fit()
		{
			if (capacity > count)
			{
				shrink_memory(count);
			}
		}

		void remove(size_t index)
		{
			if (index >= count)
			{
				throw std::out_of_range{"The index provided is out of range"};
			}

		    if constexpr (std::is_trivially_copyable_v<T>)
		    {
		        data[index] = data[count - 1];
		    }
		    else
		    {
		        std::destroy_at(&data[index]);

		        if (index < count - 1)
		        {
		            ::new(&data[index]) T{std::move(data[count - 1])};

		            std::destroy_at(&data[count - 1]);
		        }
		    }

		    --count;
		}

		void remove_ordered(size_t index)
		{
			if (index >= count)
			{
				throw std::out_of_range{"The index provided is out of range"};
			}

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                memmove(&data[index], &data[index + 1], sizeof(T) * (count - index - 1));
            }
            else
            {
                for (size_t i = index; i < count - 1; ++i)
                {
                    std::destroy_at(&data[i]);

                    ::new(&data[i]) T{std::move(data[i + 1])};
                }

                std::destroy_at(&data[count - 1]);
            }

			count--;
		}

		template<typename F>
		void remove_if(F f)
		{
			// NOTE: This type trait will check if the result of F is __convertable__ to bool
			// I'm not sure if this is desired but roll with it for now
			static_assert(std::is_invocable_r_v<bool, F, const T&>, "Predicate must be callable as bool(const T&)");
			if (count == 0)
				return;

			// FIXME: int64_t here is a narrowing conversion used to avoid underflow
			int64_t last_survivior = count - 1;
			for (int64_t i = count - 1; i >= 0; --i)
			{
				if (f(data[i]) == false)
					continue;

				if(last_survivior != i)
				{
					if constexpr (std::is_trivially_copyable_v<T> == true)
					{
						memcpy(&data[i], &data[last_survivior], sizeof(T));
					}
					else
					{
						if constexpr (std::is_trivially_destructible_v<T> == false)
						{
							std::destroy_at(&data[i]);
						}

						::new(&data[i]) T{std::move(data[last_survivior])};

						if constexpr (std::is_trivially_destructible_v<T> == false)
						{
							std::destroy_at(&data[last_survivior]);
						}
					}
				}
				else
				{
					if constexpr (std::is_trivially_destructible_v<T> == false)
					{
						std::destroy_at(&data[i]);
					}
				}
				last_survivior--;
			}
			count = last_survivior + 1;
		}

		const_iterator begin() const noexcept
		{
			return data;
		}

		const_iterator end() const noexcept
		{
			return data + count;
		}

		iterator begin() noexcept
		{
			return data;
		}

		iterator end() noexcept
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

		const T* buffer() const { return data; }

		const T& operator[](size_t index) const
		{
			if (index >= count)
			{
				throw std::out_of_range{"The index provided is out of range"};
			}

			return data[index];
		}

		T& operator[](size_t index)
		{
			if (index >= count)
			{
				throw std::out_of_range{"The index provided is out of range"};
			}

			return data[index];
		}

		// TODO: Rename to count
		size_t size() const { return count; }

	private:
        void ensure_type_traits()
        {
            static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");
            static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");
            static_assert(std::is_copy_assignable_v<T>, "T must have a copy assignment operator");
            static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");
            static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");
        }

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
				uninitialized_move_range(data, count, new_data);
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
				uninitialized_move_range(data, new_count, new_data);
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

        void uninitialized_move_range(T* src, size_t count, T* dst)
        {
            if constexpr (std::is_trivially_copyable_v<T> == true)
            {
                memcpy(dst, src, sizeof(T) * count);
            }
            else
            {
                std::uninitialized_move_n(src, count, dst);
            }
        }

		void uninitialized_value_construct_range(T* start, size_t count)
		{
			if constexpr (std::is_trivially_default_constructible_v<T> == true)
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
