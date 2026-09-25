#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>

namespace http_server {

class RuntimeMetrics {
public:
    RuntimeMetrics();

    void recordRequest(std::size_t bytesRead, std::size_t bytesWritten);
    void recordError();
    void incrementActiveConnections();
    void decrementActiveConnections();
    void recordLatency(std::chrono::microseconds latency);

    std::size_t totalRequests() const;
    std::size_t totalBytesRead() const;
    std::size_t totalBytesWritten() const;
    std::size_t totalErrors() const;
    std::size_t activeConnections() const;
    std::size_t totalLatencyMicroseconds() const;
    std::string summary() const;

private:
    std::atomic<std::size_t> totalRequests_;
    std::atomic<std::size_t> totalBytesRead_;
    std::atomic<std::size_t> totalBytesWritten_;
    std::atomic<std::size_t> totalErrors_;
    std::atomic<std::size_t> activeConnections_;
    std::atomic<std::size_t> totalLatencyMicroseconds_;
};

}  // namespace http_server
