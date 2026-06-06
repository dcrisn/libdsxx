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

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>


// silence false positives from the compiler about
// 'potential null dereference' in std::vector!
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

// Non-trivial, move-only type requested
struct non_copyable {
    std::array<std::uint32_t, 8> v;

    non_copyable() : v {} {}

    non_copyable(unsigned i) { v[0] = i; }

    // Move-only semantics
    non_copyable(const non_copyable &) = delete;
    non_copyable &operator=(const non_copyable &) = delete;

    non_copyable(non_copyable &&other) noexcept { v = std::move(other.v); }

    non_copyable &operator=(non_copyable &&other) noexcept {
        v = other.v;
        return *this;
    }

    ~non_copyable() = default;

    unsigned operator*() const { return v[0]; }
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

// Note for std::vector and tarp::deque, reserve() is called
// before the test; the reason is we want to measure steady
// state not reallocation noise.
template<typename T>
void run_mutation_benchmark(const std::string &type_label,
                            std::uint32_t operations) {
    std::cout << "\n[Workload A] Mutation (Push/Pop Front & Back) - Type: "
              << type_label << " (" << operations << " ops)\n";

    // --- std::vector ---
    {
        std::vector<T> v;
        v.reserve(operations);
        auto start = std::chrono::high_resolution_clock::now();
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
    // --- Custom Contiguous Deque ---
    {
        tarp::deque<T> d;
        d.reserve(operations);
        d.set_autoshrink(false);
        auto start = std::chrono::high_resolution_clock::now();
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
}

template<typename T>
void run_access_benchmark(const std::string &type_label, std::uint32_t size) {
    std::cout << "\n[Workload B & C] Lookup & Iteration - Type: " << type_label
              << " (Size: " << size << ")\n";

    // Setup filled containers
    std::vector<T> v;
    std::deque<T> d;
    tarp::deque<T> cd;
    cd.set_autoshrink(false);

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
            if constexpr (std::is_same_v<T, std::uint32_t>) local_sum += cd[i];
            else local_sum += *(cd[i]);
        }
        checksum += local_sum;
        auto end = std::chrono::high_resolution_clock::now();
        print_row(
          "Custom Deque",
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
}

// Helper to pre-generate a chaotic command sequence to eliminate random-number
// generation overhead inside the loops
template<typename T>
struct QueueOp {
    QueueOp(QueueOp const &op) {
        is_push = op.is_push;
        if constexpr (std::is_trivially_copyable_v<T>) {
            value = op.value;
        } else if constexpr (std::is_same_v<T, non_copyable>) {
            value = T(*op.value);
        }
    }

    QueueOp(bool is_push_, unsigned v) : is_push(is_push_), value(v) {}

    using type = T;
    bool is_push;  // true = push_front, false = pop_back

    T value {};
};

template<typename T>
std::vector<QueueOp<T>>
generate_operations_matrix(std::uint32_t target_pushes) {
    std::vector<QueueOp<T>> ops;
    ops.reserve(target_pushes * 1.5);  // Estimate space

    std::mt19937 rng(1337);  // Fixed seed for identical test runs
    std::uniform_int_distribution<int> dist(0, 10);

    std::uint32_t push_count = 0;
    std::uint32_t current_size = 0;

    std::uint32_t number = 0;

    // Run until we successfully process exactly 10 million pushes
    while (push_count < target_pushes) {
        int roll = dist(rng);

        // 70% chance to push, 30% chance to pop (biased to let the queue grow)
        if (roll < 7 || current_size == 0) {
            ops.push_back({true, number++});
            ++push_count;
            ++current_size;
        } else {
            ops.push_back({false, number++});
            --current_size;
        }
    }
    return ops;
}

template<typename T>
void run_fifo_stress_test(const std::string &type_label,
                          std::uint32_t num_pushes) {
    std::cout << "\n[FIFO Stress] Chaotic Push Back / Pop Front - Type: "
              << type_label << " with " << num_pushes << " pushes\n";

    std::chrono::duration<double, std::milli> deque_total_time {0};
    double deque_total_qlen = 0;

    std::chrono::duration<double, std::milli> custom_deque_total_time {0};
    double custom_deque_total_qlen = 0;


    auto ops = generate_operations_matrix<T>(num_pushes);

    // alternate the order of which deque runs first,
    // otherwise one gets hot cache, the other cold,
    // polluting the results.
    bool first = false;

    constexpr auto NUM_ITERS = 100;
    for (unsigned i = 0; i < NUM_ITERS; ++i) {
        decltype(ops) deque_ops;
        decltype(ops) custom_deque_ops;
        for (const auto &x : ops) {
            deque_ops.emplace_back(x);
            custom_deque_ops.emplace_back(x);
        }
        REQUIRE(ops.size() >= num_pushes);
        REQUIRE(deque_ops.size() == ops.size());
        REQUIRE(deque_ops.size() == custom_deque_ops.size());
        const auto sz = deque_ops.size();
        for (unsigned k = 0; k < sz; ++k) {
            REQUIRE(deque_ops[k].is_push == custom_deque_ops[k].is_push);
        }

        // to avoid std::unique_ptr getting destructed when popping;
        // we are trying to measure deque operations, not memory
        // allocator performance/noise!
        std::deque<T> deque_sink;
        tarp::deque<T> custom_deque_sink;
        custom_deque_sink.reserve(custom_deque_ops.size());

        const auto run_custom_deque = [&]() {
            // --- Custom Contiguous Deque ---
            {
                auto start = std::chrono::high_resolution_clock::now();
                tarp::deque<T> cd;
                // CRITICAL PERFORMANCE POLICIES FOR HIGH-THROUGHPUT:
                cd.set_autoshrink(false);
                cd.reserve(custom_deque_ops.size());

                for (auto &op : custom_deque_ops) {
                    if (op.is_push) {
                        if constexpr (std::is_same_v<T, std::uint32_t>) {
                            cd.push_back(op.value);
                        } else {
                            cd.push_back(std::move(op.value));
                        }
                    } else {
                        REQUIRE(cd.size() > 0);
                        auto &front = cd.front();
                        custom_deque_sink.push_back(std::move(front));
                        cd.pop_front();
                    }
                }

                // --- drain
                while (cd.size() > 0) {
                    custom_deque_sink.push_back(std::move(cd.front()));
                    cd.pop_front();
                }

                auto end = std::chrono::high_resolution_clock::now();
                const auto dur =
                  std::chrono::duration<double, std::milli>(end - start);
                custom_deque_total_time += dur;
                custom_deque_total_qlen += custom_deque_sink.size();
            }
        };

        const auto run_std_deque = [&]() {
            // --- std::deque ---
            {
                auto start = std::chrono::high_resolution_clock::now();
                std::deque<T> d;
                for (auto &op : deque_ops) {
                    if (op.is_push) {
                        if constexpr (std::is_same_v<T, std::uint32_t>) {
                            d.push_back(op.value);
                        } else {
                            d.push_back(std::move(op.value));
                        }
                    } else {
                        REQUIRE(d.size() > 0);
                        auto &front = d.front();
                        deque_sink.push_back(std::move(front));
                        d.pop_front();
                    }
                }

                // --- drain
                while (d.size() > 0) {
                    deque_sink.push_back(std::move(d.front()));
                    d.pop_front();
                }

                auto end = std::chrono::high_resolution_clock::now();
                const auto dur =
                  std::chrono::duration<double, std::milli>(end - start);
                deque_total_time += dur;
                deque_total_qlen += deque_sink.size();
            }
        };

        if (first) {
            run_custom_deque();
            run_std_deque();
        } else {
            run_std_deque();
            run_custom_deque();
        }
        first = !first;

        REQUIRE(custom_deque_sink.size() == deque_sink.size());
    }

    // -- averages ---
    print_row("Custom Deque", (custom_deque_total_time.count() / NUM_ITERS));
    print_row("std::deque", (deque_total_time.count() / NUM_ITERS));
}


#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

TEST_CASE("benchmarks") {
    std::cout << "============================================================="
                 "=========\n";
    std::cout << "              HIGH-PERFORMANCE CONTAINER BENCHMARK METRICS   "
                 "         \n";
    std::cout << "============================================================="
                 "=========\n";

    // Smaller operation matrix for vector mutation paths to avoid multi-minute
    // freezes
    run_mutation_benchmark<std::uint32_t>("std::uint32_t (Trivial)", 10'000);
    run_mutation_benchmark<non_copyable>("non_copyable (Non-Trivial)", 10'000);

    // Memory access tests
    run_access_benchmark<std::uint32_t>("std::uint32_t (Trivial)", 10'000);
    run_access_benchmark<non_copyable>("non_copyable (Non-Trivial)", 10'000);

    //
    //
    //

    std::uint32_t total_pushes = 1'00;
    // Run tests compiled with -O3
    run_fifo_stress_test<std::uint32_t>("std::uint32_t (Trivial)",
                                        total_pushes);
    run_fifo_stress_test<non_copyable>("non_copyable (Non-Trivial)",
                                       total_pushes);


    total_pushes = 200;
    // Run tests compiled with -O3
    run_fifo_stress_test<std::uint32_t>("std::uint32_t (Trivial)",
                                        total_pushes);
    run_fifo_stress_test<non_copyable>("non_copyable (Non-Trivial)",
                                       total_pushes);

    std::cerr << "\n --------- \n";

    total_pushes = 1'000;
    // Run tests compiled with -O3
    run_fifo_stress_test<std::uint32_t>("std::uint32_t (Trivial)",
                                        total_pushes);
    run_fifo_stress_test<non_copyable>("non_copyable (Non-Trivial)",
                                       total_pushes);

    std::cerr << "\n --------- \n";

    total_pushes = 100'000;
    // Run tests compiled with -O3
    run_fifo_stress_test<std::uint32_t>("std::uint32_t (Trivial)",
                                        total_pushes);
    run_fifo_stress_test<non_copyable>("non_copyable (Non-Trivial)",
                                       total_pushes);

    std::cerr << "\n --------- \n";

    total_pushes = 1'000'000;
    // Run tests compiled with -O3
    run_fifo_stress_test<std::uint32_t>("std::uint32_t (Trivial)",
                                        total_pushes);
    run_fifo_stress_test<non_copyable>("non_copyable (Non-Trivial)",
                                       total_pushes);

    std::cerr << "\n --------- \n";
}

// ============================================================================
// MAIN EXECUTION DRIVER
// ============================================================================
int main(int argc, const char **argv) {
    doctest::Context ctx;

    ctx.applyCommandLine(argc, argv);

    int res = ctx.run();

    if (ctx.shouldExit()) {
        // propagate the result of the tests
        return res;
    }

    return 0;
}
