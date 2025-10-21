/**
 * concurrency_utils - concurrency utility library
 * 
 * multiple producer (synchronized, blocking), multiple consumer (synchronized, blocking) queue. synchronization via mutex.
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

/** multiple producer (synchronized, blocking), multiple consumer (synchronized, blocking) queue. */
template<typename T>
class mpmc_blocking_queue
{
    /** read access mutex. */
    mutable std::mutex queue_mutex;

    /** queue data. */
    std::deque<T> data;

public:
    /** default constructor. */
    mpmc_blocking_queue() = default;

    /** copy construction may be possible, but is disabled for now. */
    mpmc_blocking_queue(const mpmc_blocking_queue&) = delete;

    /** push an element into the container. blocking, thread-safe. */
    void push(const T& f)
    {
        std::unique_lock mutex_lock{queue_mutex};
        data.push_back(f);
    }

    /** try to pop an element off the container. blocking, thread-safe. */
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

    /** clear container. blocking, thread-safe. */
    void clear()
    {
        std::unique_lock lock{queue_mutex};
        data.clear();
    }

    /** check if the container is possibly empty. blocking, thread-safe. */
    bool empty() const
    {
        std::unique_lock lock{queue_mutex};
        return data.empty();
    }

    /** return (approximate) size. blocking, thread-safe. */
    std::size_t size() const
    {
        std::unique_lock lock{queue_mutex};
        return data.size();
    }
};

}    // namespace concurrency_utils