# Concurrency Utility Library

[![License](https://img.shields.io/github/license/flubbe/concurrency_utils)](https://github.com/flubbe/concurrency_utils/blob/main/LICENSE.txt)
[![Build Status](https://travis-ci.com/flubbe/concurrency_utils.svg?branch=main)](https://travis-ci.com/flubbe/concurrency_utils)

## Description

A small collection of classes supporting concurrent evaluation of tasks. 

The library contains:
 - a templated thread pool for deferred concurrent execution of tasks: `deferred_thread_pool<queue_type>`
 - three queues to use with the thread pool.
    - single producer, multiple consumer (non-blocking): `spmc_queue`
    - single producer, multiple consumer (blocking): `spmc_blocking_queue`
    - multiple producer, multiple consumer (blocking): `mpmc_blocking_queue`

The default queue type for the thread pool is `spmc_queue`.

## Dependencies

The tests depend on [{fmt}](https://github.com/fmtlib/fmt). The benchmarks use [Google's benchmark library](https://github.com/google/benchmark).

## Setup

The library is header-only.
 - include `concurrency_utils/thread_pool.h` to use `concurrency_utils::deferred_thread_pool`.
 - include `concurrency_utils/queues.h` to use any of `concurrency_utils::spmc_queue`, `concurrency_utils::spmc_blocking_queue`, `concurrency_utils::mpmc_blocking_queue`.

Tested on Linux, GCC 11.1 (with C++17 enabled), CMake 3.20.2.
 
## References and other libraries

The thread pool draws inspiration from [thread-pool](https://github.com/bshoshany/thread-pool), but uses `std::condition_variable` for execution control.
