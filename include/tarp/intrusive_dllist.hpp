#pragma once

/**************************************************************************.
 * Dllist - general-purpose intrusive doubly-linked list                   |
 *                                                                         |
 * NOTE: this is the C++ version of the C-based dllist structure in        |
 * libtarp. The implementation and documentation has been taken from there |
 * and adjusted as appropriate. See:                                       |
 * libtarp/include/tarp/dllist.h.                                          |
 *                                                                         |
 * === API ===                                                             |
 *---------------------                                                    |
 *                                                                         |
 * ~ Notes ~                                                               |
 *                                                                         |
 * The dllist is headed by a `dllist` structure, used to store various     |
 * fields for algorithmic efficiency. For example size() becomes O(1)      |
 * rather than O(n) by storing a size field in this structure.             |
 * The dllist, as the name implies, is a doubly-linked list (of            |
 * `dlnode`s intrusively embedded inside the user's own structures.        |
 *                                                                         |
 *  ~ Example ~                                                            |
 *                                                                         |
 *    struct mystruct{                                                     |
 *       uint32_t u;                                                       |
 *       dlnode link;  // must embed a struct dlnode                       |
 *    };                                                                   |
 *                                                                         |
 *    dllist<mystruct, dlnode, &mystruct::link> fifo;                      |
 *    dllist<mystruct, dlnode, &mystruct::link> lifo;                      |
 *                                                                         |
 *    // the dllist is non-owning; so typically you have one               |
 *    // owning structure of some kind, and any number of                  |
 *    // non-owning structures. OR you can of course manually              |
 *    // manage allocation & deallocation via new and delete.              |
 *    std::vector<std::unique_ptr<mystruct>> owner;                        |
 *    for (size_t i=0; i< 10; ++i){                                        |
 *       auto a = std::make_unique<mystruct>();                            |
 *       auto b = std::make_unique<mystruct>();                            |
 *       a->u = i; b->u = i;                                               |
 *       lifo.push_back(*a);   // stack push                               |
 *       fifo.push_back(*b);   // queue enqueue                            |
 *       owner.push_back(std::move(a));                                    |
 *       owner.push_back(std::move(b));                                    |
 *    }                                                                    |
 *                                                                         |
 *   for (auto &elem: lifo.iter()){                                        |
 *      std::cerr << "LIFO value: " << elem.u << std::endl;                |
 *   }                                                                     |
 *                                                                         |
 *   for (size_t i=0; i < 10; ++i){                                        |
 *       auto *a = lifo.pop_back();                                        |
 *       std::cerr << "popped " << a->u << std::endl;                      |
 *                                                                         |
 *       a = fifo.pop_front();                                             |
 *       std::cerr << "dequeued " << a->u << std::endl;                    |
 *   }                                                                     |
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~        |
 *                                                                         |
 *                                                                         |
 * === RUNTIME COMPLEXITY CHARACTERISTICS ===                              |
 *---------------------------------------------                            |
 * The dllist has two link pointers per node. At the cost of additional    |
 * space and pointer-manipulation overhead, the structure is more general  |
 * and allows (compared to a staq, see libtarp): |
 * - insertion and deletion at either end (deque)                          |
 * - removal of any node; insertion before and after any node. This is     |
 *   possible even while iterating.                                        |
 * - more efficient bidirectional rotation + rotation to specific node     |
 * - bidirectional iteration                                               |
 *                                                                         |
 * clear                    O(n)                                           |
 * size                     O(1)                                           |
 * empty                    O(1)                                           |
 * {front,back}             O(1)                                           |
 * push_front               O(1)                                           |
 * push_back                O(1)                                           |
 * pop_front                O(1)                                           |
 * pop_back                 O(1)                                           |
 * unlink                   O(1)                                           |
 * foreach                  O(n)                                           |
 * put_after                O(1)                                           |
 * put_before               O(1)                                           |
 * insert(iterator)         O(1)                                           |
 * replace                  O(1)                                           |
 * swap (list)              O(1)                                           |
 * swap (node)              O(1)                                           |
 * join                     O(1)                                           |
 * split                    O(n)                                           |
 * upend                    O(n)                                           |
 * rotate                   O(n)                                           |
 * rotateto                 O(1)                                           |
 * find_nth                 O(n)                                           |
 *                                                                         |
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ |
 *                                                                         |
 * ~ NOTES ~                                                               |
 *                                                                         |
 * Some of the operations are perhaps more efficient than their big-O      |
 * complexity suggests.                                                    |
 * - rotate() is linear simply because it calls Dll_find_nth. LESS than    |
 *   the length of the list is traversed exactly once. The number of       |
 *   pointers changed for the actual rotation is otherwise constant.       |
 * - split() is linear but LESS than the length of the original list is    |
 *   traversed exactly once. Pointer manipulation for the actual split is  |
 *   otherwise constant.                                                   |
 *************************************************************************/

#include "intrusive.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tarp {

// forward declaration
template<typename Parent, auto Member_ptr_or_hook_tag = 0>
class dllist;

// intrusive doubly linked list node.
// Any data structure that wants to be linked into
// one or more intrusive doubly linked lists must embed
// one or more nodes of this type.

// this is to enable multiple inheritance, thereby making
// it possible to have multiple 'hooks' when using the
// inheritance-based approach.
template<unsigned Tag>
struct tagged_dlnode {
    static inline constexpr unsigned TAG = Tag;

    tagged_dlnode() = default;

    bool is_linked() const { return prev != this || next != this; }

    // next=prev=this means unlinked;
    // next=prev=null means linked as an only node
    // in a 1-element list.
    void reset() { next = prev = this; }

    struct tagged_dlnode *next = this;
    struct tagged_dlnode *prev = this;
};

using dlnode = tagged_dlnode<0>;

namespace impl {
template<typename Parent, auto HookValue>
struct node_resolver {
    // Since the specialization catches integers,
    // this is always the container_of path.
    static constexpr bool is_inheritance = false;
    using type = dlnode;
};

// 2. Specialization: Strictly for the Tagged Inheritance path
template<typename Parent, auto Tag>
    requires std::is_integral_v<decltype(Tag)>
struct node_resolver<Parent, Tag> {
    static constexpr bool is_inheritance = true;
    using type = tagged_dlnode<Tag>;
};

}  // namespace impl

// Intrusive doubly linked list template class.
// This is templated in order to avoid having to
// manually specify container_of calls everywhere which
// would be both more inconvenient and more error prone.
//
// The class works in 2 ways:
//
// 1) using container_of to derive a pointer to Parent
// from a pointer to a dlnode member. This only works with
// object for which is_standard_layout_v=true.
//
// -> Parent
// The container type that embeds a dlnode which makes it
// part of this list.
//
// -> MemberPtr
// A pointer to the dlnode member of Parent (pointer to member).
//
// These 2 parameters are used internally in container_of calls.
//
// 2) Using inheritance: in this case, Parent is the user object
// and must publically inherit from dlnode. MemberPtr in this
// case is actually a tag number which is used to make it possible
// to inherit from multiple instances of dlnode.
template<typename Parent, auto MemberPtr>
class dllist final {
    using resolve = impl::node_resolver<Parent, MemberPtr>;

public:
    static inline constexpr bool inheritance_hook_v = resolve::is_inheritance;
    using node_type = resolve::type;
    using container_type = Parent;

private:
    static Parent *derive_container(node_type *node) {
        // C++ inheritance based: this supports non-standard
        // layout objects;
        // Get Parent (the container) back from a pointer to node.
        if constexpr (inheritance_hook_v) {
            return static_cast<Parent *>(node);
        }

        // explicit container_of-based: this only supports
        // standard-layout objects.
        else {
            return tarp::container_of<Parent, node_type, MemberPtr>(node);
        }
    }

    static const Parent *derive_container(node_type const *node) {
        if constexpr (inheritance_hook_v) {
            return static_cast<const Parent *>(*node);
        } else {
            return tarp::container_of<Parent, node_type, MemberPtr>(node);
        }
    }

public:
    // Bidirectional iterator over dllist.
    struct iterator {
        node_type *ptr = nullptr;

        // used when iterating backward
        dlnode *prevptr = nullptr;

        iterator(node_type *node) noexcept : ptr(node) {}

        iterator() = default;

        // Replace the current iterator pointer.
        void assign(node_type *node) noexcept {
            ptr = node;
            prevptr = ptr ? ptr->prev : nullptr;
        }

        // Replace the current iterator pointer.
        void assign(Parent *obj) noexcept {
            if (obj) {
                node_type *node = nullptr;
                if constexpr (inheritance_hook_v) {
                    node = obj;
                } else {
                    node = &(obj->*MemberPtr);
                }
                ptr = node;
                prevptr = ptr ? ptr->prev : nullptr;
            } else {
                ptr = nullptr;
                prevptr = nullptr;
            }
        }

        Parent &operator*() const noexcept { return *derive_container(ptr); }

        Parent &operator*() noexcept { return *derive_container(ptr); }

        Parent *operator->() const noexcept { return derive_container(ptr); }

        Parent *operator->() noexcept { return derive_container(ptr); }

        Parent *prev() const noexcept { return derive_container(prevptr); }

        Parent *prev() noexcept { return derive_container(prevptr); }

        Parent *next() const noexcept { return derive_container(ptr->next); }

        Parent *next() noexcept { return derive_container(ptr->next); }

        iterator &operator++() noexcept {
            // it is the caller's responsibility not to increment
            // an end iterator. Checking here would be safer but slower
            // in tight loops.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
            prevptr = ptr;
            ptr = ptr->next;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
            return *this;
        }

        iterator &operator--() noexcept {
            ptr = prevptr;
            prevptr = ptr ? ptr->prev : nullptr;
            return *this;
        }

        bool operator==(const iterator &rhs) const {
            return rhs.ptr == ptr && rhs.prevptr == prevptr;
        }

        bool operator!=(const iterator &rhs) const { return !operator==(rhs); }

        iterator erase(dllist &list) noexcept {
            const auto curr = ptr;
            ptr = ptr->next;
            list.unlink(*curr);

            // if erased the last element, then ptr
            // will be null, in which case we take list.m_back,
            // to effectively make the iterator the _end_ iterator,
            // so it can be decremented as usual.
            prevptr = ptr ? ptr->prev : list.m_back;

            return *this;
        }

        std::string str() const {
            std::string s;
            s +=
              "{.prev=" +
              std::to_string(reinterpret_cast<std::uintptr_t>(prevptr)) +
              " .ptr=" + std::to_string(reinterpret_cast<std::uint64_t>(ptr)) +
              "}";
            return s;
        }
    };

    iterator begin() const noexcept {
        iterator it;
        it.ptr = m_front;
        return it;
    }

    iterator end() const noexcept {
        iterator it;
        it.prevptr = m_back;
        return it;
    }

    // If it is the end iterator (nullptr), nothing is done.
    // Return an iterator to the element following the erased one.
    iterator erase(iterator it) noexcept {
        if (!it.ptr) {
            return end();
        }

        const auto next = it.ptr->next;
        unlink(*it.ptr);
        it.ptr = next;
        it.prevptr = it.ptr ? it.ptr->prev : m_back;
        return it;
    }

    // Erase all elements for which pred returns true.
    template<typename Pred>
    void erase_if(Pred &&pred) {
        for (auto it = begin(); it != end();) {
            if (pred(*it)) {
                it = erase(it);
            } else {
                ++it;
            }
        }
    }

    // Invoke f for each element in the list;
    // the elements must not be erased or moved
    // and the list structure should not be mutated (insertions, erasures)
    // inside the callback.
    template<typename F>
    void for_each(F &&f) {
        for (auto &cont : *this) {
            f(cont);
        }
    }

    // Copy constructor and copy assigment operator disabled.
    // We cannot have the same exact node_type in multiple lists
    // for obious reasons: its .next and .prev would get mutated
    // in both places.
    dllist(const dllist &) = delete;
    dllist &operator=(const dllist &) = delete;

    // Move constructor and move assignment operator enabled.
    dllist(dllist &&other) { swap(other); }

    dllist &operator=(dllist &&other) {
        swap(other);
        return *this;
    }

    dllist() = default;

    // Given a pointer to a node, get a pointer to its parent container.
    Parent *get_container(node_type *node) { return derive_container(node); }

    // Given a pointer to a node, get a pointer to its parent container.
    Parent *get_container(node_type *node) const {
        return derive_container(node);
    }

    // Given a reference to a node, get a reference to its parent container.
    Parent &container_of(node_type &node) { return *derive_container(node); }

    // Given a reference to a node, get a reference to its parent container.
    const Parent &container_of(const node_type &node) const {
        return *derive_container(node);
    }

    // Remove and return the last element in the list.
    // Null if the list is empty.
    Parent *pop_back();

    // Remove and return the first element in the list.
    // Null if the list is empty.
    Parent *pop_front();

    // Get reference to the first element in the list.
    // This must not be called on an empty list.
    auto &front() { return *derive_container(m_front); }

    // Get reference to the first element in the list.
    // This must not be called on an empty list.
    auto &front() const { return *derive_container(m_front); }

    // Get reference to the last element in the list.
    // This must not be called on an empty list.
    auto &back() { return *derive_container(m_back); }

    // Get reference to the last element in the list.
    // This must not be called on an empty list.
    auto &back() const { return *derive_container(m_back); }

    // True if the front of the list has the same address
    // as the given node.
    // This must not be called on an empty list.
    bool front_equals(const node_type &node) const {
        if (m_count == 0) {
            return false;
        }
        return m_front == &node;
    }

    // True if the front of the list has the same address
    // as the given element.
    // This must not be called on an empty list.
    bool front_equals(const Parent &ref) const {
        if (m_count == 0) {
            return false;
        }
        return get_container(m_front) == &ref;
    }

    // True if the tail of the list has the same address
    // as the given node.
    // This must not be called on an empty list.
    bool back_equals(const node_type &node) const {
        if (m_count == 0) {
            return false;
        }
        return m_back == &node;
    }

    // True if the tail of the list has the same address
    // as the given element.
    // This must not be called on an empty list.
    bool back_equals(const Parent &ref) const {
        if (m_count == 0) {
            return false;
        }
        return derive_container(m_front) == &ref;
    }

    // Prepend the element to the front of the list.
    void push_front(Parent &);

    // Append the element to the back of the list.
    void push_back(Parent &);

    // Reset the list so that it is empty.
    // Note this only discards all links: nothing is actually
    // destructed or freed (deallocated) since the dllist is
    // non-owning.
    void clear() {
        // we explicitly remove every single node so that we reset
        // each node's links. Otherwise the nodes are left with
        // dangling pointers and is_linked() will erroneously
        // return true for a given node!
        while (!empty()) {
            pop_front();
        }
        m_front = m_back = nullptr;
        m_count = 0;
    }

    // Return the number of elements in the list.
    std::size_t size() const { return m_count; }

    // True if the number of elements in the list is 0.
    bool empty() const { return m_count <= 0; }

    // Apend all elements in other to the end of the
    // current list, such that other's front is joined
    // to the back of this.
    // After the operation other will be empty.
    void join(dllist &other);

    // Given a reference to the element in the list,
    // remove elem and all other elements following it
    // from the current list and move them to a new list,
    // with elem as its front. Return the new list.
    dllist split(Parent &elem);

    // Rotate the the list in the specified direction <num_rotations>
    // times.
    // if dir>0, rotate toward the front of the list;
    // if dir<0, rotate toward the back.
    void rotate(int dir, std::size_t num_rotations);

    // Given a reference to an element in the list, rotate
    // the list such that elem ends up at the front.
    void rotate_to(Parent &elem);

    // Return a pointer to the nth element in the list.
    // nullptr if no such element exists.
    Parent *find_nth(std::size_t n);

    // Reverse all elements in the list.
    void upend();

    // Given a referce to an element in the list, unlink the element
    // such that it is no longer part of the list.
    void unlink(Parent &elem);

    // Given a reference to a node not currently in the list,
    // insert it into the list after node_before.
    void put_after(Parent &node_before, Parent &node);

    // Given a reference to a node not currently in the list,
    // insert it into the list before node_after.
    void put_before(Parent &node_after, Parent &node);

    // Given a _valid_ iterator, insert node before it,
    // and return an iterator to the inserted element
    // (previous iterator is invalidated).
    iterator insert(iterator it, Parent &node);

    // Given a reference to a node that exists in the list (a)
    // and a node that does not exist in the list (b),
    // replace a with b in the list. After the operation
    // b exists in the list where a used to b, and a is no
    // longer in the list.
    Parent *replace(Parent &a, Parent &b);

    // Given a reference to two nodes a and b that both
    // exist in the list, switch their positions.
    void swap(Parent &a, Parent &b);

    // Swap the contents of this and other, including the
    // size field and all links.
    void swap(dllist &other);

private:
    void swap(node_type &a, node_type &b);
    void swap_list_heads(dllist &other);
    static std::size_t count_list_nodes(dllist &list);
    node_type *find_nth_node(std::size_t n);

    void put_after(node_type &node_before, node_type &node);
    void put_before(node_type &node_after, node_type &node);

    node_type *replace_node(node_type &a, node_type &b);
    dllist split(node_type &node);

    void push_front(node_type &);
    void push_back(node_type &);

    node_type *pop_front_node();
    node_type *pop_back_node();

    void rotate_to(node_type &);

    void unlink(node_type &);

private:
    // number of elements in list
    std::size_t m_count = 0;

    // list head
    node_type *m_front = nullptr;

    // list tail
    node_type *m_back = nullptr;
};

//

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr>::node_type *
dllist<Parent, MemberPtr>::pop_front_node() {
    node_type *node = m_front;
    switch (m_count) {
    case 0: return nullptr;
    case 1: m_front = m_back = nullptr; break;
    case 2:
        m_front = m_back = node->next;
        m_front->prev = m_front->next = nullptr;
        break;
    default:
        m_front = node->next;
        m_front->prev = nullptr;
        break;
    }
    m_count--;
    node->reset();
    return node;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::pop_front() {
    return this->get_container(pop_front_node());
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr>::node_type *
dllist<Parent, MemberPtr>::pop_back_node() {
    node_type *node = m_back;

    switch (m_count) {
    case 0: return nullptr;
    case 1: m_front = m_back = nullptr; break;
    case 2:
        m_front = m_back = node->prev;
        m_front->prev = m_front->next = nullptr;
        break;
    default:
        m_back = node->prev;
        m_back->next = nullptr;
        break;
    }
    m_count--;
    node->reset();
    return node;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::pop_back() {
    return this->get_container(pop_back_node());
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_front(node_type &node) {
    node.prev = nullptr;
    node.next = m_front;
    m_front = &node;
    if (m_count == 0) m_back = m_front;
    else m_front->next->prev = m_front;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_front(Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node = obj;
        push_front(node);
    } else {
        auto &node = obj.*MemberPtr;
        push_front(node);
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_back(node_type &node) {
    node.next = nullptr;
    node.prev = m_back;
    m_back = &node;
    if (m_count == 0) m_front = m_back;
    else m_back->prev->next = m_back;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_back(Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node = obj;
        push_back(node);
    } else {
        auto &node = obj.*MemberPtr;
        push_back(node);
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::join(dllist &other) {
    if (m_back) { /* non-empty list */
        m_back->next = other.m_front;
        other.m_front->prev = m_back;
    } else { /* empty list */
        m_front = other.m_front;
        m_back = other.m_back;
    }

    m_count += other.m_count;

    // resets the variables in other.
    [[maybe_unused]] dllist tmp = std::move(other);
}

template<typename Parent, auto MemberPtr>
std::size_t dllist<Parent, MemberPtr>::count_list_nodes(dllist &list) {
    node_type *node = list.m_front;
    std::size_t count = 0;
    while (node) {
        node = node->next;
        ++count;
    }
    return count;
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr> dllist<Parent, MemberPtr>::split(node_type &node) {
    std::size_t orig_len = m_count;
    dllist b;

    if (&node == m_front) {
        swap_list_heads(b);
        return b;
    }

    b.m_front = &node;
    if (!node.next) b.m_back = &node;

    if (&node == m_back) {
        pop_back();
    } else {
        m_back = node.prev;
        node.prev->next = nullptr;
    }

    node.prev = nullptr;
    b.m_count = count_list_nodes(b);
    m_count = (orig_len - b.m_count);
    return b;
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr> dllist<Parent, MemberPtr>::split(Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node = obj;
        return split(node);
    } else {
        auto &node = obj.*MemberPtr;
        return split(node);
    }
}

// swap the contents of this list and b
template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::swap_list_heads(dllist &b) {
    dllist tmp;
    tmp.m_count = m_count;
    tmp.m_front = m_front;
    tmp.m_back = m_back;

    m_front = b.m_front;
    m_back = b.m_back;
    m_count = b.m_count;

    b.m_front = tmp.m_front;
    b.m_back = tmp.m_back;
    b.m_count = tmp.m_count;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::swap(dllist &other) {
    swap_list_heads(other);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::upend() {
    if (m_count < 2) return;
    if (!m_front) return;

    node_type *a, *b, *c;
    a = b = c = m_front;
    while (a && a->next) {
        b = a->next;
        a->next = b->next;
        b->next = c;
        if (c) c->prev = b;
        c = b;
    }

    a->next = nullptr;
    b->prev = nullptr;
    m_back = a;
    m_front = b;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::rotate(int dir, std::size_t num_rotations) {
    if (dir == 0) return;  // invalid

    if (m_count < 2 || num_rotations == 0) return;  // nothing to do

    std::size_t remainder = num_rotations % m_count;

    /* sq->count rotations does nothing as the stack ends up unchanged;
     * so we can discount multiples of sq->count and only need to do
     * (num_rotations mod sq->count). Since a mod n == (a+kn) mod n. */
    num_rotations = remainder;
    /* no effect, stack would be left unchanged */
    if (num_rotations == 0) return;

    /* n rotations one way is the same as list->count-n rotations the other
     * way; always rotate toward the front by appropriate amount even when
     * asked to rotate toward the back */
    num_rotations = (dir > 0) ? num_rotations : (m_count - num_rotations);
    // debug("final num_rotations %zu", num_rotations);

    auto *node = find_nth(num_rotations + 1);
    if (!node) {
        throw std::logic_error("BUG: find_nth returned nullptr when rotating");
    }

    rotate_to(*node);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::rotate_to(node_type &node) {
    if (&node == m_front) return;

    /* link front and back */
    m_back->next = m_front;
    m_front->prev = m_back;

    /* move head to node and tail to node->prev */
    m_front = &node;
    m_back = node.prev;

    /* unlink front and back */
    m_front->prev = nullptr;
    m_back->next = nullptr;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::rotate_to(Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node = obj;
        rotate_to(node);
    } else {
        auto &node = obj.*MemberPtr;
        rotate_to(node);
    }
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr>::node_type *
dllist<Parent, MemberPtr>::find_nth_node(std::size_t n) {
#if 0
    std::cerr << "dllist.find_nth(" << n << "), m_count=" << m_count
              << ", m_front=" << static_cast<void *>(m_front)
              << ", m_back=" << static_cast<void *>(m_back) << std::endl;
#endif
    node_type *node = nullptr;
    if (n == 0 || n > m_count) {
        return nullptr;
    }

    /* start from the end closer to the node */
    if (n <= m_count / 2) {
        node = m_front;
        for (std::size_t i = 1; i < n && node != nullptr;
             ++i, node = node->next) {
#if 0
            std::cerr << " node= " << static_cast<void *>(node) << " prev="
                      << (node ? static_cast<void *>(node->prev) : nullptr)
                      << ", next="
                      << (node ? static_cast<void *>(node->next) : nullptr)
                      << std::endl;
#endif
        }
        return node;
    }

    n = m_count - n; /* start from the back */
    node = m_back;
    for (std::size_t i = 0; i < n && node != nullptr; ++i, node = node->prev);
    return node;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::find_nth(std::size_t n) {
    auto node = find_nth_node(n);
    if (!node) {
        return nullptr;
    }
    return this->get_container(node);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::unlink(node_type &node) {
    if (m_count <= 0) {
        return;
    }

    if (&node == m_front) {
        pop_front();
    } else if (&node == m_back) {
        pop_back();
    } else if (node.is_linked() == false) {
        return;
    } else {
        node.next->prev = node.prev;
        node.prev->next = node.next;
        m_count--;
    }

    node.reset();
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::unlink(Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node = obj;
        unlink(node);
    } else {
        auto &node = obj.*MemberPtr;
        unlink(node);
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_after(node_type &node_before,
                                          node_type &node) {
    if (&node_before == m_back) {
        push_back(node);
        return;
    }

    node.next = node_before.next;
    node.next->prev = &node;
    node.prev = &node_before;
    node_before.next = &node;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_after(Parent &obj_before, Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node_before = obj_before;
        node_type &node = obj;
        put_after(node_before, node);
    } else {
        auto &node_before = obj_before.*MemberPtr;
        auto &node = obj.*MemberPtr;
        put_after(node_before, node);
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_before(node_type &node_after,
                                           node_type &node) {
    if (&node_after == m_front) {
        push_front(node);
        return;
    }

    node.prev = node_after.prev;
    node.prev->next = &node;
    node.next = &node_after;
    node_after.prev = &node;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_before(Parent &obj_after, Parent &obj) {
    if constexpr (inheritance_hook_v) {
        node_type &node_after = obj_after;
        node_type &node = obj;
        put_before(node_after, node);
    } else {
        auto &node_after = obj_after.*MemberPtr;
        auto &node = obj.*MemberPtr;
        put_before(node_after, node);
    }
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr>::iterator
dllist<Parent, MemberPtr>::insert(iterator it, Parent &obj) {
    if (it == begin()) {
        push_front(obj);
        return begin();
    }

    if (it == end()) {
        push_back(obj);
        return --end();
    }

    // we assume here the iterator is indeed _valid_.
    put_before(*it, obj);
    return --it;
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr>::node_type *
dllist<Parent, MemberPtr>::replace_node(node_type &a, node_type &b) {
    b.next = a.next;
    b.prev = a.prev;

    if (b.next) b.next->prev = &b;
    else m_back = &b;

    if (b.prev) b.prev->next = &b;
    else m_front = &b;

    a.reset();
    return &b;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::replace(Parent &a, Parent &b) {
    if constexpr (inheritance_hook_v) {
        node_type &node_a = a;
        node_type &node_b = b;
        replace_node(node_a, node_b);
        return &b;
    } else {
        auto &node_a = a.*MemberPtr;
        auto &node_b = b.*MemberPtr;
        replace_node(node_a, node_b);
        return &b;
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::swap(node_type &a, node_type &b) {
    if (&a == &b) {
        return;
    }

    node_type *before_a = a.prev;
    unlink(a);
    replace(b, a);

    if (before_a) put_after(*before_a, b);
    else push_front(b);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::swap(Parent &a, Parent &b) {
    auto &node_a = a.*MemberPtr;
    auto &node_b = b.*MemberPtr;
    swap(node_a, node_b);
}
};  // namespace tarp
