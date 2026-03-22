cmake_minimum_required(VERSION 3.25)

project(futex_mutex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(futex_mutex_lib
    futex_mutex.h
    futex_mutex.cpp
)

target_link_libraries(futex_mutex_lib PRIVATE Threads::Threads)

add_executable(futex_mutex_example main.cpp)
target_link_libraries(futex_mutex_example PRIVATE futex_mutex_lib)

