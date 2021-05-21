/**
 * concurrency_utils - concurrency utility library
 * 
 * queue benchmarks and thread pool test.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <exception>

#include "fmt/format.h"

#include "concurrency_utils/thread_pool.h"
#include "../common/vec4.h"

/** duration of thread pool throughput benchmark */
constexpr float benchmark_time = 1000;

/** global variable to perform random calculation in example task. don't expect this to hold any valid value (also due to possible concurrent reads/writes). */
vec4 out{1, 0, 0, 0};

/** some normalized vector used in the calculations. */
const vec4 l{0.5, 0.5, 0.70710678};

/** the example task loops through blocks of this size (i.e., it loops block_size*block_size times). */
constexpr auto block_size = 8;

/** example task. does some random calculations */
void example_task()
{
    for(int i = 0; i < block_size; ++i)
    {
        for(int j = 0; j < block_size; ++j)
        {
            auto v = out;

            float angle = dot(v.normalized(), l);
            float dist_sq = (v - l).length_squared();

            v = (v + l * dist_sq * angle);
            v.normalize();

            out = v;
        }
    }
}

/** queue test. */
template<typename T>
float queue_test(std::size_t count)
{
    std::chrono::steady_clock timer;
    std::chrono::steady_clock::time_point msec_reference_time{timer.now()};

    T q;
    for(std::size_t i = 0; i < count; ++i)
    {
        // push tasks.
        for(std::size_t j = 0; j < i; ++j)
        {
            q.push(example_task);
        }

        // pop and execute tasks.
        while(!q.empty())
        {
            std::function<void()> task;
            q.try_pop(task);

            task();
        }

        q.clear();
    }

    return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::steady_clock::now() - msec_reference_time).count();
}

/** time sequential execution of tasks. */
std::pair<std::size_t, float> sequential_benchmark(std::size_t tasks, std::size_t iterations)
{
    std::chrono::steady_clock timer;
    std::chrono::steady_clock::time_point msec_reference_time{timer.now()};

    for(std::size_t i = 0;; ++i)
    {
        // execute tasks sequentially.
        for(std::size_t it = 0; it < iterations; ++it)
        {
            for(std::size_t i = 0; i < tasks; ++i)
            {
                example_task();
            }
        }

        auto msec_delta_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::steady_clock::now() - msec_reference_time).count();
        if(msec_delta_time > benchmark_time)
        {
            return std::make_pair(i, msec_delta_time);
        }
    }

    return std::make_pair(0, 0.f);
}

/** queue benchmark. */
template<typename T>
std::pair<std::size_t, float> thread_pool_benchmark(std::size_t thread_count, std::size_t tasks, std::size_t iterations)
{
    std::chrono::steady_clock timer;
    std::chrono::steady_clock::time_point msec_reference_time{timer.now()};

    // create new thread pool
    concurrency_utils::deferred_thread_pool<T> dp{thread_count};

    for(std::size_t i = 0;; ++i)
    {
        // on each iteration, fill the thread pool with tasks and execute them.
        for(std::size_t it = 0; it < iterations; ++it)
        {
            // fill pool with tasks.
            for(std::size_t i = 0; i < tasks; ++i)
            {
                dp.push_task(example_task);
            }

            // run tasks.
            dp.run_tasks_and_wait();
        }

        auto msec_delta_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::steady_clock::now() - msec_reference_time).count();
        if(msec_delta_time > benchmark_time)
        {
            return std::make_pair(i, msec_delta_time);
        }
    }

    return std::make_pair(0, 0.f);
}

/** spmc queue thread pool stress test. */
void stress_spmc_thread_pool()
{
    fmt::print("spmc queue thread pool stress test. press CTRL-C to exit.\n");

    constexpr auto thread_count = 4;
    constexpr auto tasks = 250;
    constexpr auto iterations = 20;

    fmt::print("{} threads, {} tasks, {} iterations\n", thread_count, tasks, iterations);
    fmt::print("testing...\n");

    // create new thread pool
    concurrency_utils::deferred_thread_pool<concurrency_utils::spmc_queue<std::function<void()>>> dp{thread_count};

    std::chrono::steady_clock::time_point msec_reference_time{std::chrono::steady_clock::now()};
    auto start_time = msec_reference_time;

    for(int loop_ctr{0};; ++loop_ctr)
    {
        // on each iteration, fill the thread pool with tasks and execute them.
        for(std::size_t it = 0; it < iterations; ++it)
        {
            // fill pool with tasks.
            for(std::size_t i = 0; i < tasks; ++i)
            {
                dp.push_task(example_task);
            }

            // run tasks.
            dp.run_tasks_and_wait();
        }

        auto current_time = std::chrono::steady_clock::now();
        auto msec_delta_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_time - msec_reference_time).count();
        auto time_elapsed = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(current_time - start_time).count();

        if(msec_delta_time > 2000)
        {
            auto loops_per_second = static_cast<float>(1000 * loop_ctr) / (time_elapsed);

            fmt::print("{:>6} loops  ({:#5.2f} loops per second)\n", loop_ctr, loops_per_second);

            msec_reference_time = std::chrono::steady_clock::now();
        }
    }
}

/** program entry point. */
int main(int argc, char* argv[])
{
    if(argc > 1)
    {
        if(std::string(argv[1]) == "--stress")
        {
            // this function does not exit.
            stress_spmc_thread_pool();

            return EXIT_SUCCESS;
        }
        else
        {
            fmt::print("use '{}' to invoke a thread pool benchmark using mpmc-blocking/spmc-blocking/spmc-non-blocking queues.\n", argv[0]);
            fmt::print("use '{} --stress' to invoke a stress test for the thread pool with spmc queue.\n", argv[0]);

            return EXIT_FAILURE;
        }
    }

    /*
     * tests.
     */
    constexpr auto queue_test_loops = 1000;

    fmt::print("testing sequential queue throughput ({} tasks)...\n", queue_test_loops * (queue_test_loops - 1) / 2);

    fmt::print(" * mpmc:   ");
    fmt::print("{: #4.1f} msec\n", queue_test<concurrency_utils::mpmc_blocking_queue<std::function<void()>>>(queue_test_loops));

    fmt::print(" * spmc/b: ");
    fmt::print("{: #4.1f} msec\n", queue_test<concurrency_utils::spmc_blocking_queue<std::function<void()>>>(queue_test_loops));

    fmt::print(" * spmc:   ");
    fmt::print("{: #4.1f} msec\n", queue_test<concurrency_utils::spmc_queue<std::function<void()>>>(queue_test_loops));

    fmt::print("\n");

    /*
     * benchmarks.
     */
    int seq_count{0}, mpmc_count{0}, spmc_blocking_count{0}, spmc_count{0};

    fmt::print("benchmarking thread pool with different queues...\n");

    try
    {
        const int thread_count = 4;
        const int iterations = 50;

        for(int tasks = 10; tasks <= 100; tasks += 10)
        {
            fmt::print("{} thread(s), {} iterations with {} tasks\n", thread_count, iterations, tasks);
            fmt::print("\n");

            const std::array<std::pair<std::size_t, float>, 4> results =
              {
                sequential_benchmark(iterations, tasks),
                thread_pool_benchmark<concurrency_utils::mpmc_blocking_queue<std::function<void()>>>(thread_count, iterations, tasks),
                thread_pool_benchmark<concurrency_utils::spmc_blocking_queue<std::function<void()>>>(thread_count, iterations, tasks),
                thread_pool_benchmark<concurrency_utils::spmc_queue<std::function<void()>>>(thread_count, iterations, tasks)};

            auto seq_throughput = static_cast<float>(results[0].first) / results[0].second;
            auto mpmc_throughput = static_cast<float>(results[1].first) / results[1].second;
            auto spmc_blocking_throughput = static_cast<float>(results[2].first) / results[2].second;
            auto spmc_throughput = static_cast<float>(results[3].first) / results[3].second;

            // mark best performing pool.
            bool mark_seq = seq_throughput >= mpmc_throughput && seq_throughput >= spmc_blocking_throughput && seq_throughput >= spmc_throughput;
            bool mark_mpmc = mpmc_throughput >= seq_throughput && mpmc_throughput >= spmc_blocking_throughput && mpmc_throughput >= spmc_throughput;
            bool mark_spmc_blocking = spmc_blocking_throughput >= seq_throughput && spmc_blocking_throughput >= mpmc_throughput && spmc_blocking_throughput >= spmc_throughput;
            bool mark_spmc = spmc_throughput >= seq_throughput && spmc_throughput >= mpmc_throughput && spmc_throughput >= spmc_blocking_throughput;

            fmt::print("results: \n");
            fmt::print("    type       loops    time (msec)    loops/msec   factor\n");
            fmt::print("    seq        {:>5}       {: #5.1f}       {: #4.4f}      {: #2.2}  {}\n", results[0].first, results[0].second, seq_throughput, seq_throughput / seq_throughput, mark_seq ? "*" : "");
            fmt::print("    mpmc       {:>5}       {: #5.1f}       {: #4.4f}      {: #2.2}  {}\n", results[1].first, results[1].second, mpmc_throughput, mpmc_throughput / seq_throughput, mark_mpmc ? "*" : "");
            fmt::print("    spmc/b     {:>5}       {: #5.1f}       {: #4.4f}      {: #2.2}  {}\n", results[2].first, results[2].second, spmc_blocking_throughput, spmc_blocking_throughput / seq_throughput, mark_spmc_blocking ? "*" : "");
            fmt::print("    spmc       {:>5}       {: #5.1f}       {: #4.4f}      {: #2.2}  {}\n", results[3].first, results[3].second, spmc_throughput, spmc_throughput / seq_throughput, mark_spmc ? "*" : "");
            fmt::print("----------------------------------------------------------\n");
            fmt::print("\n");

            seq_count += static_cast<int>(mark_seq);
            mpmc_count += static_cast<int>(mark_mpmc);
            spmc_blocking_count += static_cast<int>(mark_spmc_blocking);
            spmc_count += static_cast<int>(mark_spmc);
        }

        fmt::print("highest throughput:\n");
        fmt::print(" seq: {} times        mpmc: {} times        spmc/b: {} times        spmc: {} times", seq_count, mpmc_count, spmc_blocking_count, spmc_count);
    }
    catch(const std::exception& e)
    {
        fmt::print("exception caught: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}