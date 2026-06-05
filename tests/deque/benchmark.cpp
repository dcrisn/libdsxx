#include <tarp/deque.hpp>

#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

// silence false positives from the compiler about
// 'potential null dereference' in std::vector!
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

// Non-trivial, move-only type requested
struct non_copyable {
    std::unique_ptr<unsigned> v;

    non_copyable() : v(std::make_unique<unsigned>(0)) {}

    non_copyable(unsigned i) : v(std::make_unique<unsigned>(i)) {}

    // Move-only semantics
    non_copyable(const non_copyable &) = delete;
    non_copyable &operator=(const non_copyable &) = delete;

    non_copyable(non_copyable &&) noexcept = default;
    non_copyable &operator=(non_copyable &&) noexcept = default;

    ~non_copyable() = default;

    unsigned operator*() const { return v ? *v : 0; }
};

// Simple pseudo-random generator wrapper for stable cache miss tests
struct LinearCongruentialGenerator {
    std::uint32_t state;

    inline std::uint32_t next(std::uint32_t limit) {
        state = state * 1664525 + 1013904223;
        return state % limit;
    }
};

// Helper macro/formatter to output uniform benchmark results
void print_row(const std::string &container_name, double duration_ms) {
    std::cout << "  " << std::left << std::setw(18) << container_name << ": "
              << std::fixed << std::setprecision(4) << duration_ms << " ms\n";
}

// ============================================================================
// BENCHMARK ROUTINES
// ============================================================================

template<typename T>
void run_mutation_benchmark(const std::string &type_label,
                            std::uint32_t operations) {
    std::cout << "\n[Workload A] Mutation (Push/Pop Front & Back) - Type: "
              << type_label << " (" << operations << " ops)\n";

    // --- std::vector ---
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<T> v;
        for (std::uint32_t i = 0; i < operations / 4; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) {
                v.push_back(i);
                // Vector must shift all elements on front actions (Catastrophic
                // O(N))
                v.insert(v.begin(), i);
            } else {
                v.push_back(T(i));
                v.insert(v.begin(), T(i));
            }
        }
        while (!v.empty()) {
            v.pop_back();
            if (!v.empty()) v.erase(v.begin());
        }
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::vector",
          std::chrono::duration<double, std::milli>(end - start).count());
    }

    // --- std::deque ---
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::deque<T> d;
        for (std::uint32_t i = 0; i < operations / 4; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) {
                d.push_back(i);
                d.push_front(i);
            } else {
                d.push_back(T(i));
                d.push_front(T(i));
            }
        }
        while (!d.empty()) {
            d.pop_back();
            if (!d.empty()) d.pop_front();
        }
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }

    // --- Custom Contiguous Deque ---
    {
        auto start = std::chrono::high_resolution_clock::now();
        tarp::deque<T> d;
        for (std::uint32_t i = 0; i < operations / 4; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) {
                d.push_back(i);
                d.push_front(i);
            } else {
                d.push_back(T(i));
                d.push_front(T(i));
            }
        }
        while (!d.empty()) {
            d.pop_back();
            if (!d.empty()) d.pop_front();
        }
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "tarp::deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
}

template<typename T>
void run_access_benchmark(const std::string &type_label, std::uint32_t size) {
    std::cout << "\n[Workload B & C] Lookup & Iteration - Type: " << type_label
              << " (Size: " << size << ")\n";

    // Setup filled containers
    std::vector<T> v;
    std::deque<T> d;
    tarp::deque<T> cd;

    for (std::uint32_t i = 0; i < size; ++i) {
        if constexpr (std::is_same_v<T, std::uint32_t>) {
            v.push_back(i);
            d.push_back(i);
            cd.push_back(i);
        } else {
            v.push_back(T(i));
            d.push_back(T(i));
            cd.push_back(T(i));
        }
    }

    // Volatile sink ensures compiler optimizations don't completely erase
    // access operations
    volatile std::uint64_t checksum = 0;

    // --- WORKLOAD B: SEQUENTIAL INDEX ACCESS ---
    std::cout << "  -> Sub-task: Sequential Lookup ( operator[] ):\n";
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < size; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += v[i];
            else local_sum += *(v[i]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::vector",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < size; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += d[i];
            else local_sum += *(d[i]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < size; ++i) {
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += cd[i];
            else local_sum += *(cd[i]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "Custom Deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }

    // --- WORKLOAD C: RANDOM ACCESS (CACHE MISS INDUCTION) ---
    std::cout << "  -> Sub-task: Random Index Lookup:\n";
    const std::uint32_t lookup_count = size * 2;

    {
        LinearCongruentialGenerator lcg {42};
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < lookup_count; ++i) {
            auto idx = lcg.next(size);
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += v[idx];
            else local_sum += *(v[idx]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::vector",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
    {
        LinearCongruentialGenerator lcg {42};
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < lookup_count; ++i) {
            auto idx = lcg.next(size);
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += d[idx];
            else local_sum += *(d[idx]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "std::deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
    {
        LinearCongruentialGenerator lcg {42};
        auto start = std::chrono::high_resolution_clock::now();
        std::uint64_t local_sum = 0;
        for (std::uint32_t i = 0; i < lookup_count; ++i) {
            auto idx = lcg.next(size);
            if constexpr (std::is_same_v<T, std::uint32_t>)
                local_sum += cd[idx];
            else local_sum += *(cd[idx]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "Custom Deque",
          std::chrono::duration<double, std::milli>(end - start).count());
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


// ============================================================================
// MAIN EXECUTION DRIVER
// ============================================================================
int main() {
    std::cout << "============================================================="
                 "=========\n";
    std::cout << "              HIGH-PERFORMANCE CONTAINER BENCHMARK METRICS   "
                 "         \n";
    std::cout << "============================================================="
                 "=========\n";

    // Smaller operation matrix for vector mutation paths to avoid multi-minute
    // freezes
    run_mutation_benchmark<std::uint32_t>("std::uint32_t (Trivial)", 500'000);
    run_mutation_benchmark<non_copyable>("non_copyable (Non-Trivial)", 500'000);

    // Memory access tests (Vector can flex here)
    run_access_benchmark<std::uint32_t>("std::uint32_t (Trivial)", 1'000'000);
    run_access_benchmark<non_copyable>("non_copyable (Non-Trivial)", 1'000'000);

    return 0;
}
