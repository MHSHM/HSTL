#include <catch2/catch_test_macros.hpp>

#include "Generic.h"
#include "Str.h"

using namespace hstl;

struct SmallType { char data[16]; };
struct LargeType { char data[1024]; };

TEST_CASE("Generic: SBO vs Heap Logic")
{
	SECTION("Small Types reside inside the object (SBO)")
	{
		Generic g = SmallType{};
		SmallType* ptr = g.cast<SmallType>();

		REQUIRE(ptr != nullptr);
	}

	SECTION("Large Types reside on the Heap")
	{
		Generic g = LargeType{};
		LargeType* ptr = g.cast<LargeType>();

		REQUIRE(ptr != nullptr);
	}
}

TEST_CASE("Generic: Type Safety (Casting)")
{
	SECTION("Correct casts succeed")
	{
		Generic g = 42;
		REQUIRE(g.cast<int>() != nullptr);
		REQUIRE(*g.cast<int>() == 42);
	}

	SECTION("Incorrect casts fail (return nullptr)")
	{
		Generic g = 42;
		REQUIRE(g.cast<float>() == nullptr);
		REQUIRE(g.cast<double>() == nullptr);
		REQUIRE(g.cast<SmallType>() == nullptr);
	}

	SECTION("Const Correctness")
	{
		const Generic g = 3.14f;

		// Should return const float*
		const float* ptr = g.cast<float>();
		REQUIRE(ptr != nullptr);
		REQUIRE(*ptr == 3.14f);

		// Modifying cast should fail to compile (if tested), but runtime check:
		REQUIRE(g.cast<int>() == nullptr);
	}
}

TEST_CASE("Generic: Integration with HSTL Types (Str)")
{
	SECTION("Generic holding hstl::Str")
	{
		hstl::Str s("Test");
		Generic g = std::move(s);

		// Verify we can get it back
		hstl::Str* ptr = g.cast<hstl::Str>();
		REQUIRE(ptr != nullptr);
		REQUIRE(std::strcmp(ptr->c_str(), "Test") == 0);
		REQUIRE(ptr->count() == 4);
	}
}

TEST_CASE("Generic: Copy Semantics")
{
	SECTION("Copy Constructor (Deep Copy)")
	{
		Generic g1 = hstl::Str("Test");
		Generic g2 = g1;

		hstl::Str* s1 = g1.cast<hstl::Str>();
		hstl::Str* s2 = g2.cast<hstl::Str>();

		REQUIRE(s1 != nullptr);
		REQUIRE(s2 != nullptr);

		// Pointers must be different (Deep Copy)
		REQUIRE(s1 != s2);
		REQUIRE(s1->c_str() != s2->c_str());

		// Content must be identical
		REQUIRE(std::strcmp(s2->c_str(), "Test") == 0);
	}

	SECTION("Copy Assignment (Operator=)")
	{
		Generic g1 = hstl::Str("Source");
		Generic g2 = hstl::Str("Destination"); // Should be overwritten

		g2 = g1;

		hstl::Str* s2 = g2.cast<hstl::Str>();
		REQUIRE(s2 != nullptr);
		REQUIRE(std::strcmp(s2->c_str(), "Source") == 0);

		// Verify g1 is still valid
		REQUIRE(g1.cast<hstl::Str>() != nullptr);
	}

	SECTION("Self-Assignment (Copy)")
	{
		Generic g1 = hstl::Str("Self");
		g1 = g1; // Should do nothing

		REQUIRE(std::strcmp(g1.cast<hstl::Str>()->c_str(), "Self") == 0);
	}
}

TEST_CASE("Generic: Move Semantics")
{
	SECTION("Move Constructor")
	{
		Generic g1 = hstl::Str("Move Me");
		Generic g2 = std::move(g1);

		hstl::Str* s2 = g2.cast<hstl::Str>();
		REQUIRE(s2 != nullptr);
		REQUIRE(std::strcmp(s2->c_str(), "Move Me") == 0);

		// In our Move Ctor logic: source.manager_fn = nullptr;
		// So g1 is now an "Empty Generic".
		REQUIRE(g1.cast<hstl::Str>() == nullptr);
	}

	SECTION("Move Assignment (Operator=)")
	{
		Generic g1 = hstl::Str("Move Source");
		Generic g2 = hstl::Str("Move Dest");

		g2 = std::move(g1);

		hstl::Str* s2 = g2.cast<hstl::Str>();
		REQUIRE(s2 != nullptr);
		REQUIRE(std::strcmp(s2->c_str(), "Move Source") == 0);

		REQUIRE(g1.cast<hstl::Str>() == nullptr);
	}

	SECTION("Self-Assignment (Move)")
	{
		Generic g1 = hstl::Str("Self Move");
		g1 = std::move(g1);

		REQUIRE(std::strcmp(g1.cast<hstl::Str>()->c_str(), "Self Move") == 0);
	}
}
