//  g++ -Wall -pedantic -std=c++14 -march=native -O3 -g latency.cpp -o /tmp/a.out -lrt -lpthread && /tmp/a.out

#include <boost/lockfree/queue.hpp>
#include <boost/noncopyable.hpp>

#include <array>
#include <cassert>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

//#include "likely.hpp"

#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))

//
struct SimpleExecutor
{
    void execute(std::function<void ()>&& f)
    {
        f();
    }
};

//
template <typename T, std::size_t BufferCapacity>
class Buffer
{
public:
    bool empty() const { return !msgCount; }
    bool full() const { return msgCount >= BufferCapacity; }

    void clear() { msgCount = 0; }

    T & next()
    {
        assert(!full());
        return buffer[msgCount++];
    }

private:
    std::array<T, BufferCapacity> buffer;
    std::size_t msgCount { 0 };
};

//
template <typename T, std::size_t BufferCapacity, typename Executor = SimpleExecutor>
class BufferPool : private boost::noncopyable
{
public:
    using buffer_t = Buffer<T, BufferCapacity>;
    using buffer_ptr_t = buffer_t *;

    BufferPool() : m_freeBuffers(std::make_unique<free_buffers_queue_t>()) {}

    // TODO: check whether buffersCount is less than half of the size of the lock-free queue
    explicit BufferPool(std::size_t buffersCount)
        : m_buffers(buffersCount, nullptr),
          m_freeBuffers(std::make_unique<free_buffers_queue_t>())
    {
        for (auto & b : m_buffers)
        {
            b = new buffer_t();
            m_freeBuffers->push(b);
        }
    }

    ~BufferPool()
    {
        std::unique_lock<std::mutex> lock(m_lock);

        m_stopped = true;
        m_freeBuffers.reset();

        for (auto b : m_buffers)
        {
            if (!b)
                continue;

            if (!b->empty())
            {
                // TODO: some messages haven't been processed yet
                // probably we need to process the rest of the messages here...
            }

            delete b;
        }

        m_buffers.clear();
    }

    // 
    constexpr std::size_t bufferCapacity() const { return BufferCapacity; }
    bool hasFreeBuffers() const { return !m_freeBuffers->empty(); }

    //
    buffer_t * getBuffer()
    {
        buffer_t * buf { nullptr };

        // try to get a free buffer from the queue of unused buffers
        if (LIKELY(m_freeBuffers && m_freeBuffers->pop(buf)))
            return buf;

        // if there is no a free buffer then allocate the buffer and return it
        buf = new buffer_t();

        std::unique_lock<std::mutex> lock(m_lock);

        if (!m_stopped)
        {
            m_buffers.push_back(buf);
        }
        else
        {
            delete buf;
            buf = nullptr;
        }

        return buf;
    }

    void returnBuffer(buffer_t * buf)
    {
        if (!buf)
            return;

        m_executor.execute([this, buf]
        {
            // TODO: process data inside the buffer

            // clear the buffer
            buf->clear();

            // push a pointer to the queue of free buffers
            // do this in a loop to avoid queue overflow and ensure that buffer won't be lost
            while (m_freeBuffers && !m_freeBuffers->push(buf)) {};
        });
    }

    buffer_t * exchangeBuffer(buffer_t * buf)
    {
        returnBuffer(buf);
        return getBuffer();
    }

private:
    using free_buffers_queue_t = boost::lockfree::queue<buffer_ptr_t, boost::lockfree::capacity<32 * 1024>>;

    // TODO: try to replace with unique ptr
    std::deque<buffer_ptr_t> m_buffers;
    std::unique_ptr<free_buffers_queue_t> m_freeBuffers;
    std::mutex m_lock;

    Executor m_executor;

    bool m_stopped { false };
};

// test

#include <iostream>
#include <time.h>

struct InternalTimestamp
{
    uint64_t componentId;
    uint64_t eventId;
    uint64_t timestamp;
    // TODO: some meta information to tag individual events (e.g. quote id)
};

struct DapperTimestamp
{
    enum class Annotation { SR, SS, CR, CS };

    uint64_t traceId;
    uint64_t spanId;
    uint64_t parentSpanId { 0 };
    Annotation annotation;

    uint64_t timestamp;
};

namespace
{
    using timestamp_memory_manager_t = BufferPool<InternalTimestamp, 100000>;
    timestamp_memory_manager_t mm(256);
}

uint64_t get_timestamp_now()
{
    static constexpr uint64_t c_ns_multiplier(1000000000);

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return ts.tv_sec * c_ns_multiplier + ts.tv_nsec;
}

void record_timestamp(uint64_t componentId, uint64_t eventId)
{
    static thread_local timestamp_memory_manager_t::buffer_ptr_t tsBuffer { nullptr };

    // record TS before doing any exchanges
    uint64_t timestamp = get_timestamp_now();

    if (UNLIKELY(!tsBuffer || tsBuffer->full()))
    {
        tsBuffer = mm.exchangeBuffer(tsBuffer);
    }

    InternalTimestamp & its = tsBuffer->next();
    its.componentId = componentId;
    its.eventId = eventId;
    its.timestamp = timestamp;
}

int main()
{
    record_timestamp(1, 1);
    record_timestamp(1, 102);

    std::cout << mm.hasFreeBuffers() << std::endl;

    return 0;
}

