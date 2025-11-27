#pragma once

#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <type_traits>

namespace tarp {

// A tree-tree data structure that stores non-overlapping ranges.
//
// While an interval tree or segment tree store possibly
// overlapping intervals and find intervals that match
// a given query, the purpose of this data structure instead
// is to merge adjacent or overlapping intervals on insertion,
// and split or erase intervals on removal.
//
// An obvious use case for this is storing identifiers, handles,
// acknowledgements etc. A major advantage is that its size scales
// linearly with the number of _gaps_ in the ranges; if there are
// not many gaps or if gaps are filled, then the size collapses
// and the data structure remains naturally light-weight.
//
// === Performance: ===
// insertion        O(n)
// deletion         O(n)
// size()           O(n)
// range_count()    O(1)
// contains()       O(log n)
// ----
// - where n is the number of disjoint ranges in the tree.
//
// The 'bad' worst-case runtime complexity for range insertion and
// deletion is due to the fact that an input range could cover all
// ranges in the tree, hence requiring either erasure of all existing
// ranges, or merging of all ranges.
//
// The worst case space complexity is T::max/2 ranges; this pathological
// case will occur if every range is a one-element range (e.g. (1,1))
// and is followed by a one-element gap. For example:
// (0,0), (2,2), (4,4) ...
template<typename T>
class disjoint_range_tree {
    static_assert(std::is_integral_v<T>);

public:
    struct range {
        T low = 0;
        T high = 0;

        range(T l, T h) : low(l), high(h) {
            if (h < l) {
                throw std::invalid_argument(
                  "attempt to construct invalid range wtih high < low");
            }
        }

        range() = default;

        bool overlaps(const range &other) const {
            return (other.contains(low) || other.contains(high)) ||
                   contains(other.low) || contains(other.high);
        }

        bool contains(const range &other) const {
            return other.low >= low && other.high <= high;
        }

        bool contains(T v) const { return contains(range(v, v)); }

        bool equals(const range &other) const {
            return low == other.low && high == other.high;
        }

        std::size_t size() const { return (high - low) + 1; }

        std::string str() const {
            return "(" + std::to_string(low) + "," + std::to_string(high) + ")";
        }
    };

    bool empty() const { return m_ranges.empty(); }

    // Return the number of distinct ranges
    std::size_t range_count() const { return m_ranges.size(); }

    // Return the sum total space covered by all disjoint ranges
    std::size_t size() const {
        if (m_range_size.has_value()) {
            return *m_range_size;
        }

        std::size_t size = 0;

        // else expensive: linear time iteration, then cache
        for (auto &[low, high] : m_ranges) {
            size += range(low, high).size();
        }

        m_range_size = size;
        return size;
    }

    // True if the given range is contained in full
    bool contains(range r) const {
        if (m_ranges.empty()) {
            return false;
        }

        auto it = m_ranges.lower_bound(r.low);
        if (it == m_ranges.end() ||
            (it != m_ranges.begin() && it->first > r.low)) {
            it = std::prev(it);
        }

        // found the largest node with .low <= r.low;
        return range(it->first, it->second).contains(r);
    }

    bool contains(T v) const { return contains(range(v, v)); }

    bool contains(T low, T high) const { return contains(range(low, high)); }

    // Add a new range to the tree.
    // Any overlapping or adjacent ranges are merged.
    void add_range(range r) {
        auto it = m_ranges.lower_bound(r.low);

        // only try to insert if no key collision
        if (it == m_ranges.end() || it->first != r.low) {
            it = m_ranges.insert(it, std::make_pair(r.low, r.high));
        }

        // If range with r.low key already exists,
        // extend it if needed, else NOP.
        else {
            // nothing to do, existing element already covers r.
            if (it->second >= r.high) {
                return;
            }

            it->second = r.high;
        }

        // inserted or extended
        try_merge(it);
        m_range_size.reset();
    }

    void add_range(T v) { return add_range(range(v, v)); }

    void add_range(T low, T high) { return add_range(range(low, high)); }

    // Remove range from tree.
    // When removing, multiple ranges can be affected.
    // One of the following can happen to a range:
    // - it is erased if it is identical to x or contained by it
    // - it is split in two if it contains x but is not identical to it
    // - it is truncated if it overlaps x
    // Return true if any range was removed or truncated, else false.
    bool remove(range x) {
        // invalidate cache
        m_range_size.reset();

        // ------
        // first, find largest range with r.low < x.low
        // or otherwise the lowest range with r.low >= x.low.
        // ------

        auto it = m_ranges.lower_bound(x.low);

        // deal with the previous element; this will have low < x.low,
        // but may have high >= x.low or even high >= x.high
        if (it != m_ranges.begin() && !m_ranges.empty()) {
            it = std::prev(it);
        }

        auto erase_start = it;
        bool removed = false;

        while (it != m_ranges.end()) {
            const auto r = range(it->first, it->second);
            // r and x are identical: remove r, done.
            // If this case occurs, no other will occur.
            if (r.equals(x)) {
                m_ranges.erase(r.low);
                return true;
            }

            // x is contained within r: split r in two or
            // truncate it; re-insert the ensuing subrange(s).
            // If this case occurs, not other will occur.
            if (r.contains(x)) {
                m_ranges.erase(r.low);
                if (r.high > x.high) {
                    m_ranges.insert(std::make_pair(x.high + 1, r.high));
                }
                if (x.low > r.low) {
                    m_ranges.insert(std::make_pair(r.low, x.low - 1));
                }
                return true;
            }

            // r is contained within x: erase;
            // NOTE: we erase at the end after the loop
            // in this case, as it's cheaper to erase a
            // range rather than one at a time.
            if (x.contains(r)) {
                it = std::next(it);
                continue;
            }

            // gap, stop here.
            if (x.high < r.low) {
                break;
            }

            // this range is completely below r; skip it.
            // This case can only occur with the first
            // node when we start iterating.
            if (x.low > r.high) {
                it = std::next(it);
                erase_start = it;
                continue;
            }

            // else there is overlap;

            // case 1) x starts within the current range
            // and ends after it; this case can only occur
            // with the first node when we start iterating.
            if (x.low <= r.high && r.high < x.high) {
                if (r.low == r.high) {
                    throw std::logic_error(
                      "BUG: size-1 range not caught by 'contains' case");
                } else if (x.low == x.high) {
                    throw std::logic_error(
                      "BUG: size-1 range not caught by 'contains' case [2]");
                }
                // trim off the high end of the range
                it->second = x.low - 1;
                erase_start = it;
                removed = true;
                continue;
            }

            // case 2) x ends within the current range and starts
            // before it. This case can only occur once, with
            // the last node when we finish iterating.
            // note here we must erase since it's the key
            // itself (r.low) that changes, not the value (r.high).
            if (x.high >= r.low && r.high > x.high) {
                it = m_ranges.erase(erase_start, std::next(it));
                if (r.high > r.low) {
                    // cannot erase completely: must reinsert with
                    // lower end trimmed off.
                    m_ranges.insert(it, std::make_pair(x.high + 1, r.high));
                }
                return true;
            }
        }

        // now erase in one fell swoop contiguous ranges 
        // that were covered.
        m_ranges.erase(erase_start, it);

        // any removed
        return removed || erase_start != it;
    }

    bool remove(T v) { return remove(range(v, v)); }

    bool remove(T low, T high) { return remove(range(low, high)); }

    std::optional<T> lowest() const {
        if (m_ranges.empty()) {
            return std::nullopt;
        }
        return m_ranges.begin()->first;
    }

    std::optional<T> highest() const {
        if (m_ranges.empty()) {
            return std::nullopt;
        }
        return std::prev(m_ranges.end())->second;
    }

    std::string str() const {
        std::ostringstream ss;
        ss << "T(";
        const std::size_t sz = m_ranges.size();
        std::size_t i = 0;
        for (const auto &[low, high] : m_ranges) {
            if constexpr (std::is_same_v<std::uint8_t, T>) {
                ss << "(" << static_cast<unsigned>(low) << ","
                   << static_cast<unsigned>(high) << ")";
            } else if constexpr (std::is_same_v<std::int8_t, T>) {
                ss << "(" << static_cast<int>(low) << ","
                   << static_cast<int>(high) << ")";
            } else {
                ss << "(" << low << "," << high << ")";
            }
            if (++i < sz) {
                ss << ", ";
            }
        }
        ss << ")";
        return ss.str();
    }

private:
    // the newly inserted range may have have a high endpoint
    // such that it overlaps or completely encompasses
    // later ranges with a higher low endpoint.
    void try_merge(std::map<T, T>::iterator it) {
        // Start scanning at prev if the high endpoint of
        // the previous range is adjacent to or overlaps
        // the range we've inserted.
        if (it != m_ranges.begin()) {
            const auto prev = std::prev(it);

            // avoid wraparound
            if (prev->second >= it->first || prev->second + 1 == it->first) {
                it = prev;
            }
        }

        auto &high = it->second;

        using iterator_t = decltype(it);
        iterator_t i = it;

        // scan forward, merging ranges as we go until
        // we hit the first gap.
        while (i != m_ranges.end()) {
            i = std::next(i);

            if (i->first > high && i->first - high > 1) {
                // done; we have a gap, cannot merge.
                break;
            }

            // otherwise merge
            high = std::max(high, i->second);
        }

        // now erase the merged ranges that follow the one we
        // inserted; we need to erase between (it, i), without
        // including it (the range we inserted) or
        // i (first range that has not been merged).
        if (it != i) it = std::next(it);
        m_ranges.erase(it, i);
    }

private:
    mutable std::optional<std::size_t> m_range_size;
    std::map<T, T> m_ranges;
};


}  // namespace tarp
