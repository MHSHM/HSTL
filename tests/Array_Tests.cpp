#include <catch2/catch_test_macros.hpp>

#include <Array.h>

#include <string>
#include <memory>

// --- HELPER FOR MOVE SEMANTICS ---
struct MoveTracker {
    int value;
    bool moved_from = false;

    MoveTracker(int v) : value(v) {}
    MoveTracker(const MoveTracker&) = default; // Copy

    // Move Constructor
    MoveTracker(MoveTracker&& other) noexcept : value(other.value) {
        other.moved_from = true;
        other.value = -1;
    }

    MoveTracker& operator=(MoveTracker&& other) noexcept {
        if (this != &other) {
            value = other.value;
            other.moved_from = true;
            other.value = -1;
        }
        return *this;
    }
};

TEST_CASE("Array: Basic Operations (int)") {
    hstl::Array<int> arr;

    SECTION("Starts empty") {
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.capacity() == 0);
    }

    SECTION("Pushing elements increases size") {
        arr.push(10);
        arr.push(20);
        arr.push(30);

        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }
}

TEST_CASE("Array: Memory Management") {
    hstl::Array<int> arr;

    SECTION("Automatic growth triggers correctly") {
        for (int i = 0; i < 20; ++i) {
            arr.push(i);
        }
        REQUIRE(arr.size() == 20);
        REQUIRE(arr.capacity() >= 20);

        // verify integrity
        for (int i = 0; i < 20; ++i) {
            REQUIRE(arr[i] == i);
        }
    }

    SECTION("Reserve allocates memory upfront") {
        arr.reserve(100);
        REQUIRE(arr.capacity() == 100);
        REQUIRE(arr.size() == 0);

        arr.push(1);
        REQUIRE(arr.capacity() == 100); // Should not have changed
    }
}

TEST_CASE("Array: Complex Types (std::string)") {
    hstl::Array<std::string> arr;

    SECTION("Push copies correctly") {
        std::string s = "Hello World";
        arr.push(s);
        REQUIRE(arr[0] == "Hello World");
        REQUIRE(arr[0] != "");
    }

    SECTION("Emplace constructs in-place") {
        // Construct string from count + char: 5 'a's -> "aaaaa"
        arr.emplace(5, 'a');
        REQUIRE(arr[0] == "aaaaa");
        REQUIRE(arr.size() == 1);
    }
}

TEST_CASE("Array: Move Semantics") {
    hstl::Array<MoveTracker> arr;

    SECTION("Push(T&&) moves the object") {
        MoveTracker t(100);
        arr.push(std::move(t));

        REQUIRE(arr.size() == 1);
        REQUIRE(arr[0].value == 100);
        REQUIRE(t.moved_from == true); // Verify the source was pillaged
    }

    SECTION("Emplace moves if passed r-value") {
        MoveTracker t(200);
        arr.emplace(std::move(t));

        REQUIRE(arr[0].value == 200);
        REQUIRE(t.moved_from == true);
    }

    SECTION("Handling Move-Only Types (std::unique_ptr)") {
        hstl::Array<std::unique_ptr<int>> ptr_arr;

        auto ptr = std::make_unique<int>(999);
        ptr_arr.push(std::move(ptr));

        REQUIRE(ptr_arr.size() == 1);
        REQUIRE(*ptr_arr[0] == 999);
        REQUIRE(ptr == nullptr); // Original is empty
    }
}

TEST_CASE("Array: Removal Strategies", "[array][remove]") {
    hstl::Array<int> arr;
    for (int i = 0; i < 5; ++i)
        arr.push(i); // [0, 1, 2, 3, 4]

    SECTION("remove_ordered preserves order (O(N))") {
        // Remove '2' at index 2
        arr.remove_ordered(2);

        REQUIRE(arr.size() == 4);
        // Expected: [0, 1, 3, 4]
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 3); // Shifted
        REQUIRE(arr[3] == 4);
    }

    SECTION("remove performs swap-and-pop (O(1))") {
        // Remove '1' at index 1
        arr.remove(1);

        REQUIRE(arr.size() == 4);
        // Expected: [0, 4, 2, 3] -> Last element (4) moved to index 1
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[1] == 4); // The swap!
        REQUIRE(arr[2] == 2);
        REQUIRE(arr[3] == 3);
    }
}
