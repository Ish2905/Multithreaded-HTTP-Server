#include "http_server/HttpServer.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    http_server::Router router;
    router.get("/", [](const http_server::HttpRequest& request) {
        (void)request;
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello from C++ HTTP Server");
        return response;
    });

    router.get("/hello", [](const http_server::HttpRequest& request) {
        (void)request;
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello World!");
        return response;
    });

    router.post("/echo", [](const http_server::HttpRequest& request) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody(request.body);
        return response;
    });

    http_server::HttpServer server("127.0.0.1", port, 4);
    server.setRouter(router);
    server.start();
    return 0;
}
