#include <catch2/catch_test_macros.hpp>
#include "Fixed_Str.h"

template<size_t N>
bool verify_terminator(const hstl::Fixed_Str<N>& s, size_t expected_count) {
    if (s.count() != expected_count) return false;
    return s.c_str()[expected_count] == '\0';
}

TEST_CASE("Fixed_Str: Construction & Basics") {
    SECTION("Default Construction") {
        hstl::Fixed_Str<32> s;
        REQUIRE(s.count() == 0);
        REQUIRE(s.empty());
        REQUIRE(std::strcmp(s.c_str(), "") == 0);
        REQUIRE(verify_terminator(s, 0));
    }

    SECTION("C-String Construction") {
        hstl::Fixed_Str<32> s("Hello");
        REQUIRE(s.count() == 5);
        REQUIRE(std::strcmp(s.c_str(), "Hello") == 0);
        REQUIRE(verify_terminator(s, 5));
    }

    SECTION("Construction from nullptr") {
        hstl::Fixed_Str<32> s(nullptr);
        REQUIRE(s.count() == 0);
        REQUIRE(s.empty());
    }

    SECTION("Copy Construction") {
        hstl::Fixed_Str<32> original("CopyMe");
        hstl::Fixed_Str<32> copy(original);

        REQUIRE(copy.count() == 6);
        REQUIRE(std::strcmp(copy.c_str(), "CopyMe") == 0);

        // Modify copy to ensure deep copy
        copy.push('!');
        REQUIRE(std::strcmp(original.c_str(), "CopyMe") == 0);
        REQUIRE(std::strcmp(copy.c_str(), "CopyMe!") == 0);
    }
}

TEST_CASE("Fixed_Str: Push & Append") {
    hstl::Fixed_Str<16> s("Hi");

    SECTION("Push char") {
        s.push('!');
        REQUIRE(s.count() == 3);
        REQUIRE(std::strcmp(s.c_str(), "Hi!") == 0);
        REQUIRE(verify_terminator(s, 3));
    }

    SECTION("Push string") {
        s.push(" There");
        REQUIRE(s.count() == 8);
        REQUIRE(std::strcmp(s.c_str(), "Hi There") == 0);
    }

    SECTION("Push n chars (push_n)") {
        s.push_n('.', 3);
        REQUIRE(s.count() == 5);
        REQUIRE(std::strcmp(s.c_str(), "Hi...") == 0);
    }

    SECTION("Operator+=") {
        s += ' ';
        s += "World";
        REQUIRE(std::strcmp(s.c_str(), "Hi World") == 0);
    }
}

TEST_CASE("Fixed_Str: Removal") {
    SECTION("Remove single char (first occurrence)") {
        hstl::Fixed_Str<32> s("banana");
        s.remove('a', false); // Remove first 'a'

        REQUIRE(std::strcmp(s.c_str(), "bnana") == 0);
        REQUIRE(s.count() == 5);
    }

    SECTION("Remove single char (all occurrences)") {
        hstl::Fixed_Str<32> s("banana");
        s.remove('a', true); // Remove all 'a's

        REQUIRE(std::strcmp(s.c_str(), "bnn") == 0);
        REQUIRE(s.count() == 3);
        REQUIRE(verify_terminator(s, 3));
    }

    SECTION("Remove substring") {
        hstl::Fixed_Str<32> s("Hello World");
        s.remove("Hello ");

        REQUIRE(std::strcmp(s.c_str(), "World") == 0);
        REQUIRE(s.count() == 5);
    }

    SECTION("Remove substring (middle)") {
        hstl::Fixed_Str<32> s("123ABC456");
        s.remove("ABC");

        REQUIRE(std::strcmp(s.c_str(), "123456") == 0);
        REQUIRE(s.count() == 6);
    }
}

TEST_CASE("Fixed_Str: Insertion") {
    hstl::Fixed_Str<32> s("StartEnd");

    SECTION("Insert in middle") {
        s.insert("Middle", 5); // "Start" is 5 chars
        REQUIRE(std::strcmp(s.c_str(), "StartMiddleEnd") == 0);
        REQUIRE(s.count() == 14);
    }

    SECTION("Insert at beginning") {
        s.insert("X", 0);
        REQUIRE(std::strcmp(s.c_str(), "XStartEnd") == 0);
    }

    SECTION("Insert at end") {
        s.insert("!", s.count());
        REQUIRE(std::strcmp(s.c_str(), "StartEnd!") == 0);
    }
}

TEST_CASE("Fixed_Str: View Operations") {
    hstl::Fixed_Str<32> s("Test String");

    SECTION("Find") {
        REQUIRE(s.find("String") == 5);
        REQUIRE(s.find("Missing") == hstl::Fixed_Str<32>::npos);
    }

    SECTION("Starts/Ends With") {
        REQUIRE(s.starts_with("Test"));
        REQUIRE(s.ends_with("String"));
        REQUIRE_FALSE(s.starts_with("String"));
    }

    SECTION("Operator[]") {
        REQUIRE(s[0] == 'T');
        REQUIRE(s[s.count() - 1] == 'g');

        // Modification via reference
        s[0] = 'B';
        REQUIRE(std::strcmp(s.c_str(), "Best String") == 0);
    }
}

TEST_CASE("Fixed_Str: Iterators") {
    hstl::Fixed_Str<32> s("abc");

    SECTION("Range-based for loop") {
        char buffer[4] = {0};
        int i = 0;
        for (char c : s) {
            buffer[i++] = c;
        }
        REQUIRE(std::strcmp(buffer, "abc") == 0);
    }

    SECTION("Standard Algorithms") {
        // Reverse the string using iterators
        auto start = s.begin();
        auto end = s.end();
        while (start < end) {
            char temp = *start;
            *start = *--end;
            *end = temp;
            start++;
        }
        REQUIRE(std::strcmp(s.c_str(), "cba") == 0);
    }
}

TEST_CASE("Fixed_Str: Edge Cases & Limits") {
    // Capacity 4 means max 3 chars + null terminator
    hstl::Fixed_Str<4> s;

    SECTION("Full Capacity") {
        s.push("abc");
        REQUIRE(s.count() == 3);
        REQUIRE(std::strcmp(s.c_str(), "abc") == 0);
    }

    SECTION("Clear") {
        s.push("a");
        s.clear();
        REQUIRE(s.count() == 0);
        REQUIRE(s.empty());
        REQUIRE(verify_terminator(s, 0));
    }
}
