#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include "server.h"
#include "thread_pool.h"
#include "client.h"

#define DEFAULT_BACKLOG 256
#define DEFAULT_WORKERS 8
#define DEFAULT_QCAP    128

/* listener: internal representation of server_endpoint at runtime */
typedef struct {
    server_endpoint cfg;
    int fd;
    char label[128];
} listener;

/* Server: opaque in header, defined here */
struct Server {
    Pyoneer* pyoneer;
    Logger* logger;

    char* token;

    listener* listeners;
    size_t nlisteners;

    ThreadPool* pool;

    atomic_int stop_flag;
};

static void listener_format_label(listener* l) {
    if (l->cfg.type == EP_TCP) {
        const char* host = l->cfg.host ? l->cfg.host : "*";
        const char* port = l->cfg.port ? l->cfg.port : "?";
        snprintf(l->label, sizeof(l->label), "tcp:%s:%s", host, port);
    } else {
        snprintf(l->label, sizeof(l->label), "unix:%s", l->cfg.path);
    }
}

static int make_unix_listener(const char* path, int backlog) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, backlog) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int make_tcp_listener(const char* host, const char* port, int backlog) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    struct addrinfo* it = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        errno = EINVAL;
        return -1;
    }

    int fd = -1;
    for (it = res; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;

        int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(fd, it->ai_addr, it->ai_addrlen) == 0) {
            if (listen(fd, backlog) == 0) break;
        }
        
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

/* server_listeners_init: create listener sockets from cfg endpoints */
static int server_listeners_init(Server* s, const server_config* cfg) {
    s->listeners = calloc(cfg->nendpoints, sizeof(listener));
    if (!s->listeners) return -1;

    s->nlisteners = cfg->nendpoints;

    for (size_t i = 0; i < s->nlisteners; i++) {
        s->listeners[i].cfg = cfg->endpoints[i];
        s->listeners[i].fd  = -1;

        listener_format_label(&s->listeners[i]);

        int backlog = (s->listeners[i].cfg.backlog > 0) ?
            s->listeners[i].cfg.backlog : DEFAULT_BACKLOG;

        if (s->listeners[i].cfg.type == EP_UNIX) {
            if (!s->listeners[i].cfg.path) {
                logger_error(s->logger,
                    "server_create: unix endpoint missing path");
                return -1;
            }
            s->listeners[i].fd = make_unix_listener(s->listeners[i].cfg.path,
                backlog);
        } else {
            if (!s->listeners[i].cfg.port) {
                logger_error(s->logger,
                    "server_create: tcp endpoint missing port");
                return -1;
            }
            s->listeners[i].fd = make_tcp_listener(s->listeners[i].cfg.host,
                s->listeners[i].cfg.port, backlog);
        }

        if (s->listeners[i].fd < 0) {
            logger_errorf(s->logger,
                "server_create: failed to listen on %s: %s",
                s->listeners[i].label,
                strerror(errno)
            );
            return -1;
        }

        logger_infof(s->logger, "listen %s", s->listeners[i].label);
    }

    return 0;
}

/* server_listeners_destroy: Close all listener sockets + unlink unix paths */
static void server_listeners_destroy(Server* s) {
    if (!s || !s->listeners) return;

    for (size_t i = 0; i < s->nlisteners; i++) {
        if (s->listeners[i].fd >= 0) close(s->listeners[i].fd);

        if (s->listeners[i].cfg.type == EP_UNIX && s->listeners[i].cfg.path) {
            unlink(s->listeners[i].cfg.path);
        }
    }
    free(s->listeners);
    s->listeners = NULL;
    s->nlisteners = 0;
}

Server* server_create(Pyoneer* pyoneer, Logger* logger,
    const server_config* cfg) {
    if (!pyoneer || !logger || !cfg) return NULL;

    if (!cfg->endpoints || cfg->nendpoints == 0) {
        logger_error(logger, "server_create: missing endpoints");
        return NULL;
    }

    Server* s = malloc(sizeof(Server));
    if (!s) return NULL;

    s->pyoneer = pyoneer;
    s->logger  = logger;
    atomic_store(&s->stop_flag, 0);

    s->token = cfg->token ? strdup(cfg->token) : NULL;

    if (server_listeners_init(s, cfg) != 0) {
        server_destroy(s);
        return NULL;
    }

    size_t nworkers = cfg->nworkers ? cfg->nworkers : DEFAULT_WORKERS;
    size_t qcap = cfg->queue_capacity ? cfg->queue_capacity : DEFAULT_QCAP;

    // Create thread pool
    s->pool = thread_pool_create(nworkers, qcap, s);
    if (!s->pool) {
        logger_error(s->logger, "server_create: Failed to create thread pool");
        server_destroy(s);
        return NULL;
    }

    return s;
}

void server_destroy(Server* s) {
    if (!s) return;

    if (s->pool) {
        thread_pool_destroy(s->pool);
        s->pool = NULL;
    }

    server_listeners_destroy(s);

    free(s->token);
    free(s);
}

/* Server getters */
Logger* server_logger(Server* s) { return s->logger; }
Pyoneer* server_pyoneer(Server* s) { return s->pyoneer; }
const char* server_token(Server* s) { return s->token; }


/* server_stop: signal safe-ish (sets flag + wakes pool) */
void server_stop(Server* s) {
    if (!s) return;
    atomic_store(&s->stop_flag, 1);
    if (s->pool) thread_pool_shutdown(s->pool);
}

/*
 * server_run: multiple endpoint accept loop.
 * Uses poll() across all listener sockets.
 */
int server_run(Server* s) {
    if (!s) return -1;

    logger_info(s->logger, "server_run: starting");

    struct pollfd* pfds = calloc(s->nlisteners, sizeof(struct pollfd));
    if (!pfds) return -1;

    for (size_t i = 0; i < s->nlisteners; i++) {
        pfds[i].fd = s->listeners[i].fd;
        pfds[i].events = POLLIN;
    }

    while (!atomic_load(&s->stop_flag)) {
        int rc = poll(pfds, s->nlisteners, 500);
        if (rc < 0) {
            if (errno == EINTR) continue;
            logger_errorf(s->logger, "poll: %s", strerror(errno));
            break;
        }
        if (rc == 0) continue;

        for (size_t i = 0; i < s->nlisteners; i++) {
            if (!(pfds[i].revents & POLLIN)) continue;

            int client_fd = accept(pfds[i].fd, NULL, NULL);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                logger_warnf(s->logger, "accept %s: %s",
                    s->listeners[i].label, strerror(errno));
                continue;
            }

            /*
             * Submit accepted connection to thread pool.
             * ThreadPool owns the fd until client handler closes it.
             */
            if (thread_pool_submit(s->pool, client_fd, (int)i) != 0) {
                logger_warnf(s->logger,
                    "pool busy/shutting down: closing client (%s)",
                    s->listeners[i].label
                );
                close(client_fd);
            }
        }
    }

    free(pfds);
    logger_info(s->logger, "server_run: stopping");
    return 0;
}
