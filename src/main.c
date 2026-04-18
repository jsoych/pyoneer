#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "api/api.h"
#include "pyoneer/pyoneer.h"
#include "shared/shared.h"

// Globals
Site *SITE = NULL;
static Server *SERVER = NULL;

static void on_term(int signo)
{
    (void)signo;
    if (SERVER)
        server_stop(SERVER);
}

/* policy_init: Initializes the signal actions, given a server. */
static int policy_init(Server *server)
{
    if (!server)
    {
        errno = EINVAL;
        return -1;
    }

    SERVER = server;

    // 1) Ignore SIGPIPE so send() to a closed socket doesn't kill the process.
    struct sigaction sa_ign;
    memset(&sa_ign, 0, sizeof(sa_ign));
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    if (sigaction(SIGPIPE, &sa_ign, NULL) != 0)
    {
        return -1;
    }

    // 2) Graceful shutdown on SIGINT and SIGTERM.
    struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = on_term;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;

    if (sigaction(SIGINT, &sa_term, NULL) != 0)
    {
        return -1;
    }
    if (sigaction(SIGTERM, &sa_term, NULL) != 0)
    {
        return -1;
    }

    return 0;
}

// app config
typedef struct
{
    int id;
    int role;
    int level;
    const char *name;

    // networking config
    const char *tcp_host;
    const char *tcp_port;
    const char *unix_path;
    int backlog;

    // pool config
    size_t nworkers;
    size_t queue_capacity;
} app_config;

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s --role=manager|worker --id=<n> [--debug] "
            "[--host=<h>] [--port=<p>] [--unix=<path>]\n",
            prog);
}

static bool starts_with(const char *s, const char *prefix)
{
    size_t n = strlen(prefix);
    return (strlen(s) >= n && memcmp(s, prefix, n) == 0);
}

static const char *after_prefix(const char *s, const char *prefix)
{
    return (s + strlen(prefix));
}

static int parse_flags(int argc, char *argv[], app_config *cfg)
{
    for (int i = 1; i < argc; i++)
    {
        const char *a = argv[i];

        if (!starts_with(a, "--"))
        {
            fprintf(stderr, "main: invalid arg (expected --flag): %s\n", a);
            return -1;
        }

        a = after_prefix(a, "--");

        if (strcmp(a, "debug") == 0)
        {
            cfg->level = LOGGER_DEBUG;
            continue;
        }

        if (starts_with(a, "id="))
        {
            const char *v = after_prefix(a, "id=");
            cfg->id = (int)strtol(v, NULL, 10);
            continue;
        }

        if (starts_with(a, "port="))
        {
            cfg->tcp_port = after_prefix(a, "port=");
            continue;
        }

        if (starts_with(a, "host="))
        {
            cfg->tcp_host = after_prefix(a, "host=");
            continue;
        }

        if (starts_with(a, "unix="))
        {
            cfg->unix_path = after_prefix(a, "unix=");
            continue;
        }

        if (starts_with(a, "role="))
        {
            const char *v = after_prefix(a, "role=");
            if (strcmp(v, "manager") == 0)
            {
                cfg->role = PYONEER_MANAGER;
                cfg->name = "manager";
            }
            else if (strcmp(v, "worker") == 0)
            {
                cfg->role = PYONEER_WORKER;
                cfg->name = "worker";
            }
            else
            {
                fprintf(stderr, "main: unknown role: %s\n", v);
                return -1;
            }
            continue;
        }

        fprintf(stderr, "main: unknown flag: --%s\n", a);
        return -1;
    }

    if (cfg->role < 0)
    {
        fprintf(stderr, "main: missing required flag: --role=\n");
        return -1;
    }

    // If id=0 is valid for you, remove this check.
    if (cfg->id == 0)
    {
        fprintf(stderr, "main: missing required flag: --id=\n");
        return -1;
    }

    return 0;
}

static const char *env_or_default(const char *key, const char *def)
{
    const char *v = getenv(key);
    return v ? v : def;
}

int main(int argc, char *argv[])
{
    int rc = EXIT_FAILURE;

    app_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.role = -1;
    cfg.level = LOGGER_INFO;
    cfg.name = "pyoneer";

    cfg.tcp_host = "0.0.0.0";
    cfg.tcp_port = env_or_default("PYONEER_PORT", "8080");
    cfg.unix_path = getenv("PYONEER_SOCKET_PATH"); // --unix overrides
    cfg.backlog = 256;

    cfg.nworkers = 8;
    cfg.queue_capacity = 128;

    if (argc < 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_flags(argc, argv, &cfg) != 0)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    Logger *logger = NULL;
    Pyoneer *pyoneer = NULL;
    Server *server = NULL;

    if (cfg.role == PYONEER_WORKER)
    {
        const char *py = getenv("PYONEER_PYTHON");
        const char *task_dir = getenv("PYONEER_TASK_DIR");
        const char *work_dir = getenv("PYONEER_WORKING_DIR");

        if (!py || !task_dir || !work_dir)
        {
            fprintf(stderr,
                    "main: missing env for worker: PYONEER_PYTHON, PYONEER_TASK_DIR, PYONEER_WORKING_DIR\n");
            goto cleanup;
        }

        SITE = site_create(py, task_dir, work_dir);
        if (!SITE)
        {
            fprintf(stderr, "main: site_create failed\n");
            goto cleanup;
        }
    }

    logger = logger_create(cfg.level, cfg.name);
    if (!logger)
    {
        fprintf(stderr, "main: logger_create failed\n");
        goto cleanup;
    }

    pyoneer = pyoneer_create(cfg.role, cfg.id);
    if (!pyoneer)
    {
        fprintf(stderr, "main: pyoneer_create failed\n");
        goto cleanup;
    }

    const char *token = getenv("PYONEER_API_TOKEN");
    if (!token)
    {
        fprintf(stderr, "main: missing env: PYONEER_API_TOKEN\n");
        goto cleanup;
    }

    // Configure endpoints
    server_endpoint eps[2];
    size_t nep = 0;

    eps[nep++] = (server_endpoint){
        .type = EP_TCP,
        .host = cfg.tcp_host,
        .port = cfg.tcp_port,
        .path = NULL,
        .backlog = cfg.backlog};

    if (cfg.unix_path && cfg.unix_path[0] != '\0')
    {
        eps[nep++] = (server_endpoint){
            .type = EP_UNIX,
            .host = NULL,
            .port = NULL,
            .path = cfg.unix_path,
            .backlog = cfg.backlog};
    }

    server_config scfg = {
        .endpoints = eps,
        .nendpoints = nep,
        .token = token,
        .nworkers = cfg.nworkers,
        .queue_capacity = cfg.queue_capacity};

    server = server_create(pyoneer, logger, &scfg);
    if (!server)
    {
        fprintf(stderr, "main: server_create failed\n");
        goto cleanup;
    }

    if (policy_init(server) != 0)
    {
        fprintf(stderr, "main: policy_init failed: %s\n", strerror(errno));
        goto cleanup;
    }

    // Blocking run loop (poll accept across endpoints + pool)
    if (server_run(server) != 0)
    {
        fprintf(stderr, "main: server_run failed\n");
        goto cleanup;
    }

    rc = EXIT_SUCCESS;

cleanup:
    if (server)
        server_destroy(server);
    if (pyoneer)
        pyoneer_destroy(pyoneer);
    if (logger)
        logger_destroy(logger);
    if (SITE)
        site_destroy(SITE);

    return rc;
}
