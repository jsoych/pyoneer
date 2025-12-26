#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "suite.h"
#include "test_api.h"

Site* SITE;
char* SOCKET_PATH = NULL;
char* TOKEN = NULL;

int main() {
    SITE = site_create(
        getenv("PYONEER_PYTHON"),
        getenv("PYONEER_TASK_DIR"),
        getenv("PYONEER_WORKING_DIR")
    );
    if (!SITE) {
        fprintf(stderr, "main: Error: Missing environment variables\n");
        exit(EXIT_FAILURE);
    }

    SOCKET_PATH = getenv("PYONEER_SOCKET_PATH");
    TOKEN = getenv("PYONEER_API_TOKEN");
    if (!SOCKET_PATH || !TOKEN) {
        fprintf(stderr, "main: Error: Missing environment variables\n");
        site_destroy(SITE);
        exit(EXIT_FAILURE);
    }

    Suite* suite = suite_create(2);

    Unittest* logger = test_logger_create("logger");
    Unittest* server = test_server_create("server");

    suite_add(suite, logger);
    suite_add(suite, server);
    
    suite_run(suite);
    
    suite_destroy(suite);
    site_destroy(SITE);
    exit(EXIT_SUCCESS);
}
