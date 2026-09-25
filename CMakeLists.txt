cmake_minimum_required(VERSION 3.16)

project(CPPHTTPServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)

option(ENABLE_SANITIZERS "Enable ASan/UBSan for debugging" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

set(DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(httpserver
    src/HttpResponse.cpp
    src/HttpRequest.cpp
    src/Router.cpp
    src/ThreadPool.cpp
    src/HttpServer.cpp
    src/Logger.cpp
    src/RuntimeMetrics.cpp
)

target_include_directories(httpserver
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(httpserver PRIVATE -Wall -Wextra -Wpedantic)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE httpserver)

add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE httpserver)

add_executable(httpserver_tests
    tests/HttpResponseTests.cpp
    tests/HttpRequestParserTests.cpp
    tests/RouterTests.cpp
    tests/ThreadPoolTests.cpp
    tests/HttpServerIntegrationTests.cpp
)
target_link_libraries(httpserver_tests PRIVATE httpserver GTest::gtest_main)

target_compile_options(httpserver_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME httpserver_tests COMMAND httpserver_tests)
