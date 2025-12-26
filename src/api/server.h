#ifndef _SERVER_H
#define _SERVER_H

#include "pyoneer.h"
#include "blueprint.h"
#include "logger.h"

typedef enum {
    SERVER_START,
    SERVER_STOP,
    SERVER_RUN
} ServerCode;

typedef enum {
    ERR_INTERNAL = 0,
    ERR_SHUTTING_DOWN,
    ERR_CMD,
    ERR_BLUEPRINT,
    ERR_SIGNAL,
    ERR_JSON_PARSE,
    ERR_JSON_TYPE,
    ERR_JSON_MISSING,
    ERR_TOKEN
} ServerErrorCode;

typedef struct _server Server;

Server* server_create(Pyoneer* pyoneer, Logger* logger,
    const char* path, const char* token);
void server_destroy(Server* server);

int server_start(Server* server);
int server_stop(Server* server);

#endif
