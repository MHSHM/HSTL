#include <catch2/catch_test_macros.hpp>

#include <Hash_Map.h>

namespace {

	struct Key {
		int v = 0;
		size_t forced_hash = 0;
	};

	struct KeyHash {
		size_t operator()(const Key& k) const noexcept { return k.forced_hash; }
	};

	struct KeyEq {
		bool operator()(const Key& a, const Key& b) const noexcept { return a.v == b.v; }
	};

	// A helper class that counts operations to verify deep-copy vs pointer-steal
	struct Tracker {
		int id;

		static int copy_count;
		static int move_count;

		Tracker(int val = 0) : id(val) {}

		// Copy Constructor
		Tracker(const Tracker& other) : id(other.id) {
			copy_count++;
		}

		// Move Constructor
		Tracker(Tracker&& other) noexcept : id(other.id) {
			move_count++;
			other.id = -1; // Mark as "stolen"
		}

		// Copy Assignment
		Tracker& operator=(const Tracker& other) {
			if (this != &other) {
				id = other.id;
				copy_count++;
			}
			return *this;
		}

		// Move Assignment
		Tracker& operator=(Tracker&& other) noexcept {
			if (this != &other) {
				id = other.id;
				move_count++;
				other.id = -1;
			}
			return *this;
		}

		bool operator==(const Tracker& other) const { return id == other.id; }
	};

	// Initialize static counters
	int Tracker::copy_count = 0;
	int Tracker::move_count = 0;

	struct TrackerHash {
		size_t operator()(const Tracker& t) const { return static_cast<size_t>(t.id); }
	};

	static Key K(int v, size_t h) { return Key{ v, h }; }

} // namespace

TEST_CASE("Hash_Map<int, int>: insert/get/contains/remove")
{
	hstl::Hash_Map<int, int> m;

	REQUIRE(m.count() == 0);

	m.insert(10, 100);
	m.insert(20, 200);

	REQUIRE(m.count() == 2);
	REQUIRE(m.contains(10));
	REQUIRE(m.contains(20));
	REQUIRE_FALSE(m.contains(30));

	auto p10 = m.get(10);
	REQUIRE(p10 != nullptr);
	REQUIRE(*p10 == 100);

	// Insert overwrite behavior
	auto& r10 = m.insert(10, 101);
	REQUIRE(m.count() == 2);
	REQUIRE(r10 == 101);
	REQUIRE(*m.get(10) == 101);

	// ---- remove ----
	REQUIRE(m.remove(10) == true);
	REQUIRE(m.count() == 1);
	REQUIRE_FALSE(m.contains(10));
	REQUIRE(m.get(10) == nullptr);
	REQUIRE(m.contains(20));

	// Removing again should fail
	REQUIRE(m.remove(10) == false);

	// Remove the remaining element
	REQUIRE(m.remove(20) == true);
	REQUIRE(m.count() == 0);
}

TEST_CASE("Hash_Map<Key, int>: remove back-shifts inside a cluster (no wrap)")
{
	hstl::Hash_Map<Key, int, KeyHash, KeyEq> m;

	// Force a cluster:
	// A(home=2) goes to 2
	// B(home=2) goes to 3
	// D(home=3) wants 3 but it's taken -> goes to 4
	auto A = K(1, 2);
	auto B = K(2, 2);
	auto D = K(3, 3);

	m.insert(A, 100);
	m.insert(B, 200);
	m.insert(D, 300);

	REQUIRE(m.count() == 3);
	REQUIRE(m.contains(A));
	REQUIRE(m.contains(B));
	REQUIRE(m.contains(D));

	// Remove A, this must back-shift B into A's hole,
	// and then back-shift D as well to preserve findability.
	REQUIRE(m.remove(A) == true);

	REQUIRE(m.count() == 2);
	REQUIRE_FALSE(m.contains(A));
	REQUIRE(m.contains(B));
	REQUIRE(m.contains(D));

	// Verify values are intact
	REQUIRE(*m.get(B) == 200);
	REQUIRE(*m.get(D) == 300);

	// Removing again should fail
	REQUIRE(m.remove(A) == false);
}

TEST_CASE("Hash_Map<Key, int>: remove back-shifts across wrap-around")
{
	hstl::Hash_Map<Key, int, KeyHash, KeyEq> m;

	const size_t cap = m.capacity();
	REQUIRE(cap >= 4);

	const size_t home = cap - 2; // near the end to force wrap

	// These should land at: home, home+1, 0, 1
	auto k1 = K(10, home);
	auto k2 = K(11, home);
	auto k3 = K(12, home);
	auto k4 = K(13, home);

	m.insert(k1, 10);
	m.insert(k2, 20);
	m.insert(k3, 30);
	m.insert(k4, 40);

	REQUIRE(m.count() == 4);
	REQUIRE(m.contains(k1));
	REQUIRE(m.contains(k2));
	REQUIRE(m.contains(k3));
	REQUIRE(m.contains(k4));

	// Remove first element at the start of the wrapped cluster
	REQUIRE(m.remove(k1) == true);

	REQUIRE(m.count() == 3);
	REQUIRE_FALSE(m.contains(k1));
	REQUIRE(m.contains(k2));
	REQUIRE(m.contains(k3));
	REQUIRE(m.contains(k4));

	// Remove one that is likely stored after wrap (depending on probing)
	REQUIRE(m.remove(k3) == true);

	REQUIRE(m.count() == 2);
	REQUIRE_FALSE(m.contains(k3));
	REQUIRE(m.contains(k2));
	REQUIRE(m.contains(k4));

	// Verify values
	REQUIRE(*m.get(k2) == 20);
	REQUIRE(*m.get(k4) == 40);
}

TEST_CASE("Hash_Map<Key, int>: rehash preserves all elements")
{
	hstl::Hash_Map<Key, int, KeyHash, KeyEq> m;

	const size_t cap0 = m.capacity();
	REQUIRE(cap0 > 0);

	// Trigger the rehash by exceeding the load threshold.
	const size_t threshold = static_cast<size_t>(0.7f * static_cast<float>(cap0));

	const size_t N = threshold + 50;

	for (size_t i = 0; i < N; ++i)
	{
		m.insert(K(static_cast<int>(i), i), static_cast<int>(i * 10));
	}

	// Ensure everything is still findable and values are correct
	for (size_t i = 0; i < N; ++i)
	{
		auto key = K(static_cast<int>(i), i);
		REQUIRE(m.contains(key));
		REQUIRE(*m.get(key) == static_cast<int>(i * 10));
	}

	REQUIRE(m.count() == N);
}

TEST_CASE("Hash_Map<int, int>: range-based for loop")
{
	hstl::Hash_Map<int, int> map;
	map.insert(2, 20);
	map.insert(4, 40);
	map.insert(6, 60);
	map.insert(8, 80);

	size_t sum_keys = 0u;
	size_t sum_vals = 0u;

	for (auto entry : map)
	{
		sum_keys += entry.key;
		sum_vals += entry.value;
	}

	REQUIRE(sum_keys == 20u);
	REQUIRE(sum_vals == 200u);
}

TEST_CASE("Hash_Map<int, Tracker>: Copy Semantics")
{
	// Setup initial map: Key is int, Value is Tracker
	hstl::Hash_Map<int, Tracker> map_a;
	map_a.insert(1, Tracker(10));
	map_a.insert(2, Tracker(20));
	map_a.insert(3, Tracker(30));

	REQUIRE(map_a.count() == 3);

	// Reset counters
	Tracker::copy_count = 0;
	Tracker::move_count = 0;

	SECTION("Copy Constructor performs deep copy")
	{
		hstl::Hash_Map<int, Tracker> map_b{ map_a };

		// 1. Verify Independence
		REQUIRE(map_b.count() == 3);
		REQUIRE(map_b.contains(1));

		// Modifying B should not affect A
		map_b.remove(1);
		REQUIRE(map_b.count() == 2);
		REQUIRE(map_a.count() == 3);
		REQUIRE(map_a.contains(1));

		// 2. Verify Cost
		// We expect 3 copies for the Values (Keys are ints)
		REQUIRE(Tracker::copy_count == 3);
		REQUIRE(Tracker::move_count == 0);
	}

	SECTION("Copy Assignment performs deep copy")
	{
		hstl::Hash_Map<int, Tracker> map_b;
		// Insert garbage to ensure it gets cleared
		map_b.insert(999, Tracker(999));

		Tracker::copy_count = 0; // Reset again

		map_b = map_a;

		REQUIRE(map_b.count() == 3);
		REQUIRE(map_b.contains(2));
		REQUIRE(map_b.get(2)->id == 20);

		// Modifying A should not affect B
		map_a.remove(2);
		REQUIRE(map_a.count() == 2);
		REQUIRE(map_b.count() == 3);
		REQUIRE(map_b.contains(2));

		// Expect 3 copies for the transfer
		REQUIRE(Tracker::copy_count == 3);
	}
}

TEST_CASE("Hash_Map<int, Tracker>: Move Semantics")
{
	SECTION("Move Constructor steals ownership (O(1))")
	{
		hstl::Hash_Map<int, Tracker> map_a;
		map_a.insert(1, Tracker(10));
		map_a.insert(2, Tracker(20));
		map_a.insert(3, Tracker(30));

		REQUIRE(map_a.count() == 3);

		// Reset counters
		Tracker::copy_count = 0;
		Tracker::move_count = 0;

		hstl::Hash_Map<int, Tracker> map_b{ std::move(map_a) };

		// 1. Verify Ownership Transfer
		REQUIRE(map_b.count() == 3);
		REQUIRE(map_b.contains(1));
		REQUIRE(map_b.get(1)->id == 10);

		// 2. Verify Source is Empty/Valid
		REQUIRE(map_a.count() == 0);

		// 3. Verify ZERO Cost
		// Moving a container should just swap internal pointers.
		REQUIRE(Tracker::copy_count == 0);
		REQUIRE(Tracker::move_count == 0);
	}

	SECTION("Move Assignment steals ownership (O(1))")
	{
		hstl::Hash_Map<int, Tracker> map_a;
		map_a.insert(1, Tracker(10));
		map_a.insert(2, Tracker(20));
		map_a.insert(3, Tracker(30));

		REQUIRE(map_a.count() == 3);

		// Reset counters
		Tracker::copy_count = 0;
		Tracker::move_count = 0;

		hstl::Hash_Map<int, Tracker> map_b;
		map_b.insert(999, Tracker(999)); // Should be destroyed

		Tracker::copy_count = 0;
		Tracker::move_count = 0;

		map_b = std::move(map_a);

		REQUIRE(map_b.count() == 3);
		REQUIRE(map_b.contains(2));
		REQUIRE(map_a.count() == 0);

		// Cost Check:
		REQUIRE(Tracker::copy_count == 0);
		REQUIRE(Tracker::move_count == 0);
	}
}