#include <stdio.h>
#include <varargs.h>

#include "test.h"
#include "suite.h"
#include "unittest.h"

enum {
    UNITTEST_TEST,
    UNITTEST_SUITE
};

struct Unittest {
    int type;
    union {
        Test* test;
        Suite* suite;
    } as;
};

struct UnittestResult {
    int type;
    union {
        test_result_t test;
        suite_result_t suite;
    } as;
};

static int check_type(int type) {
    return (type == UNITTEST_TEST || type == UNITTEST_SUITE);
}

static UnittestResult* unittest_result_create(int type) {
    if (!check_type(type)) return NULL;
    UnittestResult* r = malloc(sizeof(UnittestResult));
    if (!r) return NULL;
    r->type = type;
    return r;
}

void unittest_result_destroy(UnittestResult* result) {
    if (!result) return;
    switch (result->type) {
        case UNITTEST_TEST:
            test_result_free(&result->as.test);
            break;
        case UNITTEST_SUITE:
            suite_result_free(&result->as.suite);
            break;
    }
    free(result);
}

static void test_result_ok(test_result_t* result) {
    result->status = TEST_OK;
}

void unittest_result_ok(UnittestResult* result) {
    if (!result) return;
    if (result->type != UNITTEST_TEST) return;
    test_result_ok(&result->as.test);
}

static char* vformat(const char* fmt, va_list ap) {
    if (!fmt) return NULL;
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (len < 0) {
        return NULL;
    }
    char* buf = malloc((len + 1)*sizeof(char));
    if (!buf) {
        return NULL;
    }
    vsnprintf(buf, len + 1, fmt, ap);
    return buf;
}

static void test_result_fail(test_result_t* result, const char* fmt, va_list ap) {
    if (!result) return;
    result->status = TEST_FAIL;
    if (result->msg) {
        free(result->msg);
        result->msg = NULL;
    }
    if (!fmt) return;
    result->msg = vformat(fmt, ap);
}

void unittest_result_fail(UnittestResult* result, const char* fmt, ...) {
    if (!result) return;
    if (result->type != UNITTEST_TEST) return;
    va_list ap;
    va_start(ap, fmt);
    test_result_fail(&result->as.test, fmt, ap);
    va_end(ap);
}

static void test_result_err(test_result_t* result, const char* fmt, va_list ap) {
    if (!result) return;
    result->status = TEST_ERROR;
    if (result->err) {
        free(result->err);
        result->err = NULL;
    }
    if (!fmt) return;
    result->err = vformat(fmt, ap);
}

void unittest_result_err(UnittestResult* result, const char* fmt, ...) {
    if (!result) return;
    if (result->type != UNITTEST_TEST) return;
    va_list ap;
    va_start(ap, fmt);
    test_result_fail(&result->as.test, fmt, ap);
    va_end(ap);
}

Unittest* unittest_create_test(const char* name, unittest_fn fn) {
    if (!name || !fn) return NULL;
    Unittest* ut = malloc(sizeof(Unittest));
    if (!ut) {
        perror("unittest_create_test: malloc");
        return NULL;
    }
    ut->type = UNITTEST_TEST;
    ut->as.test = test_create(name, fn);
    return ut;
}

Unittest* unittest_create_suite(const char* name) {
    if (!name) return;
    Unittest* ut = malloc(sizeof(Unittest));
    if (!ut) {
        perror("unittest_create_suite: malloc");
        return NULL;
    }
    ut->type = UNITTEST_SUITE;
    ut->as.suite = suite_create(name);
    return ut;
}

void unittest_destroy(Unittest* ut) {
    if (!ut) return;
    switch (ut->type) {
        case UNITTEST_TEST:
            test_destroy(ut->as.test);
            break;
        case UNITTEST_SUITE:
            suite_destroy(ut->as.suite);
            break;
    }
    free(ut);
}

int unittest_add(Unittest* suite, Unittest* test) {
    if (!suite || !test) return -1;
    if (suite->type != UNITTEST_SUITE || test->type != UNITTEST_TEST)
        return -1;
    return suite_add(&suite->as.suite, &test->as.test);
}

int unittest_run(const Unittest* ut, UnittestResult* result,
    const unittest_opts_t* opts) {
    if (!ut || !result) return -1;
    result = unittest_result_create(ut->type);
    if (!result) return -1;
    int rv;
    switch (ut->type) {
        case UNITTEST_TEST:
            rv = test_run(&ut->as.test, &result->as.test, opts);
        case UNITTEST_SUITE:
            rv = suite_run(&ut->as.suite, &result->as.suite, opts);
        default:
            rv = -1;
    }
    return rv;
}

void unittest_print_result(const Unittest* ut, const UnittestResult* result,
    unittest_verbosity_t level) {
    if (!ut || !result) return;
    if (ut->type != result->type) return;
    switch (ut->type) {
        case UNITTEST_TEST:
            test_print_result(ut->as.test, &result->as.test, level);
            return;
        case UNITTEST_SUITE:
            suite_print_result(ut->as.suite, &result->as.suite, level);
            return;
    }
    return;
}
