#include <tarp/lru_cache.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <memory>

using namespace tarp;

TEST_CASE("tarp::lru_cache basic behavior") {
    SUBCASE("insert & get: value retrieval and size") {
        lru_cache<int, std::string> c(2);
        c.put(1, "one");
        c.put(2, "two");

        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));

        REQUIRE(c.size() == 2);
        auto *v1 = c.get(1);
        REQUIRE(v1 != nullptr);
        CHECK(*v1 == "one");
        auto *v2 = c.get(2);
        REQUIRE(v2 != nullptr);
        CHECK(*v2 == "two");
    }

    SUBCASE("eviction favors LRU; get() refreshes recency") {
        lru_cache<int, std::string> c(2);
        c.put(1, "one");    // LRU = 1
        c.put(2, "two");    // MRU = 2
        (void)c.get(1);     // 1 becomes MRU, 2 is now LRU
        c.put(3, "three");  // evicts key 2
        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));
        CAPTURE(c.contains(3));
        REQUIRE(c.size() == 2);
        CHECK(c.contains(1));
        CHECK_FALSE(c.contains(2));
        CHECK(c.contains(3));
    }

    SUBCASE("peek() does not refresh recency") {
        lru_cache<int, std::string> c(2);
        c.put(1, "one");      // LRU = 1
        c.put(2, "two");      // MRU = 2
        auto *p = c.peek(1);  // peek should NOT change order
        REQUIRE(p != nullptr);
        CHECK(*p == "one");
        c.put(3, "three");  // should evict key 1 (still LRU)
        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));
        CAPTURE(c.contains(3));
        CHECK_FALSE(c.contains(1));
        CHECK(c.contains(2));
        CHECK(c.contains(3));
    }

    SUBCASE("put() on existing key updates value and refreshes recency") {
        lru_cache<int, std::string> c(2);
        c.put(1, "one");
        c.put(2, "two");
        c.put(1, "ONE");  // update + move to MRU; LRU becomes key 2
        auto *v1 = c.get(1);
        REQUIRE(v1 != nullptr);
        CHECK(*v1 == "ONE");
        c.put(3, "three");  // should evict key 2
        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));
        CAPTURE(c.contains(3));
        CHECK(c.contains(1));
        CHECK_FALSE(c.contains(2));
        CHECK(c.contains(3));
    }

    SUBCASE("erase() removes entries; clear() clears all") {
        lru_cache<int, std::string> c(3);
        c.put(1, "one");
        c.put(2, "two");
        c.put(3, "three");
        CHECK(c.erase(2));
        CHECK_FALSE(c.contains(2));
        CHECK_FALSE(c.erase(2));  // already gone
        c.clear();
        CHECK(c.size() == 0);
        CHECK_FALSE(c.contains(1));
        CHECK_FALSE(c.contains(3));
    }

    SUBCASE("supports move-only types (e.g., unique_ptr)") {
        lru_cache<int, std::unique_ptr<int>> c(2);
        c.put(1, std::make_unique<int>(11));
        c.put(2, std::make_unique<int>(22));
        auto *p1 = c.get(1);
        REQUIRE(p1);
        REQUIRE(*p1);
        REQUIRE(**p1 == 11);
        // update existing key with a new unique_ptr and ensure it sticks
        c.put(1, std::make_unique<int>(111));
        auto *p1u = c.get(1);
        REQUIRE(p1u);
        REQUIRE(*p1u);
        REQUIRE(**p1u == 111);
        // force eviction of LRU (which should now be key 2)
        c.put(3, std::make_unique<int>(33));
        CHECK_FALSE(c.contains(2));
        CHECK(c.contains(1));
        CHECK(c.contains(3));
    }

    SUBCASE("capacity==0 policy: cache stores nothing") {
        lru_cache<int, int> c(0);
        c.put(1, 10);
        c.put(2, 20);
        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));
        CHECK(c.size() == 0);
        CHECK(c.get(1) == nullptr);
        CHECK_FALSE(c.contains(1));
        CHECK_FALSE(c.contains(2));
    }
}

TEST_CASE("value-less lru_cache") {
    SUBCASE("insert & get: value retrieval and size") {
        lru_cache<int, void> c(2);
        c.put(1);
        c.put(2);

        CAPTURE(c.size());
        CAPTURE(c.contains(1));
        CAPTURE(c.contains(2));

        REQUIRE(c.size() == 2);
    }
}

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
