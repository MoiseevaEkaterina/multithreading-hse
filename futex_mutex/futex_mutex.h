#pragma once

#include <atomic>

class FutexMutex {
public:
    FutexMutex();

    void Lock();
    bool TryLock();
    void Unlock();

private:
    std::atomic<int> state_;
};

