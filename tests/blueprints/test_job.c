#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test_blueprints.h"

extern Site* SITE;

static void test_create(UnittestResult* result) {
    Job* job = job_create(VALID_ID);
    if (!job) {
        unittest_result_fail(result, "failed to create Job");
        return;
    }
    if (job_get_id(job) != VALID_ID) {
        unittest_result_fail(result, "unexpected id (%s)", job_get_id(job));
        goto cleanup;
    }
    if (job_get_status(job) != JOB_READY) {
        unittest_result_fail(result, "unexpected status (%d)", job_get_status(job));
        goto cleanup;
    }
    if (job_get_size(job) != 0) {
        unittest_result_fail(result, "unexpected size (%d)", job_get_size(job));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup: 
    job_destroy(job);
}

static void test_invalid_id(UnittestResult* result) {
    Job* job = job_create(INVALID_ID);
    if (job) {
        unittest_result_fail(result, "unexpected id (%d)", job_get_id(job));
        return;
    }
    unittest_result_ok(result);
}

static void test_destroy(UnittestResult* result) {
    job_destroy(NULL);
    Job* job = create_job(VALID_ID, TASK_NAME, 5, BUG_NAME, 3);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        return;
    }
    job_destroy(job);
    unittest_result_ok(result);
}

static void test_get_status(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 3, NULL, 0);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        return;
    }

    // check ready
    if (job_get_status(job) != JOB_READY) {
        unittest_result_fail(result, "status is not ready");
        goto cleanup;
    }

    // check running
    job_node* head = job_get_tasks(job);
    task_set_status(head->next->task, TASK_RUNNING);
    if (job_get_status(job) != JOB_RUNNING) {
        unittest_result_fail(result, "status is not running");
        goto cleanup;
    }

    // check not ready
    task_set_status(head->next->task, TASK_NOT_READY);
    if (job_get_status(job) != JOB_NOT_READY) {
        unittest_result_fail(result, "status is not not_ready");
        goto cleanup;
    }

    // check completed
    job_node* curr = job_get_tasks(job);
    while (curr) {
        task_set_status(curr->task, TASK_COMPLETED);
        curr = curr->next;
    }
    
    if (job_get_status(job) != JOB_COMPLETED) {
        unittest_result_fail(result, "status is not completed");
        goto cleanup;
    }

    // Check incomplete
    task_set_status(head->next->next->task, TASK_INCOMPLETE);
    if (job_get_status(job) != JOB_INCOMPLETE) {
        unittest_result_fail(result, "status is not incomplete");
        goto cleanup;
    }

cleanup:
    job_destroy(job);
}

static void test_run(UnittestResult* result) {
    Job* job = job_create(VALID_ID);
    JobRunner* runner = job_run(job, SITE);
    if (runner) {
        unittest_result_fail(result, "created runner");
        goto cleanup;
    }

    // add tasks to job
    for (int i = 0; i < 3; i++) {
        Task* t = task_create(TASK_NAME);
        if (!t) {
            unittest_result_err(result, "failed to create Task");
            goto cleanup;
        }
        if (job_add_task(job, t) == -1) {
            unittest_result_err(result, "failed to add Task");
            goto cleanup;
        }
    }

    // run job
    runner = job_run(job, SITE);
    if (!runner) {
        unittest_result_fail(result, "failed to run");
        goto cleanup;
    }
    job_runner_wait(runner);
    if (job_get_status(job) != JOB_COMPLETED) {
        unittest_result_fail(result, "unexpected status");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_bug(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 4, BUG_NAME, 1);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        return;
    }
    JobRunner* runner = job_run(job, SITE);
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

static void test_many_bugs(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 1, BUG_NAME, 5);
    JobRunner* runner = job_run(job, SITE);
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

static void test_parallel(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 3, BUG_NAME, 2);
    job_opts_t opts = job_opts_default();
    opts.exec_mode = JOB_EXEC_PARALLEL;
    job_set_opts(job, &opts);

    JobRunner* runner = job_run(job, SITE);
    job_runner_wait(runner);
    if (job_get_status(job) != JOB_COMPLETED) {
        unittest_result_fail(result, "unexpected status");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_repeat(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 3, NULL, 0);
    job_opts_t opts = job_opts_default();
    opts.run_mode = JOB_RUN_REPEAT;
    job_set_opts(job, &opts);

    JobRunner* runner = job_run(job, SITE);
    for (int i = 0; i < 10; i++) {
        job_runner_info_t info = job_runner_get_info(runner);
        if (info.attempts < info.completions) {
            unittest_result_fail(result, "less attempts than completions");
            goto cleanup;
        }
        if (info.completions < info.successes) {
            unittest_result_fail(result, "less completions than successes");
            goto cleanup;
        }
        sleep(1);
    }

    if (job_get_status(job) != JOB_COMPLETED) {
        unittest_result_fail(result, "unexpected status");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_parallel_repeat(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 20, BUG_NAME, 10);
    job_opts_t opts = job_opts_default();
    opts.exec_mode = JOB_EXEC_PARALLEL;
    opts.run_mode = JOB_RUN_REPEAT;
    job_set_opts(job, &opts);

    JobRunner* runner = job_run(job, SITE);
    sleep(10);

    job_runner_info_t info = job_runner_get_info(runner);
    if (info.attempts < 3) {
        unittest_result_fail(result, "fewer than 3 attempts");
        goto cleanup;
    }
    if (info.successes != 0) {
        unittest_result_fail(result, "unexpected success");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_stop(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 3, NULL, 0);
    if (!job) {
        unittest_result_fail(result, "failed to create Job");
        return;
    }
    JobRunner* runner = job_run(job, SITE);
    job_runner_stop(runner);
    if (job_get_status(job) == JOB_READY) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    if (job_get_status(job) != JOB_INCOMPLETE) {
        unittest_result_fail(result, "unexpected status");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    job_runner_destroy(runner);
    job_destroy(job);
}

static void test_add(UnittestResult* result) {
    Job* job = job_create(VALID_ID);
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        if (job_add_task(job, task) == -1) {
            unittest_result_fail(result, "failed to add Task");
            goto cleanup;
        }
    }
    if (job_get_size(job) != 3) {
        unittest_result_fail(result, "unexpected size");
        goto cleanup;
    }

    for (int i = 0; i < 7; i++) {
        Task* task = task_create(TASK_NAME);
        if (job_add_task(job, task) == -1) {
            unittest_result_fail(result, "failed to add Task");
            goto cleanup;
        }
    }
    if (job_get_size(job) != 10) {
        unittest_result_fail(result, "unexpected size");
    }
    unittest_result_ok(result);

cleanup:
    job_destroy(job);
}

static void test_encode(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 2, NULL, 0);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        return;
    }
    job_opts_t opts = {
        .exec_mode = JOB_EXEC_SEQUENTIAL,
        .run_mode = JOB_RUN_REPEAT
    };
    job_set_opts(job, &opts);

    json_value* actual = job_encode(job);
    if (!actual) {
        unittest_result_fail(result, "failed to encode Job");
        job_destroy(job);
        return;
    }

    json_value* expected = json_object_new(0);
    json_object_push_integer(expected, "id", VALID_ID);
    json_object_push_string(expected, "execMode", "sequential");
    json_object_push_string(expected, "runMode", "repeat");
    json_value* tasks = json_array_new(3);
    for (int i = 0; i < 2; i++) {
        json_value* task = json_object_new(0);
        json_object_push_string(task, "name", TASK_NAME);
        json_array_push(tasks, task);
    }
    json_object_push(expected, "tasks", tasks);

    int rv = json_value_compare(actual, expected);
    if (rv == -1) {
        unittest_result_err(result, "failed to compare JSON");
        goto cleanup;
    }
    if (rv != 0) {
        char actual_buf[BUFSIZE];
        int size = json_measure(actual);
        if (size > BUFSIZE) {
            unittest_result_fail(result, "actaul JSON too large (%d)", size);
            goto cleanup;
        }
        json_serialize(actual_buf, actual);
        actual_buf[size] = '\0';

        char expected_buf[BUFSIZE];
        size = json_measure(expected);
        if (size > BUFSIZE) {
            unittest_result_err(result, "expected JSON too large (%d)", size);
            goto cleanup;
        }
        expected_buf[size] = '\0';

        unittest_result_fail(result, "expected %s, actual %s",
            expected_buf, actual_buf);
        goto cleanup;
    }
    unittest_result_ok(result);
    
cleanup:
    json_value_free(actual);
    json_value_free(expected);
    job_destroy(job);
}

static void test_encode_status(UnittestResult* result) {
    Job* job = create_job(VALID_ID, TASK_NAME, 1, NULL, 0);
    if (!job) {
        unittest_result_err(result, "failed to create Job");
        return;
    }

    json_value* actual = job_encode_status(job);
    if (!actual) {
        unittest_result_fail(result, "failed to encode status");
        job_destroy(job);
        return;
    }

    // create expected status
    json_value* expected = json_object_new(0);
    json_object_push_integer(expected, "id", VALID_ID);
    json_object_push(expected, "status", job_status_map(JOB_READY));
    json_value* tasks = json_array_new(1);
    json_value* status = json_object_new(0);
    json_object_push(status, "status", task_status_map(TASK_READY));
    json_object_push_string(status, "name", TASK_NAME);
    json_array_push(tasks, status);
    json_object_push(expected, "tasks", tasks);

    int rv = json_value_compare(actual, expected);
    if (rv == -1) {
        unittest_result_err(result, "failed to compare JSON");
        goto cleanup;
    }
    if (rv != 0) {
        unittest_result_fail(result, "unexpected JSON");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    json_value_free(actual);
    json_value_free(expected);
    job_destroy(job);
}

static void test_decode(UnittestResult* result) {
    const char* json = "{ \
        \"id\": 33, \
        \"tasks\": [{\"name\": \"task.py\"}, {\"name\": \"task.py\"}], \
        \"execMode\": \"sequential\", \
        \"runMode\": \"repeat\" \
    }";
    json_value* obj = json_parse(json, strlen(json));
    if (!obj) {
        unittest_result_err(result, "failed to parse JSON");
        return;
    }

    Job* job = job_decode(obj);
    if (!job) {
        unittest_result_fail(result, "failed to decode JSON");
        goto cleanup;
    }
    if (job_get_id(job) != 33) {
        unittest_result_fail(result, "unexpected id");
        goto cleanup;
    }
    if (job_get_size(job) != 2) {
        unittest_result_fail(result, "unexpected size");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    json_value_free(obj);
    job_destroy(job);
}

static void test_invalid_task(UnittestResult* result) {
    const char* json = "{ \
        \"id\": 42, \
        \"tasks\": [{\"key\": \"value\"}] \
    }";
    json_value* obj = json_parse(json, strlen(json));
    if (!obj) {
        unittest_result_err(result, "failed to parse JSON");
        return;
    }

    Job* job = job_decode(obj);
    if (job) {
        unittest_result_fail(result, "decoded invalid task");
        goto cleanup;
    }
    unittest_result_ok(result);

cleanup:
    json_value_free(obj);
    job_destroy(job);
}

Unittest* create_job_suite(const char* name) {
    Unittest* suite = unittest_create_suite(name);

    unittest_add_test(suite, "job_create", test_create);

    unittest_add_test(suite, "job_create - invalid id", test_invalid_id);

    unittest_add_test(suite, "job_destroy", test_destroy);

    unittest_add_test(suite, "job_get_status", test_get_status);

    unittest_add_test(suite, "job_add_task", test_add);

    unittest_add_test(suite, "job_run", test_run);

    unittest_add_test(suite, "job_run - with bug", test_bug);
    
    unittest_add_test(suite, "job_run - many bugs", test_many_bugs);
    
    unittest_add_test(suite, "job_run - parrallel", test_parallel);
    
    unittest_add_test(suite, "job_run - repeat", test_repeat);

    unittest_add_test(suite, "job_run - parrallel repeat", test_parallel_repeat);
    
    unittest_add_test(suite, "job_run - stop", test_stop);

    unittest_add_test(suite, "job_encode", test_encode);
    
    unittest_add_test(suite, "job_encode_status", test_encode_status);
    
    unittest_add_test(suite, "job_decode", test_decode);

    unittest_add_test(suite, "job_decode - invalid task", test_invalid_task);

    return suite;
}
