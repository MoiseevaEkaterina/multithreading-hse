#pragma once

#include <atomic>
#include <optional>
#include <stdexcept>
#include <vector>
#include <thread>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size)
        : buffer_(size), capacity_(size),
          head_(0), tail_(0), size_(0), closed_(false), locked_(false) {
    }

    void Send(const T& value) {
        for (;;) {
            if (closed_.load(std::memory_order_acquire)) {
                throw std::runtime_error("send on closed channel");
            }
            if (size_.load(std::memory_order_acquire) >= capacity_) {
                std::this_thread::yield();
                continue;
            }
            AcquireLock();
            if (closed_.load(std::memory_order_relaxed)) {
                ReleaseLock();
                throw std::runtime_error("send on closed channel");
            }
            int s = size_.load(std::memory_order_relaxed);
            if (s >= capacity_) {
                ReleaseLock();
                continue;
            }
            buffer_[tail_] = value;
            if (++tail_ == capacity_) {
                tail_ = 0;
            }
            size_.store(s + 1, std::memory_order_release);
            ReleaseLock();
            return;
        }
    }

    std::optional<T> Recv() {
        for (;;) {
            int s = size_.load(std::memory_order_acquire);
            if (s > 0) {
                AcquireLock();
                s = size_.load(std::memory_order_relaxed);
                if (s > 0) {
                    T val = std::move(buffer_[head_]);
                    if (++head_ == capacity_) {
                        head_ = 0;
                    }
                    size_.store(s - 1, std::memory_order_release);
                    ReleaseLock();
                    return val;
                }
                ReleaseLock();
                continue;
            }
            if (closed_.load(std::memory_order_acquire)) {
                if (size_.load(std::memory_order_acquire) > 0) {
                    continue;
                }
                return std::nullopt;
            }
            std::this_thread::yield();
        }
    }

    void Close() {
        AcquireLock();
        closed_.store(true, std::memory_order_release);
        ReleaseLock();
    }

private:
    void AcquireLock() {
        for (;;) {
            while (locked_.load(std::memory_order_relaxed)) {
            }
            bool expected = false;
            if (locked_.compare_exchange_weak(expected, true,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
        }
    }

    void ReleaseLock() {
        locked_.store(false, std::memory_order_release);
    }

    std::vector<T> buffer_;
    int capacity_;
    int head_;
    int tail_;
    alignas(64) std::atomic<int> size_;
    alignas(64) std::atomic<bool> closed_;
    alignas(64) std::atomic<bool> locked_;
};