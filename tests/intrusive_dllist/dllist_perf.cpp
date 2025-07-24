#include <tarp/intrusive.hpp>
#include <tarp/intrusive_dllist.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <chrono>
#include <cstddef>
#include <format>
#include <list>
#include <stdexcept>
#include <vector>

#ifdef HAS_BOOST_INTRUSIVE
#include <boost/intrusive/list.hpp>
#endif

using std::make_unique;
using tarp::container_of;
using tarp::dlnode;
using tarp::is_safely_offsettable;
using tarp::is_safely_offsettable_v;
using tarp::SafelyOffsettable;

static inline constexpr std::size_t PAYLOAD_SIZE = 240;

// we use these 2 volatile variables to prevent optimization for the
// purposes of benchmarking. We need the whole loop to be run below,
// since that is what we are measuring, and so we cannot have it
// be optimized away.
volatile std::uint64_t sink1 = 0;
volatile std::uint64_t sink2 = 0;

struct testnode {
    testnode(size_t x) : red(-x), black(x) {}

    testnode() = default;

    std::int32_t red = 0;
    std::uint32_t black = 0;

    // to simulate data
    std::array<std::uint8_t, PAYLOAD_SIZE> bytes{};

    dlnode link;

    // Same as 'link', but we can't use the same name.
    // This is to compare boost-intrusive-list performance
    // with dllist.
#ifdef HAS_BOOST_INTRUSIVE
    boost::intrusive::list_member_hook<> hook;
#endif

    static auto make(std::size_t numval = 0) {
        auto ptr = make_unique<testnode>();
        ptr->red = -numval;
        ptr->black = numval;
        return ptr;
    }
};

using dllist = tarp::dllist<testnode, &testnode::link>;

#ifdef HAS_BOOST_INTRUSIVE
using member_hook_opt = boost::intrusive::
  member_hook<testnode, boost::intrusive::list_member_hook<>, &testnode::hook>;
using boostlist = boost::intrusive::list<testnode, member_hook_opt>;
#endif

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
#ifdef HAS_BOOST_INTRUSIVE
        BOOST_VECT,
        BOOST_LIST,
#endif
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
#ifdef HAS_BOOST_INTRUSIVE
        case BOOST_LIST: return "boost_list[list]";
        case BOOST_VECT: return "boost_list[vect]";
#endif
        default: throw std::logic_error("bad what value");
        }
    };

    // Indexed by what
    std::vector<std::vector<float>> measurements;

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
#ifdef HAS_BOOST_INTRUSIVE
        boostlist list_backed_boostq;
        boostlist vect_backed_boostq;
#endif
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
#ifdef HAS_BOOST_INTRUSIVE
            vect_backed_boostq.push_back(*elem);
#endif
        }

        for (auto &x : list) {
            list_backed_q.push_back(x);
#ifdef HAS_BOOST_INTRUSIVE
            list_backed_boostq.push_back(x);
#endif
        }


        // meaningless computation that depends on value of volatile
        // variables; this therefore prevents optimization
        // (e.g. dead code elimination)
        const auto compute = [](auto &x) {
            if (sink1 % 2 == 0) {
                x.black++;
                sink1 = sink1 + 1;
                sink2 = sink2 + 10;
            } else {
                x.red--;
                sink1 = sink1 - 1;
                sink2 = sink2 + 2;
            }
        };


        using namespace std::chrono;
        constexpr std::size_t NUM_PASSES = 10;
        auto measure = [&compute](auto &l) {
            const auto start = steady_clock::now();
            for (unsigned i = 0; i < NUM_PASSES; ++i) {
                for (auto &x : l) {
                    compute(x);
                }
            }
            const auto end = steady_clock::now();
            const auto elapsed = duration_cast<usecs>(end - start).count();

            return elapsed;
        };

        using namespace std::chrono;
        auto measure_ptr = [&compute](auto &l) {
            const auto start = steady_clock::now();
            for (unsigned i = 0; i < NUM_PASSES; ++i) {
                for (auto &x : l) {
                    compute(*x);
                }
            }
            const auto end = steady_clock::now();
            const auto elapsed = duration_cast<usecs>(end - start).count();
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

#ifdef HAS_BOOST_INTRUSIVE
        res[what::BOOST_LIST] = measure(list_backed_boostq);
        res[what::BOOST_VECT] = measure(vect_backed_boostq);

        // must clear else boost will throw an exception saying the nodes
        // had not been unlinked before destruction (due to the default
        // 'safe' mode being used).
        vect_backed_boostq.clear();
        list_backed_boostq.clear();
#endif

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
#if 1
                                       8 * 1000 * 1000
#endif
};

    for (auto count : counts) {
        measurements.push_back(run_test(count));
    }

    // ===== dump csv =====

    // dump CSV columns
    std::cerr << std::format("{},{},{},{},{},{},{}"
#ifdef HAS_BOOST_INTRUSIVE
                             ",{},{}\n",
#else
                             "\n",
#endif
                             "count",
                             to_string(what::VECT_INPLACE),
                             to_string(what::VECT_PTR),
                             to_string(what::LIST_INPLACE),
                             to_string(what::LIST_PTR),
                             to_string(what::DLLIST_LIST),
                             to_string(what::DLLIST_VECT)
#ifdef HAS_BOOST_INTRUSIVE
                               ,
                             to_string(what::BOOST_LIST),
                             to_string(what::BOOST_VECT)
#endif
    );

    // dump csv rows
    auto &v = measurements;
    for (unsigned i = 0; i < measurements.size(); ++i) {
        std::cerr << std::format("{},{},{},{},{},{},{}"
#ifdef HAS_BOOST_INTRUSIVE
                                 ",{},{}\n",
#else
                                 "\n",
#endif
                                 counts[i],
                                 v[i][what::VECT_INPLACE],
                                 v[i][what::VECT_PTR],
                                 v[i][what::LIST_INPLACE],
                                 v[i][what::LIST_PTR],
                                 v[i][what::DLLIST_LIST],
                                 v[i][what::DLLIST_VECT]
#ifdef HAS_BOOST_INTRUSIVE
                                 ,
                                 v[i][what::BOOST_LIST],
                                 v[i][what::BOOST_VECT]
#endif
        );
    }

    REQUIRE(sink2 >= 10 * 1000 * 1000);
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
