#ifndef TEST_H
#define TEST_H

#include "unittest_core.h"

// test.h — Opaque runnable test object.

// Test status codes
typedef enum {
    TEST_OK = 0,
    TEST_FAIL,
    TEST_ERROR,
    TEST_SYSERR
} test_status_t;

// Test results
typedef struct {
    test_status_t status;
    char* msg;
    char* out;
    char* err;
    long long time_ms;
    int sys_errno;
    int exit_code;
    int signo;
} test_result_t;

typedef struct Test Test;

typedef void (*test_fn) (test_result_t*);

const char* test_status_to_str(test_status_t status);

int test_result_init(test_result_t* result);

void test_result_free(test_result_t* result);

// test_create creates a new Test instance.
Test* test_create(const char* name, test_fn fn);

// test_destroy destroys the Test and releases its resources.
void test_destroy(Test* tc);

// test_get_name returns the name of the Test.
const char* test_get_name(const Test* test);

// test_run runs the Test and records its results. Returns a 0, if the
// test framework didn't fail and sets the result status, 
//
// Preconditions:
// - test is not NULL
// - test fn is not NULL
// - result is not NULL
//
// Postconditions:
// - result must be freed
int test_run(const Test* test, test_result_t* result,
    const unittest_opts_t* run_opts);

void test_print_result(const Test* test, const test_result_t* result,
    const unittest_verbosity_t level);

#endif
