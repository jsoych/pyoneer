#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

/* logger_create: Creates a new logger and returns it. */
Logger* logger_create(int level, const char* format) {
    Logger* logger = malloc(sizeof(Logger));
    if (logger == NULL) {
        perror("logger_create: malloc");
        return NULL;
    }

    logger->level = level;
    logger->format = strdup(format);

    int err = pthread_mutex_init(&logger->lock, NULL);
    if (err != 0) {
        fprintf(stderr, "logger_create: pthread_mutex_init: %s", strerror(err));
        free(logger->format);
        free(logger);
        return NULL;
    }

    return logger;
}

/* logger_destroy: Destroys the logger and its resources. */
void logger_destroy(Logger* logger) {
    if (!logger) return;
    free(logger->format);
    pthread_mutex_destroy(&logger->lock);
    free(logger);
}

/* logger_stream_handler: Prints the log to the stdout. */
static void logger_stream_handler(Logger* logger, const char* message) {
    printf(logger->format, message);
}

/* logger_info: Logs info message and returns 0. */
int logger_info(Logger* logger, const char* message) {
    pthread_mutex_lock(&logger->lock);
    logger_stream_handler(logger, message);
    pthread_mutex_unlock(&logger->lock);
    return 0;
}

/* logger_debug: Logs debug message and returns 0. */
int logger_debug(Logger* logger, const char* message) {
    pthread_mutex_lock(&logger->lock);
    switch (logger->level) {
        case LOGGER_DEBUG:
            logger_stream_handler(logger, message);
        case LOGGER_INFO:
            break;
    }
    pthread_mutex_unlock(&logger->lock);
    return 0;
}
