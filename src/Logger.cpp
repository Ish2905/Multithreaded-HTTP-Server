#include "http_server/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace http_server {

namespace {

std::string timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&raw, &tm);

    std::ostringstream stream;
    stream << std::put_time(&tm, "%H:%M:%S");
    return stream.str();
}

const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "INFO";
}

}  // namespace

Logger::Logger(std::string prefix) : prefix_(std::move(prefix)) {}

void Logger::log(LogLevel level, const std::string& message) const {
    std::cout << "[" << timestamp() << "] [" << prefix_ << "] [" << levelToString(level) << "] " << message << std::endl;
}

void Logger::info(const std::string& message) const {
    log(LogLevel::Info, message);
}

void Logger::warn(const std::string& message) const {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) const {
    log(LogLevel::Error, message);
}

}  // namespace http_server
