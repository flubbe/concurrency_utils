/**
 * swr - a software rasterizer
 * 
 * thread pool queue benchmark.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <vector>

/* Google benchmark */
#include <benchmark/benchmark.h>

/* concurrency uttils */
#include "concurrency_utils/thread_pool.h"

/* math header. */
#include "../common/vec4.h"

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

template<typename T>
static void bench_queue(benchmark::State& state)
{
    concurrency_utils::deferred_thread_pool<T> pool{4};

    std::size_t task_count = state.range(0);
    for(auto _: state)
    {
        // fill pool with tasks.
        for(std::size_t i = 0; i < task_count; ++i)
        {
            pool.push_task(example_task);
        }

        // run tasks.
        pool.run_tasks_and_wait();
    }
}

BENCHMARK_TEMPLATE(bench_queue, concurrency_utils::spmc_queue<std::function<void()>>)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK_TEMPLATE(bench_queue, concurrency_utils::spmc_blocking_queue<std::function<void()>>)->Arg(100)->Arg(1000)->Arg(10000);
BENCHMARK_TEMPLATE(bench_queue, concurrency_utils::mpmc_blocking_queue<std::function<void()>>)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
