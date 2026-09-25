#include "http_server/HttpServer.h"

#include "http_server/HttpRequest.h"

#include <arpa/inet.h>
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
            }
            break;
        }

        logger_.info("Accepted client connection");
        threadPool_->submit([this, clientSocket]() { handleClient(clientSocket); });
    }

    logger_.info("Server accept loop ended");
}

void HttpServer::handleClient(int clientSocket) {
    std::string rawRequest;
    char buffer[4096];
    const std::size_t maxRequestSize = 1'048'576;

    while (running_) {
        const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }

        rawRequest.append(buffer, static_cast<std::size_t>(bytesRead));
        if (rawRequest.size() > maxRequestSize) {
            logger_.warn("Request exceeded safe limit");
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
    } else if (!parseResult.ok) {
        response.setStatus(400, "Bad Request");
        response.setBody(parseResult.error);
    } else {
        response = router_.route(parseResult.request);
    }

    const std::string serialized = response.serialize();
    send(clientSocket, serialized.c_str(), serialized.size(), 0);
    close(clientSocket);

    metrics_.recordRequest(rawRequest.size(), serialized.size());
    logger_.info("Served request: " + std::to_string(response.statusCode()) + " " + response.statusMessage());
}

}  // namespace http_server
