#include "http_server/HttpServer.h"

#include "http_server/HttpRequest.h"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

namespace http_server {
namespace {

std::atomic<bool>* g_running = nullptr;

void signalHandler(int) {
    if (g_running != nullptr) {
        *g_running = false;
    }
}

bool sendAll(int socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t bytesSent = send(socket, data.data() + sent, data.size() - sent, 0);
        if (bytesSent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (bytesSent == 0) {
            return false;
        }
        sent += static_cast<std::size_t>(bytesSent);
    }
    return true;
}

}  // namespace

HttpServer::HttpServer(const std::string& host, int port, std::size_t workerCount)
    : host_(host), port_(port), running_(true), logger_("server") {
    threadPool_ = std::make_unique<ThreadPool>(workerCount);
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setRouter(const Router& router) {
    router_ = router;
}

void HttpServer::configureSignalHandlers() {
    g_running = &running_;
    struct sigaction action;
    std::memset(&action, 0, sizeof(action));
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

void HttpServer::start() {
    configureSignalHandlers();

    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ < 0) {
        logger_.error("Failed to create socket");
        return;
    }

    int opt = 1;
    if (setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_.warn("Unable to set SO_REUSEADDR");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    address.sin_addr.s_addr = inet_addr(host_.c_str());

    if (bind(listenSocket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        logger_.error("Failed to bind to port " + std::to_string(port_));
        close(listenSocket_);
        listenSocket_ = -1;
        return;
    }

    if (listen(listenSocket_, 128) < 0) {
        logger_.error("Failed to listen on port " + std::to_string(port_));
        close(listenSocket_);
        listenSocket_ = -1;
        return;
    }

    logger_.info("Listening on http://" + host_ + ":" + std::to_string(port_));
    acceptLoop();
}

void HttpServer::stop() {
    running_ = false;
    if (listenSocket_ >= 0) {
        shutdown(listenSocket_, SHUT_RDWR);
        close(listenSocket_);
        listenSocket_ = -1;
    }
    if (threadPool_) {
        threadPool_->shutdown();
    }
    logger_.info("Runtime metrics: " + metrics_.summary());
}

bool HttpServer::running() const {
    return running_.load();
}

void HttpServer::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);
        const int clientSocket = accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
        if (clientSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (running_) {
                logger_.error("accept failed");
                metrics_.recordError();
            }
            break;
        }

        logger_.info("Accepted client connection");
        threadPool_->submit([this, clientSocket]() { handleClient(clientSocket); });
    }

    logger_.info("Server accept loop ended");
}

void HttpServer::handleClient(int clientSocket) {
    const auto startTime = std::chrono::steady_clock::now();
    metrics_.incrementActiveConnections();
    auto cleanup = [&]() {
        close(clientSocket);
        metrics_.decrementActiveConnections();
    };

    std::string rawRequest;
    char buffer[4096];
    const std::size_t maxRequestSize = 1'048'576;

    while (running_) {
        const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                metrics_.recordError();
            }
            break;
        }

        rawRequest.append(buffer, static_cast<std::size_t>(bytesRead));
        if (rawRequest.size() > maxRequestSize) {
            logger_.warn("Request exceeded safe limit");
            metrics_.recordError();
            break;
        }

        const ParseResult parseResult = HttpRequestParser::parse(rawRequest);
        if (parseResult.ok) {
            break;
        }
        if (parseResult.error != "Incomplete request") {
            break;
        }
    }

    HttpResponse response;
    ParseResult parseResult = HttpRequestParser::parse(rawRequest);
    if (rawRequest.empty()) {
        response.setStatus(400, "Bad Request");
        response.setBody("Bad Request");
        metrics_.recordError();
    } else if (!parseResult.ok) {
        response.setStatus(400, "Bad Request");
        response.setBody(parseResult.error);
        metrics_.recordError();
    } else {
        try {
            response = router_.route(parseResult.request);
        } catch (const std::exception& error) {
            logger_.error("Request handler failed: " + std::string(error.what()));
            metrics_.recordError();
            response.setStatus(500, "Internal Server Error");
            response.setBody("Internal Server Error");
        } catch (...) {
            logger_.error("Request handler failed with an unknown error");
            metrics_.recordError();
            response.setStatus(500, "Internal Server Error");
            response.setBody("Internal Server Error");
        }
    }

    const std::string serialized = response.serialize();
    if (!sendAll(clientSocket, serialized)) {
        logger_.error("Failed to send full response");
        metrics_.recordError();
    }

    cleanup();
    metrics_.recordRequest(rawRequest.size(), serialized.size());
    metrics_.recordLatency(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime));
    logger_.info("Served request: " + std::to_string(response.statusCode()) + " " + response.statusMessage());
}

}  // namespace http_server
