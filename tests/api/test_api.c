#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "suite.h"
#include "test_api.h"

Site* SITE = NULL;
server_config* SCFG = NULL;

int main() {
    // define endpoint
    server_endpoint eps[] = {
    (server_endpoint){
        .type = EP_UNIX,
        .host = NULL,
        .port = NULL,
        .path = getenv("PYONEER_SOCKET_PATH"),
        .backlog = 8}
    };

    server_config scfg = (server_config){
        .endpoints = eps,
        .nendpoints = 1,
        .token = getenv("PYONEER_API_TOKEN")
    };

    SCFG = &scfg;

    SITE = site_create(
        getenv("PYONEER_PYTHON"),
        getenv("PYONEER_TASK_DIR"),
        getenv("PYONEER_WORKING_DIR")
    );

    if (!SITE) {
        fprintf(stderr, "main: Error: Missing environment variables\n");
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
