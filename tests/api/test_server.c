#include <stdio.h>
#include <stdlib.h>

#include "test_api.h"

extern char* SOCKET_PATH;
extern char* TOKEN;
extern volatile sig_atomic_t sig_flag;

static result_t test_case_create(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_DEBUG, FORMAT);
    if (!logger) {
        return UNITTEST_ERROR;
    }
    Pyoneer* pyoneer = pyoneer_create(PYONEER_WORKER, WORKER_ID);
    if (!pyoneer) {
        logger_destroy(logger);
        return UNITTEST_ERROR;
    }
    Server* server = server_create(pyoneer, logger, SOCKET_PATH, TOKEN);
    if (!server) {
        return UNITTEST_FAILURE;
    }
    server_destroy(server);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    server_destroy(NULL);
    Logger* logger = logger_create(LOGGER_DEBUG, FORMAT);
    if (!logger) {
        return UNITTEST_ERROR;
    }
    Pyoneer* pyoneer = pyoneer_create(PYONEER_WORKER, WORKER_ID);
    if (!pyoneer) {
        logger_destroy(logger);
        return UNITTEST_ERROR;
    }
    Server* server = server_create(pyoneer, logger, SOCKET_PATH, TOKEN);
    if (!server) {
        return UNITTEST_FAILURE;
    }
    server_destroy(server);
    // If we reach here without crashing, the test passes
    return UNITTEST_SUCCESS;
}

static result_t test_case_run(unittest_case* expected) {
    Logger* logger = logger_create(LOGGER_DEBUG, FORMAT);
    Pyoneer* pyoneer = pyoneer_create(PYONEER_WORKER, WORKER_ID);
    Server* server = server_create(pyoneer, logger, SOCKET_PATH, TOKEN);
    if (server_start(server) != 0) {
        server_destroy(server);
        return UNITTEST_FAILURE;
    }
    server_destroy(server);
    return UNITTEST_SUCCESS;
}

Unittest* test_server_create(const char* name) {
    Unittest* ut = unittest_create(name);
    if (!ut) return NULL;

    unittest_add(ut, "test_server_create", test_case_create, CASE_NONE, NULL);
    unittest_add(ut, "test_server_destroy", test_case_destroy, CASE_NONE, NULL);
    unittest_add(ut, "test_server_run", test_case_run, CASE_NONE, NULL);

    return ut;
}
