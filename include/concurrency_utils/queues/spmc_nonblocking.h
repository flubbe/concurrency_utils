/**
 * concurrency_utils - concurrency utility library
 * 
 * single producer (unsynchronized), multiple consumer (synchronized, non-blocking) queue.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

// include dependencies
#include <vector>
#include <atomic>
#include <mutex>

namespace concurrency_utils
{

/** single producer multiple consumer queue. */
template<typename T>
class spmc_queue
{
    /** next slot for non-blocking read. */
    std::atomic_uint next_slot{0};

    /** queue data. */
    std::vector<T> data;

public:
    /** default constructor. */
    spmc_queue() = default;

    /** copy construction may be possible, but is disabled for now. */
    spmc_queue(const spmc_queue&) = delete;

    /** push an element into the container. pushes need to done sequentially while not concurrently modifying the queue. non-blocking, not thread-safe. */
    void push(const T& f)
    {
        data.push_back(f);
    }

    /** try to pop an element off the container. popping elements is only safe when not concurrently modifying the queue otherwise (e.g., using push or clear). non-blocking, thread-safe. */
    bool try_pop(T& f)
    {
        auto read = next_slot.fetch_add(1);

        /*
         * since we are (by assumption) not concurrently pushing data
         * into the queue, data.size() returns the same value on every call
         * inside this function.
         */
        if(read < data.size())
        {
            f = data[read];
            return true;
        }
        else
        {
            // the queue is empty, but we did already add one to next_slot.
            // we set next_slot explicitly to the first invalid position to avoid uncontrolled growth.
            next_slot = data.size();
        }

        return false;
    }

    /** clear container immediately. clears need to be done sequentially while not concurrently modifying the queue. non-blocking, not thread-safe. */
    void clear()
    {
        data.clear();

        // the atomic store needs to happen after the non-atomic data.clear() to have a memory barrier.
        next_slot = 0;
    }

    /** check if the container is possibly empty. non-blocking, not thread-safe. */
    bool empty() const
    {
        return size() == 0;
    }

    /** return (approximate) size. non-blocking, not thread-safe. */
    std::size_t size() const
    {
        auto index = next_slot.load();
        auto container_size = data.size();
        if(index < container_size)
        {
            return container_size - index;
        }

        return 0;
    }
};

}    // namespace concurrency_utils