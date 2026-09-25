#pragma once

#include <string>
#include <unordered_map>

namespace http_server {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct ParseResult {
    bool ok = false;
    HttpRequest request;
    std::string error;
};

class HttpRequestParser {
public:
    static ParseResult parse(const std::string& rawRequest);
};

}  // namespace http_server
