#include <array>
#include <memory>
#include <type_traits>

#include <tarp/intrusive.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using buff_t = std::vector<std::uint8_t>;

using namespace std;

using tarp::container_of;
using tarp::is_safely_offsettable;
using tarp::is_safely_offsettable_v;
using tarp::SafelyOffsettable;

// intrusive doubly linked list node
struct dlnode {
    dlnode *next = nullptr;
    dlnode *prev = nullptr;
};

template<typename Derived>
class mybase {
public:
    int one = 0;
    int two = 0;
    int three = 0;
    std::array<unsigned, 100> data;
    dlnode list;

    Derived &derived() { return static_cast<Derived &>(*this); }
};

TEST_CASE("Test container_of") {
    SUBCASE("simple standard-layout structs") {
        struct mystruct {
            mystruct(int the_id, const std::string &the_name)
                : id(the_id), name(the_name) {}

            const int id = 0;
            dlnode q1;
            dlnode q2;
            const std::string name;

            auto get_id() const { return id; }

            auto get_name() const { return name; }
        };

        auto a = std::make_unique<mystruct>(1, "nothing");

        mystruct *recovered1 =
          container_of<mystruct, dlnode, &mystruct::q1>(&a->q1);
        REQUIRE(recovered1->id == a->id);
        REQUIRE(recovered1->name == a->name);
        REQUIRE(recovered1 == a.get());

        mystruct *recovered2 =
          container_of<mystruct, dlnode, &mystruct::q2>(&a->q2);
        REQUIRE(recovered2->id == a->id);
        REQUIRE(recovered2->name == a->name);
        REQUIRE(recovered2 == a.get());
    }

    SUBCASE("legal inheritance that preserves standard-layout-ness") {
        // static_assert(!std::is_standard_layout_v<std::vector<std::uint8_t>>);

        struct base {};

        static_assert(std::is_standard_layout_v<base>);

        struct mystruct : public base {
            mystruct(int the_id, const std::string &the_name)
                : id(the_id), name(the_name) {}

            const int id = 0;
            dlnode q1;
            dlnode q2;
            const std::string name;

            auto get_id() const { return id; }

            auto get_name() const { return name; }
        };

        static_assert(std::is_standard_layout_v<mystruct>);

        auto a = std::make_unique<mystruct>(1, "nothing");

        mystruct *recovered1 =
          container_of<mystruct, dlnode, &mystruct::q1>(&a->q1);
        REQUIRE(recovered1->id == a->id);
        REQUIRE(recovered1->name == a->name);
        REQUIRE(recovered1 == a.get());

        mystruct *recovered2 =
          container_of<mystruct, dlnode, &mystruct::q2>(&a->q2);
        REQUIRE(recovered2->id == a->id);
        REQUIRE(recovered2->name == a->name);
        REQUIRE(recovered2 == a.get());
    }

    SUBCASE("inheritance from virtual base struct should fail") {
        struct parent {
            virtual unsigned get_some_int() = 0;
        };

        struct mystruct : public parent {
            unsigned int get_some_int() override { return 1000; }

            mystruct(int the_id, const std::string &the_name)
                : id(the_id), name(the_name) {}

            const int id = 0;
            dlnode q1;
            dlnode q2;
            const std::string name;
        };

        auto a = std::make_unique<mystruct>(1, "nothing");
        REQUIRE(is_safely_offsettable_v<mystruct, dlnode, &mystruct::q1> ==
                false);
    }

    // For complex cases, move the complexity in a derived class that we can get
    // to from a simple standard-layout base class via CRTP.
    SUBCASE(
      "Test complex derived class inheriting from simple class with CRTP") {
        // static_assert(!std::is_standard_layout_v<std::vector<std::uint8_t>>);

        struct mystruct : public mybase<mystruct> {
            mystruct(int the_id, const std::string &the_name)
                : id(the_id), name(the_name) {}

        private:
            const int id = 0;
            dlnode q1;
            dlnode q2;
            const std::string name;

        public:
            auto &list() { return mybase::list; }

            auto get_id() const { return id; }

            auto get_name() const { return name; }
        };

        static_assert(std::is_standard_layout_v<mybase<mystruct>>);

        static_assert(!std::is_standard_layout_v<mystruct>);

        auto a = std::make_unique<mystruct>(1, "nothing");

        auto *recovered1 =
          container_of<mystruct::mybase, dlnode, &mystruct::mybase::list>(
            &a->list());
        REQUIRE(recovered1 == a.get());
        REQUIRE(recovered1->derived().get_id() == a->get_id());
        REQUIRE(recovered1->derived().get_name() == a->get_name());
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
