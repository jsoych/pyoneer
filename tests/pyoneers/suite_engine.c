#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_pyoneers.h"
#include "engine.h"
#include "helpers.h"

extern Site* SITE;

static void test_create(Unittest* result) {
    Engine* engine = engine_create();
    if (!engine) {
        unittest_result_fail(result, "failed to create Engine");
        return;
    }
    unittest_result_ok(result);
    engine_destroy(engine);
}

static void test_destroy(Unittest* result) {
    engine_destroy(NULL);
    Engine* engine = engine_create();
    if (!engine) {
        unittest_result_err(result, "failed to create Engine");
        return;
    }
    engine_destroy(engine);
    unittest_result_ok(result);
}

static void test_run(Unittest* result) {
    Engine* engine = engine_create();
    Job* job = create_job(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        goto cleanup;
    }
    if (engine_run(engine, job, SITE) == -1) {
        unittest_result_fail(result, "failed to run");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    job_destroy(job);
    engine_destroy(engine);
}

static void test_restart(Unittest* result) {
    Engine* engine = engine_create();
    Job* job = create_job(JOB_ID, TASK_NAME, 2, NULL, 0);
    if (engine_run(engine, job, SITE) == -1) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    engine_restart(engine);
    engine_wait(engine);
    if (job_get_status(job) == JOB_COMPLETED) {
        unittest_result_fail(result, "unexpected status (%d)",
            job_get_status(job));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    engine_destroy(engine);
    job_destroy(job);
}

static void test_stop(Unittest* result) {
    Engine* engine = engine_create();
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    engine_run(engine, job, SITE);
    engine_stop(engine);
    if (job_get_status(job) == JOB_INCOMPLETE) {
        unittest_result_fail(result, "unexpected status (%d)",
            job_get_status(job));
        goto cleanup;
    }
cleanup:
    job_destroy(job);
    engine_destroy(engine);
}

static void test_wait(Unittest* result) {
    Engine* engine = engine_create();
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 2, BUG_NAME, 1);
    engine_run(engine, job, SITE);
    engine_wait(engine);
    if (job_get_status(job) == JOB_NOT_READY) {
        unittest_result_fail(result, "unexpected status (%d)",
            job_get_status(job));
        goto cleanup;
    }
cleanup:
    engine_destroy(engine);
    job_destroy(job);
}

Unittest* suite_engine_create(const char* name) {
    Unittest* suite = unittest_create(name);
    unittest_add_test(suite, "engine_create", test_create);
    unittest_add_test(suite, "engine_destroy", test_destroy);
    unittest_add_test(suite, "engine_run", test_run);
    unittest_add_test(suite, "engine_restart", test_restart);
    unittest_add_test(suite, "engine_stop", test_run);
    unittest_add_test(suite, "engine_wait", test_wait);
    return suite;
}
