#include "test_blueprints.h"

extern Site* SITE;

static void test_destory(UnittestResult* result) {
    job_runner_destroy(NULL);
    Job* job = create_job(VALID_ID, TASK_NAME, 1, BUG_NAME, 1);
    if (!job) {
        unittest_result_fail(result, "failed to create Job");
        goto cleanup;
    }
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    job_runner_destroy(runner);
    unittest_result_ok(result);
cleanup:
    job_destroy(job);
}

static void test_wait(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 2, BUG_NAME, 2);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    job_runner_wait(runner);
    if (job_get_status(job) != JOB_NOT_READY) {
        unittest_result_fail(result, "unexpected status");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_stop(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 2, BUG_NAME, 2);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    job_runner_stop(runner);
    if (job_get_status(job) == JOB_READY) {
        unittest_result_err(result, "did not run before stopped");
        goto cleanup;
    }
    if (job_get_status(job) != JOB_INCOMPLETE) {
        unittest_result_fail(result, "failed to stop");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_restart(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 1, BUG_NAME, 1);
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }

    job_runner_wait(runner);
    job_runner_restart(runner);
    job_runner_wait(runner);

    job_runner_info_t info = job_runner_get_info(runner);
    if (info.attempts != 2) {
        unittest_result_fail(result, "unexpected attempts");
        goto cleanup;
    }
    if (info.completions != 0) {
        unittest_result_fail(result, "unexpected completions");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

Unittest* create_job_runner_suite(const char* name) {
    Unittest* suite = unittest_create_suite(name);
    if (!suite) return NULL;
    unittest_add_test(suite, "job_runner_destroy", test_destory);
    unittest_add_test(suite, "job_runner_wait", test_wait);
    unittest_add_test(suite, "job_runner_stop", test_stop);
    unittest_add_test(suite, "job_runner_restart", test_restart);
    return suite;
}
