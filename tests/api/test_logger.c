#include <stdio.h>
#include <stdlib.h>

#include "test_api.h"

static result_t test_case_create(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_INFO, NAME);
    if (!logger) return UNITTEST_FAILURE;
    if (logger_get_level(logger) != LOGGER_INFO) return UNITTEST_FAILURE;
    logger_destroy(logger);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    logger_destroy(NULL);
    Logger* logger = logger_create(LOGGER_INFO, NAME);
    logger_destroy(logger);
    return UNITTEST_SUCCESS;
}

static result_t test_case_log(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_INFO, NAME);
    logger_log(logger, LOGGER_INFO, "testing logger (%s)", "info");
    logger_destroy(logger);
    // Return error to trick unit test to print out the log
    return UNITTEST_ERROR;
}

Unittest* test_logger_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "logger_create", test_case_create, CASE_NONE, NULL);
    unittest_add(ut, "logger_destroy", test_case_destroy, CASE_NONE, NULL);
    unittest_add(ut, "logger_info", test_case_log, CASE_NONE, NULL);

    return ut;
}
