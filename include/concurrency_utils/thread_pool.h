/**
 * concurrency_utils - concurrency utility library
 * 
 * a thread pool for sequential task queueing and subsequent concurrent execution.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

// include dependencies.
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>

#include "queue.h"

namespace concurrency_utils
{

/** 
 * a C++17 thread pool that queues up jobs and (when instructed to do so) executes them by using the threads in the pool. 
 */
template<typename queue_type = mpmc_blocking_queue<std::function<void()>>>
class deferred_thread_pool
{
    /** thread count. */
    std::size_t thread_count{0};

    /** threads. */
    std::vector<std::thread> threads;

    /** An atomic variable indicating to the workers to stop. */
    std::atomic_bool stop{false};

    /** task queue. */
    queue_type tasks;

    /** mutex for task processing. */
    mutable std::mutex run_mutex;

    /** condition variable to indicate wheather we should process the submitted tasks. */
    std::condition_variable should_run;

    /** whether we should process the tasks in the queue. */
    std::atomic_bool process_tasks{false};

    /** keep track of the currently active threads. */
    std::atomic_size_t active_threads{0};

    /*
     * private helpers.
     */

    /** create the worker threads. creates at least one thread. */
    void create_threads()
    {
        // create at least one thread.
        if(!thread_count)
        {
            thread_count = 1;
        }

        tasks.clear();

        // allocate threads.
        threads.reserve(thread_count);
        for(std::size_t i = 0; i < thread_count; ++i)
        {
            threads.emplace_back(&deferred_thread_pool::worker, this);
        }

        // wait for threads to be ready.
        while(active_threads > 0)
        {
            std::this_thread::yield();
        }
    }

    /** destroy threads. */
    void destroy_threads()
    {
        if(threads.size())
        {
            run_tasks_and_wait();

            {
                std::unique_lock lock{run_mutex};
                stop = true;

                should_run.notify_all();
            }

            for(auto& it: threads)
            {
                it.join();
            }
            threads.clear();
            active_threads = 0;
        }

        tasks.clear();
    }

    /** worker function. */
    void worker()
    {
        ++active_threads;

        while(!stop)
        {
            --active_threads;

            // acquire lock.
            {
                std::unique_lock run_lock{run_mutex};
                should_run.wait(run_lock, [&]() -> bool
                                { return stop || (process_tasks && !tasks.empty()); });
            }

            // exit if the pool is stopped.
            if(stop)
            {
                break;
            }

            ++active_threads;

            // process the tasks assigned to this thread.
            std::function<void()> task;
            while(process_tasks && !tasks.empty())
            {
                if(tasks.try_pop(task))
                {
                    // execute task.
                    task();
                }
            }
        }
    }

public:
    /** default constructor does not create threads. */
    deferred_thread_pool()
    : thread_count{0}
    {
    }

    /** constructor. */
    deferred_thread_pool(std::size_t in_thread_count)
    : thread_count{in_thread_count}
    {
        create_threads();
    }

    /** destructor. */
    ~deferred_thread_pool()
    {
        destroy_threads();
    }

    /** disable copying. */
    deferred_thread_pool(const deferred_thread_pool&) = delete;
    deferred_thread_pool& operator=(const deferred_thread_pool&) = delete;

    /** start submitted tasks and wait for all tasks to be completed. */
    void run_tasks_and_wait()
    {
        if(tasks.empty())
        {
            return;
        }

        // run threads. keep notifying until we either have no threads left or the task list is empty.
        process_tasks = true;
        should_run.notify_all();

        // wait for all tasks to be processed.
        while(!tasks.empty())
        {
            std::this_thread::yield();

            // check if (more) threads should be running. (tasks.size() may return a slightly inaccurate value though).
            auto currently_active_threads = active_threads.load();
            if(currently_active_threads < tasks.size() && currently_active_threads < thread_count)
            {
                should_run.notify_all();
            }
        }

        // we are done processing the tasks.
        process_tasks = false;

        // threads might still be active.
        while(active_threads > 0)
        {
            std::this_thread::yield();
        }

        // explicitly clean up task queue. this may not have been done by the worker threads
        // during task execution.
        tasks.clear();
    }

    void wait_and_exit()
    {
        destroy_threads();
    }

    /** reset the number of threads in the pool. waits for all submitted tasks to be completed, then destroys and creates new thread pool with the number of new threads. */
    void reset(std::size_t in_thread_count)
    {
        run_tasks_and_wait();
        destroy_threads();

        thread_count = std::max<std::size_t>(in_thread_count, 1);
        stop = false;
        create_threads();
    }

    /** push a function with no arguments or return value into the task queue. this does not start the task. */
    template<typename F>
    void push_task(const F& task)
    {
        // submit task.
        tasks.push(task);
    }

    /** push a function with arguments, but no return value, into the task queue. this does not start the task. */
    template<typename F, typename... A>
    void push_task(const F& task, const A&... args)
    {
        push_task([task, args...]
                  { task(args...); });
    }

    /** return the number of threads. */
    std::size_t get_thread_count() const
    {
        return thread_count;
    }

    /** get waiting tasks. only returns a reliable answer if no threads are currently executing. */
    std::size_t get_waiting_tasks() const
    {
        return tasks.size();
    }

    /** return whether the pool is currently processing tasks. */
    bool is_processing() const
    {
        return process_tasks;
    }
};

} /* namespace concurrency_utils */