# HSTL

HSTL is a custom, high-performance, STL-like library built from scratch for systems programming and game development. It is designed with a focus on explicit memory management, cache locality, and exception-free error handling.

The goal of this project is to provide the core foundation (containers, algorithms, memory, I/O, concurrency) needed to build a game (or any real-time application).

## Key Features

### Containers
* **Array**: Dynamic array implementation with explicit allocator support.
* **Hash_Map & Hash_Set**: High-performance open-addressing hash tables using linear probing and backward-shift deletion (tombstone-free).

### Strings
* **Str**: Heap-allocated dynamic string.
* **Str_View**: Non-owning string view for efficient passing of string data.
* **Fixed_Str<N>**: Stack-allocated, fixed-capacity string to avoid dynamic memory allocations as much as possible.
* **Str_Format**: Type-safe string formatting utilities.

### Memory Management
* **Allocators**: Polymorphic `Allocator` interface used throughout all containers.
* **Linear_Allocator**: For O(1) memory allocations.
* **Pool_Allocator**: Fixed-size memory pool for object pooling.

### Error Handling
* **Result<T>**: Exception-free error handling similar to std::expected.

### Concurrency
* **Threading**: Lightweight wrapper around OS threads.
* **Mutex**: Slim Reader/Writer Lock (SRWLOCK) wrapper with exclusive and shared locking modes.

### Debugging
* **Logging**: Lightning-fast logging with automatic source location capture.
* **Memory Leak Detector**: WIP

### File I/O
* **C API Wrapper**: A lightweight, RAII wrapper around the standard C file API (`<cstdio>`).
* **Sequential Operations**: Supports standard read and write operations that automatically advance the internal file cursor.
* **Random Access**: Supports efficient random access via `read_at` and `write_at`, allowing data retrieval at specific offsets.

## Requirements

* **Standard**: C++20.
* **Platform**: Windows only (Currently uses Win32 API for the Threading API).

## Usage Example

```cpp
#include "Array.h"
#include "Str.h"
#include "File.h"
#include "Log.h"
#include "Defer.h"

using namespace hstl;

int main() {
    // Custom Linear Allocator
    Linear_Allocator allocator(1024 * 1024);

    // Defer for an automatic cleanup
    DEFER { allocator.clear(); };

    // Create Array using the custom allocator
    Array<Str> filenames(&allocator);
    filenames.push(Str("data/config.ini", &allocator));

    // File I/O with Result<T> error handling
    auto file_res = File::open(filenames[0].view(), File::FILE_OPEN_MODE::WRITE_OVERWRITE);
    
    if (file_res) {
        File& file = file_res.get_value();
        file.write("version=1.0", 11);
        log_info("Config saved successfully.");
    } else {
        log_error("Failed to save config: {}", file_res.get_err_message());
    }

    return 0;
}
