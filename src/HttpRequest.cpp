#include "http_server/HttpRequest.h"

#include <cctype>
#include <sstream>
#include <vector>

namespace http_server {
namespace {

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

bool isHeaderNameValid(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    for (char ch : name) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '-')) {
            return false;
        }
    }

    return true;
}

std::string toLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

}  // namespace

ParseResult HttpRequestParser::parse(const std::string& rawRequest) {
    ParseResult result;
    if (rawRequest.empty()) {
        result.error = "Empty request";
        return result;
    }

    const std::string headerBoundary = "\r\n\r\n";
    const auto headerEnd = rawRequest.find(headerBoundary);
    if (headerEnd == std::string::npos) {
        result.error = "Incomplete request";
        return result;
    }

    const std::string headerSection = rawRequest.substr(0, headerEnd);
    std::string body = rawRequest.substr(headerEnd + headerBoundary.size());

    std::istringstream stream(headerSection);
    std::string requestLine;
    std::getline(stream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    if (requestLine.empty()) {
        result.error = "Invalid request line";
        return result;
    }

    std::vector<std::string> parts;
    std::string token;
    std::istringstream requestStream(requestLine);
    while (requestStream >> token) {
        parts.push_back(token);
    }

    if (parts.size() != 3) {
        result.error = "Invalid request line";
        return result;
    }

    result.request.method = parts[0];
    result.request.path = parts[1];
    result.request.version = parts[2];

    if (result.request.version != "HTTP/1.1" && result.request.version != "HTTP/1.0") {
        result.error = "Invalid HTTP version";
        return result;
    }

    if (result.request.method != "GET" && result.request.method != "POST") {
        result.error = "Unsupported method";
        return result;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        const std::size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            result.error = "Malformed header";
            return result;
        }

        std::string name = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        if (!isHeaderNameValid(name)) {
            result.error = "Malformed header";
            return result;
        }

        result.request.headers[toLower(name)] = value;
    }

    const auto contentLengthIt = result.request.headers.find("content-length");
    if (contentLengthIt != result.request.headers.end()) {
        try {
            const std::size_t contentLength = std::stoul(contentLengthIt->second);
            if (contentLength > 1'048'576ULL) {
                result.error = "Request too large";
                return result;
            }
            if (body.size() < contentLength) {
                result.error = "Incomplete request";
                return result;
            }
            result.request.body = body.substr(0, contentLength);
        } catch (const std::exception&) {
            result.error = "Invalid Content-Length";
            return result;
        }
    }

    result.ok = true;
    return result;
}

}  // namespace http_server
