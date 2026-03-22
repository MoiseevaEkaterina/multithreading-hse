#pragma once
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>

struct StateBase {
    std::mutex mutex;
    std::condition_variable cv;
    std::exception_ptr exception;
    bool ready{false};

    void Wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return ready; });
        if (exception) std::rethrow_exception(exception);
    }

    void NotifyReady() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ready = true;
        }
        cv.notify_all();
    }
};

template <class T>
struct SharedState : public StateBase {
    std::optional<T> value;
};

template <>
struct SharedState<void> : public StateBase {};

template <class T>
class Future {
    std::shared_ptr<SharedState<T>> state_;
public:
    explicit Future(std::shared_ptr<SharedState<T>> s) : state_(std::move(s)) {}

    T Get() {
        state_->Wait();
        if constexpr (!std::is_void_v<T>) {
            return std::move(*(state_->value));
        }
    }
};

template <class T>
class Promise {
    std::shared_ptr<SharedState<T>> state_ = std::make_shared<SharedState<T>>();
public:
    Future<T> GetFuture() { return Future<T>(state_); }

    template <typename V = T>
    void SetValue(std::enable_if_t<!std::is_void_v<V>, V>&& value) {
        state_->value.emplace(std::forward<V>(value));
        state_->NotifyReady();
    }

    template <typename V = T>
    void SetValue(std::enable_if_t<std::is_void_v<V>>* = nullptr) {
        state_->NotifyReady();
    }

    void SetException(std::exception_ptr e) {
        state_->exception = e;
        state_->NotifyReady();
    }
};