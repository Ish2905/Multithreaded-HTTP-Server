#include "http_server/HttpResponse.h"

#include <gtest/gtest.h>

TEST(HttpResponseTests, SerializesStatusAndBody) {
    http_server::HttpResponse response;
    response.setStatus(201, "Created");
    response.setBody("hello");

    const std::string serialized = response.serialize();
    EXPECT_NE(serialized.find("HTTP/1.1 201 Created\r\n"), std::string::npos);
    EXPECT_NE(serialized.find("Content-Length: 5\r\n"), std::string::npos);
    EXPECT_NE(serialized.find("\r\nhello"), std::string::npos);
}
