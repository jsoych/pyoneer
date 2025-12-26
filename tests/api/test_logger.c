#include <stdio.h>
#include <stdlib.h>

#include "test_api.h"

static result_t test_case_create(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_INFO, FORMAT);
    if (!logger) return UNITTEST_FAILURE;
    if (logger->level != LOGGER_INFO) return UNITTEST_FAILURE;
    logger_destroy(logger);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    logger_destroy(NULL);
    Logger* logger = logger_create(LOGGER_INFO, FORMAT);
    logger_destroy(logger);
    return UNITTEST_SUCCESS;
}

static result_t test_case_info(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_INFO, FORMAT);
    logger_info(logger, "let's go!!!");
    logger_destroy(logger);
    // Return error to trick unit test to print out the log
    return UNITTEST_ERROR;
}

Unittest* test_logger_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "logger_create", test_case_create, CASE_NONE, NULL);
    unittest_add(ut, "logger_destroy", test_case_destroy, CASE_NONE, NULL);
    unittest_add(ut, "logger_info", test_case_info, CASE_NONE, NULL);

    return ut;
}
