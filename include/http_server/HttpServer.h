#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "http_server/Logger.h"
#include "http_server/Router.h"
#include "http_server/RuntimeMetrics.h"
#include "http_server/ThreadPool.h"

namespace http_server {

class HttpServer {
public:
    HttpServer(const std::string& host = "127.0.0.1", int port = 8080, std::size_t workerCount = 4);
    ~HttpServer();

    void setRouter(const Router& router);
    void start();
    void stop();
    bool running() const;

private:
    void configureSignalHandlers();
    void acceptLoop();
    void handleClient(int clientSocket);

    std::string host_;
    int port_;
    int listenSocket_ = -1;
    std::atomic<bool> running_;
    std::unique_ptr<ThreadPool> threadPool_;
    Router router_;
    Logger logger_;
    RuntimeMetrics metrics_;
};

}  // namespace http_server
