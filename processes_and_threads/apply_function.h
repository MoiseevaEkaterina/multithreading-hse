#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

template <typename T>
void ApplyFunction(std::vector<T>& data,
                   const std::function<void(T&)>& transform,
                   int threadCount = 1) {
    if (data.empty() || !transform) {
        return;
    }

    const std::size_t total = data.size();
    int threads = threadCount <= 0 ? 1 : threadCount;
    if (static_cast<std::size_t>(threads) > total) {
        threads = static_cast<int>(total);
    }

    auto applyRange = [&data, &transform](std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; ++i) {
            transform(data[i]);
        }
    };

    if (threads == 1) {
        applyRange(0, total);
        return;
    }

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    const std::size_t baseChunk = total / static_cast<std::size_t>(threads);
    std::size_t remainder = total % static_cast<std::size_t>(threads);
    std::size_t begin = 0;

    for (int i = 0; i < threads; ++i) {
        const std::size_t chunk = baseChunk + (remainder > 0 ? 1 : 0);
        if (remainder > 0) {
            --remainder;
        }

        const std::size_t end = begin + chunk;
        workers.emplace_back(applyRange, begin, end);
        begin = end;
    }

    for (auto& worker : workers) {
        worker.join();
    }
}