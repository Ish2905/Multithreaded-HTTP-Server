#include "http_server/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

TEST(ThreadPoolTests, ExecutesSubmittedTasks) {
    http_server::ThreadPool pool(3);
    std::atomic<int> count{0};

    for (int i = 0; i < 25; ++i) {
        pool.submit([&count]() {
            count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    for (int i = 0; i < 100; ++i) {
        if (count.load(std::memory_order_relaxed) == 25) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(count.load(std::memory_order_relaxed), 25);
}

TEST(ThreadPoolTests, ExecutesTasksConcurrently) {
    http_server::ThreadPool pool(4);
    std::atomic<int> running{0};
    std::atomic<int> maxRunning{0};
    std::atomic<int> completed{0};

    for (int i = 0; i < 12; ++i) {
        pool.submit([&]() {
            const int current = running.fetch_add(1, std::memory_order_relaxed) + 1;
            int previousMax = maxRunning.load(std::memory_order_relaxed);
            while (previousMax < current &&
                   !maxRunning.compare_exchange_weak(previousMax, current, std::memory_order_relaxed)) {}

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            running.fetch_sub(1, std::memory_order_relaxed);
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    for (int i = 0; i < 200; ++i) {
        if (completed.load(std::memory_order_relaxed) == 12) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(completed.load(std::memory_order_relaxed), 12);
    EXPECT_GT(maxRunning.load(std::memory_order_relaxed), 1);
}

TEST(ThreadPoolTests, ShutdownAwakesWorkersAndJoinsCleanly) {
    http_server::ThreadPool pool(2);
    std::atomic<int> executed{0};

    for (int i = 0; i < 8; ++i) {
        pool.submit([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            executed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.shutdown();
    EXPECT_EQ(executed.load(std::memory_order_relaxed), 8);
}
