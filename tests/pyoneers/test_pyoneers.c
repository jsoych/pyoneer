#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "suite.h"
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

    Suite* suite = suite_create(3);

    Unittest* engine = test_engine_create("engine");
    Unittest* worker = test_worker_create("worker");
    Unittest* pyoneer = test_pyoneer_create("pyoneer");

    suite_add(suite, engine);
    suite_add(suite, worker);
    suite_add(suite, pyoneer);

    suite_run(suite);
    
    suite_destroy(suite);
    site_destroy(SITE);
    exit(EXIT_SUCCESS);
}
