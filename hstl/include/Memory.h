#pragma once

#include <new>
#include <cstddef>
#include <assert.h>
#include <algorithm>

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
			return malloc(size);
		}

		void deallocate(void* ptr, size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			assert(ptr); // hmmm, maybe we should make it no-op?
			free(ptr);
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
		void* memory_block{nullptr};

	private:
	bool is_power_of_two(size_t x)
	{
		return (x != 0) && ((x & (x - 1)) == 0);
	}

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
			memory_block = ::operator new(capacity, std::align_val_t(alignof(std::max_align_t)));
		}

		Linear_Allocator(const Linear_Allocator&) = delete;
		Linear_Allocator& operator=(const Linear_Allocator&) = delete;
		Linear_Allocator(Linear_Allocator&&) = delete;
		Linear_Allocator& operator=(Linear_Allocator&&) = delete;

		[[nodiscard]]
		void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override
		{
			auto aligned_offset = align_up(offset, alignment);

			if (aligned_offset + size > capacity)
			{
				assert(false && "Allocator ran out of memory");
				return nullptr;
			}

			offset = aligned_offset + size;

			return (char*)memory_block + aligned_offset;
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
			if (memory_block)
			{
				::operator delete(memory_block, capacity, std::align_val_t(alignof(std::max_align_t)));
			}
		}
	};

	// TODO: This should not be an allocator, separate into its own type
	class Pool_Allocator : public Allocator
	{
	private:
		struct Node
		{
			Node* next;
		};

		size_t total_capacity{0};
		size_t alignment;
		void* memory_block{nullptr};
		Node* head{nullptr};

	private:
		bool is_power_of_two(size_t x)
		{
			return (x != 0) && ((x & (x - 1)) == 0);
		}

		size_t align_up(size_t offset, size_t alignment)
		{
			assert(is_power_of_two(alignment) && "Alignment must be a power of two");

			return (offset + alignment - 1) & ~(alignment - 1);
		}

	public:
		Pool_Allocator(size_t object_size, size_t object_alignment, size_t max_num_objects):
			alignment{object_alignment}
		{
			assert(max_num_objects > 0u);

			size_t min_block_size = std::max(object_size, sizeof(Node));

			size_t aligned_object_size = align_up(min_block_size, object_alignment);

			total_capacity = aligned_object_size * max_num_objects;

			memory_block = ::operator new(total_capacity, std::align_val_t(object_alignment));

			Node* current = static_cast<Node*>(memory_block);

			for (size_t i = 0; i < max_num_objects - 1u; ++i)
			{
				current->next = reinterpret_cast<Node*>((char*)current + aligned_object_size);

				current = current->next;
			}

			current->next = nullptr;
			head = static_cast<Node*>(memory_block);
		}

		// NOTE: size and alignment are ingored as they're known by the allocator design
		// maybe the pool allocator should be its own thing
		[[nodiscard]]
		void* allocate(size_t size = 0u, size_t alignment = alignof(std::max_align_t)) override
		{
			if (head == nullptr)
			{
				return nullptr;
			}

			void* ptr = static_cast<void*>(head);
			head = head->next;

			return ptr;
		}

		// NOTE: size and alignment are ingored as they're known by the allocator design
		// maybe the pool allocator should be its own thing
		void deallocate(void* ptr, size_t size = 0u, size_t alignment = alignof(std::max_align_t)) override
		{
			char* start = static_cast<char*>(memory_block);
			char* end = static_cast<char*>(start + total_capacity);
			assert(ptr && ptr >= start && ptr < end);

			Node* new_head = static_cast<Node*>(ptr);
			new_head->next = head;

			head = new_head;
		}

		~Pool_Allocator() override
		{
			if (memory_block)
			{
				::operator delete(memory_block, std::align_val_t(alignment));
			}
		}
	};
}
