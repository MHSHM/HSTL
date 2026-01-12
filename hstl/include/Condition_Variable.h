#pragma once

#include "Mutex.h"

#include <Windows.h>
#include <cmath>
#include <minwindef.h>

namespace hstl
{
	class Condition_Variable
	{
	private:
		CONDITION_VARIABLE handle;

	public:
		Condition_Variable()
		{
			InitializeConditionVariable(&handle);
		}

		Condition_Variable(const Condition_Variable&) = delete;
		Condition_Variable& operator=(const Condition_Variable&) = delete;
		Condition_Variable(Condition_Variable&&) = delete;
		Condition_Variable& operator=(Condition_Variable&&) = delete;

		~Condition_Variable() = default;

	public:
		bool wait(Scoped_Lock& lock)
		{
			Mutex& mutex = lock.get_mutex();
			ULONG flags = lock.get_lock_mode() == Mutex::LOCK_MODE::SHARED ? CONDITION_VARIABLE_LOCKMODE_SHARED : 0u;

			return SleepConditionVariableSRW(&handle, mutex.native_handle(), INFINITE, flags) != 0;
		}

		void signal_one()
		{
			WakeConditionVariable(&handle);
		}

		void signal_all()
		{
			WakeAllConditionVariable(&handle);
		}
	};
}
