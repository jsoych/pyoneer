// logger.h — Thread-safe structured logging.
//
// Logger provides a simple thread-safe logging interface for writing messages
// to stdout/stderr (implementation-defined). Logging is serialized internally.
//
// Usage:
// - Use logger_info/logger_warn/etc for plain messages.
// - Use logger_infof/logger_warnf/etc for printf-style formatting.
//
// Thread safety:
// - Logger is internally synchronized.
// - All public functions and macros are safe to call concurrently.
//
// Ownership:
// - logger_create() returns a new Logger instance owned by the caller.
// - logger_destroy() frees the Logger and its resources.
//
// Notes:
// - logger_log() is printf-style and expects a valid format string.

#ifndef LOGGER_H
#define LOGGER_H

// Log levels.
enum {
    LOGGER_DEBUG = 0,
    LOGGER_INFO,
    LOGGER_WARN,
    LOGGER_ERROR
};

typedef struct Logger Logger;

// logger_create allocates a new Logger instance with the given minimum level.
// Messages below `level` may be suppressed.
// Returns a new Logger on success; NULL on error.
Logger* logger_create(int level, const char* name);

// logger_destroy flushes and frees a Logger.
// Safe to call with NULL.
void logger_destroy(Logger* logger);

// logger_get_level gets the Logger level and returns it.
int logger_get_level(Logger* logger);

// logger_log writes a formatted message at the given level.
// Returns 0 on success; -1 on failure.
//
// Preconditions:
// - logger must not be NULL.
// - fmt must be a valid printf format string.
int logger_log(Logger* logger, int level, const char* fmt, ...);

/* Plain message macros (no formatting). */
#define logger_debug(logger, msg) \
    logger_log((logger), LOGGER_DEBUG, "%s", (msg))

#define logger_info(logger, msg) \
    logger_log((logger), LOGGER_INFO, "%s", (msg))

#define logger_warn(logger, msg) \
    logger_log((logger), LOGGER_WARN, "%s", (msg))

#define logger_error(logger, msg) \
    logger_log((logger), LOGGER_ERROR, "%s", (msg))

/* Formatting macros (printf-style).
 *
 * Note: These macros require at least one variadic argument.
 * Use logger_* (non-f) variants for plain messages.
 */
#define logger_debugf(logger, fmt, ...) \
    logger_log((logger), LOGGER_DEBUG, (fmt), __VA_ARGS__)

#define logger_infof(logger, fmt, ...) \
    logger_log((logger), LOGGER_INFO, (fmt), __VA_ARGS__)

#define logger_warnf(logger, fmt, ...) \
    logger_log((logger), LOGGER_WARN, (fmt), __VA_ARGS__)

#define logger_errorf(logger, fmt, ...) \
    logger_log((logger), LOGGER_ERROR, (fmt), __VA_ARGS__)

#endif
