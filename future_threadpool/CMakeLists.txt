cmake_minimum_required(VERSION 3.25)

project(future_threadpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(future_threadpool_lib
    future.h
    thread_pool.h
    thread_pool.cpp
)

target_link_libraries(future_threadpool_lib PRIVATE Threads::Threads)

add_executable(thread_pool_example main_thread_pool.cpp)
target_link_libraries(thread_pool_example PRIVATE future_threadpool_lib)

