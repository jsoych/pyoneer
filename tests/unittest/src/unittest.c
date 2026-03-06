#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "suite.h"
#include "unittest.h"

enum
{
    UNITTEST_TEST,
    UNITTEST_SUITE
};

struct Unittest
{
    union
    {
        Test *test;
        Suite *suite;
    } as;
    int kind;
};

struct UnittestResult
{
    union
    {
        test_result_t test;
        suite_result_t suite;
    } as;
    int kind;
};

static int check_kind(int kind)
{
    return (kind == UNITTEST_TEST || kind == UNITTEST_SUITE);
}

static UnittestResult *unittest_result_create(int kind)
{
    if (!check_kind(kind))
    {
        printf("DEBUG: error: unexpected kind (%d)\n", kind);
        return NULL;
    }
    UnittestResult *r = malloc(sizeof(UnittestResult));
    if (!r)
    {
        printf("DEBUG: error: failed to allocate space\n");
        return NULL;
    }
    r->kind = kind;
    return r;
}

void unittest_result_destroy(UnittestResult *result)
{
    if (!result)
        return;
    switch (result->kind)
    {
    case UNITTEST_TEST:
        test_result_free(&result->as.test);
        break;
    case UNITTEST_SUITE:
        suite_result_free(&result->as.suite);
        break;
    }
    free(result);
}

void unittest_result_ok(UnittestResult *result)
{
    if (!result)
        return;
    result->as.test.status = TEST_OK;
}

static char *vformat(const char *fmt, va_list ap)
{
    if (!fmt)
        return NULL;
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (len < 0)
        return NULL;
    char *buf = malloc((len + 1) * sizeof(char));
    if (!buf)
        return NULL;
    vsnprintf(buf, len + 1, fmt, ap);
    return buf;
}

void unittest_result_fail(UnittestResult *result, const char *fmt, ...)
{
    if (!result)
        return;
    result->as.test.status = TEST_FAIL;
    if (result->as.test.msg)
    {
        free(result->as.test.msg);
        result->as.test.msg = NULL;
    }
    if (!fmt)
        return;
    va_list ap;
    va_start(ap, fmt);
    result->as.test.msg = vformat(fmt, ap);
    va_end(ap);
}

void unittest_result_err(UnittestResult *result, const char *fmt, ...)
{
    if (!result)
        return;
    result->as.test.status = TEST_ERROR;
    if (result->as.test.msg)
    {
        free(result->as.test.msg);
        result->as.test.msg = NULL;
    }
    if (!fmt)
        return;
    va_list ap;
    va_start(ap, fmt);
    result->as.test.msg = vformat(fmt, ap);
    va_end(ap);
}

Unittest *unittest_create_test(const char *name, unittest_fn fn)
{
    if (!name || !fn)
        return NULL;
    Unittest *ut = malloc(sizeof(Unittest));
    if (!ut)
    {
        perror("unittest_create_test: malloc");
        return NULL;
    }
    ut->as.test = test_create(name, fn);
    if (!ut->as.test)
    {
        fprintf(stderr, "%s: error: failed to create Test\n", __func__);
        free(ut);
        return NULL;
    }
    ut->kind = UNITTEST_TEST;
    return ut;
}

Unittest *unittest_create_suite(const char *name)
{
    if (!name)
        return NULL;
    Unittest *ut = malloc(sizeof(Unittest));
    if (!ut)
    {
        perror("unittest_create_suite: malloc");
        return NULL;
    }
    ut->as.suite = suite_create(name);
    if (!ut->as.suite)
    {
        fprintf(stderr, "%s: error: failed to create Suite\n", __func__);
        free(ut);
        return NULL;
    }
    ut->kind = UNITTEST_SUITE;
    return ut;
}

void unittest_destroy(Unittest *ut)
{
    if (!ut)
        return;
    switch (ut->kind)
    {
    case UNITTEST_TEST:
        test_destroy(ut->as.test);
        break;
    case UNITTEST_SUITE:
        suite_destroy(ut->as.suite);
        break;
    }
    free(ut);
}

const char *unittest_get_name(Unittest *ut)
{
    if (!ut)
        return "";
    switch (ut->kind)
    {
    case UNITTEST_TEST:
        return test_get_name(ut->as.test);
    case UNITTEST_SUITE:
        return suite_get_name(ut->as.suite);
    }
    return "";
}

int unittest_add(Unittest *suite, Unittest *test)
{
    if (!suite || !test)
        return -1;
    if (suite->kind != UNITTEST_SUITE || test->kind != UNITTEST_TEST)
        return -1;
    return suite_add(suite->as.suite, test->as.test);
}

UnittestResult *unittest_run(const Unittest *ut,
                             const unittest_opts_t *run_opts, int *status)
{
    if (!ut)
        return NULL;
    UnittestResult *res = unittest_result_create(ut->kind);
    if (!res)
    {
        if (status)
            *status = -1;
        return NULL;
    }
    int rv;
    switch (ut->kind)
    {
    case UNITTEST_TEST:
        rv = test_run(ut->as.test, &res->as.test, run_opts);
        break;
    case UNITTEST_SUITE:
        rv = suite_run(ut->as.suite, &res->as.suite, run_opts);
        break;
    default:
        rv = -1;
        break;
    }
    if (status)
        *status = rv;
    return res;
}

void unittest_print_result(const Unittest *ut, const UnittestResult *result,
                           unittest_verbosity_t level)
{
    if (!ut || !result)
        return;
    if (ut->kind != result->kind)
        return;
    switch (ut->kind)
    {
    case UNITTEST_TEST:
        test_print_result(ut->as.test, &result->as.test, level);
        return;
    case UNITTEST_SUITE:
        suite_print_result(ut->as.suite, &result->as.suite, level);
        return;
    }
    return;
}
