#include <catch2/catch_test_macros.hpp>

#include "File.h"
#include "Str.h"
#include "Array.h"

#include <filesystem>
#include <cstring>

using namespace hstl;

struct ScopedFileRemover
{
    std::filesystem::path path;
    ScopedFileRemover(const char* p) : path(p)
    {
        if (std::filesystem::exists(path)) std::filesystem::remove(path);
    }
    ~ScopedFileRemover()
    {
        if (std::filesystem::exists(path)) std::filesystem::remove(path);
    }
};

TEST_CASE("File Cursor Management", "[File]")
{
    ScopedFileRemover cleaner("test_cursor.bin");
    const char* data = "0123456789";

    // Setup
    {
        auto res = File::open("test_cursor.bin", File::FILE_OPEN_MODE::WRITE_OVERWRITE);
        REQUIRE(res);
        res.get_value().write(data, 10);
    }

    auto file_res = File::open("test_cursor.bin", File::FILE_OPEN_MODE::READ);
    REQUIRE(file_res);
    auto& file = file_res.get_value();

    SECTION("tell() starts at 0")
    {
        CHECK(file.tell().get_value() == 0);
    }

    SECTION("seek() moves cursor correctly")
    {
        auto res = file.seek(5);
        REQUIRE(res);
        CHECK(res.get_value() == 5);
        CHECK(file.tell().get_value() == 5);

        char buffer[1];
        file.read(buffer, 1);
        CHECK(buffer[0] == '5');

        CHECK(file.tell().get_value() == 6);
    }
}

TEST_CASE("File Random Access (Stateless)", "[File]")
{
    ScopedFileRemover cleaner("test_random.bin");

    // Setup
    {
        auto res = File::open("test_random.bin", File::FILE_OPEN_MODE::WRITE_OVERWRITE);
        REQUIRE(res);
        res.get_value().write("0123456789", 10);
    }

    // Fix: Store the result
    auto file_res = File::open("test_random.bin", File::FILE_OPEN_MODE::READ);
    REQUIRE(file_res);
    auto& file = file_res.get_value();

    SECTION("read_at preserves original cursor position")
    {
        REQUIRE(file.seek(2));
        REQUIRE(file.tell().get_value() == 2);

        char buffer[1] = { 0 };
        auto res = file.read_at(buffer, 1, 5);

        REQUIRE(res);
        CHECK(buffer[0] == '5');

        CHECK(file.tell().get_value() == 2);

        file.read(buffer, 1);
        CHECK(buffer[0] == '2');
    }
}

TEST_CASE("File Utilities", "[File]")
{
    ScopedFileRemover cleaner("test_util.bin");
    const char* content = "TheQuickBrownFox";
    size_t len = std::strlen(content);

    // Setup: Create file with content
    {
        auto res = File::open("test_util.bin", File::FILE_OPEN_MODE::WRITE_OVERWRITE);
        REQUIRE(res);
        res.get_value().write(content, len);
    }

    // Open for reading
    auto file_res = File::open("test_util.bin", File::FILE_OPEN_MODE::READ);
    REQUIRE(file_res);
    auto& file = file_res.get_value();

    SECTION("size() returns correct byte count")
    {
        auto size_res = file.size();
        REQUIRE(size_res);
        CHECK(size_res.get_value() == len);
    }

    SECTION("read_all() gets entire content regardless of cursor")
    {
        // 1. Mess up cursor deliberately (Read first 3 bytes)
        char dummy[3];
        file.read(dummy, 3);
        CHECK(file.tell().get_value() == 3);

        // 2. Call read_all
        auto res = file.read_all();
        REQUIRE(res);

        // 3. Verify Data
        Array<uint8_t>& data = res.get_value();
        CHECK(data.count() == len);
        CHECK(std::memcmp(data.buffer(), content, len) == 0);

        // 4. Verify Cursor Stability
        // Since read_all uses read_at (stateless), the cursor should NOT have moved 
        // from position 3 (where we left it).
        CHECK(file.tell().get_value() == 3);
    }

    SECTION("read_all() on empty file")
    {
        ScopedFileRemover cleaner_empty("test_empty.bin");
        // Create empty file
        {
            File::open("test_empty.bin", File::FILE_OPEN_MODE::WRITE_OVERWRITE);
        }

        auto empty_res = File::open("test_empty.bin", File::FILE_OPEN_MODE::READ);
        REQUIRE(empty_res);

        auto res = empty_res.get_value().read_all();
        REQUIRE(res);
        CHECK(res.get_value().count() == 0);
    }
}