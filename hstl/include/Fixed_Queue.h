#pragma once

#include "Mutex.h"
#include "Condition_Variable.h"

#include <utility>

namespace hstl
{
	template<typename T, size_t capacity>
	class Fixed_Queue
	{
		static_assert(capacity > 0u, "Can't create a queue with a 0 capacity");

	private:
		size_t _count{0};
		size_t write_index{0};
		size_t read_index{0};
		mutable Mutex mutex;
		// NOTE: Since the queue is fixed in size producers can also sleep not just consumers
		// that's why we use two condition variables, one for the consumers and the other for producers
		Condition_Variable producers_cv;
		Condition_Variable consumers_cv;
		T data[capacity]; // FIXME?: Forces T to be default-constructible

	public:
		Fixed_Queue() = default;
		Fixed_Queue(const Fixed_Queue&) = delete;
		Fixed_Queue& operator=(const Fixed_Queue&) = delete;
		Fixed_Queue(Fixed_Queue&&) = delete;
		Fixed_Queue& operator=(Fixed_Queue&&) = delete;
		~Fixed_Queue() = default;

	public:
		void push(T&& value)
		{
			{
				Scoped_Lock lock(mutex);

				while (_count == capacity)
				{
					producers_cv.wait(lock);
				}

				data[write_index] = std::move(value);

				write_index = (write_index + 1u) % capacity;

				_count++;
			}

			consumers_cv.signal_one();
		}

		void push(const T& value)
		{
			{
				Scoped_Lock lock(mutex);

				while (_count == capacity)
				{
					producers_cv.wait(lock);
				}

				data[write_index] = value;

				write_index = (write_index + 1u) % capacity;

				_count++;
			}

			consumers_cv.signal_one();
		}

		void pop_into(T& value)
		{
			{
				Scoped_Lock lock(mutex);

				while (_count == 0)
				{
					consumers_cv.wait(lock);
				}

				value = std::move(data[read_index]);

				read_index = (read_index + 1u) % capacity;

				_count--;
			}

			producers_cv.signal_one();
		}

		T pop()
		{
			// NOTE: Constructing this value here and then moving the to-be-read data to it
			// inside the block is a bit redundunt, fix once scoped lock has more flexability
			T value{};

			{
				Scoped_Lock lock(mutex);

				while (_count == 0)
				{
					consumers_cv.wait(lock);
				}

				value = std::move(data[read_index]);

				read_index = (read_index + 1u) % capacity;

				_count--;
			}

			producers_cv.signal_one();

			return value;
		}

		bool empty() const
		{
			Scoped_Lock lock(mutex, Mutex::LOCK_MODE::SHARED);

			return _count == 0u;
		}

		size_t count() const
		{
			Scoped_Lock lock(mutex, Mutex::LOCK_MODE::SHARED);

			return _count;
		}
	};
}
