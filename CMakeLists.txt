cmake_minimum_required(VERSION 3.20)

project(HSTL VERSION 0.1.0 DESCRIPTION "Home-grown STL-like containers & algorithms" LANGUAGES CXX)

option(HSTL_BUILD_TESTS "Build Catch2 unit tests" ON)

set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${BIN_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (HSTL_BUILD_TESTS)
    include(CTest)
endif()

if (HSTL_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
endif()

add_subdirectory(hstl)
add_subdirectory(playground)

if (HSTL_BUILD_TESTS)
    add_subdirectory(tests)
endif()
