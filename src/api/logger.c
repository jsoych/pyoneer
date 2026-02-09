#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "logger.h"

typedef struct Logger {
    int level;
    char* name;
    pthread_mutex_t lock;
} Logger;

static const char* level_str(int level) {
    switch (level) {
        case LOGGER_DEBUG:  return "DEBUG";
        case LOGGER_INFO:   return "INFO";
        case LOGGER_WARN:   return "WARN";
        case LOGGER_ERROR:  return "ERROR";
    }
    return NULL;
}

static void timestamp_rfc3339_ms(char* out, size_t out_sz) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_utc;
    gmtime_r(&ts.tv_sec, &tm_utc);

    int ms = (int)(ts.tv_nsec / 1000000L);
    snprintf(out, out_sz,
             "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_utc.tm_year + 1900,
             tm_utc.tm_mon + 1,
             tm_utc.tm_mday,
             tm_utc.tm_hour,
             tm_utc.tm_min,
             tm_utc.tm_sec,
             ms);
}

/* logger_create: Creates a new logger and returns it. */
Logger* logger_create(int level, const char* name) {
    Logger* logger = malloc(sizeof(Logger));
    if (!logger) {
        perror("logger_create: malloc");
        return NULL;
    }

    logger->level = level;
    logger->name = strdup(name);

    int err = pthread_mutex_init(&logger->lock, NULL);
    if (err != 0) {
        fprintf(stderr, "logger_create: pthread_mutex_init: %s", strerror(err));
        free(logger->name);
        free(logger);
        return NULL;
    }

    return logger;
}

/* logger_destroy: Destroys the logger and its resources. */
void logger_destroy(Logger* logger) {
    if (!logger) return;
    free(logger->name);
    pthread_mutex_destroy(&logger->lock);
    free(logger);
}

int logger_get_level(Logger* logger) { return logger->level; }

int logger_log(Logger* logger, int level, const char* fmt, ...) {
    if (level < logger->level) return 0;

    char ts[64];
    timestamp_rfc3339_ms(ts, sizeof(ts));

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    char line[1400];
    int n = snprintf(line, sizeof(line), "[%s] %s %s %s\n",
        ts, level_str(level), logger->name, msg);
    FILE* stream = (level >= LOGGER_WARN) ? stderr : stdout;

    pthread_mutex_lock(&logger->lock);
    fwrite(line, 1, (size_t)((n > 0) ? n : (int)strlen(line)), stream);
    if (level >= LOGGER_WARN) fflush(stream);
    pthread_mutex_unlock(&logger->lock);

    return 0;
}
