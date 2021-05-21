/**
 * concurrency_utils - concurrency utility library
 * 
 * single producer (non-synchronized), multple consumer (synchronized, blocking) queue. consuming synchronization via mutex.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

// include dependencies
#include <queue>
#include <mutex>

namespace concurrency_utils
{

/** single producer (unsynchronized), multiple consumer (synchronized, blocking) queue. */
template<typename T>
class spmc_blocking_queue
{
    /** read access mutex. */
    std::mutex queue_mutex;

    /** queue data. */
    std::deque<T> data;

public:
    /** default constructor. */
    spmc_blocking_queue() = default;

    /** copy construction may be possible, but is disabled for now. */
    spmc_blocking_queue(const spmc_blocking_queue&) = delete;

    /** push an element into the container. pushes need to done sequentially while not concurrently modifying the queue. non-blocking, not thread-safe. */
    void push(const T& f)
    {
        data.push_back(f);
    }

    /** try to pop an element off the container. popping elements is only safe when not concurrently modifying the queue otherwise (e.g., using push or clear). blocking, thread-safe. */
    bool try_pop(T& f)
    {
        std::unique_lock mutex_lock{queue_mutex};

        if(data.empty())
        {
            return false;
        }

        f = std::move(data.front());
        data.pop_front();

        return true;
    }

    /** clear container immediately. clears need to be done sequentially while not concurrently modifying the queue. non-blocking, not thread-safe. */
    void clear()
    {
        data.clear();
    }

    /** check if the container is possibly empty. non-blocking, not thread-safe. */
    bool empty() const
    {
        return data.empty();
    }

    /** return (approximate) size. non-blocking, not thread-safe. */
    std::size_t size() const
    {
        return data.size();
    }
};

}    // namespace concurrency_utils