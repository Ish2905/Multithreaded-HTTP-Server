#include "http_server/Router.h"

namespace http_server {

void Router::get(const std::string& path, Handler handler) {
    getRoutes_[path] = std::move(handler);
}

void Router::post(const std::string& path, Handler handler) {
    postRoutes_[path] = std::move(handler);
}

HttpResponse Router::route(const HttpRequest& request) const {
    HttpResponse response;

    const auto path = request.path;
    const auto hasGet = getRoutes_.find(path) != getRoutes_.end();
    const auto hasPost = postRoutes_.find(path) != postRoutes_.end();

    if (request.method == "GET") {
        const auto it = getRoutes_.find(path);
        if (it != getRoutes_.end()) {
            return it->second(request);
        }

        if (hasPost) {
            response.setStatus(405, "Method Not Allowed");
            response.setBody("Method Not Allowed");
            return response;
        }

        response.setStatus(404, "Not Found");
        response.setBody("Not Found");
        return response;
    }

    if (request.method == "POST") {
        const auto it = postRoutes_.find(path);
        if (it != postRoutes_.end()) {
            return it->second(request);
        }

        if (hasGet) {
            response.setStatus(405, "Method Not Allowed");
            response.setBody("Method Not Allowed");
            return response;
        }

        response.setStatus(404, "Not Found");
        response.setBody("Not Found");
        return response;
    }

    if (hasGet || hasPost) {
        response.setStatus(405, "Method Not Allowed");
        response.setBody("Method Not Allowed");
        return response;
    }

    response.setStatus(404, "Not Found");
    response.setBody("Not Found");
    return response;
}

}  // namespace http_server
