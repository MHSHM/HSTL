#include <catch2/catch_test_macros.hpp>
// Include your headers
#include "Str.h"

// --- Helper to verify internal structure ---
// We access the raw C-string to ensure the null terminator is exactly where we expect
bool verify_terminator(const hstl::Str& s, size_t expected_count) {
    if (s.count() != expected_count) return false;
    return s.c_str()[expected_count] == '\0';
}

TEST_CASE("Str_View: Edge Cases") {

    SECTION("Substring at exact end") {
        hstl::Str_View v("Hello", 5);

        auto sub = v.substr(5, 0);
        REQUIRE(sub.count() == 0);
    }
}

TEST_CASE("Str: Construction & Invariants") {
    SECTION("Empty String Construction") {
        hstl::Str s;
        REQUIRE(s.count() == 0);
        REQUIRE(s.empty());
        REQUIRE(verify_terminator(s, 0)); // Ensure \0 is at index 0
    }

    SECTION("Nullptr Construction") {
        hstl::Str s(nullptr);
        REQUIRE(s.count() == 0);
        REQUIRE(verify_terminator(s, 0));
    }
}

TEST_CASE("Str: Insert Logic") {
    hstl::Str s("abc"); // count = 3, capacity internal includes \0

    SECTION("Insert at Beginning") {
        s.insert("X", 0);
        REQUIRE(std::strcmp(s.c_str(), "Xabc") == 0);
        REQUIRE(verify_terminator(s, 4));
    }

    SECTION("Insert in Middle") {
        s.insert("X", 1);
        REQUIRE(std::strcmp(s.c_str(), "aXbc") == 0);
    }

    SECTION("Insert at End / Append") {
        // 'abc' has count 3. Null is at 3. We insert at 3.
        s.insert("X", 3);
        REQUIRE(std::strcmp(s.c_str(), "abcX") == 0);
        REQUIRE(verify_terminator(s, 4));
    }
}

TEST_CASE("Str: Remove Logic (O(N) Implementation)") {

    SECTION("Remove Single Occurrence (False Flag)") {
        hstl::Str s("bananas");
        s.remove('a', false); // Should remove only first 'a'

        REQUIRE(std::strcmp(s.c_str(), "bnanas") == 0);
        REQUIRE(s.count() == 6);
    }

    SECTION("Remove All - Standard Case") {
        hstl::Str s("bananas");
        s.remove('a', true); // Should become "bnns"

        REQUIRE(std::strcmp(s.c_str(), "bnns") == 0);
        REQUIRE(s.count() == 4);
        REQUIRE(verify_terminator(s, 4));
    }

    SECTION("Remove All - Consecutive Characters") {
        hstl::Str s("AABBAA");
        s.remove('A', true);

        REQUIRE(std::strcmp(s.c_str(), "BB") == 0);
        REQUIRE(s.count() == 2);
        REQUIRE(verify_terminator(s, 2));
    }

    SECTION("Remove All - Entire String Matches") {
        hstl::Str s("ZZZZZ");
        s.remove('Z', true);

        REQUIRE(std::strcmp(s.c_str(), "") == 0);
        REQUIRE(s.count() == 0);
        REQUIRE(verify_terminator(s, 0));
    }

    SECTION("Remove All - No Matches") {
        hstl::Str s("Hello");
        s.remove('Z', true);

        REQUIRE(std::strcmp(s.c_str(), "Hello") == 0);
        REQUIRE(s.count() == 5);
    }
}

TEST_CASE("Str: Substring Removal") {
    SECTION("Remove Substring in Middle") {
        hstl::Str s("Hello World");
        s.remove(" World");
        REQUIRE(std::strcmp(s.c_str(), "Hello") == 0);
        REQUIRE(s.count() == 5);
    }

    SECTION("Remove Substring at Start") {
        hstl::Str s("Hello World");
        s.remove("Hello ");
        REQUIRE(std::strcmp(s.c_str(), "World") == 0);
    }

    SECTION("Remove Non-Existent Substring") {
        hstl::Str s("Hello");
        s.remove("XYZ");
        REQUIRE(std::strcmp(s.c_str(), "Hello") == 0);
    }
}

TEST_CASE("Str: Push") {
    hstl::Str s("A");

    // Internal buffer: ['A', '\0']
    // Push 'B':
    // 1. Array push 'B' -> ['A', '\0', 'B']
    // 2. Swap last two -> ['A', 'B', '\0']

    s.push('B');

    REQUIRE(s.count() == 2);
    REQUIRE(s[0] == 'A');
    REQUIRE(s[1] == 'B');
    // We can't verify s[2] == '\0' via operator[] because of assert(index < count)
    // But verify_terminator checks raw pointer
    REQUIRE(verify_terminator(s, 2));
}

TEST_CASE("Str: Comparison & Search") {
    hstl::Str s("Hello World");

    SECTION("starts_with / ends_with") {
        REQUIRE(s.starts_with("Hello"));
        REQUIRE(s.ends_with("World"));
        REQUIRE_FALSE(s.starts_with("World"));
    }

    SECTION("find") {
        REQUIRE(s.find("World") == 6);
        REQUIRE(s.find("Universe") == hstl::Str::npos);
    }
}