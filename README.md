# HSTL

**HSTL** is a custom, high-performance, STL-like library built from scratch for systems programming and game development. It is designed around explicit memory management, cache locality, and exception-free error handling.

The goal is to provide the core foundation, containers, algorithms, memory, concurrency, and I/O, needed to build a game or any real-time application, with full control over allocation and no hidden costs.

> Deep-dives on the design are written up in a companion article series:
> [Building HSTL](https://www.linkedin.com/pulse/building-hstl-engineering-high-performance-stl-real-time-hashim-7uekf/) ·
> [Memory Management in HSTL](https://www.linkedin.com/pulse/building-hstl-engineering-high-performance-stl-real-time-hashim-e4lff/) ·
> [The C++ Memory Model: The Road to Lock-Free HSTL](https://www.linkedin.com/pulse/c-memory-model-road-lock-free-hstl-muhammed-hashim-yn35f/)

## Highlights

* **Lock-free work-stealing job system** — a scheduler built on a lock-free deque. False sharing is avoided by aligning atomic counters to cache-line boundaries, and the ABA problem is eliminated with monotonic counters and strict memory ordering.
* **Tombstone-free hash tables** — open-addressing `Hash_Map` / `Hash_Set` using linear probing with backward-shift deletion, which keeps probe chains unbroken and preserves cache locality without tombstones.
* **Explicit allocators everywhere** — a polymorphic `Allocator` interface threaded through every container, with an O(1) `Linear_Allocator` and an intrusive `Pool_Allocator` to control fragmentation.
* **Small buffer optimization via type erasure** — `Generic` keeps small objects on the stack to avoid dynamic allocation.
* **Exception-free** — errors flow through `Result<T>` instead of exceptions.

## Features

### Concurrency
* **Job_System**: Lock-free, work-stealing job scheduler (see the article series above).
* **Task**: Lightweight unit of work dispatched to the job system.
* **Thread**: Thin wrapper around OS threads.
* **Mutex**: Slim Reader/Writer Lock (SRWLOCK) wrapper with exclusive and shared modes.
* **Condition_Variable**: Condition-variable primitive for coordination.

### Containers
* **Array**: Dynamic array with explicit allocator support.
* **Hash_Map & Hash_Set**: Open-addressing hash tables using linear probing and backward-shift (tombstone-free) deletion.
* **Fixed_Queue**: Stack-allocated, fixed-capacity queue.
* **Generic**: Type-erased container with small buffer optimization.

### Strings
* **Str**: Heap-allocated dynamic string.
* **Str_View**: Non-owning string view for efficient passing.
* **Fixed_Str<N>**: Stack-allocated, fixed-capacity string that avoids heap allocation.
* **Str_Format**: Type-safe string formatting.

### Memory Management
* **Allocator**: Polymorphic allocator interface used throughout all containers.
* **Linear_Allocator**: O(1) bump allocation.
* **Pool_Allocator**: Fixed-size pool for object pooling.

### Error Handling
* **Result<T>**: Exception-free error handling, similar to `std::expected`.

### Debugging & Utilities
* **Log**: Fast logging with automatic source-location capture.
* **Defer**: Scope-based cleanup (`DEFER { ... };`).
* Memory leak detector (WIP).

### I/O & Networking
* **File**: RAII wrapper around the C file API (`<cstdio>`) with sequential read/write and random access (`read_at` / `write_at`).
* **HTTP_Server**: Minimal HTTP server built on the library primitives (WIP).

## Requirements

* **Standard**: C++20
* **Platform**: Windows (the threading layer currently uses the Win32 API)

## Building

HSTL uses CMake (3.20+). Catch2 is fetched automatically for the test build.

```bash
git clone https://github.com/MHSHM/HSTL.git
cd HSTL
cmake -S . -B build
cmake --build build
```

Options:

| Option | Default | Description |
|--------|---------|-------------|
| `HSTL_BUILD_TESTS` | `ON` | Build the Catch2 unit tests |
| `HSTL_BUILD_PLAYGROUND` | `ON` | Build the playground executable |

Run the tests with `ctest` from the `build` directory.

## Usage Examples

### Parallel work with the job system

Dispatch a batch of jobs onto the lock-free work-stealing scheduler and wait for them to finish. Jobs are pushed to the calling thread's queue and stolen by idle workers.

```cpp
#include "Job_System.h"
#include "Array.h"
#include "Log.h"
#include <atomic>

using namespace hstl;

// Each job squares one value in place.
struct Square_Task { uint32_t* value; };

void square(void* data) {
    Square_Task* t = static_cast<Square_Task*>(data);
    *t->value = (*t->value) * (*t->value);
}

int main() {
    Job_System jobs;                        // one worker per hardware thread

    const uint32_t N = 1024;

    // Containers take an explicit allocator; here, the default one.
    Array<uint32_t>    values(Default_Allocator::get());
    Array<Square_Task> tasks(Default_Allocator::get());
    values.reserve(N);
    tasks.reserve(N);                       // reserve so element pointers stay stable

    std::atomic<int> pending{0};

    for (uint32_t i = 0; i < N; ++i) {
        values.push(i);
        tasks.push({ &values[i] });

        Job job;
        job.function       = square;
        job.data           = &tasks[i];
        job.parent_counter = &pending;      // wait() tracks this counter
        jobs.kick_job(job);                 // lands on this thread's work-stealing queue
    }

    jobs.wait(pending);                     // run and steal jobs until all N finish

    log_info("Squared {} values; values[7] is now {}.", N, values[7]);
    return 0;
}
```

### Allocators, files, and exception-free errors

```cpp
#include "Array.h"
#include "Str.h"
#include "File.h"
#include "Log.h"
#include "Defer.h"

using namespace hstl;

int main() {
    // Custom linear allocator
    Linear_Allocator allocator(1024 * 1024);
    DEFER { allocator.clear(); };           // scope-based cleanup

    // Array backed by the custom allocator
    Array<Str> filenames(&allocator);
    filenames.push(Str("data/config.ini", &allocator));

    // File I/O with Result<T> error handling (no exceptions)
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
```

## License

MIT. See [LICENSE](LICENSE).
