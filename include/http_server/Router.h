#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "http_server/HttpRequest.h"
#include "http_server/HttpResponse.h"

namespace http_server {

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    HttpResponse route(const HttpRequest& request) const;

private:
    std::unordered_map<std::string, Handler> getRoutes_;
    std::unordered_map<std::string, Handler> postRoutes_;
};

}  // namespace http_server
