#pragma once

#include <new>
#include <cstddef>
#include <assert.h>

namespace hstl
{
	class Allocator
	{
	public:
		[[nodiscard]] virtual void* allocate(size_t size, size_t alignment) = 0;
		virtual void deallocate(void* memory, size_t size, size_t alignment) = 0;
		virtual ~Allocator() = default;
	};

	class Default_Allocator : public Allocator
	{
	public:
		Default_Allocator() { }

		[[nodiscard]]
		void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			return ::operator new(size, std::align_val_t(alignment));
		}

		void deallocate(void* ptr, size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			::operator delete(ptr, size, std::align_val_t(alignment));
		}

		static Default_Allocator* get()
		{
			static Default_Allocator allocator;
			return &allocator;
		}
	};

	class Linear_Allocator: public Allocator
	{
	private:
		size_t capacity;
		size_t offset;
		void* data{nullptr};

	private:
	[[nodiscard]]
	bool is_power_of_two(size_t x)
	{
		return (x != 0) && ((x & (x - 1)) == 0);
	}

	[[nodiscard]]
	size_t align_up(size_t offset, size_t alignment)
	{
		assert(is_power_of_two(alignment) && "Alignment must be a power of two");

		return (offset + alignment - 1) & ~(alignment - 1);
	}

	public:
		Linear_Allocator(size_t capacity):
			capacity{capacity},
			offset{0u}
		{
			data = ::operator new(capacity, std::align_val_t(alignof(std::max_align_t)));
		}

		[[nodiscard]]
		void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			auto aligned_offset = align_up(offset, alignment);

			if (aligned_offset + size > capacity)
			{
				return nullptr;
			}

			offset = aligned_offset + size;

			return (char*)data + aligned_offset;
		}

		void deallocate(void* ptr, size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			// no-op
		}

		void clear()
		{
			offset = 0u;
		}

		~Linear_Allocator() override
		{
			::operator delete(data, capacity, std::align_val_t(alignof(std::max_align_t)));
		}
	};
}
