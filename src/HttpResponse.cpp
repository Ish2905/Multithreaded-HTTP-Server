#include "http_server/HttpResponse.h"

#include <sstream>
#include <utility>

namespace http_server {

HttpResponse::HttpResponse() : statusCode_(200), statusMessage_("OK") {
    setHeader("Content-Type", "text/plain; charset=utf-8");
}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode_ = code;
    statusMessage_ = message;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}

std::string HttpResponse::serialize() const {
    std::ostringstream stream;
    stream << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";

    auto contentTypeIt = headers_.find("Content-Type");
    if (contentTypeIt != headers_.end()) {
        stream << "Content-Type: " << contentTypeIt->second << "\r\n";
    }

    stream << "Content-Length: " << body_.size() << "\r\n";
    for (const auto& [key, value] : headers_) {
        if (key == "Content-Type") {
            continue;
        }
        stream << key << ": " << value << "\r\n";
    }
    stream << "\r\n";
    stream << body_;
    return stream.str();
}

int HttpResponse::statusCode() const {
    return statusCode_;
}

const std::string& HttpResponse::statusMessage() const {
    return statusMessage_;
}

const std::string& HttpResponse::body() const {
    return body_;
}

}  // namespace http_server
