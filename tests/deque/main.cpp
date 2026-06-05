#include <tarp/deque.hpp>

#include <memory>
#include <random>
#include <type_traits>
#include <iostream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using namespace tarp;

struct LifetimeTracker {
    static inline std::uint64_t active_instances = 0;
    std::uint32_t value = 0;

    LifetimeTracker() : value(0) { ++active_instances; }

    LifetimeTracker(std::uint32_t v) : value(v) { ++active_instances; }

    LifetimeTracker(const LifetimeTracker &o) : value(o.value) {
        ++active_instances;
    }

    LifetimeTracker(LifetimeTracker &&o) noexcept : value(o.value) {
        ++active_instances;
    }

    ~LifetimeTracker() { --active_instances; }

    LifetimeTracker &operator=(const LifetimeTracker &) = default;
    LifetimeTracker &operator=(LifetimeTracker &&) = default;
};

struct ThrowingConstructor {
    // to ensure the code is forced to use the throw-handling
    // paths, not memcpy
    std::string x;
    static_assert(std::is_trivially_copyable_v<std::string> == false);

    static inline bool should_throw = false;
    std::uint32_t value = 0;

    // to prevent compiler from using move ctor
    ThrowingConstructor(ThrowingConstructor &&) = delete;

    ThrowingConstructor(const ThrowingConstructor &) {
        std::cerr << "COPY CONSTRUCTOR called\n";
        if (should_throw)
            throw std::runtime_error("Simulated throwing constructor");
    }

    ThrowingConstructor(int v) : value(v) {
        std::cerr << "VALUE CONSTRUCTOR called\n";
        if (should_throw)
            throw std::runtime_error("Simulated throwing constructor");
    }
};

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


TEST_CASE("default constructor") {
    SUBCASE("trivial type") {
        tarp::deque<unsigned> d;
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
        d.clear();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }
    SUBCASE("non-trivial type") {
        tarp::deque<std::unique_ptr<unsigned>> d;
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
        d.clear();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }
}

TEST_CASE("constructor that default-initializes num elems") {
    SUBCASE("trivial type") {
        tarp::deque<unsigned> d(10);
        REQUIRE(d.size() == 10);
        REQUIRE(d.front() == 0);
        REQUIRE(d.back() == 0);
        for (unsigned i = 0; i < 10; ++i) {
            REQUIRE(d[i] == 0);
        }
        d.clear();
        REQUIRE(d.size() == 0);
    }

    SUBCASE("non-trivial type") {
        tarp::deque<std::unique_ptr<unsigned>> d(10);
        REQUIRE(d.size() == 10);
        REQUIRE(d.front() == nullptr);
        REQUIRE(d.back() == nullptr);
        for (unsigned i = 0; i < 10; ++i) {
            REQUIRE(d[i] == nullptr);
        }
        d.clear();
        REQUIRE(d.size() == 0);
    }
}

TEST_CASE("move constructor") {
    SUBCASE("trivial type") {
        tarp::deque<int> d;
        d.push_back(1);
        d.push_back(2);
        d.push_back(3);
        d.push_back(4);
        d.push_back(5);

        tarp::deque<int> d2(std::move(d));
        REQUIRE(d2.size() == 5);

        REQUIRE(d2[0] == 1);
        REQUIRE(d2[1] == 2);
        REQUIRE(d2[2] == 3);
        REQUIRE(d2[3] == 4);
        REQUIRE(d2[4] == 5);

        REQUIRE(d.size() == 0);
    }

    SUBCASE("nontrivial type") {
        tarp::deque<std::string> d;
        d.push_back("a");
        d.push_back("b");
        d.push_back("c");
        d.push_back("d");
        d.push_back("e");

        tarp::deque<std::string> d2(std::move(d));
        REQUIRE(d2.size() == 5);

        REQUIRE(d2[0] == "a");
        REQUIRE(d2[1] == "b");
        REQUIRE(d2[2] == "c");
        REQUIRE(d2[3] == "d");
        REQUIRE(d2[4] == "e");

        REQUIRE(d.size() == 0);
    }
}

TEST_CASE("move assignment operator") {
    SUBCASE("trivial type") {
        tarp::deque<int> d;
        d.push_back(1);
        d.push_back(2);
        d.push_back(3);
        d.push_back(4);
        d.push_back(5);

        tarp::deque<int> d2;
        REQUIRE(d2.size() == 0);
        d2 = std::move(d);

        REQUIRE(d2.size() == 5);
        REQUIRE(d.size() == 0);

        REQUIRE(d2[0] == 1);
        REQUIRE(d2[1] == 2);
        REQUIRE(d2[2] == 3);
        REQUIRE(d2[3] == 4);
        REQUIRE(d2[4] == 5);
    }

    SUBCASE("nontrivial type") {
        tarp::deque<std::string> d;
        d.push_back("a");
        d.push_back("b");
        d.push_back("c");
        d.push_back("d");
        d.push_back("e");

        tarp::deque<std::string> d2;
        REQUIRE(d2.size() == 0);
        d2 = std::move(d);

        REQUIRE(d2.size() == 5);
        REQUIRE(d.size() == 0);

        REQUIRE(d2[0] == "a");
        REQUIRE(d2[1] == "b");
        REQUIRE(d2[2] == "c");
        REQUIRE(d2[3] == "d");
        REQUIRE(d2[4] == "e");
    }
}

TEST_CASE("copy constructor") {
    SUBCASE("trivial type") {
        tarp::deque<char> d;
        d.push_back('a');
        d.push_back('b');
        d.push_back('c');
        d.push_back('d');
        d.push_back('e');

        tarp::deque<char> d2(d);
        REQUIRE(d.size() == 5);
        REQUIRE(d2.size() == 5);

        REQUIRE(d[0] == 'a');
        REQUIRE(d[1] == 'b');
        REQUIRE(d[2] == 'c');
        REQUIRE(d[3] == 'd');
        REQUIRE(d[4] == 'e');

        REQUIRE(d2[0] == 'a');
        REQUIRE(d2[1] == 'b');
        REQUIRE(d2[2] == 'c');
        REQUIRE(d2[3] == 'd');
        REQUIRE(d2[4] == 'e');
    }
    SUBCASE("nontrivial type") {
        tarp::deque<std::string> d;
        d.push_back("a");
        d.push_back("b");
        d.push_back("c");
        d.push_back("d");
        d.push_back("e");

        tarp::deque<std::string> d2(d);
        REQUIRE(d.size() == 5);
        REQUIRE(d2.size() == 5);

        REQUIRE(d[0] == "a");
        REQUIRE(d[1] == "b");
        REQUIRE(d[2] == "c");
        REQUIRE(d[3] == "d");
        REQUIRE(d[4] == "e");

        REQUIRE(d2[0] == "a");
        REQUIRE(d2[1] == "b");
        REQUIRE(d2[2] == "c");
        REQUIRE(d2[3] == "d");
        REQUIRE(d2[4] == "e");
    }
}

TEST_CASE("copy assignment operator") {
    SUBCASE("trivial type") {
        tarp::deque<char> d;
        d.push_back('a');
        d.push_back('b');
        d.push_back('c');
        d.push_back('d');
        d.push_back('e');

        tarp::deque<char> d2;
        REQUIRE(d2.size() == 0);
        d2 = d;
        REQUIRE(d.size() == 5);
        REQUIRE(d2.size() == 5);

        REQUIRE(d[0] == 'a');
        REQUIRE(d[1] == 'b');
        REQUIRE(d[2] == 'c');
        REQUIRE(d[3] == 'd');
        REQUIRE(d[4] == 'e');

        REQUIRE(d2[0] == 'a');
        REQUIRE(d2[1] == 'b');
        REQUIRE(d2[2] == 'c');
        REQUIRE(d2[3] == 'd');
        REQUIRE(d2[4] == 'e');
    }

    SUBCASE("nontrivial type") {
        tarp::deque<std::string> d;
        d.push_back("a");
        d.push_back("b");
        d.push_back("c");
        d.push_back("d");
        d.push_back("e");

        tarp::deque<std::string> d2;
        REQUIRE(d2.size() == 0);
        d2 = d;
        REQUIRE(d.size() == 5);
        REQUIRE(d2.size() == 5);

        REQUIRE(d[0] == "a");
        REQUIRE(d[1] == "b");
        REQUIRE(d[2] == "c");
        REQUIRE(d[3] == "d");
        REQUIRE(d[4] == "e");

        REQUIRE(d2[0] == "a");
        REQUIRE(d2[1] == "b");
        REQUIRE(d2[2] == "c");
        REQUIRE(d2[3] == "d");
        REQUIRE(d2[4] == "e");
    }
}

TEST_CASE("index operator works across resizing") {
    SUBCASE("trivial type") {
        tarp::deque<int> d;
        d.set_autoshrink(true);

        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
        d.push_back(0);
        REQUIRE(d.size() == 1);
        // jumps to 8 for first allocation
        REQUIRE(d.capacity() == 8);

        REQUIRE(d[0] == 0);

        for (unsigned i = 1; i < 8; ++i) {
            d.push_back(i);
        }

        d.push_back(8);
        REQUIRE(d.size() == 9);
        REQUIRE(d.capacity() == 16);

        for (unsigned i = 9; i < 16; ++i) {
            d.push_back(i);
        }

        REQUIRE(d.size() == 16);
        REQUIRE(d.capacity() == 16);

        for (unsigned i = 0; i < 16; ++i) {
            REQUIRE(d[i] == i);
        }

        // now shrink back down from 16 to 8
        for (unsigned i = 0; i < 13; ++i) {
            d.pop_back();
        }
        REQUIRE(d.size() == 3);
        REQUIRE(d.capacity() == 8);
        for (unsigned i = 0; i < 3; ++i) {
            REQUIRE(d[i] == i);
        }
    }

    SUBCASE("nontrivial type") {
        tarp::deque<non_copyable> d;
        d.set_autoshrink(true);

        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
        d.push_back(0);
        REQUIRE(d.size() == 1);
        // jumps to 8 for first allocation
        REQUIRE(d.capacity() == 8);

        REQUIRE(*d[0] == 0);

        for (unsigned i = 1; i < 8; ++i) {
            d.push_back(i);
        }

        d.push_back(8);
        REQUIRE(d.size() == 9);
        REQUIRE(d.capacity() == 16);

        for (unsigned i = 9; i < 16; ++i) {
            d.push_back(i);
        }

        REQUIRE(d.size() == 16);
        REQUIRE(d.capacity() == 16);

        for (unsigned i = 0; i < 16; ++i) {
            REQUIRE(*d[i] == i);
        }

        // now shrink back down from 16 to 8
        for (unsigned i = 0; i < 13; ++i) {
            d.pop_back();
        }
        REQUIRE(d.size() == 3);
        REQUIRE(d.capacity() == 8);
        for (unsigned i = 0; i < 3; ++i) {
            REQUIRE(*d[i] == i);
        }
    }
}

TEST_CASE("push and pop at both ends") {
    SUBCASE("trivial type") {
        tarp::deque<unsigned> d;
        d.push_back(1);
        d.push_back(2);
        d.push_back(3);
        d.push_back(4);
        d.push_back(5);
        REQUIRE(d[0] == 1);
        REQUIRE(d[1] == 2);
        REQUIRE(d[2] == 3);
        REQUIRE(d[3] == 4);
        REQUIRE(d[4] == 5);

        d.push_front(0);
        REQUIRE(d.size() == 6);
        for (unsigned i = 0; i < 6; ++i) {
            REQUIRE(d[i] == i);
        }

        d.push_front(1);
        d.push_front(2);
        d.push_front(3);
        d.push_front(4);
        d.push_front(5);
        REQUIRE(d.size() == 11);
        for (unsigned i = 0; i < 5; ++i) {
            d.pop_back();
        }
        REQUIRE(d.size() == 6);
        for (unsigned i = 0; i < 6; ++i) {
            REQUIRE(d[i] == 5 - i);
        }
    }

    SUBCASE("nontrivial type") {
        tarp::deque<non_copyable> d;
        d.push_back(1);
        d.push_back(2);
        d.push_back(3);
        d.push_back(4);
        d.push_back(5);
        REQUIRE(*d[0] == 1);
        REQUIRE(*d[1] == 2);
        REQUIRE(*d[2] == 3);
        REQUIRE(*d[3] == 4);
        REQUIRE(*d[4] == 5);

        d.push_front(0);
        REQUIRE(d.size() == 6);
        for (unsigned i = 0; i < 6; ++i) {
            REQUIRE(*d[i] == i);
        }

        d.push_front(1);
        d.push_front(2);
        d.push_front(3);
        d.push_front(4);
        d.push_front(5);
        REQUIRE(d.size() == 11);
        for (unsigned i = 0; i < 5; ++i) {
            d.pop_back();
        }
        REQUIRE(d.size() == 6);
        for (unsigned i = 0; i < 6; ++i) {
            REQUIRE(*d[i] == 5 - i);
        }
    }
}

TEST_CASE("Basic Push, Pop, and Power-of-Two Index Wrapping") {
    tarp::deque<int> d;

    REQUIRE(d.size() == 0);
    REQUIRE(d.capacity() == 0);

    // Fill past initial capacity default boundary
    for (int i = 0; i < 10; ++i) {
        d.push_back(i);
    }
    REQUIRE(d.size() == 10);
    REQUIRE(d.capacity() == 16);  // std::bit_ceil(10)
    REQUIRE(d.front() == 0);
    REQUIRE(d.back() == 9);

    // Induce a wrapped memory layout state by dropping elements from front
    for (int i = 0; i < 5; ++i) {
        REQUIRE(d.front() == i);
        d.pop_front();
    }
    REQUIRE(d.size() == 5);
    REQUIRE(d.front() == 5);
    REQUIRE(d.back() == 9);

    // Push back over the array bounds to execute bitwise wrap masking logic
    for (int i = 10; i < 15; ++i) {
        d.push_back(i);
    }
    REQUIRE(d.size() == 10);

    // Ensure relative random access mapping bypasses physical memory layout
    // shifts
    REQUIRE(d[0] == 5);
    REQUIRE(d[9] == 14);
    REQUIRE(d.front() == 5);
    REQUIRE(d.back() == 14);

    // with autoshrink=off, there is no shrinking.
    REQUIRE(d.capacity() == 16);
}

TEST_CASE("Rule of Five Memory Lifecycle Management") {
    LifetimeTracker::active_instances = 0;

    SUBCASE("Deep Copy Construction") {
        deque<LifetimeTracker> d1;
        d1.emplace_back(10);
        d1.emplace_front(20);
        REQUIRE(LifetimeTracker::active_instances == 2);

        deque<LifetimeTracker> d2(d1);
        REQUIRE(d2.size() == 2);
        REQUIRE(LifetimeTracker::active_instances == 4);
        REQUIRE(d2[0].value == 20);
    }
    // Memory drops cleanly out of subcase scope
    REQUIRE(LifetimeTracker::active_instances == 0);

    SUBCASE("O(1) Move Semantics and Theft Verification") {
        deque<LifetimeTracker> d1;
        d1.emplace_back(10);
        d1.emplace_back(20);

        deque<LifetimeTracker> d2(std::move(d1));
        REQUIRE(d2.size() == 2);
        REQUIRE(d1.size() == 0);  // Content stolen
        REQUIRE(d1.capacity() == 0);
        REQUIRE(LifetimeTracker::active_instances == 2);
    }
    REQUIRE(LifetimeTracker::active_instances == 0);

    SUBCASE("Copy and Move Assignment Operators") {
        deque<LifetimeTracker> d1;
        d1.emplace_back(10);

        deque<LifetimeTracker> d2;
        d2 = d1;  // Copy assignment
        REQUIRE(d2.size() == 1);
        REQUIRE(LifetimeTracker::active_instances == 2);

        deque<LifetimeTracker> d3;
        d3 = std::move(d2);  // Move assignment
        REQUIRE(d3.size() == 1);
        REQUIRE(d2.size() == 0);
        REQUIRE(LifetimeTracker::active_instances == 2);
    }
    REQUIRE(LifetimeTracker::active_instances == 0);
}

TEST_CASE("Strong Exception Safety Rules (Rollback Checks)") {
    ThrowingConstructor::should_throw = false;

    deque<ThrowingConstructor> d;
    d.emplace_back(1);
    d.emplace_back(2);
    d.emplace_back(3);

    ThrowingConstructor::should_throw = true;  // Activate fault injector

    SUBCASE("Emplace/Push Failure leaves container untouched") {
        REQUIRE_THROWS_AS(d.emplace_back(4), std::runtime_error);

        // Container state must be completely preserved
        REQUIRE(d.size() == 3);
        REQUIRE(d[0].value == 1);
        REQUIRE(d.front().value == 1);
        REQUIRE(d[2].value == 3);
        REQUIRE(d.back().value == 3);
    }

    SUBCASE("Reserve/Growth Multi-element allocation failure handling") {
        std::cerr << "HERE; cap=" << d.capacity() << std::endl;
        REQUIRE(d.capacity() < 32);
        REQUIRE_THROWS_AS(d.reserve(32), std::runtime_error);

        // Rollback must restore original memory structure flawlessly
        REQUIRE(d.size() == 3);
        REQUIRE(d[0].value == 1);
        REQUIRE(d.front().value == 1);
        REQUIRE(d[2].value == 3);
        REQUIRE(d.back().value == 3);
    }

    ThrowingConstructor::should_throw = false;  // Teardown trigger state
}

TEST_CASE("Autoshrink Mechanics and Manual Sizing") {
    deque<int> d;
    d.set_autoshrink(true);

    for (int i = 0; i < 32; ++i) d.push_back(i);
    std::uint32_t baseline_capacity = d.capacity();

    // Evict items down past 1/4 allocation threshold
    while (d.size() > (baseline_capacity >> 3)) {
        d.pop_front();
    }

    // Auto allocation reduction pass must fire
    REQUIRE(d.capacity() < baseline_capacity);

    // Explicit fit compaction
    d.clear();
    d.push_back(1);
    d.push_back(2);
    d.push_back(3);

    d.shrink_to_fit();
    // Clamped strictly to power-of-two (std::bit_ceil(3))
    REQUIRE(d.capacity() == 4);
}

TEST_CASE("Compile-Time Trivial Data Path Optimization") {
    // Double is trivially copyable, compiling out item loops
    // into standard memcpy blocks
    deque<double> d;
    for (int i = 0; i < 100; ++i) {
        d.push_back(static_cast<double>(i));
    }

    // Hits the raw memcpy optimization branch internally
    d.reserve(256);

    REQUIRE(d.size() == 100);
    REQUIRE(d[0] == 0.0);
    REQUIRE(d[99] == 99.0);
}

TEST_CASE("reserve avoids reallocations") {
    tarp::deque<unsigned> d;
    d.reserve(1000);
    REQUIRE(d.capacity() == 1024);
    REQUIRE(d.size() == 0);
    for (unsigned i = 0; i < 1024; ++i) {
        d.push_back(i);
    }
    REQUIRE(d.capacity() == 1024);
    REQUIRE(d.size() == 1024);
    REQUIRE(d.front() == 0);
    REQUIRE(d.back() == 1023);
}

TEST_CASE("autoshrink=off means memory does not shrink") {
    tarp::deque<unsigned> d;
    d.set_autoshrink(false);

    d.reserve(1000);
    REQUIRE(d.capacity() == 1024);
    REQUIRE(d.size() == 0);
    for (unsigned i = 0; i < 1024; ++i) {
        d.push_back(i);
    }
    REQUIRE(d.capacity() == 1024);
    REQUIRE(d.size() == 1024);
    REQUIRE(d.front() == 0);
    REQUIRE(d.back() == 1023);

    for (unsigned i = 0; i < 1000; ++i) {
        d.pop_back();
    }

    // no shrinkage
    REQUIRE(d.capacity() == 1024);
    REQUIRE(d.size() == 24);
    REQUIRE(d.front() == 0);
    REQUIRE(d.back() == 23);

    d.set_autoshrink(true);
    d.pop_front();
    REQUIRE(d.capacity() == 64);
    REQUIRE(d.size() == 23);
    REQUIRE(d.front() == 1);
    REQUIRE(d.back() == 23);
}

TEST_CASE("clear does not release memory; reset does") {
    SUBCASE("trivial type") {
        tarp::deque<unsigned> d;
        for (unsigned i = 0; i < 500; ++i) {
            d.push_front(i);
        }
        d.clear();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 512);

        for (unsigned i = 0; i < 500; ++i) {
            d.push_front(i);
        }
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 512);
        d.reset();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }

    SUBCASE("non-trivial type") {
        tarp::deque<non_copyable> d;
        for (unsigned i = 0; i < 500; ++i) {
            d.push_front(i);
        }
        d.clear();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 512);

        for (unsigned i = 0; i < 500; ++i) {
            d.push_front(i);
        }
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 512);
        d.reset();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }
}

TEST_CASE("shrink to fit shrinks to nearest greater power of 2") {
    SUBCASE("trivial type") {
        tarp::deque<unsigned> d;
        d.set_autoshrink(false);
        for (unsigned i = 0; i < 8000; ++i) {
            d.push_back(i);
        }
        REQUIRE(d.size() == 8000);
        REQUIRE(d.capacity() == 8192);
        d.shrink_to_fit();
        // unchanged
        REQUIRE(d.size() == 8000);
        REQUIRE(d.capacity() == 8192);

        d.clear();
        for (unsigned i = 0; i < 500; ++i) {
            d.push_back(i);
        }
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 8192);
        d.shrink_to_fit();
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 512);

        d.clear();
        d.push_back(1);
        d.shrink_to_fit();
        REQUIRE(d.size() == 1);
        REQUIRE(d.capacity() == 1);
        REQUIRE(d.back() == 1);
        REQUIRE(d.front() == 1);

        d.clear();
        d.shrink_to_fit();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }
    SUBCASE("nontrivial type") {
        tarp::deque<non_copyable> d;
        d.set_autoshrink(false);
        for (unsigned i = 0; i < 8000; ++i) {
            d.push_back(i);
        }
        REQUIRE(d.size() == 8000);
        REQUIRE(d.capacity() == 8192);
        d.shrink_to_fit();
        // unchanged
        REQUIRE(d.size() == 8000);
        REQUIRE(d.capacity() == 8192);

        d.clear();
        for (unsigned i = 0; i < 500; ++i) {
            d.push_back(i);
        }
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 8192);
        d.shrink_to_fit();
        REQUIRE(d.size() == 500);
        REQUIRE(d.capacity() == 512);

        d.clear();
        d.push_back(1);
        d.shrink_to_fit();
        REQUIRE(d.size() == 1);
        REQUIRE(d.capacity() == 1);
        REQUIRE(*d.back() == 1);
        REQUIRE(*d.front() == 1);

        d.clear();
        d.shrink_to_fit();
        REQUIRE(d.size() == 0);
        REQUIRE(d.capacity() == 0);
    }
}

template<typename T>
T pick(T min, T max) {
    static thread_local std::mt19937_64 engine {std::random_device {}()};
    if constexpr (std::is_integral_v<std::decay_t<T>>) {
        return std::uniform_int_distribution<T> {min, max}(engine);
    } else {
        return std::uniform_real_distribution<T> {min, max}(engine);
    }
}

bool toss() {
    return pick(0, 1) == 1;
}

// silence false positives from the compiler about
// 'potential null dereference' in std::vector!
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

TEST_CASE_TEMPLATE("fuzz insertions at ends",
                   Type,
                   std::uint32_t,
                   non_copyable) {
    SUBCASE("trivial type") {
        std::vector<Type> oracle;
        tarp::deque<Type> d;
        // constexpr auto N = 1'000'000;
        constexpr auto N = 100'000;
        for (unsigned i = 0; i < N; ++i) {
            bool push = toss();
            if (push || d.size() == 0) {
                const bool front = toss();
                const auto value = pick<std::uint32_t>(
                  0, std::numeric_limits<std::uint32_t>::max());
                if (front) {
                    std::cerr << "PUSH FRONT\n";
                    oracle.insert(oracle.begin(), Type(value));
                    d.push_front(Type(value));
                } else {
                    std::cerr << "PUSH BACK\n";
                    oracle.push_back(Type(value));
                    d.push_back(Type(value));
                }
            } else {
                const bool back = toss();
                if (back) {
                    std::cerr << "POP BACK\n";
                    oracle.pop_back();
                    d.pop_back();
                } else {
                    std::cerr << "POP FRONT\n";
                    oracle.erase(oracle.begin());
                    d.pop_front();
                }
            }

            REQUIRE(d.size() == oracle.size());
            if (d.size() > 0) {
                if constexpr (std::is_same_v<Type, std::uint32_t>) {
                    REQUIRE(d.front() == oracle.front());
                    REQUIRE(d.back() == oracle.back());
                    for (unsigned j = 0; j < d.size(); ++j) {
                        REQUIRE(d[j] == oracle[j]);
                    }
                } else if constexpr (std::is_same_v<Type, non_copyable>) {
                    REQUIRE(*d.front() == *oracle.front());
                    REQUIRE(*d.back() == *oracle.back());
                    for (unsigned j = 0; j < d.size(); ++j) {
                        REQUIRE(*d[j] == *oracle[j]);
                    }
                } else {
                    static_assert(false);
                }
            }
        }
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif



int main(int argc, char *argv[]) {
    doctest::Context ctx;

    ctx.applyCommandLine(argc, argv);

    int res = ctx.run();

    if (ctx.shouldExit()) {
        // propagate the result of the tests
        return res;
    }

    return res;
}
