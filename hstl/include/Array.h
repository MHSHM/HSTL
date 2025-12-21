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
			static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");

			grow_memory(_count);

			uninitialized_value_construct_range(data, _count);

			count = _count;
		}

		Array(const Array& source):
			data{static_cast<T*>(::operator new(source._capacity * sizeof(T)))},
			count{source.count},
			_capacity{source._capacity}
		{
			static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");

			if (source.count > 0u)
			{
				// FIXME: If this throws we will leak "data"
				uninitialized_copy_range(source.data, source.count, data);
			}
		}

		Array& operator=(const Array& source)
		{
			static_assert(std::is_copy_assignable_v<T>, "T must have a copy assignment operator");

			if (this == &source)
			{
				return *this;
			}

			if (_capacity < source.count)
			{
				grow_memory(source.count, true);
			}
			else
			{
				std::destroy_n(data, count);

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
			_capacity{source._capacity}
		{
			source.data = nullptr;
			source.count = 0u;
			source._capacity = 0u;
		}

		Array& operator=(Array&& source) noexcept
		{
			if (this == &source)
			{
				return *this;
			}

			std::destroy_n(data, count);
			::operator delete(data);

			data = source.data;
			count = source.count;
			_capacity = source._capacity;

			source.data = nullptr;
			source.count = 0u;
			source._capacity = 0u;

			return *this;
		}

		~Array() noexcept
		{
			std::destroy_n(data, count);
			::operator delete(data);
		}

	public:
		void reserve(size_t _cap, bool discard_old_data = false)
		{
			grow_memory(_cap, discard_old_data);
		}

		void resize(size_t new_count)
		{
			static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");

			if (new_count == count)
			{
				return;
			}

			if (new_count > _capacity)
			{
				grow_memory(new_count);
			}

			if (new_count > count)
			{
				uninitialized_value_construct_range(data + count, new_count - count);
			}
			else
			{
				std::destroy_n(data + new_count, count - new_count);
			}

			count = new_count;
		}

		T& push(const T& element)
		{
			static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");

			if (count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new(&data[count++]) T(element);

			return data[count - 1];
		}

		T& push(T&& element)
		{
			static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

			if (count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new(&data[count++]) T(std::move(element));

			return data[count - 1];
		}

		template<typename... Args>
		T& emplace(Args&&... args)
		{
			static_assert(std::is_constructible_v<T, Args...>, "T doesn't have a constructor that matches the provided arguments");

			if (count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new (&data[count++]) T(std::forward<Args>(args)...);

			return data[count - 1];
		}

		void shrink_to_fit()
		{
			if (_capacity > count)
			{
				shrink_memory(count);
			}
		}

		void remove(size_t index)
		{
		    if constexpr (std::is_scalar_v<T>)
		    {
		    	memcpy(&data[index], &data[count - 1], sizeof(T));
		    }
		    else
		    {
		        if (index < count - 1)
		        {
					static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");

		        	data[index] = std::move(data[count - 1]);
		        }

		       	std::destroy_at(&data[count - 1]);
		    }

		    --count;
		}

		void remove_ordered(size_t index)
		{
            if constexpr (std::is_scalar_v<T>)
            {
                memmove(&data[index], &data[index + 1], sizeof(T) * (count - index - 1));
            }
            else
            {
                for (size_t i = index; i < count - 1; ++i)
                {
					static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");

                	data[i] = std::move(data[i + 1]);
                }

                std::destroy_at(&data[count - 1]);
            }

			count--;
		}

		template<typename F>
		void remove_if(F f)
		{
			// TODO: Rework this function and decide whether it should follow the standard and maintain order or not.

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
					if constexpr (std::is_scalar_v<T> == true)
					{
						memcpy(&data[i], &data[last_survivior], sizeof(T));
					}
					else
					{
						static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");

						data[i] = std::move(data[last_survivior]);

						std::destroy_at(&data[last_survivior]);
					}
				}
				else
				{
					std::destroy_at(&data[i]);
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
			std::destroy_n(data, count);

			count = 0;
		}

		const T* buffer() const { return data; }

		T* buffer() { return data; }

		const T& operator[](size_t index) const
		{
			return data[index];
		}

		T& operator[](size_t index)
		{
			return data[index];
		}

		// TODO: Rename to count
		size_t size() const { return count; }

		size_t capacity() const { return _capacity; }

	private:
		void grow_memory(size_t _cap, bool discard_old_data = false)
		{
			if (_cap <= _capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(::operator new(sizeof(T) * _cap));

			if (data && count > 0 && discard_old_data == false)
			{
				static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

				uninitialized_move_range(data, count, new_data);
			}

			std::destroy_n(data, count);
			::operator delete(data);

			data = new_data;
			_capacity = _cap;

			if (discard_old_data == true)
			{
				count = 0u;
			}
		}

		void shrink_memory(size_t _cap)
		{
			if (_cap >= _capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(::operator new(sizeof(T) * _cap));

			size_t new_count = std::min(count, _cap);

			if (data && new_count > 0u)
			{
				static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

				uninitialized_move_range(data, new_count, new_data);
			}

			std::destroy_n(data, count);
			::operator delete(data);

			data = new_data;
			_capacity = _cap;
			count = new_count;
		}

		void uninitialized_copy_range(T* src, size_t count, T* dst)
		{
			if constexpr (std::is_scalar_v<T> == true)
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
            if constexpr (std::is_scalar_v<T> == true)
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
			if constexpr (std::is_scalar_v<T> == true)
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
		size_t _capacity{0u};
	};
};
