#pragma once

#include "Memory.h"

#include <cstring>
#include <memory>
#include <type_traits>

namespace hstl
{
	class Generic
	{
	private:
		static constexpr size_t MAX_STACK_ALLOCATED_BUFFER_SIZE = 64u;

		union
		{
			void* dynamically_allocated_data;
			// FIXME: Align to std::max_align_t which is usually 16 bytes
			// Won't work for types that fit in the buffer but has a bigger alignment
			alignas(std::max_align_t) char stack_allocated_buffer[MAX_STACK_ALLOCATED_BUFFER_SIZE];
		} storage;

		using Destroy_FN = void(*)(Generic& value, Allocator* allocator);
		using Copy_FN    = void(*)(const Generic& src, Generic& dst, Allocator* allocator);
		using Move_FN    = void(*)(Generic&& src, Generic& dst, Allocator* allocator);

		Destroy_FN destroy_fn{nullptr};
		Copy_FN copy_fn{nullptr};
		Move_FN move_fn{nullptr};

		Allocator* allocator{nullptr};

	public:
		template<typename T>
		static void destroy(Generic& value, Allocator* allocator)
		{
			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				T* data = reinterpret_cast<T*>(value.storage.stack_allocated_buffer);

				data->~T();
			}
			else
			{
				T* data = reinterpret_cast<T*>(value.storage.dynamically_allocated_data);

				data->~T();

				allocator->deallocate(data, sizeof(T), alignof(T));
			}
		}

	public:
		template<typename T>
		Generic(const T& value, Allocator* allocator = Default_Allocator::get()):
			allocator{allocator}
		{
			using decay_T = std::decay_t<T>;

			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				new (reinterpret_cast<T*>(storage.stack_allocated_buffer)) T(value);
			}
			else
			{
				T* data = static_cast<T*>(allocator->allocate(sizeof(T), alignof(T)));

				new (data) T(value);

				storage.dynamically_allocated_data = data;
			}

			destroy_fn = &destroy<decay_T>;
		}

		template<typename T>
		Generic(T&& value, Allocator* allocator):
			allocator{allocator}
		{
			using decay_T = std::decay_t<T>;

			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				new (reinterpret_cast<T*>(storage.stack_allocated_buffer)) T(std::move(value));
			}
			else
			{
				T* data = static_cast<T*>(allocator->allocate(sizeof(T), alignof(T)));

				new (data) T(std::move(value));

				storage.dynamically_allocated_data = data;
			}

			destroy_fn = &destroy<decay_T>;
		}

		~Generic()
		{
			if (destroy_fn)
			{
				destroy_fn(*this, allocator);
			}
		}
	};
}
