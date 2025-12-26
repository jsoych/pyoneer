#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "server.h"
#include "json-builder.h"
#include "json-helpers.h"

#define BACKLOG 5
#define BUFLEN 4096
#define CAPACITY 8
#define server_ERROR -1

// Globals
volatile sig_atomic_t sig_flag = 0;

// Client status codes
static enum {
    CLIENT_ACTIVE,
    CLIENT_INACTIVE
};

// Client structure
static struct server_client {
    int status;
    int client_fd;
    pthread_t tid;
    Server* server;
    struct server_client* next;
};

static const char* const API_MSG[] = {
    [SERVER_START]          = "API: starting server",
    [SERVER_STOP]           = "API: stopping server",
    [SERVER_RUN]            = "API: pyoneer is running"
};

static const char* const API_ERR_MSG[] = {
    [ERR_INTERNAL]          = "API: internal error",
    [ERR_SHUTTING_DOWN]     = "API: shutting down",
    [ERR_CMD]               = "API: command error",
    [ERR_BLUEPRINT]         = "API: blueprint error",
    [ERR_SIGNAL]            = "API: signal error",
    [ERR_JSON_PARSE]        = "API: invalid JSON payload",
    [ERR_JSON_TYPE]         = "API: invalid JSON type",
    [ERR_JSON_MISSING]      = "API: missing JSON value",
    [ERR_TOKEN]             = "API: invalid token"
};

// Api server
typedef struct _server {
    Pyoneer* pyoneer;
    Logger* logger;
    char* socket_path;
    char* token;
    int server_fd;
    struct server_client* clients;
} Server;

/* server_client_thread: Handles the clients api requests. */
static void* server_client_thread(void* arg) {
    struct server_client* client = (struct server_client*)arg;
    Pyoneer* pyoneer = client->server->pyoneer;
    Logger* logger = client->server->logger;

    json_settings settings = {0};
    settings.value_extra = json_builder_extra;
    char error[json_error_max];

    int nbytes;
    char buf[BUFLEN];
    while ((nbytes = recv(client->client_fd, buf, BUFLEN, 0)) > 0) {
        buf[nbytes] = '\0';
        logger_debug(logger, buf);

        // Create response
        json_value* resp = json_object_new(0);
        if (!resp) {
            logger_debug(logger, API_ERR_MSG[ERR_INTERNAL]);
            break;
        }

        // Parse request
        json_value* req = json_parse_ex(&settings, buf, nbytes, error);
        if (!req) {
            logger_info(logger, API_ERR_MSG[ERR_JSON_PARSE]);
            logger_debug(logger, error);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_PARSE]);
            goto send;
        }

        // Check request
        if (req->type != json_object) {
            logger_info(logger, API_ERR_MSG[ERR_JSON_TYPE]);
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_TYPE]);
            goto send;
        }

        // Check token
        json_value* token = json_object_get_value(req, "token");
        if (!token) {
            logger_info(logger, API_ERR_MSG[ERR_JSON_MISSING]);
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_MISSING]);
            goto send;
        }

        if (token->type != json_string) {
            logger_info(logger, API_ERR_MSG[ERR_JSON_TYPE]);
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_TYPE]);
            goto send;
        }

        if (strcmp(token->u.string.ptr, client->server->token) != 0) {
            logger_info(logger, API_ERR_MSG[ERR_TOKEN]);
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_TOKEN]);
            goto send;
        }

        json_value* cmd = json_object_get_value(req, "command");
        if (!cmd) {
            logger_info(logger, API_ERR_MSG[ERR_JSON_MISSING]);
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_MISSING]);
            goto send;
        }

        // Call pyoneer commands
        if (strcmp(cmd->u.string.ptr, "run") == 0) {
            Blueprint* blueprint = pyoneer_blueprint_decode(pyoneer, req);
            if (!blueprint) {
                logger_info(logger, API_ERR_MSG[ERR_BLUEPRINT]);
                logger_debug(logger, buf);
                json_object_push_string(resp, "Error", API_ERR_MSG[ERR_BLUEPRINT]);
                goto send;
            }
            
            json_value* status = pyoneer->run(pyoneer, blueprint);
            json_object_push(resp, "pyoneer", status);
        }
        // get_status
        else if (strcmp(cmd->u.string.ptr, "get_status") == 0) {
            json_value* status = pyoneer->get_status(pyoneer);
            json_object_push(resp, "pyoneer", status);
        }
        // assign
        else if (strcmp(cmd->u.string.ptr, "assign") == 0) {
            Blueprint* blueprint = pyoneer_blueprint_decode(pyoneer, req);
            if (!blueprint) {
                logger_info(logger, API_ERR_MSG[ERR_BLUEPRINT]);
                logger_debug(logger, buf);
                json_object_push_string(resp, "Error", API_ERR_MSG[ERR_BLUEPRINT]);
                goto send;
            }

            json_value* status = pyoneer->assign(pyoneer, blueprint);
            json_object_push(resp, "pyoneer", status);
        }
        // Restart
        else if (strcmp(cmd->u.string.ptr, "restart") == 0) {
            pyoneer->restart(pyoneer);
        }
        // Start
        else if (strcmp(cmd->u.string.ptr, "start") == 0) {
            pyoneer->start(pyoneer);
        }
        // Stop
        else if (strcmp(cmd->u.string.ptr, "stop") == 0) {
            pyoneer->stop(pyoneer);
        }
        // Unknown command
        else {
            logger_debug(logger, buf);
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_CMD]);
        }

        send:
        if (json_measure(resp) > BUFLEN) {
            sig_flag = -1; // Exit loop
            logger_info(logger, API_ERR_MSG[ERR_INTERNAL]);
            logger_info(logger, API_ERR_MSG[ERR_SHUTTING_DOWN]);
            logger_debug(logger, buf);
            strcpy(buf, "{\"error\":\"api failure\"}");
        } else {
            json_serialize(buf, resp);
            logger_debug(logger, buf);
        }

        if (send(client->client_fd, buf, strlen(buf), 0) == -1) {
            sig_flag = -1; // Exit loop
            perror("api: send");
        }

        json_builder_free(resp);
    }

    // close socket
    close(client->client_fd);
    return NULL;
}

/* server_signal_handler: Handles the signal to the Api server. */
static void server_signal_handler(int signo) {
    sig_flag = 1;
}

/* server_create: Creates a new server and returns it. */
Server* server_create(Pyoneer* pyoneer, Logger* logger,
    const char* socket_path, const char* token) {
    Server* server = malloc(sizeof(Server));
    if (!server) {
        perror("server_create: malloc");
        return NULL;
    }
    server->pyoneer = pyoneer;
    server->logger = logger;
    server->socket_path = strdup(socket_path);
    server->token = strdup(token);
    server->server_fd = -1;
    server->clients = NULL;

    struct sigaction sa = {0};
    sa.sa_handler = server_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("server_create: sigaction");
        free(server->socket_path);
        free(server);
        return NULL;
    }
    return server;
}

/* server_destroy: Stops the server and frees its resources. */
void server_destroy(Server* server) {
    if (!server) return;
    free(server->socket_path);
    free(server->token);
    free(server);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGINT, &sa, NULL) == -1)
        perror("server_destroy: sigaction");
}

/* server_add_client: Adds a client to the server and returns a pointer to the
    newly created client */
static struct server_client* server_add_client(Server* server, int client_fd) {
    struct server_client* client = malloc(sizeof(struct server_client));
    if (client == NULL) {
        perror("server_add_client: malloc");
        return NULL;
    }

    client->client_fd = client_fd;
    client->status = CLIENT_ACTIVE;
    client->server = server;
    client->next = NULL;

    struct server_client* curr = server->clients;
    struct server_client* prev = NULL;
    while (curr) {
        if (curr->status == CLIENT_INACTIVE) {
            struct server_client* tmp = curr->next;
            if (prev){
                prev->next = tmp;
            } else {
                server->clients = tmp;
            }
            free(curr);
            curr = tmp;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    if (prev) {
        prev->next = client;
    } else {
        server->clients = client;
    }

    return client;
}

/* server_start: Creates a socket and listens for client connections. If the
    process encounters an interupt, the server stops listening for new
    client connections and returns 0. Otherwise, returns -1. */
int server_start(Server* server) {
    if (!server) return -1;

    // Create local socket
    server->server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server->server_fd == -1) {
        perror("server_start: socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, server->socket_path, sizeof(addr.sun_path));

    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("server_server_run: bind");
        close(server->server_fd);
        return -1;
    }

    if (listen(server->server_fd, BACKLOG) == -1) {
        perror("server_server_run: listen");
        close(server->server_fd);
        return -1;
    }

    logger_info(server->logger, API_MSG[SERVER_START]);
    logger_debug(server->logger, server->socket_path);

    while (sig_flag != 1) {
        int client_fd = accept(server->server_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            perror("server_server_run: accept");
            break;
        }

        struct server_client* client = server_add_client(server, client_fd);
        if (client == NULL) {
            close(client_fd);
            continue;
        }

        int err = pthread_create(&client->tid, NULL, server_client_thread, client);
        if (err != 0) {
            fprintf(stderr, "server_server_run: pthread_create: %s\n", strerror(err));
            break;
        }
    }

    if (sig_flag == 1) {
        server_stop(server);
        unlink(server->socket_path);
    }

    return 0;
}

/* server_stop: Stops the server and frees all resources, and returns 0. */
int server_stop(Server* server) {
    if (!server) return 0;
    logger_info(server->logger, API_MSG[SERVER_STOP]);
    struct server_client* curr = server->clients;
    while (curr) {
        struct server_client* next = curr->next;
        if (curr->status == CLIENT_ACTIVE) {
            int err = pthread_cancel(curr->tid);
            if (err != 0)
                fprintf(stderr, "server_stop: pthread_cancel: %s\n", strerror(err));
        }
        free(curr);
        curr = next;
    }
    server->clients = NULL;
    close(server->server_fd);
    server->server_fd = -1;
    return 0;
}
