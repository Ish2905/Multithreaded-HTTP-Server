#pragma once

#include <string>

namespace http_server {

enum class LogLevel {
    Info,
    Warning,
    Error
};

class Logger {
public:
    explicit Logger(std::string prefix = "server");

    void log(LogLevel level, const std::string& message) const;
    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;

private:
    std::string prefix_;
};

}  // namespace http_server
