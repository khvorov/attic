#include <cstddef>
#include <type_traits>

/*
struct Blk
{
    void * ptr { nullptr };
    size_t length { 0 };
};

class AllocatorApi
{
public:
    static constexpr unsigned alignment;
    static constexpr goodSize(size_t);
    Blk allocate(size_t);
    Blk allocateAll();
    bool expand(Blk &, size_t delta);
    void reallocate(Blk &, size_t);
    bool owns(Blk);
    void deallocate(Blk);
    void deallocateAll();
};
*/

template <std::size_t N, bool AlignedAllocation = true>
class stack_arena
{
public:
    using value_type = unsigned char;
    using pointer = value_type *;

    static constexpr std::size_t const alignment = alignof(std::max_align_t);

public:
    stack_arena() noexcept : m_ptr(m_buffer) {}
    stack_arena(const stack_arena&) = delete;

    ~stack_arena() = default;

    stack_arena & operator=(const stack_arena &) = delete;

    pointer allocate(std::size_t size) noexcept
    {
        pointer result { nullptr };
        auto aligned_size = align(size);

        if (m_ptr + aligned_size < m_buffer + N)
        {
            result = m_ptr;
            m_ptr += aligned_size;
        }

        return result;
    }

    void deallocate(pointer p, std::size_t n) noexcept
    {
        auto aligned_size = align(n);

        if (owns(p) && p + aligned_size == m_ptr)
        {
            m_ptr = p;
        }
    }

private:
    constexpr std::size_t align(const std::size_t n) noexcept
    {
        return n + ((AlignedAllocation && (n % alignment != 0)) ? (alignment - n % alignment) : 0);
    }

    bool owns(pointer p) const noexcept
    {
        return p && (m_buffer <= p) && (p < m_buffer + N);
    }

private:
    using storage_t = alignas(alignment) value_type [N];

    storage_t m_buffer;
    pointer m_ptr;
};

// template <typename T, std::size_t N>
// class stack_allocator
// {
// public:
//     constexpr std::size_t max_size() const noexcept
//     {
//         return sizeof(T) * N;
//     }

// private:
//     stack_arena<sizeof(T)> m_buffer;
// };

template <typename Primary, typename Fallback>
class fallback_allocator : private Primary, private Fallback
{
public:
    using pointer = typename Primary::pointer;

    bool owns(pointer p) const
    {
        return Primary::owns(p) || Fallback::owns(p);
    }
};

int main()
{
    stack_arena<128, false> s;

    auto p1 = s.allocate(10);
    auto p2 = s.allocate(18);

    s.deallocate(p1, 10);
    s.deallocate(p2, 18);

    return 0;
}
