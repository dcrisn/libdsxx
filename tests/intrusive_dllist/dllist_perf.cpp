#include <tarp/intrusive.hpp>
#include <tarp/intrusive_dllist.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <chrono>
#include <format>
#include <list>
#include <stdexcept>
#include <vector>

using std::make_unique;
using tarp::container_of;
using tarp::dlnode;
using tarp::is_safely_offsettable;
using tarp::is_safely_offsettable_v;
using tarp::SafelyOffsettable;

static inline constexpr std::size_t PAYLOAD_SIZE = 600;

struct testnode {
    testnode(size_t x) : num(x) {}

    testnode() = default;

    size_t num = 0;

    // to simulate data
    std::array<std::uint8_t, PAYLOAD_SIZE> bytes;

    dlnode link;

    static auto make(std::size_t numval = 0) {
        auto ptr = make_unique<testnode>();
        ptr->num = numval;
        return ptr;
    }
};

using dllist = tarp::dllist<testnode, &testnode::link>;

TEST_CASE("dllist perf comparison") {
    using namespace std::chrono;
    using usecs = duration<float, std::micro>;

    enum what {
        VECT_INPLACE = 0,
        VECT_PTR,
        LIST_INPLACE,
        LIST_PTR,
        DLLIST_VECT,
        DLLIST_LIST,
        N
    };

    auto to_string = [](what x) {
        switch (x) {
        case VECT_INPLACE: return "vect<elem>";
        case VECT_PTR: return "vect<elem*>";
        case LIST_INPLACE: return "list<elem>";
        case LIST_PTR: return "list<elem*>";
        case DLLIST_LIST: return "dllist[list]";
        case DLLIST_VECT: return "dllist[vect]";
        default: throw std::logic_error("bad what value");
        }
    };

    // Indexed by what
    std::vector<std::vector<float>> measurements;

    volatile size_t sink = 0;

    auto run_test = [&](std::size_t len) {
        // size_t num = 80 * 1000 * 1000;
        // size_t num = 87;

        // Have all the objects stored in and owned by a vector.
        // The simply store pointers in a 1)std::list 2)std::vector
        // and 3) dllist  and compare iteration speed.
        std::vector<testnode> owner;
        std::list<testnode *> listptr;
        std::vector<testnode *> vectptr;
        dllist list_backed_q;
        dllist vect_backed_q;

        // also compare iteration speed with a std::list and std::vector
        // with in-place constructed elements, eliminating a pointer
        // indirection.
        std::list<testnode> list;

        for (size_t i = 0; i < len; i++) {
            owner.push_back(testnode(0));
            list.push_back(testnode(0));
        }

        for (size_t i = 0; i < owner.size(); ++i) {
            auto *elem = &owner[i];
            listptr.push_back(elem);
            vectptr.push_back(elem);
            vect_backed_q.push_back(*elem);
        }

        for (auto &x : list) {
            list_backed_q.push_back(x);
        }

        using namespace std::chrono;
        constexpr std::size_t NUM_PASSES = 10;
        auto measure = [&sink](auto &l) {
            const auto start = steady_clock::now();
            for (unsigned i = 0; i < NUM_PASSES; ++i) {
                for (auto &x : l) {
                    x.num++;
                }
            }
            const auto end = steady_clock::now();
            const auto elapsed = duration_cast<usecs>(end - start).count();

            for (auto &x : l) {
                sink += x.num;
            }

            return elapsed;
        };

        using namespace std::chrono;
        auto measure_ptr = [&sink](auto &l) {
            const auto start = steady_clock::now();
            for (unsigned i = 0; i < NUM_PASSES; ++i) {
                for (auto &x : l) {
                    x->num++;
                }
            }
            const auto end = steady_clock::now();
            const auto elapsed = duration_cast<usecs>(end - start).count();
            for (auto &x : l) {
                sink += x->num;
            }
            return elapsed;
        };

        std::vector<float> res;
        res.resize(what::N);
        res[what::VECT_INPLACE] = measure(owner);
        res[what::VECT_PTR] = measure_ptr(vectptr);
        res[what::LIST_INPLACE] = measure(list);
        res[what::LIST_PTR] = measure_ptr(listptr);
        res[what::DLLIST_LIST] = measure(list_backed_q);
        res[what::DLLIST_VECT] = measure(vect_backed_q);
        return res;
    };

    std::vector<std::size_t> counts = {10,
                                       20,
                                       50,
                                       100,
                                       200,
                                       500,
                                       1000,
                                       2000,
                                       5000,
                                       10000,
                                       20000,
                                       50000,
                                       80000,
                                       100000,
                                       200000,
                                       500000,
                                       800000,
                                       1000 * 1000,
                                       8 * 1000 * 1000};

    for (auto count : counts) {
        measurements.push_back(run_test(count));
    }

    // ===== dump csv =====

    // dump CSV columns
    std::cerr << std::format("{},{},{},{},{},{},{}\n",
                             "count",
                             to_string(what::VECT_INPLACE),
                             to_string(what::VECT_PTR),
                             to_string(what::LIST_INPLACE),
                             to_string(what::LIST_PTR),
                             to_string(what::DLLIST_LIST),
                             to_string(what::DLLIST_VECT));

    // dump csv rows
    auto &v = measurements;
    for (unsigned i = 0; i < measurements.size(); ++i) {
        std::cerr << std::format("{},{},{},{},{},{},{}\n",
                                 counts[i],
                                 v[i][what::VECT_INPLACE],
                                 v[i][what::VECT_PTR],
                                 v[i][what::LIST_INPLACE],
                                 v[i][what::LIST_PTR],
                                 v[i][what::DLLIST_LIST],
                                 v[i][what::DLLIST_VECT]);
    }

    REQUIRE(sink > 10 * 1000 * 1000);
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
