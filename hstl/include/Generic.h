#pragma once

#include "Memory.h"

#include <cstring>
#include <memory>
#include <type_traits>
#include <new>

namespace hstl
{
	class Generic
	{
	private:
		static constexpr size_t MAX_STACK_ALLOCATED_BUFFER_SIZE = 48;

		union
		{
			void* dynamically_allocated_data;
			// NOTE: This will almost always align the memory 16 bytes which is what std::max_align_t reports
			// if the object to be stored in that memory could fit in that stach buffer but had an alignment larger than
			// 16 the memory will be mis-aligned for that object
			alignas(std::max_align_t) char stack_allocated_buffer[MAX_STACK_ALLOCATED_BUFFER_SIZE];
		} storage;

		enum class OP
		{
			DESTRUCTOR,
			COPY,
			MOVE
		};

		using Manager_FN = void(*)(Generic* src, Generic* dst, OP op, Allocator* allocator);
		Manager_FN manager_fn{nullptr};

		Allocator* allocator{nullptr};

	public:
		template<typename T>
		static void destroy(Generic* value, Allocator* allocator)
		{
			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				T* data = reinterpret_cast<T*>(value->storage.stack_allocated_buffer);
				data->~T();
			}
			else
			{
				if (value->storage.dynamically_allocated_data)
				{
					T* data = reinterpret_cast<T*>(value->storage.dynamically_allocated_data);
					data->~T();
					allocator->deallocate(data, sizeof(T), alignof(T));
				}
			}
		}

		template<typename T>
		static void copy(Generic* src, Generic* dst, Allocator* allocator)
		{
			T* src_data = nullptr;
			T* dst_data = nullptr;

			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				src_data = reinterpret_cast<T*>(src->storage.stack_allocated_buffer);
				dst_data = reinterpret_cast<T*>(dst->storage.stack_allocated_buffer);
			}
			else
			{
				src_data = reinterpret_cast<T*>(src->storage.dynamically_allocated_data);

				void* raw_mem = allocator->allocate(sizeof(T), alignof(T));
				dst->storage.dynamically_allocated_data = raw_mem;

				dst_data = static_cast<T*>(raw_mem);
			}

			if constexpr (std::is_trivially_copyable_v<T>)
			{
				std::memcpy(dst_data, src_data, sizeof(T));
			}
			else
			{
				new (dst_data) T(*src_data);
			}
		}

		template<typename T>
		static void move(Generic* src, Generic* dst, Allocator*)
		{
			if constexpr (sizeof(T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				T* src_data = reinterpret_cast<T*>(src->storage.stack_allocated_buffer);
				T* dst_data = reinterpret_cast<T*>(dst->storage.stack_allocated_buffer);

				if constexpr (std::is_trivially_copyable_v<T>)
				{
					std::memcpy(dst_data, src_data, sizeof(T));
				}
				else
				{
					new (dst_data) T(std::move(*src_data));
				}
			}
			else
			{
				dst->storage.dynamically_allocated_data = src->storage.dynamically_allocated_data;
				src->storage.dynamically_allocated_data = nullptr;
			}
		}

		template<typename T>
		static void manage(Generic* src, Generic* dst, OP op, Allocator* allocator)
		{
			switch (op)
			{
			case OP::DESTRUCTOR: destroy<T>(src, allocator); break;
			case OP::COPY:       copy<T>(src, dst, allocator); break;
			case OP::MOVE:       move<T>(src, dst, allocator); break;
			default: break;
			}
		}

	public:
		template<typename T>
		requires (!std::is_same_v<std::decay_t<T>, Generic>)
		Generic(T&& value, Allocator* allocator = Default_Allocator::get()):
			allocator{allocator}
		{
			using decay_T = std::decay_t<T>;

			if constexpr (sizeof(decay_T) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				new (reinterpret_cast<decay_T*>(storage.stack_allocated_buffer)) decay_T(std::forward<T>(value));
			}
			else
			{
				storage.dynamically_allocated_data = static_cast<decay_T*>(allocator->allocate(sizeof(decay_T), alignof(decay_T)));
				new (storage.dynamically_allocated_data) decay_T(std::forward<T>(value));
			}

			manager_fn = &manage<decay_T>;
		}

		Generic(const Generic& source):
			allocator{source.allocator},
			manager_fn{source.manager_fn}
		{
			manager_fn(const_cast<Generic*>(&source), this, OP::COPY, allocator);
		}

		Generic(Generic&& source):
			allocator{source.allocator},
			manager_fn{source.manager_fn}
		{
			manager_fn(&source, this, OP::MOVE, allocator);

			source.manager_fn = nullptr;
			source.allocator = nullptr;
		}

		Generic& operator=(const Generic& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (manager_fn)
			{
				// This will blindly destroy and release the destination memory even if the source data
				// can fit in it, I think we can do better but roll with it for now
				manager_fn(this, nullptr, OP::DESTRUCTOR, allocator);
			}

			if (source.manager_fn)
			{
				source.manager_fn(const_cast<Generic*>(&source), this, OP::COPY, source.allocator);
			}

			manager_fn = source.manager_fn;
			allocator  = source.allocator;

			return *this;
		}

		Generic& operator=(Generic&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (manager_fn)
			{
				manager_fn(this, nullptr, OP::DESTRUCTOR, allocator);
			}

			if (source.manager_fn)
			{
				source.manager_fn(&source, this, OP::MOVE, nullptr /*not needed*/);
			}

			allocator = source.allocator;
			manager_fn = source.manager_fn;

			source.manager_fn = nullptr;
			source.allocator = nullptr;

			return *this;
		}

		template<typename U>
		U* cast()
		{
			using decay_U = std::decay_t<U>;

			// NOTE: This will generate a unique function per cast type which can increase the size
			// of your binaries and inherently increase the size of compilation times, it's very fast though
			if (manager_fn != &manage<decay_U>)
			{
				return nullptr;
			}

			if constexpr (sizeof(decay_U) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				return reinterpret_cast<U*>(storage.stack_allocated_buffer);
			}
			else
			{
				return static_cast<U*>(storage.dynamically_allocated_data);
			}
		}

		template<typename U>
		const U* cast() const
		{
			using decay_U = std::decay_t<U>;

			if (manager_fn != &manage<decay_U>)
			{
				return nullptr;
			}

			if constexpr (sizeof(decay_U) <= MAX_STACK_ALLOCATED_BUFFER_SIZE)
			{
				return reinterpret_cast<const U*>(storage.stack_allocated_buffer);
			}
			else
			{
				return static_cast<const U*>(storage.dynamically_allocated_data);
			}
		}

		~Generic()
		{
			if (manager_fn)
			{
				manager_fn(this, nullptr, OP::DESTRUCTOR, allocator);
			}
		}
	};
}
