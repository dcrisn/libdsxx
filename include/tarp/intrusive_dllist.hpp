#pragma once

#include "tarp/intrusive.hpp"
#include <cstdint>
#include <stdexcept>
#include <tuple>

namespace tarp {

template<typename Parent, auto Member_ptr>
class dllist;

struct dlnode {
    template<typename Parent, auto Member_ptr>
    auto *container() {
        return container_of<Parent, dlnode, Member_ptr>(this);
    }

    bool is_linked() const { return prev != nullptr || next != nullptr; }

    void unlink(auto &list) {
        if (is_linked() || list.front_equals(*this) ||
            list.back_equals(*this)) {
            list.unlink(*this);
        }
    }

    struct dlnode *next = nullptr;
    struct dlnode *prev = nullptr;
};

template<typename Parent, auto MemberPtr>
class dllist final {
public:
    struct iterator {
        dlnode *ptr = nullptr;

        dlnode &operator*() const { return *ptr; }

        dlnode *operator->() const { return ptr; }

        iterator &operator++() {
            if (ptr) ptr = ptr->next;
            return *this;
        }

        bool operator!=(const iterator &other) const {
            return ptr != other.ptr;
        }
    };

    struct reverse_iterator {
        dlnode *ptr = nullptr;

        dlnode &operator*() const { return *ptr; }

        dlnode *operator->() const { return ptr; }

        reverse_iterator &operator++() {
            if (ptr) ptr = ptr->prev;
            return *this;
        }

        bool operator!=(const reverse_iterator &other) const {
            return ptr != other.ptr;
        }
    };

    struct container_iterator {
        container_iterator(dlnode *first) { ptr = first; }

        dlnode *ptr = nullptr;

        Parent &operator*() const {
            return *(tarp::container_of<Parent, dlnode, MemberPtr>(ptr));
        }

        Parent &operator*() {
            return *(tarp::container_of<Parent, dlnode, MemberPtr>(ptr));
        }

        Parent *operator->() const {
            return tarp::container_of<Parent, dlnode, MemberPtr>(ptr);
        }

        Parent *operator->() {
            return tarp::container_of<Parent, dlnode, MemberPtr>(ptr);
        }

        container_iterator &operator++() {
            if (ptr) ptr = ptr->next;
            return *this;
        }

        bool operator!=(const container_iterator &other) const {
            return ptr != other.ptr;
        }

        container_iterator erase(dllist &list) {
            if (ptr == nullptr) {
                return *this;
            }
            const auto curr = ptr;
            ptr = ptr->next;
            list.unlink(*curr);
            return *this;
        }
    };

    struct container_reverse_iterator {
        container_reverse_iterator(dlnode &last) : ptr(&last) {}

        dlnode *ptr;

        Parent *operator*() const {
            return container_of<Parent, dlnode, MemberPtr>(ptr);
        }

        container_reverse_iterator &operator++() {
            if (ptr) ptr = ptr->prev;
            return *this;
        }

        bool operator!=(const container_reverse_iterator &other) const {
            return ptr != other.ptr;
        }

        container_reverse_iterator erase(dllist &list) {
            if (ptr == nullptr) {
                return *this;
            }
            const auto curr = ptr;
            ptr = ptr->prev;
            list.unlink(*curr);
            return *this;
        }
    };

    auto iter() {
        struct it {
            it(dlnode *first) : start(first) {}

            dlnode *start = nullptr;

            auto begin() { return container_iterator {start}; }

            auto end() { return container_iterator {nullptr}; }

            auto iterator(Parent *current) {
                if (!current) {
                    return container_iterator(nullptr);
                }

                auto *node = &((*current).*MemberPtr);
                return container_iterator {node};
            }
        };

        return it(m_front);
    }

    auto iter() const {
        struct it {
            it(dlnode *first) : start(first) {}

            dlnode *start = nullptr;

            auto begin() const { return container_iterator {start}; }

            auto end() const { return container_iterator {nullptr}; }

            auto iterator(Parent *current) {
                if (!current) {
                    return container_iterator(nullptr);
                }

                auto *node = &((*current).*MemberPtr);
                return container_iterator {node};
            }
        };

        return it(m_front);
    }

    auto riter() {
        struct it {
            it(dlnode *last) : start(last) {}

            dlnode *start = nullptr;

            auto begin() { return container_reverse_iterator {start}; }

            auto end() { return container_reverse_iterator {nullptr}; }

            auto iterator(dlnode *current) {
                return container_reverse_iterator {current};
            }
        };

        return it(m_back);
    }

    auto riter() const {
        struct it {
            it(dlnode *last) : start(last) {}

            dlnode *start = nullptr;

            auto begin() const { return container_reverse_iterator {start}; }

            auto end() const { return container_reverse_iterator {nullptr}; }
        };

        return it(m_back);
    }

    iterator begin() const { return {m_front}; }

    iterator end() const { return {nullptr}; }

    reverse_iterator rbegin() const { return {m_back}; }

    reverse_iterator rend() const { return {nullptr}; }

    iterator erase(iterator it) {
        if (!it.ptr) {
            return end();
        }

        const auto next = it.ptr->next;
        unlink(*it.ptr);
        it.ptr = next;
        return it;
    }

    reverse_iterator erase(reverse_iterator it) {
        if (!it.ptr) {
            return rend();
        }

        const auto prev = it.ptr->prev;
        unlink(*it.ptr);
        it.ptr = prev;
        return it;
    }

    template<typename Pred>
    void erase_if() {
        for (auto it = begin(); it != end();) {
            if (Pred(*it)) {
                it = erase(it);
            } else {
                ++it;
            }
        }
    }

    template<typename Pred>
    void erase_if(Pred &&pred) {
        for (auto it = begin(); it != end();) {
            auto cont = it->template container<Parent, MemberPtr>();
            if (pred(cont)) {
                it = erase(it);
            } else {
                ++it;
            }
        }
    }

#if 0
    template<typename F>
    void for_each(F &&f) {
        for (auto it = begin(); it != end();) {
            f(*it);
            ++it;
        }
    }
#endif

    template<typename F>
    void for_each(F &&f) {
        for (auto it = begin(); it != end();) {
            auto cont = it->template container<Parent, MemberPtr>();
            f(*cont);
            ++it;
        }
    }

    dllist(const dllist &) = delete;
    dllist &operator=(const dllist &) = delete;

    dllist(dllist &&other) { swap(other); }

    dllist &operator=(dllist &&other) {
        swap(other);
        return *this;
    }

    dllist() = default;

    Parent *get_container(dlnode *node) {
        return tarp::container_of<Parent, dlnode, MemberPtr>(node);
    }

    Parent *get_container(dlnode *node) const {
        return tarp::container_of<Parent, dlnode, MemberPtr>(node);
    }

    Parent &container_of(dlnode &node) {
        return *(tarp::container_of<Parent, dlnode, MemberPtr>(&node));
    }

    const Parent &container_of(const dlnode &node) {
        return *(tarp::container_of<Parent, dlnode, MemberPtr>(&node));
    }

    static Parent &get_parent(dlnode &node) {
        return *(tarp::container_of<Parent, dlnode, MemberPtr>(&node));
    }

    static const Parent &get_parent(const dlnode &node) {
        return *(tarp::container_of<Parent, dlnode, MemberPtr>(&node));
    }

    Parent *pop_back();

    Parent *pop_front();

    auto &front() { return this->container_of(*m_front); }

    auto &front() const { return this->container_of(*m_front); }

    auto &back() { return this->container_of(*m_back); }

    auto &back() const { return this->container_of(*m_back); }

    bool front_equals(const dlnode &node) const {
        if (m_count == 0) {
            return false;
        }
        return m_front == &node;
    }

    bool front_equals(const Parent &ref) const {
        if (m_count == 0) {
            return false;
        }
        return (this->get_container(m_front)) == &ref;
    }

    bool back_equals(const dlnode &node) const {
        if (m_count == 0) {
            return false;
        }
        return m_back == &node;
    }

    bool back_equals(const Parent &ref) const {
        if (m_count == 0) {
            return false;
        }
        return &(this->container_of(m_front)) == &ref;
    }

    void push_front(Parent &);

    void push_back(Parent &);

    // only unlinks, does not free anything.
    void clear() {
        m_front = m_back = nullptr;
        m_count = 0;
    }

    std::size_t size() const { return m_count; }

    bool empty() const { return m_count <= 0; }

    void join(dllist &other);

    void rotate(int dir, std::size_t num_rotations);

    void rotate_to(dlnode &);
    void rotate_to(Parent &);

    Parent *find_nth(std::size_t n);

    dllist split(Parent &);

    void upend();

    void unlink(dlnode &);
    void unlink(Parent &);

    void put_after(Parent &node_before, Parent &node);
    void put_before(Parent &node_after, Parent &node);

    Parent *replace(Parent &a, Parent &b);

    void swap(Parent &a, Parent &b);
    void swap(dlnode &a, dlnode &b);
    void swap(dllist &other);

private:
    void swap_list_heads(dllist &other);
    static std::size_t count_list_nodes(dllist &list);
    dlnode *find_nth_node(std::size_t n);

    void put_after(dlnode &node_before, dlnode &node);
    void put_before(dlnode &node_after, dlnode &node);

    dlnode *replace_node(dlnode &a, dlnode &b);
    dllist split(dlnode &node);

    void push_front(dlnode &);
    void push_back(dlnode &);

    dlnode *pop_front_node();
    dlnode *pop_back_node();

private:
    std::size_t m_count = 0;
    dlnode *m_front = nullptr;
    dlnode *m_back = nullptr;
};

//


template<typename Parent, auto MemberPtr>
dlnode *dllist<Parent, MemberPtr>::pop_front_node() {
    dlnode *node = m_front;
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
    return node;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::pop_front() {
    return this->get_container(pop_front_node());
}

template<typename Parent, auto MemberPtr>
dlnode *dllist<Parent, MemberPtr>::pop_back_node() {
    dlnode *node = m_back;

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
    return node;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::pop_back() {
    return this->get_container(pop_back_node());
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_front(dlnode &node) {
    node.prev = nullptr;
    node.next = m_front;
    m_front = &node;
    if (m_count == 0) m_back = m_front;
    else m_front->next->prev = m_front;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_front(Parent &obj) {
    auto &node = obj.*MemberPtr;
    push_front(node);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_back(dlnode &node) {
    node.next = nullptr;
    node.prev = m_back;
    m_back = &node;
    if (m_count == 0) m_front = m_back;
    else m_back->prev->next = m_back;
    m_count++;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::push_back(Parent &obj) {
    auto &node = obj.*MemberPtr;
    push_back(node);
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
    other.clear();
}

template<typename Parent, auto MemberPtr>
std::size_t dllist<Parent, MemberPtr>::count_list_nodes(dllist &list) {
    struct dlnode *node = list.m_front;
    std::size_t count = 0;
    while (node) {
        node = node->next;
        ++count;
    }
    return count;
}

template<typename Parent, auto MemberPtr>
dllist<Parent, MemberPtr> dllist<Parent, MemberPtr>::split(dlnode &node) {
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
    auto &node = obj.*MemberPtr;
    return split(node);
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

    struct dlnode *a, *b, *c;
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
void dllist<Parent, MemberPtr>::rotate_to(dlnode &node) {
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
    auto &node = obj.*MemberPtr;
    rotate_to(node);
}

template<typename Parent, auto MemberPtr>
dlnode *dllist<Parent, MemberPtr>::find_nth_node(std::size_t n) {
#if 0
    std::cerr << "dllist.find_nth(" << n << "), m_count=" << m_count
              << ", m_front=" << static_cast<void *>(m_front)
              << ", m_back=" << static_cast<void *>(m_back) << std::endl;
#endif
    struct dlnode *node = nullptr;
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
void dllist<Parent, MemberPtr>::unlink(dlnode &node) {
    if (m_count <= 0) {
        throw std::invalid_argument("trying to unlink node from empty list");
    }

    if (&node == m_front) {
        pop_front();
    } else if (&node == m_back) {
        pop_back();
    } else {
        node.next->prev = node.prev;
        node.prev->next = node.next;
        m_count--;
    }
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::unlink(Parent &obj) {
    auto &node = obj.*MemberPtr;
    unlink(node);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_after(dlnode &node_before, dlnode &node) {
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
    auto &node_before = obj_before.*MemberPtr;
    auto &node = obj.*MemberPtr;
    put_after(node_before, node);
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::put_before(dlnode &node_after, dlnode &node) {
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
    auto &node_after = obj_after.*MemberPtr;
    auto &node = obj.*MemberPtr;
    put_before(node_after, node);
}

template<typename Parent, auto MemberPtr>
dlnode *dllist<Parent, MemberPtr>::replace_node(dlnode &a, dlnode &b) {
    b.next = a.next;
    b.prev = a.prev;

    if (b.next) b.next->prev = &b;
    else m_back = &b;

    if (b.prev) b.prev->next = &b;
    else m_front = &b;

    return &b;
}

template<typename Parent, auto MemberPtr>
Parent *dllist<Parent, MemberPtr>::replace(Parent &a, Parent &b) {
    auto &node_a = a.*MemberPtr;
    auto &node_b = b.*MemberPtr;
    replace_node(node_a, node_b);
    return &b;
}

template<typename Parent, auto MemberPtr>
void dllist<Parent, MemberPtr>::swap(dlnode &a, dlnode &b) {
    if (&a == &b) {
        return;
    }

    struct dlnode *before_a = a.prev;
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
