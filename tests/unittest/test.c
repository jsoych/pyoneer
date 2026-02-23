#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "test.h"

#define BUFSIZE 256
#define READ 0
#define WRITE 1

struct Test {
    char* name;
    test_fn fn;
};

// Ring buffer
struct ring_buffer {
    int start;
    int size;
    char buf[BUFSIZE];
};

// Wire format: [u8 status][u32 msg_len][msg bytes...]
// msg_len does NOT include a NUL terminator.
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint32_t time_ms;
    uint32_t msg_len;
} test_result_hdr_t;

static void pipe_close(int p[2]) {
    if (p[READ] != -1) close(p[READ]);
    if (p[WRITE] != -1) close(p[WRITE]);
}

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

// ring_buf_init: Initializes ring buffer.
static void ring_buf_init(struct ring_buffer* rb) {
    rb->start = 0;
    rb->size = 0;
    memset(rb->buf, 0, BUFSIZE);
}

// ring_buf_write: Writes to ring buffer.
static void ring_buf_write(struct ring_buffer* rb, const char* data, size_t n) {
    for (size_t i = 0; i < n; i++) {
        size_t end = (rb->start + rb->size) % BUFSIZE;
        rb->buf[end] = data[i];

        if (rb->size < BUFSIZE) {
            rb->size++;
        } else {
            // Buffer full, overwrite oldest
            rb->start = (rb->start + 1) % BUFSIZE;
        }
    }
}

// ring_buf_read: Reads from ring buffer.
static void ring_buf_read(struct ring_buffer* rb, char* out) {
    for (int i = 0; i < rb->size; i++) {
        out[i] = rb->buf[(rb->start + i) % BUFSIZE];
    }
    out[rb->size] = '\0';
}

// ring_buf_to_str: Converts the buffer contents into a string and returns it.
static char* ring_buf_to_str(struct ring_buffer* rb) {
    if (rb->size == 0) return NULL;
    char* s = malloc((rb->size + 1)*sizeof(char));
    if (!s) {
        perror("ring_buf_to_str: malloc");
        return NULL;
    }
    ring_buf_read(rb, s);
    s[rb->size] = '\0';
    return s;
}

// Drain fd into ring buffer until EAGAIN/EWOULDBLOCK or EOF.
// Returns 0 on success, -1 on fatal read error.
static int ring_buf_drain_nb(struct ring_buffer* rb, int fd) {
    char buf[BUFSIZE];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            ring_buf_write(rb, buf, (size_t)n);
            continue;
        }
        if (n == 0) {
            return 0; // EOF
        }
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; // drained for now
        return -1;
    }
}

static int write_all(int fd, const void* buf, size_t n) {
    const uint8_t* p = (const uint8_t*)buf;
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)w;
        n -= (size_t)w;
    }
    return 0;
}

static int read_all(int fd, void* buf, size_t n) {
    uint8_t* p = (uint8_t*)buf;
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r == 0) return -1;
        // EOF before full read
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)r;
        n -= (size_t)r;
    }
    return 0;
}

// Send result to fd. Returns 0 on success, -1 on write_all error.
static int send_test_result(int fd, const test_result_t* r) {
    if (!r) return -1;

    const char* msg = r->msg ? r->msg : "";
    uint32_t len = (uint32_t)strlen(msg);

    test_result_hdr_t hdr;
    hdr.status = (uint8_t)r->status;
    hdr.time_ms = (uint32_t)r->time_ms;
    hdr.msg_len = len;

    if (write_all(fd, &hdr, sizeof(hdr)) == -1) return -1;
    if (len > 0 && write_all(fd, msg, len) == -1) return -1;

    return 0;
}

// Receive result from fd. Return 0 on sucess, -1 on read_all error.
static int recv_test_result(int fd, test_result_t* r) {
    if (!r) return -1;

    test_result_hdr_t hdr;
    if (read_all(fd, &hdr, sizeof(hdr)) == -1) return -1;

    // Check msg_len
    const uint32_t MAX_MSG = 4096;
    if (hdr.msg_len > MAX_MSG) return -1;

    r->status = (test_status_t)hdr.status;
    r->time_ms = (long long)hdr.time_ms;

    if (hdr.msg_len == 0) {
        r->msg = NULL;
        return 0;
    }

    r->msg = malloc((hdr.msg_len + 1)*sizeof(char));
    if (!r->msg) return -1;

    if (read_all(fd, r->msg, hdr.msg_len) == -1) {
        free(r->msg);
        r->msg = NULL;
        return -1;
    }
    r->msg[hdr.msg_len] = '\0';
    return 0;
}

static const char test_status_to_char(test_status_t status) {
    switch (status) {
        case TEST_OK:       return '.';
        case TEST_FAIL:     return 'F';
        case TEST_ERROR:    return 'E';
        case TEST_SYSERR:   return 'S';
        default:            return 'U';
    }
}

const char* test_status_to_str(test_status_t status) {
    switch (status) {
        case TEST_OK:       return "ok";
        case TEST_FAIL:     return "FAIL";
        case TEST_ERROR:    return "ERROR";
        case TEST_SYSERR:   return "SYSERR";
        default:            return "UNKNOWN";
    }
}

int test_result_init(test_result_t* result) {
    result->status = TEST_ERROR;
    result->msg = NULL;
    result->out = NULL;
    result->err = NULL;
    result->time_ms = 0;
    result->sys_errno = 0;
    result->exit_code = 0xFF;
    result->signo = 0;
    return 0;
}

void test_result_free(test_result_t* result) {
    if (result->msg) free(result->msg);
    if (result->out) free(result->out);
    if (result->err) free(result->err);
    test_result_init(result);
}

Test* test_create(const char* name, test_fn fn) {
    Test* test = malloc(sizeof(Test));
    if (!test) {
        perror("test_create: malloc");
        return NULL;
    }
    test->name = strdup(name);
    test->fn = fn;
    return test;
}

void test_destroy(Test* test) {
    if (!test) return;
    free(test->name);
    free(test);
}

const char* test_get_name(const Test* test) {
    return test->name ? test->name : "";
}

static int status_is_valid(test_status_t s) {
    return s == TEST_OK || s == TEST_FAIL || s == TEST_ERROR;
}

static void print_name(const char* name, unittest_verbosity_t l) {
    if (l == UNITTEST_VERBOSE) {
        printf("%s ... ", name);
    }
}

static void print_status(test_status_t s, unittest_verbosity_t l) {
    if (l == UNITTEST_VERBOSE) {
        printf("%s\n", test_status_to_str(s));
        return;
    }
    putchar(test_status_to_char(s));
}

int test_run(const Test* test, test_result_t* result,
    const unittest_opts_t* run_opts) {
    if (!test || !test->fn || !result) return -1;

    if (test_result_init(result) == -1) return -1;
    unittest_opts_t opts = (run_opts) ? *run_opts : unittest_opts_default();
    print_name(test->name, opts.level);

    int rc = -1;

    int stdout_pipe[2] = { -1, -1 };
    int stderr_pipe[2] = { -1, -1 };
    int result_pipe[2] = { -1, -1 };

    pid_t child_id = -1;
    int status = 0;
    int child_done = 0;
    int want_kill = 0;

    // --- create pipes
    if (pipe(stdout_pipe) == -1) { result->sys_errno = errno; goto syserr; }
    if (pipe(stderr_pipe) == -1) { result->sys_errno = errno; goto syserr; }
    if (pipe(result_pipe) == -1) { result->sys_errno = errno; goto syserr; }

    child_id = fork();
    if (child_id == -1) { result->sys_errno = errno; goto syserr; }

    if (child_id == 0) {
        // child
        close(stdout_pipe[READ]);
        close(stderr_pipe[READ]);
        close(result_pipe[READ]);

        (void)dup2(stdout_pipe[WRITE], STDOUT_FILENO);
        (void)dup2(stderr_pipe[WRITE], STDERR_FILENO);

        close(stdout_pipe[WRITE]);
        close(stderr_pipe[WRITE]);

        test_result_t r;
        (void)test_result_init(&r);
        r.time_ms = 0 - now_ms();
        test->fn(&r);
        r.time_ms += now_ms();

        (void)send_test_result(result_pipe[WRITE], &r);
        close(result_pipe[WRITE]);

        _exit((int)r.status & 0xFF);
    }

    // parent: close write ends
    close(stdout_pipe[WRITE]); stdout_pipe[WRITE] = -1;
    close(stderr_pipe[WRITE]); stderr_pipe[WRITE] = -1;
    close(result_pipe[WRITE]); result_pipe[WRITE] = -1;

    if (set_nonblocking(stdout_pipe[READ]) == -1 ||
        set_nonblocking(stderr_pipe[READ]) == -1) {
        result->sys_errno = errno;
        want_kill = 1;
        goto syserr;
    }

    struct ring_buffer stdout_rb, stderr_rb;
    ring_buf_init(&stdout_rb);
    ring_buf_init(&stderr_rb);

    int out_open = 1, err_open = 1;
    long long deadline = (opts.timeout_ms >= 0)
        ? (now_ms() + (long long)opts.timeout_ms)
        : 0;

    while ((!child_done) || out_open || err_open) {
        int remaining = -1;
        if (opts.timeout_ms >= 0) {
            remaining = (int)(deadline - now_ms());
            if (remaining <= 0) {
                // timeout is a TEST_ERROR, not SYSERR
                want_kill = 1;
                kill(child_id, SIGKILL);
                if (!result->msg) result->msg = strdup("timeout");
                result->status = TEST_ERROR;
                remaining = 0;
            }
        }

        struct pollfd pfds[2];
        int nfds = 0;
        int out_idx = -1, err_idx = -1;

        if (out_open) {
            out_idx = nfds;
            pfds[nfds++] = (struct pollfd){
                .fd = stdout_pipe[READ],
                .events = POLLIN
            };
        }
        if (err_open) {
            err_idx = nfds;
            pfds[nfds++] = (struct pollfd){
                .fd = stderr_pipe[READ],
                .events = POLLIN
            };
        }

        int prc = (nfds > 0) ? poll(pfds, nfds, remaining) : 0;
        if (prc == -1) {
            if (errno == EINTR) continue;
            result->sys_errno = errno;
            want_kill = 1;
            goto syserr;
        }

        if (out_open && out_idx != -1) {
            short re = pfds[out_idx].revents;
            if (re & POLLIN)
                (void)ring_buf_drain_nb(&stdout_rb, stdout_pipe[READ]);
            if (re & (POLLHUP | POLLERR)) {
                (void)ring_buf_drain_nb(&stdout_rb, stdout_pipe[READ]);
                close(stdout_pipe[READ]); stdout_pipe[READ] = -1;
                out_open = 0;
            }
            if (re & POLLNVAL) out_open = 0;
        }

        if (err_open && err_idx != -1) {
            short re = pfds[err_idx].revents;
            if (re & POLLIN)
                (void)ring_buf_drain_nb(&stderr_rb, stderr_pipe[READ]);
            if (re & (POLLHUP | POLLERR)) {
                (void)ring_buf_drain_nb(&stderr_rb, stderr_pipe[READ]);
                close(stderr_pipe[READ]); stderr_pipe[READ] = -1;
                err_open = 0;
            }
            if (re & POLLNVAL) err_open = 0;
        }

        // Reap child if finished
        if (!child_done) {
            pid_t w = waitpid(child_id, &status, WNOHANG);
            if (w == -1) {
                result->sys_errno = errno;
                want_kill = 1;
                goto syserr;
            }
            if (w == child_id) child_done = 1;
        }
    }

    // Allocate captured output
    result->out = ring_buf_to_str(&stdout_rb);
    result->err = ring_buf_to_str(&stderr_rb);

    // Read structured result
    int have_wire = 1;
    if (recv_test_result(result_pipe[READ], result) == -1) {
        have_wire = 0;
    }
    if (result_pipe[READ] != -1) {
        close(result_pipe[READ]);
        result_pipe[READ] = -1;
    }

    // Final status from wait status
    if (WIFSIGNALED(status)) {
        result->signo = WTERMSIG(status);
        result->status = TEST_ERROR;
        if (!result->msg) result->msg = strdup("terminated by signal");
    } else if (WIFEXITED(status)) {
        result->exit_code = WEXITSTATUS(status);
        if (!have_wire || !status_is_valid(result->status)) {
            result->status = TEST_ERROR;
            if (!result->msg) result->msg = strdup("no result from test");
        }
    } else {
        result->sys_errno = 0;
        goto syserr;
    }

    print_status(result->status, opts.level);
    rc = 0;
    goto cleanup;

syserr:
    result->status = TEST_SYSERR;
    if (result->sys_errno == 0) result->sys_errno = errno;
    print_status(result->status, opts.level);
    rc = -1;

cleanup:
    if (child_id > 0 && want_kill) {
        kill(child_id, SIGKILL);
        (void)waitpid(child_id, NULL, 0);
    }

    pipe_close(stdout_pipe);
    pipe_close(stdout_pipe);
    pipe_close(result_pipe);

    return rc;
}

void test_print_result(const Test* test, const test_result_t* result,
    const unittest_verbosity_t level) {
    if (!test || !result) return;
    if (level == UNITTEST_QUIET) return;
    printf(DLINE);
    printf("\n%s: %s",
        test_status_to_str(result->status),
        test_get_name(test)
    );
    if (result->msg) {
        printf(HLINE);
        printf("\n%s", result->msg);
    }
    if (result->out) {
        printf(HLINE);
        printf("\n%s", result->out);
    }
    if (result->err) {
        printf(HLINE);
        printf("\n%s", result->err);
    }
    if (level == UNITTEST_VERBOSE)
        printf("\nRan test in %.03fs", (double)result->time_ms / 1000.0f);
    
    if (level == UNITTEST_VERBOSE && result->status == TEST_SYSERR) {
        if (result->exit_code > 0 || result->sys_errno > 0 || result->signo > 0)
            printf(HLINE);
        if (result->exit_code)
            printf("\nexit code: %02x", result->exit_code);
        if (result->sys_errno)
            printf("\nsystem error: %s (%d)", strerror(result->sys_errno), result->sys_errno);
        if (result->signo)
            printf("\nsignal number: %s (%d)", strsignal(result->signo), result->signo);
    }
}

#undef READ
#undef WRITE
