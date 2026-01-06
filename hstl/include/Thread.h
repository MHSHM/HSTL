#pragma once

#include "Memory.h"
#include "Result.h"

#include <functional>
#include <utility>
#include <new>
#include <type_traits>

#include <Windows.h>

namespace hstl
{
	template<typename Task>
	struct Thread_Data
	{
		Task task;
		Allocator* allocator;

		// NOTE: This is static to maintain the signature required by CreateThread
		static DWORD WINAPI thread_entry(LPVOID data)
		{
			auto typed_data = static_cast<Thread_Data*>(data);

			typed_data->task();

			typed_data->allocator->deallocate(typed_data, sizeof(Thread_Data), alignof(Thread_Data));

			return 0u;
		}
	};

	class Thread
	{
	private:
		HANDLE handle{nullptr};
		uint32_t id{0u};

	private:
		Thread() = default;

	private:
		template<typename Func, typename... Args>
		static Result<Thread> _create_impl(Allocator* allocator, Func&& func, Args&&... args)
		{
			// NOTE: Mark the lambda as mutable to be able to move func and args to std::invoke
			// NOTE: Creating a storage package where the function and arguments are stored, std::forward will move
			// what needs to be moved and copy what needs to be copied
			auto task = [func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
				std::invoke(std::move(func), std::move(args)...);
			};

			using Task_Type = decltype(task);
			using Thread_Data_Type = Thread_Data<Task_Type>;

			auto thread_data = static_cast<Thread_Data_Type*>(
				allocator->allocate(sizeof(Thread_Data_Type), alignof(Thread_Data_Type))
			);

			new (thread_data) Thread_Data_Type{std::move(task), allocator};

			Thread thread;
			thread.handle = CreateThread(
				nullptr,
				0u,
				&Thread_Data_Type::thread_entry,
				thread_data,
				0u,
				reinterpret_cast<LPDWORD>(&thread.id)
			);

			if (thread.handle == nullptr)
			{
				allocator->deallocate(thread_data, sizeof(Thread_Data_Type), alignof(Thread_Data_Type));

				return Err("Failed to create the thread");
			}

			// Move thread to Result
			return std::move(thread);
		}

	public:

		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;

		Thread(Thread&& source):
			handle{source.handle},
			id{source.id}
		{
			source.handle = nullptr;
		}

		Thread& operator=(Thread&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			if (handle)
			{
				// hmmmm
				WaitForSingleObject(handle, INFINITY);

				CloseHandle(handle);
			}

			handle = source.handle;
			id = source.id;

			source.handle = nullptr;

			return *this;
		}

		~Thread()
		{
			if (handle)
			{
				WaitForSingleObject(handle, INFINITY);

				CloseHandle(handle);
			}
		}

	public:
		template<typename Func, typename... Args>
		static Result<Thread> create(Allocator* allocator, Func&& func, Args&&... args)
		{
			return _create_impl(allocator, std::forward<Func>(func), std::forward<Args>(args)...);
		}

		// NOTE: doing create(Derived_Allocator*, ...) will actuall pick this overload over the other one
		// which will cause a compilation error so force it to pick the first overload in case Func is convertable to
		// Allocator*
		template<typename Func, typename... Args>
		requires (!std::is_convertible_v<Func, Allocator*>)
		static Result<Thread> create(Func&& func, Args&&... args)
		{
			return _create_impl(Default_Allocator::get(), std::forward<Func>(func), std::forward<Args>(args)...);
		}

		void join()
		{
			if (handle)
			{
				WaitForSingleObject(handle, INFINITY);
				CloseHandle(handle);
				handle = nullptr;
			}
		}

		bool is_joinable() const
		{
			return handle != nullptr;
		}

		void detach()
		{
			if (handle)
			{
				CloseHandle(handle);
				handle = nullptr;
			}
		}
	};
};