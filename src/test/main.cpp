/**
 * concurrency_utils - concurrency utility library
 * 
 * thread pool test.
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
const vec4 l{0.5f, 0.5f, 0.70710678f};

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

/** program entry point. */
int main(int argc, char* argv[])
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

    return EXIT_SUCCESS;
}