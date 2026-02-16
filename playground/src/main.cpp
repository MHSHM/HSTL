#include "Task.h"
#include "Log.h"
#include "Array.h"
#include <vector>
#include <chrono>
#include <functional>

using namespace hstl;

// Simple workload to prevent compiler optimization
volatile int g_dummy = 0;
void workload(int a, int b) {
    g_dummy += (a + b);
}

template<typename Func>
long long benchmark_creation_and_run(const char* name, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // Create the object
        Func f([=]() { workload(i, i + 1); });

        // Run it immediately
        f();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Using hstl::log_info format from your Log.h
    log_info("{}: {} ns total ({} ns/op)", name, ns, ns / iterations);
    return ns;
}

// Specialization for hstl::Task since it has a different API (invoke vs operator())
long long benchmark_task_creation_and_run(const char* name, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // NOTE: Explicit template argument needed until Deduction Guide is added
        hstl::Task<void> t([=]() { workload(i, i + 1); });
        t.invoke();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    log_info("{}: {} ns total ({} ns/op)", name, ns, ns / iterations);
    return ns;
}

int main() {
    const int ITERATIONS = 1000000000;

    log_info("Benchmarking Task System ({} iterations)...", ITERATIONS);

    // 1. Baseline: Raw Lambda (Direct inlining likely)
    // We can't easily isolate "creation" of a raw lambda in a loop without it being just execution,
    // so this mostly measures the workload itself + loop overhead.
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        auto l = [=]() { workload(i, i + 1); };
        l();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    log_info("Raw Lambda: {} ns total ({} ns/op)", ns, ns / ITERATIONS);

    // 2. std::function (May allocate heap if capture is large, usually SBO for small)
    benchmark_creation_and_run<std::function<void()>>("std::function", ITERATIONS);

    // 3. hstl::Task (Strictly Stack/SBO)
    benchmark_task_creation_and_run("hstl::Task", ITERATIONS);

    return 0;
}
