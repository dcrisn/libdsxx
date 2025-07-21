#include <tarp/intrusive.hpp>
#include <tarp/intrusive_dllist.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <format>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace std;

using std::make_unique;
using tarp::container_of;
using tarp::dlnode;
using tarp::is_safely_offsettable;
using tarp::is_safely_offsettable_v;
using tarp::SafelyOffsettable;

struct testnode {
    size_t num = 0;
    dlnode link;

    static auto make(std::size_t numval = 0) {
        auto ptr = make_unique<testnode>();
        ptr->num = numval;
        return ptr;
    }
};

using dllist = tarp::dllist<testnode, &testnode::link>;

TEST_CASE("test push_back, pop_back, pop_front") {
    dllist list;
    REQUIRE(list.empty());
    REQUIRE(list.size() == 0);

    auto a = make_unique<testnode>();
    a->num = 1;

    list.push_back(*a);
    REQUIRE(list.empty() == false);
    REQUIRE(list.size() == 1);

    auto a_recovered = list.pop_back();
    REQUIRE(list.empty() == true);
    REQUIRE(list.size() == 0);
    REQUIRE(a_recovered->num == a->num);
    REQUIRE(a_recovered == a.get());
}

void test_enqdq_pushpop(size_t size, bool reversed_front_back, bool stackmode) {
    dllist list;

    std::vector<std::unique_ptr<testnode>> owner;

    /* push size elements to the list one by one, then pop them
     * off and see if the count remains consistent throughout */
    for (size_t i = 1; i <= size; ++i) {
        // debug("pushing %s %zu", reversed_front_back ? "front" : "back", i);
        auto node = testnode::make(i);
        if (reversed_front_back) {
            list.push_front(*node);
        } else {
            list.push_back(*node);
        }
        owner.push_back(std::move(node));

        REQUIRE_MESSAGE(list.size() == i,
                        std::format("expected {} got {}", i, list.size()));
    }

#if 0
    struct testnode *tmp;
    Dll_foreach(list, tmp, struct testnode, link){
        debug("tmp->num %zu", tmp->num);
    }
#endif

    for (size_t i = 1; i <= size; ++i) {
        struct testnode *node;
        if (stackmode) {
            node = reversed_front_back ? list.pop_front() : list.pop_back();
            REQUIRE(node);
            REQUIRE_MESSAGE(
              node->num == size + 1 - i,
              std::format("expected {} got {}", size + 1 - i, node->num));
        } else {
            node = reversed_front_back ? list.pop_back() : list.pop_front();

            REQUIRE(node);
            REQUIRE_MESSAGE(node->num == i,
                            std::format("expected {}, got {}", i, node->num));
        }

        REQUIRE_MESSAGE(
          list.size() == size - i,
          std::format("expected {} got {}", size - i, list.size()));
    }
}

/*
 * Test whether the values we put on the queue and get back from it
 * are consistent with FIFO semantics through a number of enqueue-dequeue
 * operations. Also check the count stays correct throughout. */
TEST_CASE("test count, enqueue, dequeue") {
    std::vector<std::size_t> inputs = {
      1, 2, 3, 10, 100, 1000, 100 * 1000, 1000 * 1000};

    SUBCASE("DLLIST with len 0") {
        test_enqdq_pushpop(0, false, true);
        test_enqdq_pushpop(0, true, false);
    }
    SUBCASE("DLLIST with len 1") {
        test_enqdq_pushpop(1, false, true);
        test_enqdq_pushpop(1, true, false);
    }
    SUBCASE("DLLIST with len 2") {
        test_enqdq_pushpop(2, false, true);
        test_enqdq_pushpop(2, true, false);
    }
    SUBCASE("DLLIST with len 3") {
        test_enqdq_pushpop(3, false, true);
        test_enqdq_pushpop(3, true, false);
    }
    SUBCASE("DLLIST with len 10") {
        test_enqdq_pushpop(10, false, true);
        test_enqdq_pushpop(10, true, false);
    }
    SUBCASE("DLLIST with len 100") {
        test_enqdq_pushpop(100, false, true);
        test_enqdq_pushpop(100, true, false);
    }
    SUBCASE("DLLIST with len 1000") {
        test_enqdq_pushpop(1000, false, true);
        test_enqdq_pushpop(1000, true, false);
    }
    SUBCASE("DLLIST with len 100'000") {
        test_enqdq_pushpop(100'000, false, true);
        test_enqdq_pushpop(100'000, true, false);
    }
    SUBCASE("DLLIST with len 1e6") {
        test_enqdq_pushpop(1000 * 1000, false, true);
        test_enqdq_pushpop(1000 * 1000, true, false);
    }
}

/*
 * Test whether the values get put on the stack and get back from it
 * are consistent with LIFO semantics through a number of push-pop
 * operations. Also check the count stays correct throughout. */
TEST_CASE("test count,push,pop") {
    SUBCASE("DLLIST with len 0") {
        test_enqdq_pushpop(0, false, true);
        test_enqdq_pushpop(0, true, true);
    }

    SUBCASE("DLLIST with len 0") {
        test_enqdq_pushpop(0, false, true);
        test_enqdq_pushpop(0, true, true);
    }
    SUBCASE("DLLIST with len 1") {
        test_enqdq_pushpop(1, false, true);
        test_enqdq_pushpop(1, true, true);
    }
    SUBCASE("DLLIST with len 2") {
        test_enqdq_pushpop(2, false, true);
        test_enqdq_pushpop(2, true, true);
    }
    SUBCASE("DLLIST with len 3") {
        test_enqdq_pushpop(3, false, true);
        test_enqdq_pushpop(3, true, true);
    }
    SUBCASE("DLLIST with len 10") {
        test_enqdq_pushpop(10, false, true);
        test_enqdq_pushpop(10, true, true);
    }
    SUBCASE("DLLIST with len 100") {
        test_enqdq_pushpop(100, false, true);
        test_enqdq_pushpop(100, true, true);
    }
    SUBCASE("DLLIST with len 1000") {
        test_enqdq_pushpop(1000, false, true);
        test_enqdq_pushpop(1000, true, true);
    }
    SUBCASE("DLLIST with len 100'000") {
        test_enqdq_pushpop(100'000, false, true);
        test_enqdq_pushpop(100'000, true, true);
    }
    SUBCASE("DLLIST with len 1e6") {
        test_enqdq_pushpop(1000 * 1000, false, true);
        test_enqdq_pushpop(1000 * 1000, true, true);
    }
}

// test clear and destroy
TEST_CASE("test list destruction") {
    dllist list;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr std::size_t N = 10;

    // for (size_t i = 0; i < 200'000; ++i) {
    for (size_t i = 0; i < 10; ++i) {
        auto node = testnode::make(i);
        list.push_front(*node);
        // utdbgprint("PUSH: list.size()=={}, expected={}", list.size(), i+1);
        REQUIRE(list.size() == i + 1);
        owner.push_back(std::move(node));
    }

    std::size_t sz = list.size();
    REQUIRE(sz == N);

    for (auto &node : owner) {
        list.unlink(*node);
        node.reset();
        REQUIRE(list.size() == sz - 1);
        --sz;
    }

    REQUIRE(list.empty());
}

// test if the nth node can be found
TEST_CASE("find nth") {
    dllist list;
    std::vector<std::unique_ptr<testnode>> owner;
    constexpr std::size_t num = 500;

    for (size_t i = 1; i <= num; i++) {
        auto node = testnode::make(i);
        list.push_back(*node);
        owner.push_back(std::move(node));
    }

    for (size_t i = 1; i <= num; ++i) {
        auto node = list.find_nth(i);
        REQUIRE(node != nullptr);
        REQUIRE_MESSAGE(node->num == i,
                        std::format("expected {} got {}", i, node->num));
    }

    auto node = list.find_nth(list.size() + 1);
    REQUIRE_MESSAGE(node == nullptr,
                    std::format("expected nullptr, got {}", node->num));
}

// test reversing the list nodes
TEST_CASE("test list_upend") {
    auto test = [](std::size_t size) {
        dllist list;
        std::vector<std::unique_ptr<testnode>> owner;

        // numbers get pushed so they decrease front-to-back
        for (size_t j = 1; j <= size; ++j) {
            auto node = testnode::make(j);
            list.push_front(*node);
            owner.push_back(std::move(node));
        }

        list.upend();

        // on upending the whole list, expect value to match j exactly
        for (size_t j = 1; j <= size; ++j) {
            auto node = list.pop_front();
            REQUIRE(node != nullptr);
            REQUIRE_MESSAGE(node->num == j,
                            std::format("expected {} got {}", j, node->num));
        }
    };

    SUBCASE("DLLIST with len 1") {
        test(1);
    }
    SUBCASE("DLLIST with len 2") {
        test(2);
    }
    SUBCASE("DLLIST with len 3") {
        test(3);
    }
    SUBCASE("DLLIST with len 14") {
        test(14);
    }
    SUBCASE("DLLIST with len 233") {
        test(233);
    }
    SUBCASE("DLLIST with len 1521") {
        test(1521);
    }
}

// Note this test is the same as the corresponding staq one (see tarp/staq in
// libtarp).
// NOTE read the comments in the staq test. The test assumes a stack that
// rotates to the top when dir=1, and toward the bottom when dir=-1. The dll
// thinks in terms of front/back not top/bottom. dir=1 means toward the front.
// This means we must have a stack growing out the front so that the semantics
// of dir is as this test expects. In other words, when dir=1, rotate toward
// the front of the list, which we must ensure is also toward the top of the
// stack ==> use pushfront, popfront, NOT pushback, popback.
void test_list_rotation__(size_t size, size_t rotations, int dir) {
    /* Run 'rotations' number of tests. For each test:
     *  - construct a list of size SIZE with elements that take values
     *    0..size-1. That is, the elements take all the values modulo size.
     *  - rotate the list 'num_rotations' number of times.
     *  - pop the elements from the list and verify the elements in the list
     *    have been rotated by num_rotations positions */
    for (size_t num_rotations = 0; num_rotations <= rotations;
         ++num_rotations) {
        dllist list;
        std::vector<std::unique_ptr<testnode>> owner;

        // insert items with values 0...size-1, i.e. all values mod size
        for (size_t i = 0; i < size; ++i) {
            auto node = testnode::make(i);
            list.push_front(*node);
            owner.push_back(std::move(node));
        }

        // debug("rotating %zu times to the %s, staq size=%zu", num_rotations,
        // dir == 1 ? "front" : "back", size);
        list.rotate(dir, num_rotations);

        // count should remain unchanged
        REQUIRE_MESSAGE(list.size() == size,
                        std::format("expected {} got {}", size, list.size()));

        size_t modulus = size;
        for (size_t i = 0; i < size; ++i) {
            auto node = list.pop_front();

            /* discard congruent multiples (a%n == (a+(k*n))%n) */
            size_t numrot = num_rotations % size;

            // The explanation for this is exactly the same as the one in the
            // in the equivalent staq rotation test. In fact the test is
            // copy-paste (though it may change in the future)
            size_t expected;
            if (dir == 1) { /* rotate to top of stack */
                expected = ((modulus - 1 - i) + (modulus - numrot)) % modulus;
            } else if (dir == -1) { /* rotate to bottom of stack */
                expected = ((modulus - 1 - i) + numrot) % modulus;
            } else (throw std::logic_error("bad logic"));

            /* the position is the same as the actual value that was assigned to
             * node->num */
            REQUIRE(node != nullptr);

            // utdbgprint("expcted={}, actual={}", expected, cont->num);
            REQUIRE_MESSAGE(
              expected == node->num,
              std::format("expected {} got {}", expected, node->num));
        }
    }
}

TEST_CASE("test list rotation") {
    const auto test = [](std::size_t sz) {
        test_list_rotation__(sz, sz * 2 + 1, 1);
        test_list_rotation__(sz, sz * 2 + 1, -1);
    };

    SUBCASE("dllist with len 0") {
        test(0);
    }
    SUBCASE("dllist with len 1") {
        test(1);
    }
    SUBCASE("dllist with len 2") {
        test(2);
    }
    SUBCASE("dllist with len 3") {
        test(3);
    }
    SUBCASE("dllist with len 10") {
        test(10);
    }
    SUBCASE("dllist with len 539") {
        test(539);
    }
}

// rotate list as many times as neeed to make a specified node the front
TEST_CASE("test list_rotation_to_node") {
    dllist list;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr size_t size = 350;
    for (size_t i = 1; i <= size; ++i) {
        auto node = testnode::make(i);
        list.push_back(*node);
        owner.push_back(std::move(node));
    }

    for (size_t i = 1; i <= size; ++i) {
        auto node = list.find_nth(i);
        REQUIRE(node != nullptr);
        list.rotate_to(*node);

        REQUIRE(list.front_equals(*node));
    }
}

// test that modifying the list while iterating over it is fine
TEST_CASE("test for each forward iteration") {
    dllist list;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr size_t num = 9;
    for (size_t i = 1; i <= num; ++i) {
        auto node = testnode::make(i);
        // std::cerr << "pushing node: " << static_cast<void *>(node.get())
        //           << std::endl;
        list.push_back(*node);
        owner.push_back(std::move(node));
    }

    /* make the following changes:
     * delete all items with values <=2 and >=7
     * replace items with values ==3 and ==4 with nodes with values 0xff
     * insert 0x01 before items with values 5 and 6
     * insert 0x2 after items with values 5 and 6 */
    for (auto it = list.begin(); it != list.end();) {
        if (it->num <= 2 || it->num >= 7) {
            it.erase(list);
            continue;
        }

        else if (it->num == 3 || it->num == 4) {
            auto tmp = testnode::make(0xff);
            auto node = list.replace(*it, *tmp);
            it.assign(node);
            owner.push_back(std::move(tmp));
        }
        ++it;
    }

    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->num == 5 || it->num == 6) {
            auto tmp1 = testnode::make(0x1);
            list.put_before(*it, *tmp1);
            owner.push_back(std::move(tmp1));

            auto tmp2 = testnode::make(0x2);
            list.put_after(*it, *tmp2);
            owner.push_back(std::move(tmp2));
        }
    }

    // after all this, the sequence shoud be:
    // 0xff 0xff 0x1 5 0x2 0x1 6 0x2
    // and the count should be 8
    REQUIRE_MESSAGE(list.size() == num - 1,
                    std::format("expected {} got {}", num - 1, list.size()));

    list.for_each([]([[maybe_unused]] const auto &cont) {
        // std::cerr << std::format("cont->num={}\n", cont.num);
    });

    std::vector<std::size_t> values = {
      0xff, 0xff, 0x1, 0x5, 0x2, 0x1, 0x6, 0x2};

    for (size_t i = 0; i < (num - 1); ++i) {
        auto &cont = list.front();
        REQUIRE_MESSAGE(values[i] == cont.num,
                        std::format("expected {} got {}", values[i], cont.num));
        list.rotate(1, 1);
    }
}

// test that 2 lists can swap he
TEST_CASE("test swap heads") {
    dllist a, b;

    std::vector<std::unique_ptr<testnode>> owner;

    std::vector<std::uint64_t> vals = {1, 2, 3, 4, 5, 6, 7};

    // create 2 lists, one with all items, one only with the last 3, then
    // swap their heads and verify
    for (size_t i = 0; i < vals.size(); ++i) {
        auto node = testnode::make(vals[i]);
        a.push_back(*node);
        owner.push_back(std::move(node));
    }
    for (size_t i = vals.size() - 4; i < vals.size(); ++i) {
        auto node = testnode::make(vals[i]);
        b.push_back(*node);
        owner.push_back(std::move(node));
    }

    a.swap(b);

    REQUIRE_MESSAGE(
      a.size() == vals.size() - 3,
      std::format("expected {} got {}", vals.size() - 3, a.size()));

    REQUIRE_MESSAGE(b.size() == vals.size(),
                    std::format("expected {} got {}", vals.size(), b.size()));

    for (size_t i = 0; i < vals.size(); ++i) {
        auto cont = b.pop_front();
        REQUIRE_MESSAGE(cont->num == vals[i],
                        std::format("expected {} got {}", vals[i], cont->num));
    }
    for (size_t i = vals.size() - 4; i < vals.size(); ++i) {
        auto cont = a.pop_front();
        REQUIRE_MESSAGE(cont->num == vals[i],
                        std::format("expected {} got {}", vals[i], cont->num));
    }
}

// can front and back be removed and are they freed correctly
TEST_CASE("remove front and back") {
    dllist a;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr size_t num = 2300;

    // add every time both front and back
    for (size_t i = 0; i < num; ++i) {
        auto nodea = testnode::make();
        auto nodeb = testnode::make();
        a.push_back(*nodea);
        a.push_front(*nodeb);
        owner.push_back(std::move(nodea));
        owner.push_back(std::move(nodeb));
    }

    // verify after deleting from both sides there are no leaks and such
    // (run with sanitizers) and the count is 0
    for (size_t i = 0; i < num; ++i) {
        a.pop_front();
        a.pop_back();
    }

    REQUIRE_MESSAGE(a.size() == 0,
                    std::format("expected {} got {}", 0, a.size()));
}

TEST_CASE("test list join") {
    dllist a;
    dllist b;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr size_t len = 7482;
    for (size_t i = 0; i < len; ++i) {
        auto node = testnode::make(i);
        a.push_back(*node);
        owner.push_back(std::move(node));

        node = testnode::make(i);
        b.push_back(*node);
        owner.push_back(std::move(node));
    }

    // verify that joining b to a doubles the count and gives a list
    // with the correct concatenation 0...n,0...n
    a.join(b);
#if 0
    Dll_foreach(&a, node, struct testnode, link){
        info("val %zu", node->num);
    }
#endif

    REQUIRE(b.empty());

    REQUIRE_MESSAGE(
      a.size() == len * 2,
      std::format("expected len(a)={} len(b)={} ; got len(a)={} len(b)={} ",
                  len * 2,
                  a.size(),
                  0,
                  b.size()));

    for (size_t i = 0; i < len * 2; ++i) {
        auto cont = a.pop_front();
        REQUIRE_MESSAGE(cont->num == i % len,
                        std::format("expected {} got {}", i % len, cont->num));
    }
}

TEST_CASE("test list split") {
    dllist a;
    std::vector<std::unique_ptr<testnode>> owner;

    constexpr size_t len = 15;
    for (size_t i = 0; i < len; ++i) {
        auto node = testnode::make(i);
        a.push_back(*node);
        owner.push_back(std::move(node));
    }

    // split a down the middle; the len/2+1 th node becomes the head of a new
    // list breaking off from a; a should be left with len/2 items; the new
    // list should have len/2 items as well (or (len/2)+1, if the len is odd);
    // e.g. if len=15, len(a) should be 7 and len(b) should be 8
    dllist b = a.split(*a.find_nth((len / 2) + 1));

    // round the length up for b since the lenth was divided in half and it
    // could've been truncated if len is odd;
    size_t b_len = (len % 2 == 0) ? (len / 2) : (len / 2 + 1);
    REQUIRE(a.size() == len / 2);
    REQUIRE(b.size() == b_len);

    for (size_t i = 0; i < (len / 2); ++i) {
        auto cont = a.pop_front();
        REQUIRE(cont->num == i);
    }

    for (size_t i = b_len - 1; i < len; ++i) {
        auto cont = b.pop_front();
        REQUIRE(cont->num == i);
    }
}

/*
 * Push massive number of nodes;
 * With each pushed node:
 *  - rotate the list 100 times to the back/bottom
 *  - upend the whole staq
 *  - rotate the staq 100 times to the front/top
 */
TEST_CASE("perf") {
    constexpr size_t num = 80 * 1000;
    // size_t num = 80 * 1000 * 1000;
    // size_t num = 87;

    dllist q;
    std::vector<std::unique_ptr<testnode>> owner;

    for (size_t i = 0; i < num; i++) {
        auto node = testnode::make(i);
        q.push_back(*node);
        owner.push_back(std::move(node));
        q.rotate(1, 100);
        q.upend();
        q.rotate(-1, 100);
    }

    for (size_t i = 0; i < num; ++i) {
        q.pop_front();
    }

    q.clear();
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
