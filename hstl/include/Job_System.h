#pragma once

#include <atomic>
#include <Memory.h>
#include <Log.h>
#include <cstring>

namespace hstl
{
	struct Job
	{
		using Function = void(*)(void* data);
		Function function{nullptr};
		void* data{nullptr};
		std::atomic<int>* parent_counter{nullptr};
	}

	class Work_Stealing_Queue
	{
	private:
		Job* jobs{nullptr};
		Allocator* allocator{nullptr};
		// NOTE: Has to be a power of 2
		size_t capacity{4096u};
		// NOTE: Align to cache line boundary to prevent false sharing
		// NOTE: Monotonic counters to avoid the ABA problem
		alignas(64u) std::atomic<size_t> bottom;
		alignas(64u) std::atomic<size_t> top;

	public:
		Work_Stealing_Queue(Allocator* allocator = Default_Allocator::get()):
			allocator{allocator}
		{
			bottom.store(0, std::memory_order_relaxed);
			top.store(0, std::memory_order_relaxed);
			jobs = static_cast<Job*>(allocator->allocate(capacity * sizeof(Job), alignof(Job)));
		}

		~Work_Stealing_Queue()
		{
			if (jobs)
			{
				allocator->deallocate(jobs, capacity * sizeof(Job), alignof(Job));
			}
		}

	public:
		void push(const Job& job)
		{
			// NOTE: Relaxed ordering as only the owner thread can write the bottom
			size_t b = bottom.load(std::memory_order_relaxed);

			// NOTE: Acquire to make sure we get the latest value published by thieves threads
			size_t t = top.load(std::memory_order_acquire);

			if (b - t >= capacity)
			{
				log_warn("Deque full (cap: {}). Dropping Job.", capacity);
				return;
			}

			memcpy(jobs + (b & (capacity - 1)), &job, sizeof(Job));

			// NOTE: A release fence to make sure the write to the internal buffer
			// is visible for subsequent reads
			std::atomic_thread_fence(std::memory_order_release);

			bottom.store(b + 1, std::memory_order_relaxed);
		}

		bool pop(Job& out_job)
		{
			// NOTE: Assuming the queue is not empty
			size_t b = bottom.load(std::memory_order_relaxed) - 1u;

			// NOTE: Relaxed store as bottom is only written by the owner thread
			bottom.store(b, std::memory_order_relaxed);

			// NOTE: Prevent Load-Store reordering
			std::atomic_thread_fence(std::memory_order_seq_cst);

			size_t t = top.load(std::memory_order_relaxed);

			ptrdiff_t size = static_cast<ptrdiff_t>(b) - static_cast<ptrdiff_t>(t);

			// NOTE: The queue was empty, rollback
			if (size < 0u)
			{
				bottom.store(t, std::memory_order_relaxed);
				return false;
			}

			Job job = jobs[b & (capacity - 1u)];

			// NOTE: The easy case, the queue has data and both the owner and the thief access different ends
			if (size > 0u)
			{
				memcpy(&out_job, &job, sizeof(Job));
				return true;
			}

			// NOTE: Here you (the owner) are fighting with the thieves over the only entry in the queue
			if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
			{
				bottom.store(t + 1, std::memory_order_relaxed);
				return false;
			}

			memcpy(&out_job, &job, sizeof(Job));
			bottom.store(t + 1, std::memory_order_relaxed);
			return true;
		}

		bool steal(Job& out_job)
		{
		    size_t t = top.load(std::memory_order_acquire);

		    std::atomic_thread_fence(std::memory_order_seq_cst); // Synchronize with pop's fence
		    size_t b = bottom.load(std::memory_order_acquire);

		    ptrdiff_t size = static_cast<ptrdiff_t>(b) - static_cast<ptrdiff_t>(t);

		    if (size <= 0)
		    {
		        return false;
		    }

		    Job job = jobs[t & (capacity - 1u)];

		    if (!top.compare_exchange_strong(t, t + 1,
		                                     std::memory_order_seq_cst,
		                                     std::memory_order_relaxed))
		    {
		        return false;
		    }

		    // 6. Success
		    out_job = job;
		    return true;
		}
	};
};
