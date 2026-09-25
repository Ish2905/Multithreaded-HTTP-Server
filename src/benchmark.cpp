#include "http_server/HttpRequest.h"
#include "http_server/HttpResponse.h"
#include "http_server/Router.h"
#include "http_server/ThreadPool.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>

namespace {

struct BenchmarkOutcome {
    std::size_t workers;
    long long milliseconds;
};

long long measureWorkers(std::size_t workerCount, std::size_t tasks) {
    http_server::Router router;
    router.get("/", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("benchmark");
        return response;
    });

    http_server::ThreadPool pool(workerCount);
    std::mutex mutex;
    std::atomic<std::size_t> completed{0};

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < tasks; ++i) {
        pool.submit([&router, &mutex, &completed]() {
            http_server::HttpRequest request;
            request.method = "GET";
            request.path = "/";
            const auto response = router.route(request);
            std::lock_guard<std::mutex> lock(mutex);
            (void)response;
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    while (completed.load(std::memory_order_relaxed) < tasks) {
        std::this_thread::yield();
    }

    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

}  // namespace

int main() {
    const std::vector<std::size_t> workerCounts = {1, 2, 4, 8};
    const std::size_t tasksPerBenchmark = 1000;

    std::cout << "Worker count benchmark\n";
    for (std::size_t workers : workerCounts) {
        const long long elapsedMs = measureWorkers(workers, tasksPerBenchmark);
        std::cout << "workers=" << workers << ", tasks=" << tasksPerBenchmark
                  << ", elapsed_ms=" << elapsedMs << '\n';
    }

    return 0;
}
