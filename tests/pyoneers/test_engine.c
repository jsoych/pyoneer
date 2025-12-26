#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_pyoneers.h"
#include "engine.h"

extern Site* SITE;

static result_t test_case_create(unittest_case* expected) {
    Engine* engine = engine_create();
    if (!engine) return UNITTEST_FAILURE;
    engine_destroy(engine);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    Engine* engine = engine_create();
    if (!engine) return UNITTEST_ERROR;
    engine_destroy(engine);
    return UNITTEST_SUCCESS;
}

static result_t test_case_run(unittest_case* expected) {
    Engine* engine = engine_create();
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    int result = engine_run(engine, job, SITE);
    engine_destroy(engine);
    job_destroy(job);
    RETURN_RESULT(result);
}

static result_t test_case_wait(unittest_case* expected) {
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    Engine* engine = engine_create();
    engine_run(engine, job, SITE);
    engine_wait(engine);
    int actual = job_get_status(job);
    engine_destroy(engine);
    job_destroy(job);
    if (actual == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_stop(unittest_case* expected) {
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    Engine* engine = engine_create();
    engine_run(engine, job, SITE);
    engine_stop(engine);
    int actual = job_get_status(job);
    engine_destroy(engine);
    job_destroy(job);
    if (actual == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_restart(unittest_case* expected) {
    Engine* engine = engine_create();
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, NULL, 0);
    engine_run(engine, job, SITE);
    engine_restart(engine);
    engine_wait(engine);
    int actual = job_get_status(job);
    engine_destroy(engine);
    job_destroy(job);
    if (actual == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

Unittest* test_engine_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "engine_create", test_case_create, CASE_NONE, NULL);

    unittest_add(ut, "engine_destroy", test_case_destroy, CASE_NONE, NULL);

    unittest_add(ut, "engine_run", test_case_run, CASE_NONE, NULL);

    int incomplete = JOB_INCOMPLETE;
    unittest_add(ut, "engine_stop", test_case_run, CASE_INT, &incomplete);

    int not_ready = JOB_NOT_READY;
    unittest_add(ut, "engine_wait", test_case_wait, CASE_INT, &not_ready);

    int completed = JOB_COMPLETED;
    unittest_add(ut, "engine_start", test_case_restart, CASE_INT, &completed);

    return ut;
}
