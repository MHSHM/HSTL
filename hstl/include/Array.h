#pragma once

#include <type_traits>
#include <memory>
#include <algorithm>
#include <assert.h>
#include <cstring>

#include "Memory.h"

namespace hstl
{
	class Str;

	template<typename T>
	class Array
	{
		friend class Str;

	private:
		size_t _count{0u};
		size_t _capacity{0u};
		Allocator* allocator;
		T* data{nullptr};

	private:
		void grow_memory(size_t _cap, bool discard_old_data = false)
		{
			if (_cap <= _capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(allocator->allocate(sizeof(T) * _cap, alignof(T)));

			assert(new_data != nullptr);

			if (data && _count > 0 && discard_old_data == false)
			{
				static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

				uninitialized_move_range(data, _count, new_data);
			}

			if (data)
			{
				std::destroy_n(data, _count);
				allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
			}

			data = new_data;
			_capacity = _cap;

			if (discard_old_data == true)
			{
				_count = 0u;
			}
		}

		void shrink_memory(size_t _cap)
		{
			if (_cap >= _capacity)
			{
				return;
			}

			T* new_data = static_cast<T*>(allocator->allocate(sizeof(T) * _cap, alignof(T)));

			size_t new_count = std::min(_count, _cap);

			if (data && new_count > 0u)
			{
				static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

				uninitialized_move_range(data, new_count, new_data);
			}

			if (data)
			{
				std::destroy_n(data, _count);
				allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
			}

			data = new_data;
			_capacity = _cap;
			_count = new_count;
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
			if constexpr (std::is_trivially_copyable_v<T> == true)
			{
				memset(start, 0, sizeof(T) * count);
			}
			else
			{
				std::uninitialized_value_construct_n(start, count);
			}
		}

	public:
		using iterator = T*;
		using const_iterator = const T*;

		Array(Allocator* allocator = Default_Allocator::get()):
			allocator{allocator}
		{

		};

		Array(size_t count, Allocator* allocator = Default_Allocator::get()):
			allocator{allocator}
		{
			static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");

			grow_memory(count);

			uninitialized_value_construct_range(data, count);

			_count = count;
		}

		Array(const Array& source):
			_count{source._count},
			_capacity{source._capacity},
			allocator{source.allocator},
			data{static_cast<T*>(source.allocator->allocate(sizeof(T) * source._capacity, alignof(T)))}
		{
			static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");

			if (source._count > 0u)
			{
				uninitialized_copy_range(source.data, source._count, data);
			}
		}

		Array& operator=(const Array& source)
		{
			static_assert(std::is_copy_assignable_v<T>, "T must have a copy assignment operator");

			if (this == &source)
			{
				return *this;
			}

			if (allocator != source.allocator) // allocator mismatch case
			{
				// Destroy the data using the current allocator
				if (data)
				{
					std::destroy_n(data, _count);
					allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
				}

				// Allocate new data using the source allocator
				T* new_data = nullptr;

				if (source._count > 0u)
				{
					new_data = static_cast<T*>(source.allocator->allocate(sizeof(T) * source._count, alignof(T)));
					uninitialized_copy_range(source.data, source._count, new_data);
				}

				data = new_data;
				allocator = source.allocator;
				_capacity = source._count;
			}
			else
			{
				if (source._count > _capacity)
				{
					grow_memory(source._count, true);
				}
				else
				{
					std::destroy_n(data, _count);
				}

				if (source._count > 0u)
				{
					uninitialized_copy_range(source.data, source._count, data);
				}
			}

			_count = source._count;
			return *this;
		}

		Array(Array&& source) noexcept:
			_count{ source._count },
			_capacity{source._capacity},
			allocator{source.allocator},
			data{source.data}
		{
			source.data = nullptr;
			source._count = 0u;
			source._capacity = 0u;
			source.allocator = nullptr;
		}

		Array& operator=(Array&& source) noexcept
		{
			if (this == &source)
			{
				return *this;
			}

			if (data)
			{
				std::destroy_n(data, _count);
				allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
			}

			data = source.data;
			_count = source._count;
			_capacity = source._capacity;
			allocator = source.allocator;

			source.data = nullptr;
			source._count = 0u;
			source._capacity = 0u;
			source.allocator = nullptr;

			return *this;
		}

		~Array() noexcept
		{
			std::destroy_n(data, _count);

			if (data)
			{
				allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
			}
		}

	public:
		void reserve(size_t _cap, bool discard_old_data = false)
		{
			grow_memory(_cap, discard_old_data);
		}

		void resize(size_t new_count)
		{
			static_assert(std::is_default_constructible_v<T>, "T must have a default constructor");

			if (new_count == _count)
			{
				return;
			}

			if (new_count > _capacity)
			{
				grow_memory(new_count);
			}

			if (new_count > _count)
			{
				uninitialized_value_construct_range(data + _count, new_count - _count);
			}
			else
			{
				std::destroy_n(data + new_count, _count - new_count);
			}

			_count = new_count;
		}

		T& push(const T& element)
		{
			static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");

			if (_count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new(&data[_count++]) T(element);

			return data[_count - 1];
		}

		T& push(T&& element)
		{
			static_assert(std::is_move_constructible_v<T>, "T must have a move constructor");

			if (_count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new(&data[_count++]) T(std::move(element));

			return data[_count - 1];
		}

		template<typename... Args>
		T& emplace(Args&&... args)
		{
			static_assert(std::is_constructible_v<T, Args...>, "T doesn't have a constructor that matches the provided arguments");

			if (_count == _capacity)
			{
				grow_memory(_capacity == 0u ? 10u : _capacity * 2u);
			}

			new (&data[_count++]) T(std::forward<Args>(args)...);

			return data[_count - 1];
		}

		void shrink_to_fit()
		{
			if (_capacity > _count)
			{
				shrink_memory(_count);
			}
		}

		void remove(size_t index)
		{
		    if constexpr (std::is_trivially_copyable_v<T>)
		    {
		    	memcpy(&data[index], &data[_count - 1], sizeof(T));
		    }
		    else
		    {
		        if (index < _count - 1)
		        {
					static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");

		        	data[index] = std::move(data[_count - 1]);
		        }

		       	std::destroy_at(&data[_count - 1]);
		    }

		    _count--;
		}

		void remove_ordered(size_t index)
		{
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                memmove(&data[index], &data[index + 1], sizeof(T) * (_count - index - 1));
            }
            else
            {
                for (size_t i = index; i < _count - 1; ++i)
                {
					static_assert(std::is_move_assignable_v<T>, "T must have a move assignment operator");

                	data[i] = std::move(data[i + 1]);
                }

                std::destroy_at(&data[_count - 1]);
            }

			_count--;
		}

		template<typename F>
		void remove_if(F f)
		{
			// TODO: Rework this function and decide whether it should follow the standard and maintain order or not.

			// NOTE: This type trait will check if the result of F is __convertable__ to bool
			// I'm not sure if this is desired but roll with it for now
			static_assert(std::is_invocable_r_v<bool, F, const T&>, "Predicate must be callable as bool(const T&)");
			if (_count == 0)
				return;

			// FIXME: int64_t here is a narrowing conversion used to avoid underflow
			int64_t last_survivior = _count - 1;
			for (int64_t i = _count - 1; i >= 0; --i)
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

			_count = last_survivior + 1;
		}

		bool reassign_allocator(Allocator* new_allocator)
		{
			if (new_allocator == allocator)
			{
				return true;
			}

			// Allocate new memory with the new allocator
			T* new_data = static_cast<T*>(new_allocator->allocate(sizeof(T) * _capacity, alignof(T)));

			// Move the data to the new memory if any
			if (data && _count > 0u)
			{
				uninitialized_move_range(data, _count, new_data);

				if constexpr (std::is_trivially_destructible_v<T> == false)
				{
					std::destroy_n(data, _count);
				}
			}

			// Free the old memory with the old allocator
			if (data && _capacity > 0u)
			{
				allocator->deallocate(data, sizeof(T) * _capacity, alignof(T));
			}

			// Update the allocator pointer
			allocator = new_allocator;

			return true;
		}

		const_iterator begin() const noexcept
		{
			return data;
		}

		const_iterator end() const noexcept
		{
			return data + _count;
		}

		iterator begin() noexcept
		{
			return data;
		}

		iterator end() noexcept
		{
			return data + _count;
		}

		void clear() noexcept
		{
			std::destroy_n(data, _count);

			_count = 0;
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
		size_t count() const { return _count; }

		size_t capacity() const { return _capacity; }
	};
};
