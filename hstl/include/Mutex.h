#pragma once

#include <cstdint>

#include <Windows.h>

namespace hstl
{
	// This is a thin wrapper around SRWLOCK allowing for acquiring shared or exclusive access
	// to the lock (default is exclusive access). This is conceptually similar to std::shared_mutex.

	class Mutex
	{
	private:
		SRWLOCK handle;

	public:
		Mutex()
		{
			InitializeSRWLock(&handle);
		}

		Mutex(const Mutex&) = delete;
		Mutex& operator=(const Mutex&) = delete;
		Mutex(Mutex&&) = delete;
		Mutex& operator=(Mutex&&) = delete;

		~Mutex() = default;

	public:
		enum class LOCK_MODE : uint8_t
		{
			EXCLUSIVE, // Writer
			SHARED     // Reader
		};

		void lock(LOCK_MODE mode = LOCK_MODE::EXCLUSIVE)
		{
			switch (mode)
			{
			case LOCK_MODE::EXCLUSIVE:
				AcquireSRWLockExclusive(&handle);
				break;
			case LOCK_MODE::SHARED:
				AcquireSRWLockShared(&handle);
				break;
			}
		}

		void unlock(LOCK_MODE mode = LOCK_MODE::EXCLUSIVE)
		{
			switch (mode)
			{
			case LOCK_MODE::EXCLUSIVE:
				ReleaseSRWLockExclusive(&handle);
				break;
			case LOCK_MODE::SHARED:
				ReleaseSRWLockShared(&handle);
				break;
			}
		}

		bool try_lock(LOCK_MODE mode = LOCK_MODE::EXCLUSIVE)
		{
			switch (mode)
			{
			case LOCK_MODE::EXCLUSIVE:
				return TryAcquireSRWLockExclusive(&handle) != 0;
			case LOCK_MODE::SHARED:
				return TryAcquireSRWLockShared(&handle) != 0;
			}

			return false;
		}

		SRWLOCK* native_handle()
		{
			return &handle;
		}
	};

	class Scoped_Lock
	{
	private:
		Mutex& mutex;
		Mutex::LOCK_MODE mode;

	public:
		Scoped_Lock(Mutex& mutex, Mutex::LOCK_MODE mode = Mutex::LOCK_MODE::EXCLUSIVE):
			mutex{mutex},
			mode{mode}
		{
			mutex.lock(mode);
		}

		Scoped_Lock(const Scoped_Lock&) = delete;
		Scoped_Lock& operator=(const Scoped_Lock&) = delete;
		Scoped_Lock(Scoped_Lock&&) = delete;
		Scoped_Lock& operator=(Scoped_Lock&&) = delete;

		~Scoped_Lock()
		{
			mutex.unlock(mode);
		}
	};
}
