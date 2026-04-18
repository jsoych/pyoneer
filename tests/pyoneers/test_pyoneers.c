#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_pyoneers.h"

Site* SITE;

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
    Unittest* suites[3] = {
        suite_engine_create("engine"),
        suite_worker_create("worker"),
        suite_pyoneer_create("pyoneer")
    };

    int rv = EXIT_SUCCESS;
    for (int i = 0; i < 3; i++) {
        if (!suites[i]) {
            fprintf(stderr, "main: error: failed to create suite (%d)\n", i);
            rv = EXIT_FAILURE;
            goto cleanup;
        }
    }

    // run suites
    UnittestResult* result;
    for (int i = 0; i < 3; i++) {
        result = unittest_run(suites[i], NULL, NULL);
        if (!result) {
            fprintf(stderr, "main: warning: failed to run suite (%s)\n",
                unittest_get_name(suites[i]));
            rv = EXIT_FAILURE;
            continue;
        }
        unittest_print_result(suites[i], result, UNITTEST_VERBOSE);
        unittest_result_destroy(result);
    }
    
cleanup:
    for (int i = 0; i < 3; i++) {
        unittest_destroy(suites[i]);
    }
    site_destroy(SITE);
    exit(rv);
}
