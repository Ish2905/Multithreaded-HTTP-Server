# C++ Multithreaded HTTP Server

This project follows the specification in the attached document, "C++ Multithreaded HTTP Server". It is built as a practical systems-programming exercise in C++ that starts with a basic TCP socket server and progressively evolves into a compact HTTP/1.1 server with routing, multithreading, synchronization, graceful shutdown, logging, benchmarking, and testing.

The goal is not to build a production-grade framework or the largest possible server. The goal is to build a relatively small system that is easy to understand deeply, explain clearly, and defend in technical interviews.

## What this project teaches

This project covers the following areas:

- POSIX socket programming
- TCP server lifecycle: create, bind, listen, accept, recv, send, close
- HTTP fundamentals: request line, headers, body, status line, response serialization
- Object-oriented design for systems code
- Threading and synchronization in C++
- Thread pool architecture and task queues
- Request routing and handler dispatch
- Robust error handling and malformed request rejection
- Graceful startup and shutdown patterns
- Runtime metrics and basic observability
- Load testing and performance benchmarking
- Sanitizer-based debugging workflow
- CMake-based project organization

## Core project scope

At the core, the server includes the following capabilities:

- TCP socket server
- HTTP request parsing
- HTTP response generation
- GET requests
- POST requests
- Request body handling
- Routing
- Error handling
- Multithreading
- Custom thread pool
- Thread-safe task queue
- Shared-state synchronization
- CMake build system
- Unit and integration tests

- Graceful shutdown
- Structured logging
- Runtime metrics
- Load testing
- Performance benchmarking
- Worker-count comparisons
- Sanitizer-based debugging
- Strong technical documentation

## Architecture overview

```mermaid
flowchart TD
    Client[Client / curl / browser]
    Client -->|TCP| Socket[Socket Manager]
    Socket --> TaskQueue[Task Queue]
    TaskQueue --> WorkerPool[Thread Pool]
    WorkerPool --> Parser[HTTP Request Parser]
    Parser --> Router[Router]
    Router --> Handler[Route Handlers]
    Handler --> Response[HTTP Response Builder]
    Response --> Socket

    subgraph 
        Logging[Structured logging]
        Metrics[Runtime metrics]
        Shutdown[Graceful shutdown]
        Benchmark[Performance benchmarking]
    end

    WorkerPool --> Logging
    Response --> Metrics
    Socket --> Shutdown
    Router --> Benchmark
```

## Design philosophy

This project is intentionally built bottom-up instead of starting with the final architecture.

The progression is:

- TCP
- HTTP
- Routing
- Single-threaded server
- Concurrency
- Thread pool
- Synchronization
- Robustness
- Testing
- Logging and metrics
- Benchmarking

This structure matters because it isolates faults. If the server fails in a later phase, the developer knows which layer introduced the problem instead of debugging a large monolithic design at once.

## Key design decisions

### 1. Start with simple TCP

The first goal is to understand that TCP is only a byte stream. It provides reliability and ordered delivery, but it does not understand HTTP. The application layer decides what those bytes mean.

### 2. Represent HTTP objects explicitly

Before generating raw bytes on the socket, the code models the response as an object. This design makes it easier to validate status codes, headers, and bodies cleanly.

### 3. Parse requests carefully

The parser reads the request line, headers, and optional body, validates malformed input, rejects oversized requests, and returns clear error responses instead of crashing.

### 4. Keep the router simple and explicit

The router maps method + path combinations to handlers. For example:

- GET /
- GET /hello
- POST /echo

This keeps the routing layer clear and easy to explain.

### 5. Use a bounded thread pool instead of one-thread-per-connection

A naive implementation would spawn a new thread for every client. That breaks down at scale because threads consume memory and scheduling resources. The thread pool keeps concurrency bounded and predictable.

### 6. Synchronize shared state correctly

The task queue and metrics are shared across threads. The design uses mutexes and condition variables so workers can sleep when idle and wake up when work arrives.

### 7. Make shutdown graceful

The shutdown path is deliberately not a hard process kill. It preserves the server lifecycle so active work can stop cleanly and the final metrics are still reported.

## Build

Native development flow (recommended for this project):

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional Docker packaging for reproducible CI or deployment testing:

```bash
docker build -t cpp-http-server .
docker run --rm -p 8080:8080 cpp-http-server
```

## Run the server

```bash
./build/server
```

Then test with:

```bash
curl http://localhost:8080/
curl http://localhost:8080/hello
curl -X POST http://localhost:8080/echo -d "hello"
```

## Expected routes

The server implements the initial route set from the specification:

- GET / → Hello from C++ HTTP Server
- GET /hello → Hello World!
- POST /echo → echoes the request body

Unknown routes return a 404 response.
Known routes with unsupported methods return a 405 response.

## Testing

The project includes unit tests for:

- HTTP response serialization
- HTTP request parsing
- HTTP request body handling
- Route matching and method validation

Run the tests with:

```bash
ctest --test-dir build --output-on-failure
```

## Benchmarking

The benchmark target measures how different worker counts affect task throughput for the thread pool.

```bash
./build/benchmark
```

This is useful for illustrating trade-offs between concurrency, overhead, and throughput in a bounded worker model.

## Sanitizer debugging

The CMake project includes a sanitizer option for debugging memory and undefined behavior issues.

```bash
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

This is particularly useful for catching socket lifecycle mistakes and concurrency defects during development.




