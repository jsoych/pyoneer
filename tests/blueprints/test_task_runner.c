#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_blueprints.h"

extern Site* SITE;

static void test_case_destroy(UnittestResult* result) {
    task_runner_destroy(NULL);
    Task* task = task_create(SHORT_NAME);
    if (!task) {
        unittest_result_err(result, "failed to create Task");
        goto cleanup;
    }
    TaskRunner* runner = task_run(task, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to create TaskRunner");
        goto cleanup;
    }
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

static void test_case_wait(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    TaskRunner* runner = task_run(task, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to create TaskRunner");
        goto cleanup;
    }
    task_runner_wait(runner);
    task_runner_info_t info = task_runner_get_info(runner);
    if (info.exit_code != 0 || info.signo != 0) {
        unittest_result_fail(result, "unexpected exit_code (%d) or signo (%d)", info.exit_code, info.signo);
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

static void test_case_bug(UnittestResult* result) {
    Task* task = task_create(BUG_NAME);
    TaskRunner* runner = task_run(task, SITE);
    task_runner_wait(runner);
    task_runner_info_t info = task_runner_get_info(runner);
    if (info.exit_code == 0 || info.signo != 0) {
        unittest_result_fail(result, "unexpected exit_code (%d) or signo (%d)", info.exit_code, info.signo);
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

static void test_case_stop(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    TaskRunner* runner = task_run(task, SITE);
    task_runner_stop(runner);
    task_runner_info_t info = task_runner_get_info(runner);
    if (info.exit_code == 0xff && info.signo == 0) {
        unittest_result_err(result, "the Task was stopped before it ran");
        goto cleanup;
    }
    if (info.signo != SIGINT) {
        unittest_result_fail(result, "unexpected signo (%d)", info.signo);
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

Unittest* create_task_runner_suite(const char* name) {
    Unittest* suite = unittest_create_suite(name);
    if (!suite) return NULL;

    Unittest* test = unittest_create_test("task_destroy", test_case_destroy);
    unittest_add(suite, test);

    test = unittest_create_test("task_runner_wait", test_case_wait);
    unittest_add(suite, test);

    test = unittest_create_test("task_runner_wait - with bug", test_case_bug);
    unittest_add(suite, test);

    test = unittest_create_test("task_runner_stop", test_case_stop);
    unittest_add(suite, test);

    return suite;
}
