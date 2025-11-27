#pragma once

#include <list>
#include <unordered_map>

namespace tarp {

// A class implementing a Cache with a Least-Recently-Used eviction policy.
// put() and get() both run in O(1).
template<typename Key, typename T>
class lru_cache {
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
            it->second->second = T(std::forward<Args>(args)...);
            m_cache.splice(m_cache.end(), m_cache, it->second);
            return;
        }

        if (m_max_size == 0) {
            return;
        }

        evict_if_full_();

        // add new [k, T(...)] as MRU
        m_cache.emplace_back(k, T(std::forward<Args>(args)...));
        auto node = std::prev(m_cache.end());
        m_lookup.emplace(node->first, node);
    }

    // Get and mark as MRU.
    T *get(const Key &k) {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return nullptr;
        m_cache.splice(m_cache.end(), m_cache, it->second);
        return &it->second->second;
    }

    // Read without affecting LRU
    const T *peek(const Key &k) const {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return nullptr;
        return &it->second->second;
    }

    void clear() {
        m_cache.clear();
        m_lookup.clear();
    }

    bool erase(const Key &k) {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return false;
        m_cache.erase(it->second);
        m_lookup.erase(it);
        return true;
    }

private:
    void evict_if_full_() {
        if (m_max_size == 0) {
            return;
        }
        if (m_cache.size() >= m_max_size) {
            const auto &oldest = m_cache.front();
            m_lookup.erase(oldest.first);
            m_cache.pop_front();
        }
    }

private:
    std::size_t m_max_size = 0;

    std::list<std::pair<Key, T>> m_cache;
    using cache_iterator = typename decltype(m_cache)::iterator;
    std::unordered_map<Key, cache_iterator> m_lookup;
};

// Specialization for key-only cache (i.e. set-based rather than map-based)
template<typename Key>
class lru_cache<Key, void> {
public:
    lru_cache(std::size_t max_size) : m_max_size(max_size) {}

    std::size_t size() const { return m_lookup.size(); }

    bool contains(const Key &k) const noexcept { return m_lookup.contains(k); }

    // Insert or update (updates value and marks as MRU). Perfect-forwards args
    // to T.
    void put(const Key &k) {
        auto it = m_lookup.find(k);
        if (it != m_lookup.end()) {
            // update, then move to MRU
            m_cache.splice(m_cache.end(), m_cache, it->second);
            return;
        }

        if (m_max_size == 0) {
            return;
        }

        evict_if_full_();

        // add new k as MRU
        m_cache.emplace_back(k);
        auto node = std::prev(m_cache.end());
        m_lookup.emplace(*node, node);
    }

    // Get and mark as MRU.
    bool update(const Key &k) {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return false;
        m_cache.splice(m_cache.end(), m_cache, it->second);
        return true;
    }

    void clear() {
        m_cache.clear();
        m_lookup.clear();
    }

    bool erase(const Key &k) {
        auto it = m_lookup.find(k);
        if (it == m_lookup.end()) return false;
        m_cache.erase(it->second);
        m_lookup.erase(it);
        return true;
    }

private:
    void evict_if_full_() {
        if (m_max_size == 0) {
            return;
        }
        if (m_cache.size() >= m_max_size) {
            const auto &oldest = m_cache.front();
            m_lookup.erase(oldest);
            m_cache.pop_front();
        }
    }

private:
    std::size_t m_max_size = 0;
    std::list<Key> m_cache;
    using cache_iterator = typename decltype(m_cache)::iterator;
    std::unordered_map<Key, cache_iterator> m_lookup;
};

}  // namespace tarp
