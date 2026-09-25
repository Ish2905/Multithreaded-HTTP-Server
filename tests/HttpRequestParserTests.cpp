#include "http_server/HttpRequest.h"

#include <gtest/gtest.h>

TEST(HttpRequestParserTests, ParsesGetRequest) {
    const std::string raw =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: curl\r\n"
        "\r\n";

    const auto result = http_server::HttpRequestParser::parse(raw);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.request.method, "GET");
    EXPECT_EQ(result.request.path, "/hello");
    EXPECT_EQ(result.request.version, "HTTP/1.1");
    EXPECT_EQ(result.request.headers.at("host"), "localhost:8080");
}

TEST(HttpRequestParserTests, ParsesAnyValidMethodToken) {
    const std::string raw =
        "DELETE /hello HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "\r\n";

    const auto result = http_server::HttpRequestParser::parse(raw);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.request.method, "DELETE");
    EXPECT_EQ(result.request.path, "/hello");
}

TEST(HttpRequestParserTests, ParsesPostWithBody) {
    const std::string raw =
        "POST /echo HTTP/1.1\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";

    const auto result = http_server::HttpRequestParser::parse(raw);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.request.method, "POST");
    EXPECT_EQ(result.request.path, "/echo");
    EXPECT_EQ(result.request.body, "hello");
}

TEST(HttpRequestParserTests, RejectsInvalidContentLength) {
    const std::string raw =
        "POST /echo HTTP/1.1\r\n"
        "Content-Length: bad\r\n"
        "\r\n";

    const auto result = http_server::HttpRequestParser::parse(raw);
    EXPECT_FALSE(result.ok);
}
