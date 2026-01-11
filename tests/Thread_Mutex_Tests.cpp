#include <catch2/catch_test_macros.hpp>

#include <Thread.h>
#include <Mutex.h>
#include <Array.h>

#include <thread>
#include <atomic>
#include <chrono>

using namespace hstl;

TEST_CASE("Thread: Basic Lifecycle")
{
	SECTION("Creation and Join")
	{
		bool ran = false;

		auto thread_res = Thread::create([&ran]() {
			ran = true;
		});

		REQUIRE(thread_res);

		auto& thread = thread_res.get_value();
		REQUIRE(thread.is_joinable());

		thread.join();

		REQUIRE(ran == true);
		REQUIRE_FALSE(thread.is_joinable());
	}

	SECTION("Move Semantics")
	{
		auto t1_res = Thread::create([]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		});

		REQUIRE(t1_res);

		Thread t1 = std::move(t1_res.get_value());
		uint32_t id1 = t1.get_id();

		Thread t2 = std::move(t1);

		REQUIRE(t2.get_id() == id1);
		REQUIRE(t2.is_joinable());

		REQUIRE_FALSE(t1.is_joinable());

		t2.join();
	}
}

TEST_CASE("Mutex: Exclusive Locking (Mutual Exclusion)")
{
	Mutex mtx;
	int shared_counter = 0;
	const int iterations = 1000;

	auto worker = [&shared_counter, iterations, &mtx]() {
		for (int i = 0; i < iterations; ++i)
		{
			Scoped_Lock lock(mtx);
			shared_counter++;
		}
	};

	auto t1_res = Thread::create(worker);
	auto t2_res = Thread::create(worker);

	REQUIRE(t1_res);
	REQUIRE(t2_res);

	t1_res.get_value().join();
	t2_res.get_value().join();

	REQUIRE(shared_counter == iterations * 2);
}

TEST_CASE("Mutex: Shared Locking (Concurrent Readers)")
{
	Mutex mtx;
	std::atomic<int> active_readers{ 0 };
	bool overlap_occurred = false;

	// If the lock works, multiple threads should be able to hold it at once.
	auto reader = [&]() {
		Scoped_Lock lock(mtx, Mutex::LOCK_MODE::SHARED);

		active_readers++;

		// Wait to allow other threads to enter the shared section
		// This test may fail as it's not deterministic i.e. nothing guarantees that sleeping for 50ms
		// is enough for the spawned threads to overlap but it should have a very high-success rate
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		if (active_readers > 1)
		{
			overlap_occurred = true;
		}

		active_readers--;
	};

	// Create 4 threads
	hstl::Array<Thread> threads;
	for (int i = 0; i < 4; ++i)
	{
		auto res = Thread::create(reader);

		if (res)
		{
			threads.push(std::move(res.get_value()));
		}
	}

	for (size_t i = 0; i < threads.count(); ++i)
	{
		threads[i].join();
	}

	REQUIRE(overlap_occurred == true);
}

TEST_CASE("Mutex: Writer vs Readers Interaction")
{
	Mutex mtx;

	SECTION("Writer blocks new Readers")
	{
		// 1. Lock Exclusively (Writer)
		mtx.lock(Mutex::LOCK_MODE::EXCLUSIVE);

		std::atomic<bool> reader_failed{ false };
		std::atomic<bool> reader_finished{ false };

		auto t_res = Thread::create([&]() {
			// 2. Try to Lock Shared (Reader) - Should fail immediately because Writer has it
			if (mtx.try_lock(Mutex::LOCK_MODE::SHARED) == false)
			{
				reader_failed = true;
			}
			else
			{
				// Should not happen
				mtx.unlock(Mutex::LOCK_MODE::SHARED);
			}
			reader_finished = true;
		});

		// Wait for thread to finish its attempt
		while(!reader_finished);
		t_res.get_value().join();

		REQUIRE(reader_failed == true);

		mtx.unlock(Mutex::LOCK_MODE::EXCLUSIVE);
	}
}

TEST_CASE("Mutex: Manual Try Lock")
{
	Mutex mtx;

	// Exclusive
	REQUIRE(mtx.try_lock(Mutex::LOCK_MODE::EXCLUSIVE) == true);
	mtx.unlock(Mutex::LOCK_MODE::EXCLUSIVE);

	// Shared
	REQUIRE(mtx.try_lock(Mutex::LOCK_MODE::SHARED) == true);
	mtx.unlock(Mutex::LOCK_MODE::SHARED);
}
