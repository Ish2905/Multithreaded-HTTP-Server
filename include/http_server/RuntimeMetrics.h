#pragma once

#include <atomic>
#include <cstddef>
#include <string>

namespace http_server {

class RuntimeMetrics {
public:
    RuntimeMetrics();

    void recordRequest(std::size_t bytesRead, std::size_t bytesWritten);
    void recordError();
    std::size_t totalRequests() const;
    std::size_t totalBytesRead() const;
    std::size_t totalBytesWritten() const;
    std::size_t totalErrors() const;
    std::string summary() const;

private:
    std::atomic<std::size_t> totalRequests_;
    std::atomic<std::size_t> totalBytesRead_;
    std::atomic<std::size_t> totalBytesWritten_;
    std::atomic<std::size_t> totalErrors_;
};

}  // namespace http_server
