#pragma once

#include <new>
#include <cstddef>

namespace hstl
{
	class Allocator
	{
	public:
		virtual void* allocate(size_t size, size_t alignment) = 0;
		virtual void deallocate(void* memory, size_t size) = 0;
		virtual ~Allocator() = default;
	};

	class Default_Allocator : public Allocator
	{
	public:
		Default_Allocator() { }

		void* allocate(size_t size, size_t alignment) override
		{
			return ::operator new(size, std::align_val_t(alignment));
		}

		void deallocate(void* ptr, size_t size) override
		{
			::operator delete(ptr, size);
		}

		static Default_Allocator* get()
		{
			static Default_Allocator allocator;
			return &allocator;
		}
	};
}
