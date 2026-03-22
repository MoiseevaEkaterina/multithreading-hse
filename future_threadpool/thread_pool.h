#pragma once

#include "future.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t thread_count);
    ~ThreadPool();

    template <class F, class... Args>
    auto Submit(F&& f, Args&&... args) -> Future<std::invoke_result_t<F, Args...>> {
        using Result = std::invoke_result_t<F, Args...>;

        Promise<Result> promise;
        auto future = promise.GetFuture();

        auto task = [promise = std::move(promise),
                     func = std::bind(std::forward<F>(f), std::forward<Args>(args)...)]() mutable {
            try {
                if constexpr (std::is_void_v<Result>) {
                    func();
                    promise.SetValue();
                } else {
                    promise.SetValue(func());
                }
            } catch (...) {
                promise.SetException(std::current_exception());
            }
        };

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("ThreadPool is stopping");
            }
            tasks_.emplace(std::move(task));
        }
        cv_.notify_one();
        return future;
    }

private:
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_{false};
};

