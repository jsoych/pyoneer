#include <stdio.h>
#include <stdlib.h>

#include "test_pyoneers.h"

extern Site* SITE;

static result_t test_case_create(unittest_case* expected) {
    Pyoneer* pyoneer = pyoneer_create(PYONEER_WORKER, WORKER_ID);
    if (!pyoneer) return UNITTEST_FAILURE;
    if (pyoneer->role != PYONEER_WORKER ||
        pyoneer->as.worker == NULL) return UNITTEST_FAILURE;
    worker_destroy(pyoneer->as.worker);
    free(pyoneer);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    // Check NULL
    pyoneer_destroy(NULL);

    Pyoneer* pyoneer = pyoneer_create(PYONEER_WORKER, WORKER_ID);
    if (!pyoneer) return UNITTEST_ERROR;
    pyoneer_destroy(pyoneer);
    return UNITTEST_SUCCESS;
}

Unittest* test_pyoneer_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "pyoneer_create", test_case_create, CASE_NONE, NULL);

    unittest_add(ut, "pyoneer_destroy", test_case_destroy, CASE_NONE, NULL);

    return ut;
}