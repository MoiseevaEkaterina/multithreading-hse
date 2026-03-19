#include "futex_mutex.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {

void FutexWait(void* value, int expectedValue) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expectedValue, nullptr, nullptr, 0);
}

void FutexWake(void* value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

}

FutexMutex::FutexMutex()
    : state_(0) {
}

void FutexMutex::Lock() {
    int expected = 0;
    if (state_.compare_exchange_strong(expected, 1, std::memory_order_acquire)) {
        return;
    }

    while (true) {
        int old = state_.exchange(2, std::memory_order_acquire);
        if (old == 0) {
            return;
        }
        FutexWait(&state_, 2);
    }
}

bool FutexMutex::TryLock() {
    int expected = 0;
    return state_.compare_exchange_strong(expected, 1, std::memory_order_acquire);
}

void FutexMutex::Unlock() {
    int old = state_.fetch_sub(1, std::memory_order_release);
    if (old != 1) {
        state_.store(0, std::memory_order_release);
        FutexWake(&state_, 1);
    }
}

