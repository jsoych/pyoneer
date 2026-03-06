#ifndef UNITTEST_H
#define UNITTEST_H

#include "unittest_core.h"

typedef struct Unittest Unittest;

/* Destroys the UnittestResult and all of its resources. */
void unittest_result_destroy(UnittestResult *result);

/*
 * Marks the UnittestResult as ok and sets its status to TEST_OK.
 *
 * Preconditions:
 * - UnittestResult is a test result
 */
void unittest_result_ok(UnittestResult *result);

/*
 * Marks the UnittestResult as fail and sets its status to TEST_FAIL. If
 * fmt is not NULL, then a formatted message is recorded with the test
 * result.
 *
 * Preconditions:
 * - UnittestResult is a test result */
void unittest_result_fail(UnittestResult *result, const char *fmt, ...);

/*
 * Marks the UnittestResult as error and sets its status to TEST_ERROR. If
 * fmt is not NULL, then a formatted message is recorded with the test
 * result.
 *
 * Precoditions:
 * - UnittestResult is a test result */
void unittest_result_err(UnittestResult *result, const char *fmt, ...);

/*
 * Creates a new test with the given name and unittest function. Returns
 * a Unittest on success, and NULL on failure.
 */
Unittest *unittest_create_test(const char *name, unittest_fn fn);

/*
 * Creates a new suite with the given name. Returns a Unittest on success,
 * and NULL on failure.
 */
Unittest *unittest_create_suite(const char *name);

/* Destroys the Unittest and all of its resources. */
void unittest_destroy(Unittest *ut);

/* Returns the name of the Unittest. */
const char *unittest_get_name(Unittest *ut);

/* Adds a new test to the suite. Returns 0 on success, and -1 on failure. */
int unittest_add(Unittest *suite, Unittest *test);

/*
 * Runs the Unittest with the given run options. Returns a UnittestResult and
 * sets the status to 0 on success or -1 on failure. Otherwise, returns NULL
 * sets the status to -1.
 *
 * Preconditions:
 * - ut is not NULL
 */
UnittestResult *unittest_run(const Unittest *ut,
                             const unittest_opts_t *run_opts, int *status);

/*
 * Prints the Unittest and its UnittestResult at the given verbosity level.
 *
 * Preconditions:
 * - ut and result are not NULL
 * - ut and result types match (both tests or both suites)
 */
void unittest_print_result(const Unittest *ut, const UnittestResult *result,
                           unittest_verbosity_t level);

#endif
