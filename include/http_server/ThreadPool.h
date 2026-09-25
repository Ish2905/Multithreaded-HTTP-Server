#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace http_server {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t workerCount);
    ~ThreadPool();

    template <typename F>
    void submit(F&& task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                return;
            }
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }

    void shutdown();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_ = false;
};

}  // namespace http_server
