cmake_minimum_required(VERSION 3.20)

project(HSTL VERSION 0.1.0 DESCRIPTION "Home-grown STL-like containers & algorithms" LANGUAGES CXX)

set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${BIN_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(hstl)
add_subdirectory(playground)
