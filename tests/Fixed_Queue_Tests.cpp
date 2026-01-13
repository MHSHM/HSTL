#include <catch2/catch_test_macros.hpp>

#include "Fixed_Queue.h"
#include "Thread.h"
#include "Array.h"

#include <atomic>
#include <chrono>
#include <memory>

using namespace hstl;

TEST_CASE("Fixed_Queue: Basic")
{
	Fixed_Queue<int, 3> q;

	SECTION("Initial state")
	{
		REQUIRE(q.empty());
		REQUIRE(q.count() == 0);
	}

	SECTION("Push and Pop behavior")
	{
		q.push(10);
		q.push(20);
		q.push(30);

		REQUIRE(q.count() == 3);
		REQUIRE_FALSE(q.empty());

		REQUIRE(q.pop() == 10);
		REQUIRE(q.pop() == 20);
		REQUIRE(q.pop() == 30);

		REQUIRE(q.empty());
	}

	SECTION("Circular Buffer Wrapping")
	{
		// 1. Fill the queue
		q.push(1);
		q.push(2);
		q.push(3);

		// 2. Pop one (Head moves forward)
		REQUIRE(q.pop() == 1);
		REQUIRE(q.count() == 2);

		// 3. Push one (Tail wraps around to index 0)
		q.push(4);

		REQUIRE(q.count() == 3);

		// 4. Verify order
		REQUIRE(q.pop() == 2);
		REQUIRE(q.pop() == 3);
		REQUIRE(q.pop() == 4);
	}
}

TEST_CASE("Fixed_Queue: Concurrency with HSTL Thread")
{
	SECTION("Simple Ping-Pong")
	{
		Fixed_Queue<int, 10> q;
		const int iterations = 100;
		std::atomic<int> sum{0};

		// Producer Task
		auto producer_task = [&]() {
			for (int i = 0; i < iterations; ++i) {
				q.push(1);
			}
		};

		// Consumer Task
		auto consumer_task = [&]() {
			for (int i = 0; i < iterations; ++i) {
				sum += q.pop();
			}
		};

		// Launch HSTL Threads
		auto t1 = Thread::create(producer_task);
		auto t2 = Thread::create(consumer_task);

		REQUIRE(t1);
		REQUIRE(t2);

		t1.get_value().join();
		t2.get_value().join();

		REQUIRE(sum == iterations);
		REQUIRE(q.empty());
	}

	SECTION("Torture Test (Heavy Contention)")
	{
		// Extremely small capacity to force frequent blocking/waking
		Fixed_Queue<int, 4> q;

		const int num_producers = 4;
		const int num_consumers = 4;
		const int items_per_thread = 5000;

		std::atomic<int> total_consumed{0};

		Array<Thread> threads;
		threads.reserve(num_producers + num_consumers);

		auto producer = [&]() {
			for (int i = 0; i < items_per_thread; ++i) {
				q.push(i);
			}
		};

		auto consumer = [&]() {
			for (int i = 0; i < items_per_thread; ++i) {
				q.pop();
				total_consumed++;
			}
		};

		// Launch Consumers
		for (int i = 0; i < num_consumers; ++i) {
			threads.push(std::move(Thread::create(consumer).get_value()));
		}

		// Launch Producers
		for (int i = 0; i < num_producers; ++i) {
			threads.push(std::move(Thread::create(producer).get_value()));
		}

		// Join All
		for (size_t i = 0; i < threads.count(); ++i) {
			threads[i].join();
		}

		REQUIRE(q.empty());
		REQUIRE(total_consumed == (num_consumers * items_per_thread));
	}
}
