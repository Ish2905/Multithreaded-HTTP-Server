#include "http_server/Router.h"

#include <gtest/gtest.h>

TEST(RouterTests, RoutesGetRequest) {
    http_server::Router router;
    router.get("/hello", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("Hello World!");
        return response;
    });

    http_server::HttpRequest request;
    request.method = "GET";
    request.path = "/hello";

    const auto response = router.route(request);
    EXPECT_EQ(response.statusCode(), 200);
    EXPECT_EQ(response.body(), "Hello World!");
}

TEST(RouterTests, Returns404ForUnknownRoute) {
    http_server::Router router;
    http_server::HttpRequest request;
    request.method = "GET";
    request.path = "/missing";

    const auto response = router.route(request);
    EXPECT_EQ(response.statusCode(), 404);
}

TEST(RouterTests, Returns405ForUnsupportedMethod) {
    http_server::Router router;
    router.get("/", [](const http_server::HttpRequest&) {
        http_server::HttpResponse response;
        response.setStatus(200, "OK");
        response.setBody("ok");
        return response;
    });

    http_server::HttpRequest request;
    request.method = "POST";
    request.path = "/";

    const auto response = router.route(request);
    EXPECT_EQ(response.statusCode(), 405);
    EXPECT_EQ(response.body(), "Method Not Allowed");
}
