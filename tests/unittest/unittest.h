#ifndef UNITTEST_H
#define UNITTEST_H

#include "unittest_core.h"

typedef struct Unittest Unittest;

typedef struct UnittestResult UnittestResult;

typedef void (*unittest_fn)(UnittestResult*);

void unittest_result_destroy(UnittestResult* result);

void unittest_result_ok(UnittestResult* result);

void unittest_result_fail(UnittestResult* result, const char* fmt, ...);

void unittest_result_err(UnittestResult* result, const char* fmt, ...);

Unittest* unittest_create_test(const char* name, unittest_fn fn);

Unittest* unittest_create_suite(const char* name);

void unittest_destroy(Unittest* ut);

int unittest_add(Unittest* suite, Unittest* test);

int unittest_run(const Unittest* ut, UnittestResult* result,
    const unittest_opts_t* opts);

// Prints the Unittest and its UnittestResult at the given verbosity level.
//
// Preconditions:
// - ut and result are not NULL
// - ut and result types match (both tests or both suites)
void unittest_print_result(const Unittest* ut, const UnittestResult* result,
    unittest_verbosity_t level);

#endif
