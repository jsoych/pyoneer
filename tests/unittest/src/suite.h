#ifndef SUITE_H
#define SUITE_H

// suite.h - Opaque runnable test object.

#include "test.h"
#include "unittest.h"

typedef struct
{
    int nok;
    int nfail;
    int nerror;
    int nsyserr;

    long long time_ms;

    int size;
    test_result_t *test_results;
} suite_result_t;

typedef struct Suite Suite;

int suite_result_init(suite_result_t *summary, int size);

void suite_result_free(suite_result_t *summary);

Suite *suite_create(const char *name);

void suite_destroy(Suite *suite);

const char *suite_get_name(Suite *suite);

int suite_add(Suite *suite, Test *test);

int suite_run(const Suite *suite, suite_result_t *result,
              const unittest_opts_t *run_opts);

void suite_print_result(const Suite *suite, const suite_result_t *result,
                        const unittest_verbosity_t level);

#endif
