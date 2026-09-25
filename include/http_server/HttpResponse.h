#pragma once

#include <string>
#include <unordered_map>

namespace http_server {

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string serialize() const;

    int statusCode() const;
    const std::string& statusMessage() const;
    const std::string& body() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

}  // namespace http_server
