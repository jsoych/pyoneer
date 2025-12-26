#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_blueprints.h"

extern Site* SITE;

static result_t test_case_wait(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 2, BUG_NAME, 2);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        job_destroy(job);
        return UNITTEST_ERROR;
    }
    job_runner_wait(runner);
    job_destroy(job);
    free(runner);
    return UNITTEST_SUCCESS;
}

static result_t test_case_stop(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 2, BUG_NAME, 2);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        job_destroy(job);
        return UNITTEST_ERROR;
    }
    job_runner_stop(runner);
    job_destroy(job);
    free(runner);
    return UNITTEST_SUCCESS;
}

static result_t test_case_restart(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 1, BUG_NAME, 1);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        job_destroy(job);
        return UNITTEST_ERROR;
    }
    job_runner_restart(runner);
    job_runner_wait(runner);
    job_destroy(job);
    free(runner);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destory(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 1, BUG_NAME, 1);
    JobRunner* runner = job_run(job, SITE);
    job_runner_destroy(runner);
    job_destroy(job);
    return UNITTEST_SUCCESS;
}

Unittest* test_job_runner_create(const char* name) {
    Unittest* ut = unittest_create(name);
    if (!ut) return NULL;

    unittest_add(ut, "job_runner_wait", test_case_wait, CASE_NONE, NULL);
    unittest_add(ut, "job_runner_stop", test_case_stop, CASE_NONE, NULL);
    unittest_add(ut, "job_runner_start", test_case_restart, CASE_NONE, NULL);
    unittest_add(ut, "job_runner_destroy", test_case_destory, CASE_NONE, NULL);
    
    return ut;
}
