cmake_minimum_required(VERSION 3.22)
project(concurrency_utils LANGUAGES CXX)

include(FetchContent)

set(CONCURRENCY_UTILS_IS_TOPLEVEL OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CONCURRENCY_UTILS_IS_TOPLEVEL ON)
endif()

option(CONCURRENCY_UTILS_BUILD_TOOLS "Build manual test/tools binaries" ${CONCURRENCY_UTILS_IS_TOPLEVEL})
option(CONCURRENCY_UTILS_BUILD_BENCHMARKS "Build concurrency_utils benchmarks" ${CONCURRENCY_UTILS_IS_TOPLEVEL})

#
# Thread library
#
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#
# Main target
#
add_library(concurrency_utils INTERFACE)
target_include_directories(concurrency_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(concurrency_utils INTERFACE cxx_std_20)
target_link_libraries(concurrency_utils INTERFACE
    Threads::Threads
)

#
# Local dev/tool/bench compile settings
#
add_library(concurrency_utils_dev_config INTERFACE)
target_compile_options(concurrency_utils_dev_config INTERFACE
    -Wall
    -Wextra
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(concurrency_utils_dev_config INTERFACE
        -stdlib=libc++
        -O3
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(concurrency_utils_dev_config INTERFACE
        -O3
    )
endif()

# x86/x64 only; do not add on Apple Silicon / ARM
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i.86)$")
    target_compile_options(concurrency_utils_dev_config INTERFACE
        -msse
        -msse2
        -msse3
        -msse4
        -msse4.1
        -msse4.2
        -mfpmath=sse
    )
endif()

#
# fmt is only needed for tools / benchmarks
#
if(CONCURRENCY_UTILS_BUILD_TOOLS OR CONCURRENCY_UTILS_BUILD_BENCHMARKS)
    find_package(fmt CONFIG QUIET)
    if(NOT fmt_FOUND)
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 11.0.2
        )
        FetchContent_MakeAvailable(fmt)
    endif()
endif()

#
# Manual tool / diagnostic binary
#
if(CONCURRENCY_UTILS_BUILD_TOOLS)
    add_executable(concurrency_utils_tool src/test/main.cpp)
    target_link_libraries(concurrency_utils_tool PRIVATE
        concurrency_utils
        concurrency_utils_dev_config
        fmt::fmt
        Threads::Threads
    )
endif()

#
# Benchmarks
#
if(CONCURRENCY_UTILS_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)

    add_executable(bench_queues src/bench/main.cpp)
    target_link_libraries(bench_queues PRIVATE
        concurrency_utils
        concurrency_utils_dev_config
        fmt::fmt
        Threads::Threads
        benchmark::benchmark
    )
endif()
