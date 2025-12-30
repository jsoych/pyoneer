#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>

enum {
    LOGGER_DEBUG = 0,
    LOGGER_INFO,
    LOGGER_WARN,
    LOGGER_ERROR
};

typedef struct _logger {
    int level;
    char* name;
    pthread_mutex_t lock;
} Logger;

Logger* logger_create(int level, const char* name);
void logger_destroy(Logger* logger);

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 3, 4)))
#endif
int logger_log(Logger* logger, int level, const char* fmt, ...);

/* Plain message macros (no formatting) */
#define logger_debug(logger, msg) \
    logger_log((logger), LOGGER_DEBUG, "%s", (msg))

#define logger_info(logger, msg) \
    logger_log((logger), LOGGER_INFO, "%s", (msg))

#define logger_warn(logger, msg) \
    logger_log((logger), LOGGER_WARN, "%s", (msg))

#define logger_error(logger, msg) \
    logger_log((logger), LOGGER_ERROR, "%s", (msg))

/* Formatting macros (printf-style) */
#define logger_debugf(logger, fmt, ...) \
    logger_log((logger), LOGGER_DEBUG, (fmt), __VA_ARGS__)

#define logger_infof(logger, fmt, ...) \
    logger_log((logger), LOGGER_INFO, (fmt), __VA_ARGS__)

#define logger_warnf(logger, fmt, ...) \
    logger_log((logger), LOGGER_WARN, (fmt), __VA_ARGS__)

#define logger_errorf(logger, fmt, ...) \
    logger_log((logger), LOGGER_ERROR, (fmt), __VA_ARGS__)

#endif
