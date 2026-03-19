cmake_minimum_required(VERSION 3.25)

project(ipc_mpsc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(ipc_mpsc_lib
    shared_queue.h
    shared_queue.cpp
)

target_link_libraries(ipc_mpsc_lib PRIVATE Threads::Threads rt)

add_executable(ipc_producer main_producer.cpp)
target_link_libraries(ipc_producer PRIVATE ipc_mpsc_lib)

add_executable(ipc_consumer main_consumer.cpp)
target_link_libraries(ipc_consumer PRIVATE ipc_mpsc_lib)
