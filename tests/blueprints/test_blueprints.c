#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "test_blueprints.h"
#include "print_macros.h"

Site* SITE;

int main() {
    SITE = site_create(
        getenv("PYONEER_PYTHON"),
        getenv("PYONEER_TASK_DIR"),
        getenv("PYONEER_WORKING_DIR")
    );
    
    if (!SITE) {
        ERROR("missing environment variables");
        exit(EXIT_FAILURE);
    }

    Unittest* suites[4] = {
        create_task_suite("task"),
        create_task_runner_suite("task runner"),
        create_job_suite("job"),
        create_job_runner_suite("job_runner")
    };
    
    int rv = EXIT_SUCCESS;
    for (int i = 0; i < 4; i++) {
        if (!suites[i]) {
            rv = EXIT_FAILURE;
            goto cleanup;
        }
        UnittestResult* result = unittest_run(suites[i], NULL, NULL);
        if (result) {
            unittest_print_result(suites[i], result, UNITTEST_VERBOSE);
            unittest_result_destroy(result);
        }
    }

cleanup:
    for (int i = 0; i < 4; i++) {
        unittest_destroy(suites[i]);
    }
    site_destroy(SITE);
    exit(rv);
}

Job* create_job(int id, const char* task, int ntask, const char* bug, int nbug) {
    Job* j = job_create(id);
    if (!j) return NULL;
    for (int i = 0; i < ntask; i++) {
        Task* t = task_create(task);
        if (!t) {
            job_destroy(j);
            return NULL;
        }
        if (job_add_task(j, t) == -1) {
            job_destroy(j);
            return NULL;
        }
    }

    for (int i = 0; i < nbug; i++) {
        Task* b = task_create(bug);
        if (!b) {
            job_destroy(j);
            return NULL;
        }
        if (job_add_task(j, b) == -1) {
            job_destroy(j);
            return NULL;
        }
    }

    return j;
}
