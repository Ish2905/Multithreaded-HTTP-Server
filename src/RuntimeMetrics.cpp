#include "http_server/RuntimeMetrics.h"

#include <sstream>

namespace http_server {

RuntimeMetrics::RuntimeMetrics()
    : totalRequests_(0),
      totalBytesRead_(0),
      totalBytesWritten_(0),
      totalErrors_(0),
      activeConnections_(0),
      totalLatencyMicroseconds_(0) {}

void RuntimeMetrics::recordRequest(std::size_t bytesRead, std::size_t bytesWritten) {
    totalRequests_.fetch_add(1, std::memory_order_relaxed);
    totalBytesRead_.fetch_add(bytesRead, std::memory_order_relaxed);
    totalBytesWritten_.fetch_add(bytesWritten, std::memory_order_relaxed);
}

void RuntimeMetrics::recordError() {
    totalErrors_.fetch_add(1, std::memory_order_relaxed);
}

void RuntimeMetrics::incrementActiveConnections() {
    activeConnections_.fetch_add(1, std::memory_order_relaxed);
}

void RuntimeMetrics::decrementActiveConnections() {
    if (activeConnections_.load(std::memory_order_relaxed) > 0) {
        activeConnections_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void RuntimeMetrics::recordLatency(std::chrono::microseconds latency) {
    totalLatencyMicroseconds_.fetch_add(static_cast<std::size_t>(latency.count()), std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::totalRequests() const {
    return totalRequests_.load(std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::totalBytesRead() const {
    return totalBytesRead_.load(std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::totalBytesWritten() const {
    return totalBytesWritten_.load(std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::totalErrors() const {
    return totalErrors_.load(std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::activeConnections() const {
    return activeConnections_.load(std::memory_order_relaxed);
}

std::size_t RuntimeMetrics::totalLatencyMicroseconds() const {
    return totalLatencyMicroseconds_.load(std::memory_order_relaxed);
}

std::string RuntimeMetrics::summary() const {
    std::ostringstream stream;
    stream << "requests=" << totalRequests() << ", errors=" << totalErrors()
           << ", active_connections=" << activeConnections() << ", bytes_in=" << totalBytesRead()
           << ", bytes_out=" << totalBytesWritten() << ", total_latency_us=" << totalLatencyMicroseconds();
    return stream.str();
}

}  // namespace http_server
