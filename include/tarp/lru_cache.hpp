#pragma once

#include <unordered_map>

#include "intrusive_dllist.hpp"

#include <iostream>

namespace tarp {

// A class implementing a Cache with a Least-Recently-Used eviction policy.
// put() and get() both run in O(1).
template<typename Key, typename T>
class lru_cache {
private:
    struct lru_node {
        tarp::dlnode link;

        // map lookup key
        Key k {};

        // application data
        T v {};
    };

    using lru_list_t = tarp::dllist<lru_node, &lru_node::link>;

public:
    lru_cache(std::size_t max_size) : m_max_size(max_size) {}

    std::size_t size() const { return m_lookup.size(); }

    bool contains(const Key &k) const noexcept { return m_lookup.contains(k); }

    // Insert or update (updates value and marks as MRU). Perfect-forwards args
    // to T.
    template<typename... Args>
    void put(const Key &k, Args &&...args) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            // update value, then move to MRU
            it->second.k = k;
            it->second.v = T(std::forward<Args>(args)...);
            m_cache.unlink(it->second);
            m_cache.push_back(it->second);
            return;
        }

        if (m_max_size == 0) {
            return;
        }

        evict_if_full_();

        // add new [k, T(...)] as MRU
        auto &x = m_lookup[k];
        x.k = k;
        x.v = T(std::forward<Args>(args)...);
        m_cache.push_back(x);
    }

    // Get and mark as MRU.
    T *get(const Key &k) {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return nullptr;
        m_cache.unlink(it->second);
        m_cache.push_back(it->second);
        return &it->second.v;
    }

    // mark as MRU
    bool touch(const Key &k) {
        return get(k) != nullptr;
    }

    // Read without affecting LRU
    const T *peek(const Key &k) const {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            return &it->second.v;
        }
        return nullptr;
    }

    // Read without affecting LRU
    const T *peek_lru() const {
        if (m_cache.size() > 0) {
            return &m_cache.front().v;
        }
        return nullptr;
    }

    void clear() {
        m_cache.clear();
        m_lookup.clear();
    }

    bool erase(const Key &k) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            m_cache.unlink(it->second);
            m_lookup.erase(it);
            return true;
        }
        return false;
    }

    // evict LRU
    bool evict_one() {
        if (m_cache.size() > 0) {
            auto k = m_cache.front().k;
            m_cache.pop_front();
            std::cerr << "ERASING: " << k << std::endl;
            m_lookup.erase(k);
            return true;
        }
        return false;
    }

private:
    void evict_if_full_() {
        if (m_max_size == 0) {
            return;
        }
        if (m_cache.size() >= m_max_size) {
            auto k = m_cache.front().k;
            m_cache.pop_front();
            m_lookup.erase(k);
        }
    }

private:
    std::size_t m_max_size = 0;
    lru_list_t m_cache;
    std::unordered_map<Key, lru_node> m_lookup;
};

// Specialization for key-only cache (i.e. set-based rather than map-based)
template<typename Key>
class lru_cache<Key, void> {
private:
    struct lru_node {
        tarp::dlnode link;

        // map lookup key
        Key k {};
    };

    using lru_list_t = tarp::dllist<lru_node, &lru_node::link>;

public:
    lru_cache(std::size_t max_size) : m_max_size(max_size) {}

    std::size_t size() const { return m_lookup.size(); }

    bool contains(const Key &k) const noexcept { return m_lookup.contains(k); }

    // Insert or update (updates value and marks as MRU). Perfect-forwards args
    // to T.
    void put(const Key &k) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            // update value, then move to MRU
            it->second.k = k;
            m_cache.unlink(it->second);
            m_cache.push_back(it->second);
            return;
        }

        if (m_max_size == 0) {
            return;
        }

        evict_if_full_();

        // add new [k, T(...)] as MRU
        auto &x = m_lookup[k];
        x.k = k;
        m_cache.push_back(x);
    }

    // mark as MRU.
    bool touch(const Key &k) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            m_cache.unlink(it->second);
            m_cache.push_back(it->second);
            return true;
        }
        return false;
    }

    void clear() {
        m_cache.clear();
        m_lookup.clear();
    }

    bool erase(const Key &k) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            m_cache.unlink(it->second);
            m_lookup.erase(it);
            return true;
        }
        return false;
    }

    // evict LRU
    bool evict_one() {
        if (m_cache.size() > 0) {
            auto k = m_cache.front().k;
            m_cache.pop_front();
            m_lookup.erase(k);
            return true;
        }
        return false;
    }


private:
    void evict_if_full_() {
        if (m_max_size == 0) {
            return;
        }
        if (m_cache.size() >= m_max_size) {
            auto k = m_cache.front().k;
            m_cache.pop_front();
            m_lookup.erase(k);
        }
    }

private:
    std::size_t m_max_size = 0;
    lru_list_t m_cache;
    std::unordered_map<Key, lru_node> m_lookup;
};

}  // namespace tarp
