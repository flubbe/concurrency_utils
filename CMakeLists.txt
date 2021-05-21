cmake_policy(VERSION 3.6.0)
cmake_minimum_required (VERSION 3.6.0)

project (concurrency_utils)

set(CMAKE_BUILD_TYPE Release)

#
# Enable C++17, SSE and optimize code.
#
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++ -Wall -O3 -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2 -mfpmath=sse")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -O3 -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2 -mfpmath=sse")
endif()

#
# Thread library.
#
set(CMAKE_THREAD_PREFER_PTHREAD ON)                                                                                                                                                                                                           
set(THREADS_PREFER_PTHREAD_FLAG ON)                                                                                                                                                                                                           
find_package(Threads REQUIRED)    

#
# Set up output directories
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin )

#
# Set up project include directories
#
include_directories(include)

#
# build tests
#
add_executable(test_thread_pool.linux src/test/main.cpp)
target_link_libraries(test_thread_pool.linux fmt Threads::Threads)

#
# build benchmark
#
add_executable(bench_thread_pool.linux src/bench/main.cpp)
target_link_libraries(bench_thread_pool.linux fmt Threads::Threads)
