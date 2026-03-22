#include "thread_pool.h"

ThreadPool::ThreadPool(std::size_t thread_count) {
    if (thread_count == 0) {
        thread_count = 1;
    }
    workers_.reserve(thread_count);
    for (std::size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

