#include "http_server/HttpServer.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace {

int getAvailablePort() {
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 0;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close(sock);
        return 0;
    }

    socklen_t len = sizeof(address);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&address), &len) != 0) {
        close(sock);
        return 0;
    }
    const int port = ntohs(address.sin_port);
    close(sock);
    return port;
}

std::string readResponse(int clientSocket) {
    std::string response;
    char buffer[2048];
    while (true) {
        const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(bytesRead));
        if (bytesRead < static_cast<ssize_t>(sizeof(buffer))) {
            break;
        }
    }
    return response;
}

TEST(HttpServerIntegrationTests, HandlesSingleRequest) {
    const int port = getAvailablePort();
    http_server::Router router;
    router.get("/hello", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello!");
        return response;
    });

    http_server::HttpServer server("127.0.0.1", port, 2);
    server.setRouter(router);

    std::thread serverThread([&server]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    const int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(clientSocket, -1);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(static_cast<uint16_t>(port));
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ASSERT_EQ(connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)), 0);

    const std::string request = "GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    ASSERT_GT(send(clientSocket, request.data(), request.size(), 0), 0);

    const std::string response = readResponse(clientSocket);
    EXPECT_NE(response.find("HTTP/1.1 200 OK"), std::string::npos);
    EXPECT_NE(response.find("Hello!"), std::string::npos);

    close(clientSocket);
    server.stop();
    serverThread.join();
}

TEST(HttpServerIntegrationTests, HandlesConcurrentRequests) {
    const int port = getAvailablePort();
    http_server::Router router;
    router.get("/hello", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello!");
        return response;
    });

    http_server::HttpServer server("127.0.0.1", port, 4);
    server.setRouter(router);

    std::thread serverThread([&server]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::atomic<int> successCount{0};
    std::vector<std::thread> clients;
    clients.reserve(20);

    for (int i = 0; i < 20; ++i) {
        clients.emplace_back([&]() {
            const int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (clientSocket < 0) {
                return;
            }

            sockaddr_in serverAddress{};
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(static_cast<uint16_t>(port));
            serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
                close(clientSocket);
                return;
            }

            const std::string request = "GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
            if (send(clientSocket, request.data(), request.size(), 0) <= 0) {
                close(clientSocket);
                return;
            }

            const std::string response = readResponse(clientSocket);
            if (response.find("HTTP/1.1 200 OK") != std::string::npos && response.find("Hello!") != std::string::npos) {
                successCount.fetch_add(1, std::memory_order_relaxed);
            }
            close(clientSocket);
        });
    }

    for (std::thread& client : clients) {
        client.join();
    }

    EXPECT_EQ(successCount.load(std::memory_order_relaxed), 20);

    server.stop();
    serverThread.join();
}

TEST(HttpServerIntegrationTests, StopsListeningServerGracefully) {
    const int port = getAvailablePort();
    http_server::HttpServer server("127.0.0.1", port, 2);

    std::thread serverThread([&server]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    ASSERT_TRUE(server.running());

    server.stop();
    serverThread.join();

    EXPECT_FALSE(server.running());
}

TEST(HttpServerIntegrationTests, Returns500WhenHandlerFails) {
    const int port = getAvailablePort();
    http_server::Router router;
    router.get("/failure", [](const http_server::HttpRequest&) -> http_server::HttpResponse {
        throw std::runtime_error("handler failure");
    });

    http_server::HttpServer server("127.0.0.1", port, 2);
    server.setRouter(router);

    std::thread serverThread([&server]() {
        server.start();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(clientSocket, -1);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(static_cast<uint16_t>(port));
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ASSERT_EQ(connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)), 0);
    const std::string request = "GET /failure HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    ASSERT_GT(send(clientSocket, request.data(), request.size(), 0), 0);

    const std::string response = readResponse(clientSocket);
    EXPECT_NE(response.find("HTTP/1.1 500 Internal Server Error"), std::string::npos);

    close(clientSocket);
    server.stop();
    serverThread.join();
}

}  // namespace
