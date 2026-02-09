// server.h — API server object.
//
// Server accepts client connections on one or more endpoints (UNIX and/or TCP),
// authenticates requests, dispatches commands to Pyoneer, and writes responses.
//
// Design:
// - Endpoints are configured at startup via server_config.
// - Server may listen on multiple endpoints simultaneously.
// - Requests are authenticated using a shared token.
// - Server uses a worker thread pool to process accepted connections.
//
// Ownership:
// - Server does not own Pyoneer or Logger; they must outlive the Server.
// - Server copies endpoint configuration as needed (implementation-defined).
//
// Thread safety:
// - Server is intended to be started once and run until stopped.
// - server_stop() is safe to call from a signal handler or another thread
//   (it should only set a stop flag and wake blocking syscalls).
// - Requests may be handled concurrently by worker threads; Pyoneer and Logger
//   must be thread-safe for concurrent use.
//

#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>

#include "logger.h"
#include "pyoneer.h"

// Server error codes used in logs and JSON responses (implementation-defined).
enum {
    ERR_INTERNAL = 0,
    ERR_SHUTTING_DOWN,
    ERR_CMD,
    ERR_BLUEPRINT,
    ERR_SIGNAL,
    ERR_JSON_PARSE,
    ERR_JSON_TYPE,
    ERR_JSON_MISSING,
    ERR_TOKEN
};

// Supported listening endpoint types.
// EP_UNIX is intended for local/dev/admin.
// EP_TCP is intended for network deployments (e.g., behind a proxy).
typedef enum {
    EP_UNIX = 0,
    EP_TCP  = 1
} endpoint_t;

// server_endpoint describes one listening socket.
typedef struct {
    endpoint_t type;

    // TCP endpoint configuration (used when type == EP_TCP).
    // host may be NULL to bind to wildcard (implementation-defined).
    const char* host;     // e.g. "127.0.0.1", "0.0.0.0", "::"
    const char* port;     // e.g. "8080"

    // UNIX endpoint configuration (used when type == EP_UNIX).
    const char* path;     // e.g. "/tmp/pyoneer.sock"

    // listen() backlog for this endpoint.
    int backlog;
} server_endpoint;

// server_config describes startup configuration for Server.
typedef struct {
    // Array of endpoints to listen on.
    const server_endpoint* endpoints;
    size_t nendpoints;

    // Shared authentication token required by clients.
    const char* token;

    // Worker thread pool configuration.
    size_t nworkers;
    size_t queue_capacity;
} server_config;

// Server is an opaque object responsible for endpoint management and request dispatch.
typedef struct Server Server;

// server_create allocates a Server using the provided configuration.
// Returns a new Server on success; NULL on error.
//
// Preconditions:
// - pyoneer and logger must be valid and remain alive for the Server lifetime.
// - cfg must be valid for the duration of server_create.
// - cfg->endpoints must contain at least one endpoint.
Server* server_create(Pyoneer* pyoneer, Logger* logger, const server_config* cfg);

// server_destroy stops the server (if running) and frees server resources.
// Safe to call with NULL.
void server_destroy(Server* server);

// Accessors (borrowed pointers; Server retains ownership of internal state).
Logger*     server_logger(Server* server);
Pyoneer*    server_pyoneer(Server* server);
const char* server_token(Server* server);

// server_run enters the main accept loop and blocks until the server stops.
// Returns 0 on clean shutdown; -1 on error.
int server_run(Server* server);

// server_stop requests that server_run() exit.
// This function is intended to be async-safe (implementation-defined).
void server_stop(Server* server);

#endif
