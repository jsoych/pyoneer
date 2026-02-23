#ifndef UNITTEST_CORE_H
#define UNITTEST_CORE_H

#define DLINE "\n======================================================================"
#define HLINE "\n----------------------------------------------------------------------"

typedef enum {
    UNITTEST_QUIET = 0,
    UNITTEST_DEFAULT = 1,
    UNITTEST_VERBOSE = 2
} unittest_verbosity_t;

typedef struct {
    int timeout_ms;
    unittest_verbosity_t level;
} unittest_opts_t;

unittest_opts_t unittest_opts_default();

#endif