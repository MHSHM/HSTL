#include <catch2/catch_test_macros.hpp>

#include <Hash_Set.h>

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

	static Key K(int v, size_t h) { return Key{ v, h }; }

} // namespace

TEST_CASE("Hash_Set<int>: insert/get/contains/duplicates/remove")
{
	hstl::Hash_Set<int> s;

	REQUIRE(s.count() == 0);

	s.insert(10);
	s.insert(20);

	REQUIRE(s.count() == 2);
	REQUIRE(s.contains(10));
	REQUIRE(s.contains(20));
	REQUIRE_FALSE(s.contains(30));

	auto p10 = s.get(10);
	REQUIRE(p10 != nullptr);
	REQUIRE(*p10 == 10);

	// Duplicate insert should do nothing and keep the same stored element
	auto& r10 = s.insert(10);
	REQUIRE(s.count() == 2);
	REQUIRE(&r10 == s.get(10));

	// ---- remove ----
	REQUIRE(s.remove(10) == true);
	REQUIRE(s.count() == 1);
	REQUIRE_FALSE(s.contains(10));
	REQUIRE(s.get(10) == nullptr);
	REQUIRE(s.contains(20));

	// Removing again should fail
	REQUIRE(s.remove(10) == false);

	// Remove the remaining element
	REQUIRE(s.remove(20) == true);
	REQUIRE(s.count() == 0);
	REQUIRE_FALSE(s.contains(20));
	REQUIRE(s.get(20) == nullptr);

	// Removing from empty set should fail
	REQUIRE(s.remove(999) == false);
}

TEST_CASE("Hash_Set<Key>: remove back-shifts inside a cluster (no wrap)")
{
	hstl::Hash_Set<Key, KeyHash, KeyEq> s;

	// Force a cluster:
	// A(home=2) goes to 2
	// B(home=2) goes to 3
	// D(home=3) wants 3 but it's taken -> goes to 4
	auto A = K(1, 2);
	auto B = K(2, 2);
	auto D = K(3, 3);

	s.insert(A);
	s.insert(B);
	s.insert(D);

	REQUIRE(s.count() == 3);
	REQUIRE(s.contains(A));
	REQUIRE(s.contains(B));
	REQUIRE(s.contains(D));

	// Remove A, this must back-shift B into A's hole,
	// and then back-shift D as well to preserve findability.
	REQUIRE(s.remove(A) == true);

	REQUIRE(s.count() == 2);
	REQUIRE_FALSE(s.contains(A));
	REQUIRE(s.contains(B));
	REQUIRE(s.contains(D));

	// Removing again should fail
	REQUIRE(s.remove(A) == false);

	// And get() should still find the remaining keys
	REQUIRE(s.get(B) != nullptr);
	REQUIRE(s.get(D) != nullptr);
}

TEST_CASE("Hash_Set<Key>: remove back-shifts across wrap-around")
{
	hstl::Hash_Set<Key, KeyHash, KeyEq> s;

	const size_t cap = s.capacity();
	REQUIRE(cap >= 4);

	const size_t home = cap - 2; // near the end to force wrap

	// These should land at: home, home+1, 0, 1
	auto k1 = K(10, home);
	auto k2 = K(11, home);
	auto k3 = K(12, home);
	auto k4 = K(13, home);

	s.insert(k1);
	s.insert(k2);
	s.insert(k3);
	s.insert(k4);

	REQUIRE(s.count() == 4);
	REQUIRE(s.contains(k1));
	REQUIRE(s.contains(k2));
	REQUIRE(s.contains(k3));
	REQUIRE(s.contains(k4));

	// Remove first element at the start of the wrapped cluster
	REQUIRE(s.remove(k1) == true);

	REQUIRE(s.count() == 3);
	REQUIRE_FALSE(s.contains(k1));
	REQUIRE(s.contains(k2));
	REQUIRE(s.contains(k3));
	REQUIRE(s.contains(k4));

	// Remove one that is likely stored after wrap (depending on probing)
	REQUIRE(s.remove(k3) == true);

	REQUIRE(s.count() == 2);
	REQUIRE_FALSE(s.contains(k3));
	REQUIRE(s.contains(k2));
	REQUIRE(s.contains(k4));
}

TEST_CASE("Hash_Set<Key>: rehash preserves all elements")
{
	hstl::Hash_Set<Key, KeyHash, KeyEq> s;

	const size_t cap0 = s.capacity();
	REQUIRE(cap0 > 0);

	// Trigger the rehash by exceeding the load threshold.
	const size_t threshold = static_cast<size_t>(0.7f * static_cast<float>(cap0));

	const size_t N = threshold + 50; // safely past threshold to force at least one rehash

	for (size_t i = 0; i < N; ++i)
	{
		// Spread hashes to avoid massive clustering and keep test fast
		s.insert(K(static_cast<int>(i), i));
	}

	// Ensure everything is still findable
	for (size_t i = 0; i < N; ++i)
	{
		REQUIRE(s.contains(K(static_cast<int>(i), i)));
		REQUIRE(s.get(K(static_cast<int>(i), i)) != nullptr);
	}

	REQUIRE(s.count() == N);
}
