#include <sstream>
#include <tarp/disjoint_range_tree.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <random>

using namespace tarp;

TEST_CASE("can insert one") {
    disjoint_range_tree<std::uint32_t> t;

    t.add_range(1, 4);
    REQUIRE(t.contains(0) == false);
    REQUIRE(t.contains(1) == true);
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE(t.contains(4) == true);
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4));
    REQUIRE(t.contains(1, 1));
    REQUIRE(t.contains(1, 2));
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(4, 4));
    REQUIRE(t.contains(4, 5) == false);
    REQUIRE(t.contains(5, 5) == false);

    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 4);
}

TEST_CASE("can insert the same exact range twice (NOP)") {
    disjoint_range_tree<std::uint32_t> t;

    t.add_range(1, 4);
    REQUIRE(t.contains(0) == false);
    REQUIRE(t.contains(1) == true);
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE(t.contains(4) == true);
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4));
    REQUIRE(t.contains(1, 1));
    REQUIRE(t.contains(1, 2));
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(4, 4));
    REQUIRE(t.contains(4, 5) == false);
    REQUIRE(t.contains(5, 5) == false);

    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 4);

    // insert again; should be unchanged
    t.add_range(1, 4);
    REQUIRE(t.contains(0) == false);
    REQUIRE(t.contains(1) == true);
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE(t.contains(4) == true);
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4));
    REQUIRE(t.contains(1, 1));
    REQUIRE(t.contains(1, 2));
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(4, 4));
    REQUIRE(t.contains(4, 5) == false);
    REQUIRE(t.contains(5, 5) == false);

    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 4);

    // and again; should be unchanged
    t.add_range(1, 4);
    REQUIRE(t.contains(0) == false);
    REQUIRE(t.contains(1) == true);
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE(t.contains(4) == true);
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4));
    REQUIRE(t.contains(1, 1));
    REQUIRE(t.contains(1, 2));
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(4, 4));
    REQUIRE(t.contains(4, 5) == false);
    REQUIRE(t.contains(5, 5) == false);

    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 4);
}

TEST_CASE("insertion with same key but different value") {
    disjoint_range_tree<std::uint32_t> t;
    t.add_range(5, 10);
    // adding with _smaller range_ should be a NOP
    t.add_range(5, 7);
    REQUIRE(t.contains(5, 10));
    REQUIRE(t.size() == 6);

    t.add_range(15, 20);

    // adding with _bigger_ range should extend the range
    // and perform merges if needed
    t.add_range(5, 14);
    REQUIRE(t.contains(5, 20));
    REQUIRE(t.size() == 16);
}

TEST_CASE("can insert non-overlapping ranges ") {
    disjoint_range_tree<std::uint32_t> t;

    t.add_range(0);
    t.add_range(5, 6);
    t.add_range(8, 9);
    t.add_range(100, 150);
    REQUIRE(t.range_count() == 4);
    REQUIRE(t.size() == 56);

    std::cerr << t.str() << std::endl;
    REQUIRE(t.contains(0) == true);
    REQUIRE(t.contains(1) == false);
    REQUIRE(t.contains(2) == false);
    REQUIRE(t.contains(4) == false);
    REQUIRE(t.contains(5) == true);
    REQUIRE(t.contains(6) == true);
    REQUIRE(t.contains(7) == false);
    REQUIRE(t.contains(99) == false);
    REQUIRE(t.contains(100) == true);
    REQUIRE(t.contains(149) == true);
    REQUIRE(t.contains(150) == true);
    REQUIRE(t.contains(151) == false);
}

TEST_CASE("insertion of adjacent or overlapping ranges causes merge") {
    SUBCASE("adjacent") {
        disjoint_range_tree<std::uint32_t> t;
        t.add_range(0);
        t.add_range(1, 2);
        REQUIRE_MESSAGE(t.range_count() == 1, t.str());
        REQUIRE(t.size() == 3);

        std::cerr << t.str() << std::endl;
        REQUIRE(t.contains(0) == true);
        REQUIRE(t.contains(1) == true);
        REQUIRE(t.contains(2) == true);
        REQUIRE(t.contains(3) == false);

        REQUIRE(t.contains(10) == false);
        t.add_range(10);
        REQUIRE(t.contains(10) == true);
        t.add_range(5, 9);
        REQUIRE(t.contains(4) == false);
        REQUIRE(t.contains(5) == true);
        REQUIRE(t.contains(9) == true);
        REQUIRE(t.contains(10) == true);

        t.add_range(3, 4);
        REQUIRE(t.contains(0) == true);
        REQUIRE(t.contains(1) == true);
        REQUIRE(t.contains(2) == true);
        REQUIRE(t.contains(3) == true);
        REQUIRE(t.contains(4) == true);
        REQUIRE(t.contains(5) == true);
        REQUIRE(t.contains(6) == true);
        REQUIRE(t.contains(7) == true);
        REQUIRE(t.contains(8) == true);
        REQUIRE(t.contains(9) == true);
        REQUIRE(t.contains(10) == true);

        std::cerr << t.str() << std::endl;
    }

    SUBCASE("overlapping") {
        disjoint_range_tree<std::uint8_t> t;
        t.add_range(5, 7);
        t.add_range(5, 8);
        REQUIRE_MESSAGE(t.range_count() == 1, t.str());
        REQUIRE(t.size() == 4);

        t.add_range(2, 5);
        REQUIRE_MESSAGE(t.range_count() == 1, t.str());
        REQUIRE(t.size() == 7);

        t.add_range(20, 25);
        t.add_range(30, 35);
        t.add_range(15, 16);
        REQUIRE(t.range_count() == 4);
        REQUIRE(t.size() == 21);

        t.add_range(4, 39);
        REQUIRE(t.range_count() == 1);
        REQUIRE(t.size() == 38);

        t.add_range(1, 39);
        REQUIRE(t.range_count() == 1);
        REQUIRE(t.size() == 39);

        REQUIRE(t.contains(0) == false);
        REQUIRE(t.contains(1));
        REQUIRE(t.contains(2));
        REQUIRE(t.contains(5));
        REQUIRE(t.contains(6));
        REQUIRE(t.contains(7));
        REQUIRE(t.contains(8));
        REQUIRE(t.contains(10));
        REQUIRE(t.contains(11));
        REQUIRE(t.contains(12));
        REQUIRE(t.contains(20));
        REQUIRE(t.contains(21));
        REQUIRE(t.contains(23));
        REQUIRE(t.contains(38));
        REQUIRE(t.contains(39));

        std::cerr << t.str() << std::endl;
    }
}

TEST_CASE("remove") {
    disjoint_range_tree<std::uint16_t> t;

    t.add_range(1, 4);

    // removing nonexistent range returns false
    REQUIRE_FALSE(t.remove(0));
    REQUIRE_FALSE(t.remove(5));
    REQUIRE_FALSE(t.remove(500));

    // case: trim lower edge
    REQUIRE(t.remove(1));
    REQUIRE(t.contains(0) == false);
    REQUIRE_MESSAGE(t.contains(1) == false, t.str());
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE(t.contains(4) == true);
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4) == false);
    REQUIRE(t.contains(1, 1) == false);
    REQUIRE(t.contains(1, 2) == false);
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(4, 4));
    REQUIRE(t.contains(4, 5) == false);
    REQUIRE(t.contains(5, 5) == false);
    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 3);

    // case: trim upper edge
    REQUIRE(t.remove(4));
    REQUIRE(t.contains(0) == false);
    REQUIRE_MESSAGE(t.contains(1) == false, t.str());
    REQUIRE(t.contains(2) == true);
    REQUIRE(t.contains(3) == true);
    REQUIRE_MESSAGE(t.contains(4) == false, t.str());
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4) == false);
    REQUIRE(t.contains(1, 1) == false);
    REQUIRE(t.contains(1, 2) == false);
    REQUIRE(t.contains(2, 2));
    REQUIRE(t.contains(2, 3));
    REQUIRE(t.contains(3, 3));
    REQUIRE(t.contains(3, 4) == false);
    REQUIRE(t.contains(4, 4) == false);
    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 2);

    // case: exact range erase
    REQUIRE(t.remove(2, 3));
    REQUIRE(t.contains(0) == false);
    REQUIRE_MESSAGE(t.contains(1) == false, t.str());
    REQUIRE(t.contains(2) == false);
    REQUIRE(t.contains(3) == false);
    REQUIRE_MESSAGE(t.contains(4) == false, t.str());
    REQUIRE(t.contains(5) == false);
    REQUIRE(t.contains(0, 1) == false);
    REQUIRE(t.contains(0, 0) == false);
    REQUIRE(t.contains(1, 4) == false);
    REQUIRE(t.contains(1, 1) == false);
    REQUIRE(t.contains(1, 2) == false);
    REQUIRE(t.contains(2, 2) == false);
    REQUIRE(t.contains(2, 3) == false);
    REQUIRE(t.contains(3, 3) == false);
    REQUIRE(t.contains(3, 4) == false);
    REQUIRE(t.contains(4, 4) == false);
    REQUIRE(t.range_count() == 0);
    REQUIRE(t.size() == 0);

    t.add_range(20, 27);
    REQUIRE(t.range_count() == 1);
    REQUIRE(t.size() == 8);

    // case: split
    REQUIRE(t.remove(23, 23));
    REQUIRE(t.range_count() == 2);
    REQUIRE(t.size() == 7);
    REQUIRE(t.contains(19) == false);
    REQUIRE(t.contains(20));
    REQUIRE(t.contains(21));
    REQUIRE(t.contains(22));
    REQUIRE(t.contains(23) == false);
    REQUIRE(t.contains(24));
    REQUIRE(t.contains(25));
    REQUIRE(t.contains(26));
    REQUIRE(t.contains(27));
    REQUIRE(t.contains(28) == false);

    // case: overlap at lower edge
    REQUIRE(t.remove(15, 21));
    REQUIRE(t.range_count() == 2);
    REQUIRE(t.size() == 5);
    REQUIRE(t.contains(19) == false);
    REQUIRE(t.contains(20) == false);
    REQUIRE(t.contains(21) == false);
    REQUIRE(t.contains(22));
    REQUIRE(t.contains(23) == false);
    REQUIRE(t.contains(24));
    REQUIRE(t.contains(25));
    REQUIRE(t.contains(26));
    REQUIRE(t.contains(27));
    REQUIRE(t.contains(28) == false);

    // case: overlap at upper edge
    REQUIRE_MESSAGE(t.remove(26, 30), t.str());
    REQUIRE(t.range_count() == 2);
    REQUIRE(t.size() == 3);
    REQUIRE(t.contains(19) == false);
    REQUIRE(t.contains(20) == false);
    REQUIRE(t.contains(21) == false);
    REQUIRE(t.contains(22));
    REQUIRE(t.contains(23) == false);
    REQUIRE(t.contains(24));
    REQUIRE(t.contains(25));
    REQUIRE(t.contains(26) == false);
    REQUIRE(t.contains(27) == false);
    REQUIRE(t.contains(28) == false);

    t.add_range(35);
    t.add_range(37, 39);
    t.add_range(41, 41);
    t.add_range(400, 1000);
    REQUIRE(t.range_count() == 6);
    REQUIRE(t.size() == 609);
    REQUIRE(t.contains(19) == false);
    REQUIRE(t.contains(20) == false);
    REQUIRE(t.contains(21) == false);
    REQUIRE(t.contains(22));
    REQUIRE(t.contains(23) == false);
    REQUIRE(t.contains(24));
    REQUIRE(t.contains(25));
    REQUIRE(t.contains(26) == false);
    REQUIRE(t.contains(27) == false);
    REQUIRE(t.contains(28) == false);
    REQUIRE(t.contains(34) == false);
    REQUIRE(t.contains(35) == true);
    REQUIRE(t.contains(36) == false);
    REQUIRE(t.contains(37) == true);
    REQUIRE(t.contains(38) == true);
    REQUIRE(t.contains(39) == true);
    REQUIRE(t.contains(40) == false);
    REQUIRE(t.contains(41) == true);
    REQUIRE(t.contains(42) == false);
    REQUIRE(t.contains(399) == false);
    REQUIRE(t.contains(400) == true);
    REQUIRE(t.contains(401) == true);
    REQUIRE(t.contains(999) == true);
    REQUIRE(t.contains(999) == true);
    REQUIRE(t.contains(1000) == true);
    REQUIRE(t.contains(1001) == false);

    // case: erase many
    REQUIRE(t.remove(20, 1001));
    REQUIRE(t.range_count() == 0);
    REQUIRE(t.size() == 0);
    REQUIRE(t.contains(19) == false);
    REQUIRE(t.contains(20) == false);
    REQUIRE(t.contains(21) == false);
    REQUIRE(t.contains(22) == false);
    REQUIRE(t.contains(23) == false);
    REQUIRE(t.contains(24) == false);
    REQUIRE(t.contains(25) == false);
    REQUIRE(t.contains(26) == false);
    REQUIRE(t.contains(27) == false);
    REQUIRE(t.contains(28) == false);
    REQUIRE(t.contains(34) == false);
    REQUIRE(t.contains(35) == false);
    REQUIRE(t.contains(36) == false);
    REQUIRE(t.contains(37) == false);
    REQUIRE(t.contains(38) == false);
    REQUIRE(t.contains(39) == false);
    REQUIRE(t.contains(40) == false);
    REQUIRE(t.contains(41) == false);
    REQUIRE(t.contains(42) == false);
    REQUIRE(t.contains(399) == false);
    REQUIRE(t.contains(400) == false);
    REQUIRE(t.contains(401) == false);
    REQUIRE(t.contains(999) == false);
    REQUIRE(t.contains(999) == false);
    REQUIRE(t.contains(1000) == false);
    REQUIRE(t.contains(1001) == false);
}

TEST_CASE("remove at upper edge") {
    disjoint_range_tree<std::uint16_t> t;
    t.add_range(450, 460);
    t.add_range(476, 520);
    REQUIRE(t.remove(520, 832));

    REQUIRE(t.range_count() == 2);
    REQUIRE(t.size() == 55);
    REQUIRE(t.contains(449) == false);
    REQUIRE(t.contains(450, 460));
    REQUIRE(t.contains(476, 519));
    REQUIRE(t.contains(500));
    REQUIRE(t.contains(520) == false);
}

TEST_CASE("range construction and basic properties") {
    SUBCASE("valid range construction") {
        disjoint_range_tree<int>::range r(5, 10);
        REQUIRE(r.low == 5);
        REQUIRE(r.high == 10);
        REQUIRE(r.size() == 6);
    }

    SUBCASE("single element range") {
        disjoint_range_tree<int>::range r(7, 7);
        REQUIRE(r.size() == 1);
    }

    SUBCASE("invalid range throws") {
        CHECK_THROWS_AS(disjoint_range_tree<int>::range(10, 5),
                        std::invalid_argument);

        CHECK_THROWS_AS(disjoint_range_tree<int>::range(1, 0),
                        std::invalid_argument);
    }

    SUBCASE("range equality") {
        disjoint_range_tree<int>::range r1(5, 10);
        disjoint_range_tree<int>::range r2(5, 10);
        disjoint_range_tree<int>::range r3(5, 11);
        REQUIRE(r1.equals(r2));
        CHECK_FALSE(r1.equals(r3));
    }

    SUBCASE("range contains point") {
        disjoint_range_tree<int>::range r(5, 10);
        REQUIRE(r.contains(disjoint_range_tree<int>::range(5, 5)));
        REQUIRE(r.contains(disjoint_range_tree<int>::range(7, 7)));
        REQUIRE(r.contains(disjoint_range_tree<int>::range(10, 10)));
        CHECK_FALSE(r.contains(disjoint_range_tree<int>::range(4, 4)));
        CHECK_FALSE(r.contains(disjoint_range_tree<int>::range(11, 11)));
    }

    SUBCASE("range contains range") {
        disjoint_range_tree<int>::range r(5, 10);
        REQUIRE(r.contains(disjoint_range_tree<int>::range(5, 10)));
        REQUIRE(r.contains(disjoint_range_tree<int>::range(6, 9)));
        CHECK_FALSE(r.contains(disjoint_range_tree<int>::range(4, 10)));
        CHECK_FALSE(r.contains(disjoint_range_tree<int>::range(5, 11)));
        CHECK_FALSE(r.contains(disjoint_range_tree<int>::range(4, 11)));
    }

    SUBCASE("range overlaps") {
        disjoint_range_tree<int>::range r(5, 10);
        REQUIRE(r.overlaps(disjoint_range_tree<int>::range(5, 10)));
        REQUIRE(r.overlaps(disjoint_range_tree<int>::range(3, 7)));
        REQUIRE(r.overlaps(disjoint_range_tree<int>::range(8, 12)));
        REQUIRE(r.overlaps(disjoint_range_tree<int>::range(3, 12)));
        REQUIRE(r.overlaps(disjoint_range_tree<int>::range(6, 9)));
        CHECK_FALSE(r.overlaps(disjoint_range_tree<int>::range(1, 4)));
        CHECK_FALSE(r.overlaps(disjoint_range_tree<int>::range(11, 15)));
    }
}

TEST_CASE("empty tree") {
    disjoint_range_tree<int> tree;

    REQUIRE(tree.range_count() == 0);
    REQUIRE(tree.size() == 0);
    CHECK_FALSE(tree.contains(5));
    CHECK_FALSE(tree.contains(0, 10));
    REQUIRE(tree.lowest() == std::nullopt);
    REQUIRE(tree.highest() == std::nullopt);
}

TEST_CASE("single range operations") {
    disjoint_range_tree<int> tree;

    SUBCASE("add single element") {
        tree.add_range(5);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 1);
        REQUIRE(tree.contains(5));
        CHECK_FALSE(tree.contains(4));
        CHECK_FALSE(tree.contains(6));
        REQUIRE(tree.lowest() == 5);
        REQUIRE(tree.highest() == 5);
    }

    SUBCASE("add single range") {
        tree.add_range(5, 10);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 6);
        REQUIRE(tree.contains(5));
        REQUIRE(tree.contains(7));
        REQUIRE(tree.contains(10));
        CHECK_FALSE(tree.contains(4));
        CHECK_FALSE(tree.contains(11));
        REQUIRE(tree.lowest() == 5);
        REQUIRE(tree.highest() == 10);
    }

    SUBCASE("remove from single range - exact match") {
        tree.add_range(5, 10);
        REQUIRE(tree.remove(5, 10));
        REQUIRE(tree.range_count() == 0);
        REQUIRE(tree.size() == 0);
    }

    SUBCASE("remove from single range - split in middle") {
        tree.add_range(5, 10);
        REQUIRE(tree.remove(7));
        REQUIRE(tree.range_count() == 2);
        REQUIRE(tree.size() == 5);
        REQUIRE(tree.contains(5, 6));
        REQUIRE(tree.contains(8, 10));
        CHECK_FALSE(tree.contains(7));
    }

    SUBCASE("remove from single range - split with range") {
        tree.add_range(5, 15);
        REQUIRE(tree.remove(8, 12));
        REQUIRE(tree.range_count() == 2);
        REQUIRE(tree.size() == 6);
        REQUIRE(tree.contains(5, 7));
        REQUIRE(tree.contains(13, 15));
        CHECK_FALSE(tree.contains(8, 12));
    }

    SUBCASE("remove from single range - truncate left") {
        tree.add_range(5, 10);
        REQUIRE(tree.remove(5, 7));
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 3);
        REQUIRE(tree.contains(8, 10));
        CHECK_FALSE(tree.contains(5, 7));
    }

    SUBCASE("remove from single range - truncate right") {
        tree.add_range(5, 10);
        REQUIRE(tree.remove(8, 10));
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 3);
        REQUIRE(tree.contains(5, 7));
        CHECK_FALSE(tree.contains(8, 10));
    }

    SUBCASE("remove non-existent element") {
        tree.add_range(5, 10);
        CHECK_FALSE(tree.remove(15));
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 6);
    }
}

TEST_CASE("merging adjacent ranges") {
    disjoint_range_tree<int> tree;

    SUBCASE("merge two adjacent ranges - touching") {
        tree.add_range(5, 10);
        tree.add_range(11, 15);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 11);
        REQUIRE(tree.contains(5, 15));
    }

    SUBCASE("merge two adjacent ranges - gap of 1") {
        tree.add_range(5, 10);
        tree.add_range(12, 15);
        REQUIRE(tree.range_count() == 2);
        REQUIRE(tree.size() == 10);
    }

    SUBCASE("merge by filling gap") {
        tree.add_range(5, 10);
        tree.add_range(15, 20);
        tree.add_range(11, 14);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 16);
        REQUIRE(tree.contains(5, 20));
    }

    SUBCASE("merge multiple ranges at once") {
        REQUIRE(tree.empty());
        tree.add_range(5, 10);
        tree.add_range(15, 20);
        tree.add_range(25, 30);
        tree.add_range(35, 40);
        tree.add_range(12, 38);
        REQUIRE_MESSAGE(tree.range_count() == 2, tree.str());
        REQUIRE_MESSAGE(tree.size() == 35, tree.str());
        REQUIRE(tree.contains(5, 40) == false);
        REQUIRE(tree.contains(5, 10) == true);
        REQUIRE(tree.contains(12, 40) == true);
    }

    SUBCASE("overlapping ranges merge") {
        tree.add_range(5, 10);
        tree.add_range(8, 15);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 11);
        REQUIRE(tree.contains(5, 15));
    }

    SUBCASE("contained range doesn't increase count") {
        tree.add_range(5, 20);
        tree.add_range(10, 15);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.size() == 16);
        REQUIRE(tree.contains(5, 20));
    }
}

TEST_CASE("multiple disjoint ranges") {
    disjoint_range_tree<int> tree;

    SUBCASE("add multiple disjoint ranges") {
        tree.add_range(5, 10);
        tree.add_range(20, 25);
        tree.add_range(40, 45);
        REQUIRE(tree.range_count() == 3);
        REQUIRE(tree.size() == 18);
        REQUIRE(tree.contains(5, 10));
        REQUIRE(tree.contains(20, 25));
        REQUIRE(tree.contains(40, 45));
        CHECK_FALSE(tree.contains(15));
        CHECK_FALSE(tree.contains(30));
    }

    SUBCASE("remove spanning multiple ranges") {
        tree.add_range(5, 10);
        tree.add_range(20, 25);
        tree.add_range(40, 45);
        REQUIRE(tree.remove(8, 42));
        REQUIRE(tree.range_count() == 2);
        REQUIRE(tree.contains(5, 7));
        REQUIRE(tree.contains(43, 45));
        CHECK_FALSE(tree.contains(8, 42));
    }

    SUBCASE("remove entire ranges in middle") {
        tree.add_range(5, 10);
        tree.add_range(20, 25);
        tree.add_range(40, 45);
        tree.add_range(60, 65);
        REQUIRE(tree.remove(15, 50));
        REQUIRE(tree.range_count() == 2);
        REQUIRE(tree.contains(5, 10));
        REQUIRE(tree.contains(60, 65));
    }
}

TEST_CASE("lowest and highest") {
    disjoint_range_tree<int> tree;

    SUBCASE("single range") {
        tree.add_range(10, 20);
        REQUIRE(tree.lowest() == 10);
        REQUIRE(tree.highest() == 20);
    }

    SUBCASE("multiple ranges") {
        tree.add_range(10, 20);
        tree.add_range(30, 40);
        tree.add_range(5, 8);
        REQUIRE(tree.lowest() == 5);
        REQUIRE(tree.highest() == 40);
    }

    SUBCASE("after removal") {
        tree.add_range(10, 20);
        tree.add_range(30, 40);
        tree.remove(10, 15);
        REQUIRE(tree.lowest() == 16);
        REQUIRE(tree.highest() == 40);
    }
}

TEST_CASE("edge cases") {
    disjoint_range_tree<int> tree;

    SUBCASE("boundaries - INT_MAX") {
        tree.add_range(INT_MAX - 5, INT_MAX);
        REQUIRE(tree.contains(INT_MAX));
        REQUIRE(tree.size() == 6);
    }

    SUBCASE("boundaries - INT_MIN") {
        tree.add_range(INT_MIN, INT_MIN + 5);
        REQUIRE(tree.contains(INT_MIN));
        REQUIRE(tree.size() == 6);
    }

    SUBCASE("add and remove same range repeatedly") {
        for (int i = 0; i < 10; ++i) {
            tree.add_range(5, 10);
            REQUIRE(tree.range_count() == 1);
            tree.remove(5, 10);
            REQUIRE(tree.range_count() == 0);
        }
    }

    SUBCASE("alternating pattern") {
        // Add: [0], [2], [4], [6], [8]
        for (int i = 0; i < 10; i += 2) {
            tree.add_range(i);
        }
        REQUIRE(tree.range_count() == 5);
        REQUIRE(tree.size() == 5);

        // Fill gaps: [1], [3], [5], [7]
        for (int i = 1; i < 9; i += 2) {
            tree.add_range(i);
        }
        REQUIRE(tree.range_count() == 1);
        REQUIRE_MESSAGE(tree.size() == 9, tree.str());
        REQUIRE(tree.contains(0, 8));
    }

    SUBCASE("reverse order insertion") {
        tree.add_range(40, 45);
        tree.add_range(30, 35);
        tree.add_range(20, 25);
        tree.add_range(10, 15);
        REQUIRE(tree.range_count() == 4);
        REQUIRE(tree.size() == 24);
    }
}

TEST_CASE("stress test - ID allocation scenario") {
    disjoint_range_tree<int> tree;

    SUBCASE("simulate client connect/disconnect") {
        // Start with all IDs available
        tree.add_range(1, 1000);
        REQUIRE(tree.size() == 1000);

        // Allocate first 100 IDs
        for (int i = 1; i <= 100; ++i) {
            REQUIRE(tree.contains(i));
            tree.remove(i);
        }
        REQUIRE(tree.size() == 900);
        REQUIRE(tree.range_count() == 1);
        REQUIRE(tree.lowest() == 101);

        // Free some IDs in the middle
        tree.add_range(50, 60);
        REQUIRE(tree.size() == 911);
        REQUIRE(tree.range_count() == 2);

        // Free some more, creating gaps
        tree.add_range(25);
        tree.add_range(75);
        REQUIRE(tree.range_count() == 4);

        // Verify specific ranges
        REQUIRE(tree.contains(25));
        REQUIRE(tree.contains(50, 60));
        REQUIRE(tree.contains(75));
        REQUIRE(tree.contains(101, 1000));
        CHECK_FALSE(tree.contains(26, 49));
    }
}

TEST_CASE("comprehensive fuzz test") {
    std::mt19937 rng(4318);  // Fixed seed for reproducibility
    std::uniform_int_distribution<int> value_dist(0, 1000);
    std::uniform_int_distribution<int> op_dist(
      0, 2);  // 0=add single, 1=add range, 2=remove

    disjoint_range_tree<int> tree;
    std::set<int> reference;  // Ground truth

    const int NUM_OPERATIONS = 60000;

    for (int op = 0; op < NUM_OPERATIONS; ++op) {
        std::cerr << "\n=========== OP " << op << " ==============\n";
        int operation = op_dist(rng);

        if (operation == 0) {
            auto before = tree.str();
            // Add single value
            int val = value_dist(rng);
            tree.add_range(val);
            reference.insert(val);

            std::cerr << "inserting value: " << val << "; before=" << before
                      << " after=" << tree.str() << std::endl;

        } else if (operation == 1) {
            // Add range
            int low = value_dist(rng);
            int high = value_dist(rng);
            if (low > high) std::swap(low, high);
            auto before = tree.str();
            tree.add_range(low, high);
            for (int i = low; i <= high; ++i) {
                reference.insert(i);
            }

            std::cerr << "inserting value: "
                      << decltype(tree)::range(low, high).str()
                      << "; before=" << before << " after=" << tree.str()
                      << std::endl;

        } else {
            // Remove
            int low = value_dist(rng);
            int high = value_dist(rng);
            if (low > high) std::swap(low, high);
            const auto before = tree.str();
            tree.remove(low, high);
            std::cerr << "---- removing "
                      << decltype(tree)::range(low, high).str()
                      << "; before=" << before << ", after=" << tree.str()
                      << std::endl;

            for (int i = low; i <= high; ++i) {
                reference.erase(i);
                // std::stringstream ss;
                // for (auto x : reference) {
                //     ss << x << ", ";
                // }

                // std::string s =
                //   "tree=" + tree.str() + ";\n ref=" + ss.str() + "\n\n";
                // std::cerr << s << std::endl;
            }
        }

        // Periodically verify consistency
        // if (op % 100 == 0) {
        if (true) {
            std::stringstream ss;
            for (auto x : reference) {
                ss << x << ", ";
            }

            std::string s = "tree=" + tree.str() + ";\n ref=" + ss.str();
            REQUIRE_MESSAGE(tree.size() == reference.size(), s);

            // Verify all values in reference are in tree
            for (int val : reference) {
                REQUIRE(tree.contains(val));
            }

            // Sample some values not in reference
            for (int i = 0; i < 10; ++i) {
                int val = value_dist(rng);
                if (reference.find(val) == reference.end()) {
                    CHECK_FALSE(tree.contains(val));
                }
            }

            // Check lowest/highest
            if (!reference.empty()) {
                REQUIRE(tree.lowest() == *reference.begin());
                REQUIRE(tree.highest() == *reference.rbegin());
            } else {
                REQUIRE(tree.lowest() == std::nullopt);
                REQUIRE(tree.highest() == std::nullopt);
            }
        }
    }

    // Final comprehensive check
    REQUIRE(tree.size() == reference.size());

    INFO("Final range count: " << tree.range_count());
    INFO("Final size: " << tree.size());
    INFO("Reference size: " << reference.size());

    for (int val : reference) {
        REQUIRE(tree.contains(val));
    }
}

TEST_CASE("fuzz test - pathological fragmentation") {
    disjoint_range_tree<int> tree;
    std::set<int> reference;

    std::size_t num_ranges = 0;

    // Create maximum fragmentation: alternate add/skip
    for (int i = 0; i < 1000; i += 2) {
        std::cerr << " --- inserting " << i << std::endl;
        num_ranges++;
        tree.add_range(i);
        reference.insert(i);
        REQUIRE_MESSAGE(tree.contains(i), tree.str());
    }

    REQUIRE_MESSAGE(tree.range_count() == num_ranges, tree.str());
    REQUIRE(num_ranges == 500);
    REQUIRE(tree.size() == 500);

    // Verify
    for (int i = 0; i < 1000; ++i) {
        if (i % 2 == 0) {
            REQUIRE(tree.contains(i));
        } else {
            CHECK_FALSE(tree.contains(i));
        }
    }

    // Now fill in the gaps
    for (int i = 1; i < 1000; i += 2) {
        tree.add_range(i);
        reference.insert(i);
    }

    REQUIRE(tree.range_count() == 1);
    REQUIRE(tree.size() == 1000);
    REQUIRE(tree.contains(0, 999));
}

TEST_CASE("cache invalidation") {
    disjoint_range_tree<int> tree;

    tree.add_range(1, 100);
    REQUIRE(tree.size() == 100);

    // Size should be cached now
    REQUIRE(tree.size() == 100);

    // Add more - cache should invalidate
    tree.add_range(200, 300);
    REQUIRE(tree.size() == 201);

    // Remove - cache should invalidate
    tree.remove(50, 60);
    REQUIRE(tree.size() == 190);
}

TEST_CASE("benchmark - medium scale u32 (10M ranges)" * doctest::timeout(60)) {
    using tree_type = disjoint_range_tree<uint32_t>;
    tree_type tree;

    const uint32_t MAX = 20000000;  // 20M range, creates 10M ranges

    std::cerr << "Phase 1: Creating 10M fragmented ranges" << std::endl;
    auto start_fragment = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 0; i < MAX; i += 2) {
        tree.add_range(i);
    }

    auto end_fragment = std::chrono::high_resolution_clock::now();
    auto fragment_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_fragment -
                                                            start_fragment);

    std::cerr << "Phase 1 complete in " << fragment_duration.count() << "ms"
              << std::endl;
    std::cerr << "Range count: " << tree.range_count() << std::endl;
    REQUIRE(tree.range_count() == 10000000);

    std::cerr << "Phase 2: Collapsing to single range" << std::endl;
    auto start_collapse = std::chrono::high_resolution_clock::now();

    for (uint32_t i = 1; i < MAX; i += 2) {
        tree.add_range(i);

        // Progress indicator
        if (i % 2000000 == 1) {
            auto current = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                             current - start_collapse)
                             .count();
            std::cerr << "Progress: " << (i * 100 / MAX) << "%, "
                      << "range_count: " << tree.range_count()
                      << ", elapsed: " << elapsed << "s" << std::endl;
        }
    }

    auto end_collapse = std::chrono::high_resolution_clock::now();
    auto collapse_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_collapse -
                                                            start_collapse);

    std::cerr << "Phase 2 complete in " << collapse_duration.count() << "ms"
              << std::endl;
    std::cerr << "Total time: "
              << (fragment_duration.count() + collapse_duration.count())
              << "ms" << std::endl;

    REQUIRE(tree.range_count() == 1);
    REQUIRE(tree.lowest() == 0);
    REQUIRE(tree.highest() == MAX - 1);
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
