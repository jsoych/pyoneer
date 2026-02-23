#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "suite.h"

#define CAPACITY 8

struct Suite {
    char* name;
    int size;
    int capacity;
    Test** tests;
};

int suite_result_init(suite_result_t* result, int size) {
    result->nok = 0;
    result->nfail = 0;
    result->nerror = 0;
    result->nsyserr = 0;
    result->time_ms = 0;
    if ((result->size = size) == 0) {
        result->test_results = NULL;
        return 0;
    }
    result->test_results = malloc(size*sizeof(test_result_t));
    if (!result->test_results) {
        perror("suite_result_init: malloc");
        return -1;
    }
    return 0;
}

void suite_result_free(suite_result_t* result) {
    if (result->test_results) free(result->test_results);
    suite_result_init(result, 0);
}

Suite* suite_create(const char* name) {
    Suite* suite = malloc(sizeof(Suite));
    if (!suite) {
        perror("suite_create: malloc");
        return NULL;
    }
    suite->name = strdup(name);
    suite->size = 0;
    suite->capacity = CAPACITY;
    suite->tests = malloc(CAPACITY*sizeof(Test*));
    if (!suite->tests) {
        perror("suite_create: malloc");
        free(suite);
        return NULL;
    }
    for (int i = 0; i < CAPACITY; i++) {
        suite->tests = NULL;
    }
    return suite;
}

void suite_destroy(Suite* suite) {
    if (!suite) return;
    free(suite->name);
    for (int i = 0; i < suite->size; i++) {
        test_case_destroy(suite->tests[i]);
    }
    free(suite);
}

const char* suite_get_name(Suite* suite) { return suite->name; }

int suite_add(Suite* suite, Test* test) {
    if (suite->size == suite->capacity) {
        int capacity = 2*suite->capacity;
        Test** tests = realloc(suite->tests, capacity*sizeof(Test*));
        if (!tests) {
            perror("suite_add: malloc");
            return -1;
        }
        for (int i = suite->size; i < capacity; i++) {
            tests[i] = NULL;
        }
        suite->capacity = capacity;
        suite->tests = tests;
    }
    suite->tests[suite->size++] = test;
    return 0;
}

int suite_run(const Suite* suite, suite_result_t* result,
    const unittest_opts_t* run_opts) {
    if (!suite || !result) return -1;
    if (suite_result_init(result, suite->size) == -1) return -1;

    int rv = 0;
    for (int i = 0; i < suite->size; i++) {
        if (test_run(suite->tests[i], &result->test_results[i], run_opts) == -1) {
            rv = -1;
            continue;
        }
        result->time_ms += result->test_results[i].time_ms;
    }
    return rv;
}

static void print_summary(const suite_result_t* result) {
    printf("Ran %d tests in %.3fs\n\n", result->size, (double)(result->time_ms/1000));

    if (result->nfail > 0 || result->nerror > 0 || result->nsyserr > 0) {
        printf("FAILED (");
        if (result->nfail) printf("failures=%d", result->nfail);
        if (result->nfail && result->nerror) printf(", ");
        if (result->nerror) printf("errors=%d", result->nerror);
        if ((result->nfail > 0 || result->nerror > 0) && result->nsyserr) printf(", ");
        if (result->nsyserr) printf("syserrs=%d", result->nsyserr);
        printf(")\n");
        return;
    }

    printf("OK\n");
}

void suite_print_result(const Suite* suite, const suite_result_t* result,
    const unittest_verbosity_t level) {
    if (!suite || !result) return;
    
    if (level == UNITTEST_QUIET) {
        printf(HLINE);
        print_summary(result);
        return;
    }

    printf(DLINE);
    print_summary(result);
    for (int i = 0; i < result->size; i++) {
        if (result->test_results[i].status == TEST_OK) continue;
        test_print_result(suite->tests[i], &result->test_results[i], level);
    }
}
