#include "http_server/HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
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

std::string makeHttpRequest(const std::string& method, const std::string& path, const std::string& body = "") {
    std::string request = method + " " + path + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n";
    if (!body.empty()) {
        request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }
    request += "\r\n";
    request += body;
    return request;
}

std::string sendRequest(int port, const std::string& request) {
    const int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        return "";
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(static_cast<uint16_t>(port));
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) != 0) {
        close(clientSocket);
        return "";
    }

    std::string response;
    if (send(clientSocket, request.data(), request.size(), 0) <= 0) {
        close(clientSocket);
        return "";
    }

    char buffer[4096];
    while (true) {
        const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(bytesRead));
    }

    close(clientSocket);
    return response;
}

struct BenchmarkResult {
    std::size_t workers;
    std::size_t requests;
    double requestsPerSecond;
    double averageLatencyMs;
    double p95LatencyMs;
    std::size_t errors;
};

BenchmarkResult measureWorkerCount(std::size_t workerCount, std::size_t requestsPerRun) {
    const int port = getAvailablePort();
    if (port == 0) {
        return {workerCount, requestsPerRun, 0.0, 0.0, 0.0, requestsPerRun};
    }

    http_server::Router router;
    router.get("/hello", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello!");
        return response;
    });

    http_server::HttpServer server("127.0.0.1", port, workerCount);
    server.setRouter(router);

    std::thread serverThread([&server]() {
        server.start();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::atomic<std::size_t> success{0};
    std::atomic<std::size_t> errors{0};
    std::vector<long long> latencies(requestsPerRun, 0);
    std::vector<std::thread> clients;
    clients.reserve(requestsPerRun);

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < requestsPerRun; ++i) {
        clients.emplace_back([port, i, &success, &errors, &latencies]() {
            const auto requestStart = std::chrono::steady_clock::now();
            const std::string request = makeHttpRequest("GET", "/hello");
            const std::string response = sendRequest(port, request);
            const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - requestStart).count();
            latencies[i] = elapsedMs;

            if (response.find("HTTP/1.1 200 OK") != std::string::npos && response.find("Hello!") != std::string::npos) {
                success.fetch_add(1, std::memory_order_relaxed);
            } else {
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (std::thread& client : clients) {
        client.join();
    }

    const auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    server.stop();
    serverThread.join();

    std::vector<long long> sortedLatencies = latencies;
    std::sort(sortedLatencies.begin(), sortedLatencies.end());
    const std::size_t p95Index = std::max<std::size_t>(1, sortedLatencies.size() * 95 / 100) - 1;
    const double averageLatencyMs = static_cast<double>(std::accumulate(sortedLatencies.begin(), sortedLatencies.end(), 0LL)) /
        static_cast<double>(sortedLatencies.size());
    const double p95LatencyMs = static_cast<double>(sortedLatencies[p95Index]);
    const double requestsPerSecond = totalDuration > 0 ?
        static_cast<double>(requestsPerRun) / (static_cast<double>(totalDuration) / 1000.0) :
        0.0;

    return {workerCount, requestsPerRun, requestsPerSecond, averageLatencyMs, p95LatencyMs, errors.load(std::memory_order_relaxed)};
}

}  // namespace

int main() {
    const std::vector<std::size_t> workerCounts = {1, 2, 4, 8};
    const std::size_t requestsPerRun = 100;

    std::cout << "HTTP server benchmark: actual TCP + HTTP requests\n";
    std::cout << "workers,requests,throughput_rps,avg_latency_ms,p95_latency_ms,errors\n";
    for (std::size_t workers : workerCounts) {
        const auto result = measureWorkerCount(workers, requestsPerRun);
        std::cout << workers << "," << result.requests << "," << result.requestsPerSecond << ","
                  << result.averageLatencyMs << "," << result.p95LatencyMs << "," << result.errors << '\n';
    }

    return 0;
}
