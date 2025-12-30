#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include "logger.h"
#include "pyoneer.h"

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

typedef enum {
    EP_UNIX = 0,
    EP_TCP  = 1
} endpoint_t;

typedef struct {
    endpoint_t type;
    const char* host;
    const char* port;
    const char* path;
    int backlog;
} server_endpoint;

typedef struct {
    const server_endpoint* endpoints;
    size_t nendpoints;
    const char* token;
    size_t nworkers;
    size_t queue_capacity;
} server_config;

typedef struct server Server;

Server* server_create(Pyoneer* pyoneer, Logger* logger,
    const server_config* cfg);
void server_destroy(Server* server);

Logger* server_logger(Server* server);
Pyoneer* server_pyoneer(Server* server);
const char* server_token(Server* server);
int server_run(Server* server);
void server_stop(Server* server);

#endif
