#pragma once

// Implementation of a double-ended queue that allows
// O(1) erasure and insertion at both ends.
// Insertion and erasure anywhere else is not allowed.
// This implementation is different from std::deque
// and avoids its performance overheads by using
// contiguous memory similar to std::vector. IOW,
// this class does not make the comprimises that
// std::deque makes and instead focuses solely on
// fast operations at the two ends of the queue
// and fast sequential traversal and lookups.

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace tarp {
template<typename T>
class deque final {
    // Max power of 2 we accept for the capacity;
    // cannot grow to a capacity greater than this.
    static inline std::uint32_t MAX_CAPACITY =
      (std::numeric_limits<std::uint32_t>::max() >> 1) + 1;

    T *m_buff = nullptr;

    // always a power of 2, in order to use
    // fast masking instead of modulo.
    std::uint32_t m_capacity = 0;

    // index of first existing element
    std::uint32_t m_head = 0;
    // index of last existing element + 1
    std::uint32_t m_tail = 0;

    // total number of existing elements (<= m_capacity);
    std::uint32_t m_size = 0;

    // if true, shrink the memory when the (capacity - size) < thresh.
    bool m_autoshrink = false;

private:
    inline std::uint32_t mask() const noexcept { return m_capacity - 1; }

    [[gnu::always_inline]] [[nodiscard]] inline std::uint32_t
    wrap(std::uint32_t i) const noexcept {
        return i & mask();
    }

    inline void maybe_grow() {
        if (size() == m_capacity) [[unlikely]] {
            grow();
        }
    }

    inline void maybe_shrink() {
        const auto sz = size();
        // if size is < 1/4 the capacity -> halve the capacity.
        if (m_capacity > 8 && sz < (m_capacity >> 2)) {
            // we make the new capacity std::bit_ceil(sz)<<1
            // rather than simply m_capacity >> 1, because
            // autoshrink may have been enabled partway
            // through, hence when maybe_shrink is called
            // sz may be e.g. < m_capacity >> 8!
            const auto newcap = std::bit_ceil(sz) << 1;
            shrink(newcap);
        }
    }

    // double in size
    inline void grow() {
        std::uint32_t new_cap = m_capacity == 0 ? 8 : m_capacity << 1;
        reserve(new_cap);
    }

    // memcpy current elements to newbuff
    [[gnu::always_inline]] inline void memcpy(T *dst,
                                              T *src,
                                              std::uint32_t src_head,
                                              std::uint32_t src_size,
                                              std::uint32_t src_cap)
        requires(std::is_trivially_copyable_v<T>)
    {
        if (src_size > 0) {
            const std::uint32_t first_leg = src_cap - src_head;
            // if head before tail, then copy all in one go
            if (src_size <= first_leg) {
                std::memcpy(dst, src + src_head, src_size * sizeof(T));
            }
            // else tail before head (wrapped tail), must copy in two steps
            else {
                // copy from head to end
                std::memcpy(dst, src + src_head, first_leg * sizeof(T));
                // copy from start to tail
                std::memcpy(
                  dst + first_leg, src, (src_size - first_leg) * sizeof(T));
            }
        }
    }

    // halve in size
    void shrink(std::uint32_t newcap) {
        // Allocate raw memory (may throw std::bad_alloc,
        // which is safe to propagate)
        T *newbuff = static_cast<T *>(::operator new(newcap * sizeof(T)));
        std::uint32_t num_constructed = 0;
        const auto MASK = mask();

        try {
            // fast block copy for trivial types
            if constexpr (std::is_trivially_copyable_v<T>) {
                memcpy(newbuff, m_buff, m_head, m_size, m_capacity);
            }
            // slower: iteratively call constructor of each element
            else {
                for (std::uint32_t i = 0; i < m_size; ++i) {
                    const auto idx = (m_head + i) & MASK;
                    // Strong guarantee: if move cannot throw, move it
                    if constexpr (std::is_nothrow_move_constructible_v<T>) {
                        ::new (static_cast<void *>(newbuff + i))
                          T(std::move(m_buff[idx]));
                    } else {
                        // If move can throw, copy instead (which may throw,
                        // and we catch).
                        ::new (static_cast<void *>(newbuff + i)) T(m_buff[idx]);
                    }
                    ++num_constructed;
                }
            }
        } catch (...) {
            for (std::uint32_t i = 0; i < num_constructed; ++i) {
                newbuff[i].~T();
            }
            ::operator delete(newbuff);
            throw;
        }

        // Clean up old memory blocks if needed
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (std::uint32_t i = 0; i < m_size; ++i) {
                const auto idx = (m_head + i) & MASK;
                m_buff[idx].~T();
            }
        }
        ::operator delete(m_buff);

        m_buff = newbuff;
        m_capacity = newcap;
        m_head = 0;
    }

    // Fast fill append of a specific value;
    // Assumes m_size + count <= m_capacity (memory already grown/reserved).
    inline void fill_append(std::uint32_t count, const T &value)
        requires(std::is_trivially_copyable_v<T>)
    {
        if (count == 0) [[unlikely]] {
            return;
        }

        // Helper lambda to fill a contiguous chunk as fast as possible
        auto fill_span = [](T *dst, std::uint32_t n, const T &v) {
            // If filling with 0/all-zeros, memset is the absolute fastest
            // possible path
            if constexpr (sizeof(T) == 1) {
                std::memset(
                  dst, *reinterpret_cast<const unsigned char *>(&v), n);
            } else {
                std::fill(dst, dst + n, v);
            }
        };

        // Calculate how much linear space we have from m_tail
        // to the end of the buffer
        std::uint32_t space_at_back = m_capacity - m_tail;

        if (m_head > m_tail) {
            // Free space is bounded by m_head. The precondition is that
            // (m_size + count <= m_capacity), so count is guaranteed to
            // fit before m_head. Therefore, it is perfectly contiguous.
            fill_span(m_buff + m_tail, count, value);
        } else {
            // m_head <= m_tail: Free space goes to the end
            // of the buffer, then wraps to front
            if (space_at_back >= count) {
                fill_span(m_buff + m_tail, count, value);
            } else {
                // Fill up to the physical memory edge
                fill_span(m_buff + m_tail, space_at_back, value);
                // Wrap around to index 0 and fill the remainder
                fill_span(m_buff, count - space_at_back, value);
            }
        }

        // Update the layout trackers cleanly at the end
        m_tail = wrap(m_tail + count);
        m_size += count;
    }

public:
    deque() = default;

    inline explicit deque(std::uint32_t num_elems)
        requires(std::is_default_constructible_v<T>)
    {
        if (num_elems > 0) {
            resize(num_elems);
        }
    }

    deque(const deque &other) {
        if (other.m_size == 0) [[unlikely]] {
            return;
        }

        m_capacity = other.m_capacity;
        m_buff = static_cast<T *>(::operator new(other.m_capacity * sizeof(T)));

        if constexpr (std::is_trivially_copyable_v<T>) {
            memcpy(m_buff,
                   other.m_buff,
                   other.m_head,
                   other.m_size,
                   other.m_capacity);
        } else {
            try {
                const auto MASK = other.mask();
                for (std::uint32_t i = 0; i < other.m_size; ++i) {
                    const auto idx = (other.m_head + i) & MASK;
                    ::new (static_cast<void *>(m_buff + idx))
                      T(other.m_buff[idx]);
                }
            } catch (...) {
                clear();
                ::operator delete(m_buff);
                throw;
            }
        }

        m_size = other.m_size;
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_autoshrink = other.m_autoshrink;
    }

    deque(deque &&other) noexcept
        : m_buff(other.m_buff)
        , m_capacity(other.m_capacity)
        , m_head(other.m_head)
        , m_tail(other.m_tail)
        , m_size(other.m_size)
        , m_autoshrink(other.m_autoshrink) {
        other.m_buff = nullptr;
        other.m_capacity = 0;
        other.m_head = 0;
        other.m_tail = 0;
        other.m_size = 0;
    }

    deque &operator=(const deque &other) {
        if (this != &other) {
            deque temp(other);
            swap(temp);
        }
        return *this;
    }

    deque &operator=(deque &&other) noexcept {
        if (this != &other) {
            // Clear our resource context cleanly
            reset();

            m_buff = other.m_buff;
            m_capacity = other.m_capacity;
            m_head = other.m_head;
            m_size = other.m_size;
            m_autoshrink = other.m_autoshrink;

            other.m_buff = nullptr;
            other.m_capacity = 0;
            other.m_head = 0;
            other.m_tail = 0;
            other.m_size = 0;
        }
        return *this;
    }

    ~deque() { reset(); }

    void swap(deque &other) noexcept {
        using std::swap;
        swap(m_buff, other.m_buff);
        swap(m_capacity, other.m_capacity);
        swap(m_head, other.m_head);
        swap(m_tail, other.m_tail);
        swap(m_size, other.m_size);
        swap(m_autoshrink, other.m_autoshrink);
    }

    [[nodiscard]] inline std::uint32_t size() const noexcept { return m_size; }

    [[nodiscard]] inline bool empty() const noexcept { return m_size == 0; }

    [[nodiscard]] inline std::uint32_t capacity() const noexcept {
        return m_capacity;
    }

    // Direct O(1) random access
    [[nodiscard]] inline T &operator[](std::uint32_t index) noexcept {
        return m_buff[wrap(m_head + index)];
    }

    [[nodiscard]] inline const T &
    operator[](std::uint32_t index) const noexcept {
        return m_buff[wrap(m_head + index)];
    }

    [[nodiscard]] inline T &front() noexcept { return m_buff[m_head]; }

    [[nodiscard]] inline const T &front() const noexcept {
        return m_buff[m_head];
    }

    [[nodiscard]] inline T &back() noexcept { return m_buff[wrap(m_tail - 1)]; }

    [[nodiscard]] inline const T &back() const noexcept {
        return m_buff[wrap(m_tail - 1)];
    }

    inline void push_back(const T &value) { emplace_back(value); }

    inline void push_back(T &&value) { emplace_back(std::move(value)); }

    template<typename... Args>
    inline T &emplace_back(Args &&...args) {
        // If this throws due to reserve, state remains
        // completely untouched.
        maybe_grow();

        // Construct the element directly in the next available slot.
        // If the constructor throws, m_tail and m_size remain unchanged.
        ::new (static_cast<void *>(m_buff + m_tail))
          T(std::forward<Args>(args)...);

        // Commit changes only after successful construction
        T &emplaced_element = m_buff[m_tail];
        m_tail = wrap(m_tail + 1);
        ++m_size;

        // C++17 style return for easy access to the created object
        return emplaced_element;
    }

    template<typename... Args>
    inline T &emplace_front(Args &&...args) {
        maybe_grow();

        // Calculate the new head target without updating m_head yet
        const std::uint32_t new_head = wrap(m_head - 1);

        // Construct the element in-place. If it throws, container state is
        // perfectly safe.
        ::new (static_cast<void *>(m_buff + new_head))
          T(std::forward<Args>(args)...);

        // Commit changes
        m_head = new_head;
        ++m_size;

        return m_buff[m_head];
    }

    inline void push_front(const T &value) { emplace_front(value); }

    inline void push_front(T &&value) { emplace_front(std::move(value)); }

    void reserve(std::uint32_t new_cap) {
        if (new_cap <= m_capacity) [[unlikely]] {
            return;
        }

        if (new_cap > MAX_CAPACITY) [[unlikely]] {
            return;
        }

        // next greater power of 2
        new_cap = std::bit_ceil(new_cap);
        const auto MASK = mask();

        // Allocate raw memory (may throw std::bad_alloc,
        // which is safe to propagate)
        T *newbuff = static_cast<T *>(::operator new(new_cap * sizeof(T)));
        std::uint32_t num_constructed = 0;

        if constexpr (std::is_trivially_copyable_v<T>) {
            memcpy(newbuff, m_buff, m_head, m_size, m_capacity);
        } else {
            try {
                for (std::uint32_t i = 0; i < m_size; ++i) {
                    if constexpr (std::is_nothrow_move_constructible_v<T>) {
                        ::new (static_cast<void *>(newbuff + i))
                          T(std::move(m_buff[(m_head + i) & MASK]));
                    } else {
                        ::new (static_cast<void *>(newbuff + i))
                          T(m_buff[(m_head + i) & MASK]);
                    }
                    ++num_constructed;
                }
            } catch (...) {
                for (std::uint32_t i = 0; i < num_constructed; ++i) {
                    newbuff[i].~T();
                }
                ::operator delete(newbuff);
                throw;
            }
        }

        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (std::uint32_t i = 0; i < m_size; ++i) {
                m_buff[(m_head + i) & MASK].~T();
            }
        }
        ::operator delete(m_buff);

        m_buff = newbuff;
        m_capacity = new_cap;
        m_head = 0;

        // when copying to the new buffer, we unwrap the indices,
        // so that the head starts at 0, and the tail at m_size.
        m_tail = m_size;
    }

    inline void resize(std::uint32_t num_elems)
        requires std::is_default_constructible_v<T>
    {
        if (num_elems > m_size) {
            reserve(num_elems);
            if constexpr (std::is_trivially_copyable_v<T>) {
                fill_append(num_elems - m_size, T());
            } else {
                std::uint32_t num_constructed = 0;
                try {
                    while (m_size < num_elems) {
                        const auto MASK = mask();
                        const auto i = m_size & MASK;
                        ::new (static_cast<void *>(m_buff + i)) T();
                        m_tail = (m_tail + 1) & MASK;
                        ++m_size;
                        ++num_constructed;
                    }
                }
                // Rollback default elements created during loop
                catch (...) {
                    if constexpr (std::is_trivially_destructible_v<T>) {
                        m_size -= num_constructed;
                        m_tail = wrap(m_tail - num_constructed);
                    } else {
                        for (std::uint32_t i = 0; i < num_constructed; ++i) {
                            m_tail = wrap(m_tail - 1);
                            m_buff[m_tail].~T();
                            --m_size;
                        }
                    }
                    throw;
                }
            }
        } else {
            while (m_size > num_elems) {
                pop_back();
            }
        }
    }

    // clear all elements, do not touch memory
    // (even if autoshrink is enabled).
    inline void clear() noexcept {
        const auto MASK = mask();

        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (std::uint32_t i = 0; i < m_size; ++i) {
                m_buff[(m_head + i) & MASK].~T();
            }
        }
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    // clear elements AND free all memory
    inline void reset() noexcept {
        clear();
        ::operator delete(m_buff);
        m_buff = nullptr;
        m_capacity = 0;
        m_head = m_tail = 0;
    }

    inline void pop_front() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            m_buff[m_head].~T();
        }
        m_head = wrap(m_head + 1);
        --m_size;
        if (m_autoshrink) maybe_shrink();
    }

    inline void pop_back() {
        m_tail = wrap(m_tail - 1);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            m_buff[m_tail].~T();
        }

        --m_size;

        if (m_autoshrink) maybe_shrink();
    }

    inline void shrink_to_fit() {
        const auto sz = size();
        if (sz == 0) {
            reset();
            return;
        }

        // not _exactly_ to fit -- we need
        // this to be a power of 2 at all
        // times. So e.g. if sz is 9, the
        // new capacity will be 16.
        const auto newcap = std::bit_ceil(sz);
        if (newcap < m_capacity) {
            shrink(newcap);
        }
    }

    // if true, shrink the memory when the (capacity - size) < thresh;
    // if false, the memory is never reduced, only grown.
    void inline set_autoshrink(bool enabled) noexcept {
        m_autoshrink = enabled;
        if (m_autoshrink) {
            maybe_shrink();
        }
    }
};

}  // namespace tarp
