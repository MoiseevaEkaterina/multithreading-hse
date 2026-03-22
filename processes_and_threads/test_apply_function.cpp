#include "apply_function.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

TEST(ApplyFunctionTest, SingleThreadBasicTransform) {
    std::vector<int> data{1, 2, 3, 4, 5};

    ApplyFunction<int>(data, [](int& x) { x *= 2; }, 1);

    EXPECT_EQ((std::vector<int>{2, 4, 6, 8, 10}), data);
}

TEST(ApplyFunctionTest, MultiThreadSameResultAsSingleThread) {
    constexpr std::size_t n = 1'000;
    std::vector<int> data1(n, 1);
    std::vector<int> data2(n, 1);

    auto transform = [](int& x) { x += 5; };

    ApplyFunction<int>(data1, transform, 1);
    ApplyFunction<int>(data2, transform, 8);

    EXPECT_EQ(data1, data2);
}

TEST(ApplyFunctionTest, ThreadCountGreaterThanElements) {
    std::vector<int> data{1, 2, 3};

    ApplyFunction<int>(data, [](int& x) { x += 1; }, 8);

    EXPECT_EQ((std::vector<int>{2, 3, 4}), data);
}

TEST(ApplyFunctionTest, HandlesEmptyVector) {
    std::vector<int> data;
    ApplyFunction<int>(data, [](int& x) { x += 1; }, 4);
    EXPECT_TRUE(data.empty());
}

TEST(ApplyFunctionTest, HandlesZeroThreadCountAsSingleThread) {
    std::vector<int> data{1, 2, 3};
    ApplyFunction<int>(data, [](int& x) { x *= 3; }, 0);
    EXPECT_EQ((std::vector<int>{3, 6, 9}), data);
}

TEST(ApplyFunctionTest, AllElementsProcessedExactlyOnce) {
    constexpr std::size_t n = 10'000;
    std::vector<int> data(n, 0);

    ApplyFunction<int>(data, [](int& x) { ++x; }, 16);

    for (auto v : data) {
        EXPECT_EQ(1, v);
    }
}

