#include "apply_function.h"

#include <benchmark/benchmark.h>

#include <chrono>
#include <random>
#include <thread>
#include <vector>

namespace {

std::vector<int> MakeVector(std::size_t size) {
    std::vector<int> v(size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 1000);
    for (auto& x : v) {
        x = dist(gen);
    }
    return v;
}

void HeavyTransform(int& x) {
    for (int i = 0; i < 1'000; ++i) {
        x = (x * 13 + 7) ^ (x >> 3);
    }
}

void LightTransform(int& x) {
    ++x;
}

}

static void BM_SmallVector_LightTransform_SingleThread(benchmark::State& state) {
    auto data = MakeVector(1'000);
    for (auto _ : state) {
        auto copy = data;
        ApplyFunction<int>(copy, LightTransform, 1);
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_SmallVector_LightTransform_SingleThread);

static void BM_SmallVector_LightTransform_MultiThread(benchmark::State& state) {
    auto data = MakeVector(1'000);
    for (auto _ : state) {
        auto copy = data;
        ApplyFunction<int>(copy, LightTransform, std::thread::hardware_concurrency());
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_SmallVector_LightTransform_MultiThread);

static void BM_LargeVector_HeavyTransform_SingleThread(benchmark::State& state) {
    auto data = MakeVector(1'000'000);
    for (auto _ : state) {
        auto copy = data;
        ApplyFunction<int>(copy, HeavyTransform, 1);
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_LargeVector_HeavyTransform_SingleThread);

static void BM_LargeVector_HeavyTransform_MultiThread(benchmark::State& state) {
    auto data = MakeVector(1'000'000);
    const int threads = static_cast<int>(std::thread::hardware_concurrency());
    for (auto _ : state) {
        auto copy = data;
        ApplyFunction<int>(copy, HeavyTransform, threads > 1 ? threads : 4);
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_LargeVector_HeavyTransform_MultiThread);

BENCHMARK_MAIN();

