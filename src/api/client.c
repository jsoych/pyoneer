#include "client.h"

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "client.h"
#include "blueprint.h"
#include "json-builder.h"
#include "json-helpers.h"

/* If you have these error strings in server.c today, keep them centralized.
 * For now we replicate minimal ones here. */
static const char* const API_ERR_MSG[] = {
    [ERR_INTERNAL]      = "API: internal error",
    [ERR_SHUTTING_DOWN] = "API: shutting down",
    [ERR_CMD]           = "API: command error",
    [ERR_BLUEPRINT]     = "API: blueprint error",
    [ERR_SIGNAL]        = "API: signal error",
    [ERR_JSON_PARSE]    = "API: invalid JSON payload",
    [ERR_JSON_TYPE]     = "API: invalid JSON type",
    [ERR_JSON_MISSING]  = "API: missing JSON value",
    [ERR_TOKEN]         = "API: invalid token"
};

#define BUFLEN 4096

/* ---------- internal logging contexts (lowercase) ---------- */

typedef struct {
    uint64_t seq;
    const char* cmd;
    const char* user;
    uint64_t in_bytes;
    uint64_t out_bytes;
} req_ctx;

typedef struct {
    int conn_id;
    uint64_t seq;
    char peer[64];   // "203.0.113.42:51233" or "unix"
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t msgs;
} conn_ctx;

/* ---------- internal helpers ---------- */

/* Simple monotonic connection id generator (process-wide) */
static int next_conn_id(void) {
    static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    static int id = 1;
    pthread_mutex_lock(&m);
    int out = id++;
    pthread_mutex_unlock(&m);
    return out;
}

static void format_peer(int fd, int is_unix, char out[64]) {
    if (is_unix) {
        strncpy(out, "unix", 64);
        out[63] = '\0';
        return;
    }

    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    if (getpeername(fd, (struct sockaddr*)&ss, &slen) != 0) {
        strncpy(out, "unknown", 64);
        out[63] = '\0';
        return;
    }

    if (ss.ss_family == AF_INET) {
        struct sockaddr_in* a = (struct sockaddr_in*)&ss;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip));
        snprintf(out, 64, "%s:%u", ip, (unsigned)ntohs(a->sin_port));
        return;
    }

    if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6* a = (struct sockaddr_in6*)&ss;
        char ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &a->sin6_addr, ip, sizeof(ip));
        snprintf(out, 64, "[%s]:%u", ip, (unsigned)ntohs(a->sin6_port));
        return;
    }

    strncpy(out, "unknown", 64);
    out[63] = '\0';
}

/* returns 1 if json object has {"Error": "..."} */
static const char* resp_error_string(json_value* resp) {
    if (!resp || resp->type != json_object) return NULL;
    json_value* errv = json_object_get_value(resp, "Error");
    if (!errv || errv->type != json_string) return NULL;
    return errv->u.string.ptr;
}

/* ---------- public handler ---------- */

void client_handle_fd(Server* server, int client_fd, int listener_index) {
    (void)listener_index; // reserved for per-endpoint behavior later

    Logger* logger = server_logger(server);
    Pyoneer* pyoneer = server_pyoneer(server);
    const char* token = server_token(server);

    conn_ctx c = {0};
    c.conn_id = next_conn_id();
    c.seq = 0;
    c.bytes_in = 0;
    c.bytes_out = 0;
    c.msgs = 0;

    /* Determine whether this is a unix socket (best-effort) */
    int is_unix = 0;
    {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        if (getsockname(client_fd, (struct sockaddr*)&ss, &slen) == 0) {
            if (ss.ss_family == AF_UNIX) is_unix = 1;
        }
    }
    format_peer(client_fd, is_unix, c.peer);

    logger_infof(logger, "conn=%d accept peer=%s", c.conn_id, c.peer);

    json_settings settings = (json_settings){0};
    settings.value_extra = json_builder_extra;
    char jerr[json_error_max];

    char buf[BUFLEN + 1];
    size_t need = 0;

    for (;;) {
        int nbytes = recv(client_fd, buf, BUFLEN, 0);
        if (nbytes == 0) {
            logger_infof(logger,
                "conn=%d close peer=%s reason=EOF msgs=%" PRIu64 " bytes_in=%" PRIu64 "B bytes_out=%" PRIu64 "B",
                c.conn_id, c.peer, c.msgs, c.bytes_in, c.bytes_out
            );
            break;
        }
        if (nbytes < 0) {
            logger_warnf(logger,
                "conn=%d close peer=%s reason=RECV_ERR err=%s msgs=%" PRIu64 " bytes_in=%" PRIu64 "B bytes_out=%" PRIu64 "B",
                c.conn_id, c.peer, strerror(errno), c.msgs, c.bytes_in, c.bytes_out
            );
            break;
        }

        buf[nbytes] = '\0';

        req_ctx r = {0};
        r.seq = ++c.seq;
        r.in_bytes = (uint64_t)nbytes;
        c.msgs++;
        c.bytes_in += r.in_bytes;

        logger_debugf(logger,
            "conn=%d seq=%" PRIu64 " event=recv in=%" PRIu64 "B",
            c.conn_id, r.seq, r.in_bytes
        );

        /* Create response object */
        json_value* resp = json_object_new(0);
        if (!resp) {
            logger_errorf(logger,
                "conn=%d seq=%" PRIu64 " event=internal_err err=RESP_ALLOC_FAILED",
                c.conn_id, r.seq
            );
            // fatal for this connection
            break;
        }

        /* Parse request */
        json_value* req = json_parse_ex(&settings, buf, (size_t)nbytes, jerr);
        if (!req) {
            logger_warnf(logger,
                "conn=%d seq=%" PRIu64 " event=parse_err err=JSON_PARSE",
                c.conn_id, r.seq
            );
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_PARSE]);
            goto send_resp;
        }

        if (req->type != json_object) {
            logger_warnf(logger,
                "conn=%d seq=%" PRIu64 " event=parse_err err=JSON_NOT_OBJECT",
                c.conn_id, r.seq
            );
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_TYPE]);
            goto send_resp;
        }

        /* command */
        json_value* cmd = json_object_get_value(req, "command");
        if (!cmd || cmd->type != json_string) {
            logger_warnf(logger,
                "conn=%d seq=%" PRIu64 " event=parse_err err=CMD_MISSING_OR_BAD_TYPE",
                c.conn_id, r.seq
            );
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_MISSING]);
            goto send_resp;
        }
        r.cmd = cmd->u.string.ptr;

        logger_debugf(logger,
            "conn=%d seq=%" PRIu64 " event=parse_ok cmd=%s",
            c.conn_id, r.seq, r.cmd
        );

        /* token */
        json_value* tok = json_object_get_value(req, "token");
        if (!tok || tok->type != json_string) {
            logger_warnf(logger,
                "conn=%d seq=%" PRIu64 " event=auth_err err=TOKEN_MISSING_OR_BAD_TYPE cmd=%s",
                c.conn_id, r.seq, r.cmd
            );
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_JSON_MISSING]);
            goto send_resp;
        }

        if (token && strcmp(tok->u.string.ptr, token) != 0) {
            logger_warnf(logger,
                "conn=%d seq=%" PRIu64 " event=auth_err err=INVALID_TOKEN cmd=%s",
                c.conn_id, r.seq, r.cmd
            );
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_TOKEN]);
            goto send_resp;
        }

        r.user = "authed"; // later: map token -> user id
        logger_debugf(logger,
            "conn=%d seq=%" PRIu64 " event=auth_ok user=%s",
            c.conn_id, r.seq, r.user
        );

        /* Dispatch (your existing command logic) */
        if (strcmp(r.cmd, "run") == 0) {
            Blueprint* blueprint = pyoneer_blueprint_decode(pyoneer, req);
            if (!blueprint) {
                json_object_push_string(resp, "Error", API_ERR_MSG[ERR_BLUEPRINT]);
                goto send_resp;
            }
            json_value* status = pyoneer->run(pyoneer, blueprint);
            json_object_push(resp, "pyoneer", status);
        } else if (strcmp(r.cmd, "get_status") == 0) {
            json_value* status = pyoneer->get_status(pyoneer);
            json_object_push(resp, "pyoneer", status);
        } else if (strcmp(r.cmd, "assign") == 0) {
            Blueprint* blueprint = pyoneer_blueprint_decode(pyoneer, req);
            if (!blueprint) {
                json_object_push_string(resp, "Error", API_ERR_MSG[ERR_BLUEPRINT]);
                goto send_resp;
            }
            json_value* status = pyoneer->assign(pyoneer, blueprint);
            json_object_push(resp, "pyoneer", status);
        } else if (strcmp(r.cmd, "restart") == 0) {
            pyoneer->restart(pyoneer);
            // empty response {}
        } else if (strcmp(r.cmd, "start") == 0) {
            pyoneer->start(pyoneer);
        } else if (strcmp(r.cmd, "stop") == 0) {
            pyoneer->stop(pyoneer);
        } else {
            json_object_push_string(resp, "Error", API_ERR_MSG[ERR_CMD]);
        }

send_resp:
        /* Serialize response */
        if ((need = json_measure(resp)) > BUFLEN) {
            // keep connection alive but return a small error
            logger_errorf(logger,
                "conn=%d seq=%" PRIu64 " event=internal_err err=RESP_TOO_LARGE cmd=%s",
                c.conn_id, r.seq, r.cmd ? r.cmd : "?"
            );
            strncpy(buf, "{\"Error\":\"api failure\"}", BUFLEN);
            buf[BUFLEN] = '\0';
        } else {
            json_serialize(buf, resp);
        }

        r.out_bytes = (uint64_t)strlen(buf);
        c.bytes_out += r.out_bytes;

        const char* errstr = resp_error_string(resp);
        if (errstr) {
            logger_warnf(logger,
                "conn=%d peer=%s seq=%" PRIu64 " user=%s cmd=%s err=%s out=%" PRIu64 "B",
                c.conn_id, c.peer, r.seq,
                r.user ? r.user : "?",
                r.cmd ? r.cmd : "?",
                errstr,
                r.out_bytes
            );
        } else {
            logger_infof(logger,
                "conn=%d peer=%s seq=%" PRIu64 " user=%s cmd=%s ok out=%" PRIu64 "B",
                c.conn_id, c.peer, r.seq,
                r.user ? r.user : "?",
                r.cmd ? r.cmd : "?",
                r.out_bytes
            );
        }

        /* Send response */
        if (send(client_fd, buf, (size_t)r.out_bytes, 0) < 0) {
            logger_warnf(logger,
                "conn=%d close peer=%s reason=SEND_ERR err=%s msgs=%" PRIu64 " bytes_in=%" PRIu64 "B bytes_out=%" PRIu64 "B",
                c.conn_id, c.peer, strerror(errno), c.msgs, c.bytes_in, c.bytes_out
            );
            json_builder_free(resp);
            if (req) json_builder_free(req);
            break;
        }

        /* Cleanup */
        json_builder_free(resp);
        if (req) json_builder_free(req);
        break;
    }

    close(client_fd);
}
