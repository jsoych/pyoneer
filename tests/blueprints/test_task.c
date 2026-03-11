#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_blueprints.h"
#include "json-helpers.h"

extern Site* SITE;

// Test Cases
void test_case_create(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    if (!task) {
        unittest_result_fail(result, "failed to create Task");
        return;
    }
    // Check name
    if (task_get_status(task) != TASK_READY ||
        strcmp(task_get_name(task), SHORT_NAME) != 0) {
        unittest_result_fail(result, "unexpected name (%s)",
            task_get_name(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_destroy(task);
}

void test_case_destroy(UnittestResult* result) {
    task_destroy(NULL);
    Task* task = task_create(SHORT_NAME);
    if (!task) {
        unittest_result_err(result, "failed to create Task");
        return;
    }
    unittest_result_ok(result);
    task_destroy(task);
}

void test_case_status(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    task_set_status(task, TASK_NOT_READY);
    if (task_get_status(task) != TASK_NOT_READY) {
        unittest_result_fail(result, "unexpected status (%s)",
            task_get_status(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_destroy(task);
}

void test_case_run(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    if (!task) {
        unittest_result_err(result, "failed to create Task");
        return;
    }
    TaskRunner* runner = task_run(task, SITE);
    if (!runner) {
        unittest_result_fail(result, "failed to run");
        goto cleanup;
    }
    task_runner_wait(runner);
    if (task_get_status(task) != TASK_COMPLETED) {
        unittest_result_fail(result, "unexpected status (%d)", task_get_status(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

void test_case_bug(UnittestResult* result) {
    Task* task = task_create(BUG_NAME);
    if (!task) {
        unittest_result_err(result, "failed to create test");
        return;
    }
    TaskRunner* runner = task_run(task, SITE);
    if (!runner) {
        unittest_result_err(result, "failed to run");
        goto cleanup;
    }
    task_runner_wait(runner);
    if (task_get_status(task) != TASK_NOT_READY) {
        unittest_result_fail(result, "unexpected status (%d)", task_get_status(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

void test_case_write(UnittestResult* result) {
    Task* task = task_create(WRITE_NAME);
    TaskRunner* runner = task_run(task, SITE);
    task_runner_wait(runner);
    if (task_get_status(task) != TASK_COMPLETED) {
        unittest_result_fail(result, "unexpected status (%d)",
            task_get_status(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    task_runner_destroy(runner);
    task_destroy(task);
}

void test_case_encode(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    json_value* actual = task_encode(task);
    if (!actual) {
        unittest_result_fail(result, "failed to create JSON");
        task_destroy(task);
        return;
    }
    json_value* expected = json_object_new(0);
    json_object_push_string(expected, "name", SHORT_NAME);
    if (!json_value_compare(actual, expected)) {
        unittest_result_fail(result, "unexpected JSON");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(actual);
    json_value_free(expected);
    task_destroy(task);
}

void test_case_encode_status(UnittestResult* result) {
    Task* task = task_create(SHORT_NAME);
    json_value* actual = task_encode_status(task);
    if (!actual) {
        unittest_result_fail(result, "failed to create JSON status");
        task_destroy(task);
        return;
    }
    json_value* expected = json_object_new(0);
    json_object_push_string(expected, "status", "ready");
    if (!json_value_compare(actual, expected)) {
        unittest_result_fail(result, "unexpected JSON status");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(actual);
    json_value_free(expected);
    task_destroy(task);
}

void test_case_encode_bug(UnittestResult* result) {
    Task* task = task_create(BUG_NAME);
    TaskRunner* runner = task_run(task, SITE);
    task_runner_wait(runner);
    json_value* actual = task_encode_status(task);
    json_value* expected = json_object_new(0);
    json_object_push_string(expected, "status", "not_ready");
    if (json_value_compare(actual, expected) != 0) {
        unittest_result_fail(result, "unexpected JSON status");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(actual);
    json_value_free(expected);
    task_runner_destroy(runner);
    task_destroy(task);
}

void test_case_decode(UnittestResult* result) {
    json_value* obj = json_object_new(0);
    json_object_push_string(obj, "name", SHORT_NAME);
    Task* task = task_decode(obj);
    if (!task) {
        unittest_result_fail(result, "failed to create JSON");
        goto cleanup;
    }
    if (strcmp(task_get_name(task), SHORT_NAME) > 0) {
        unittest_result_fail(result, "unexpected name (%s)",
            task_get_name(task));
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(obj);
    task_destroy(task);
}

void test_case_missing(UnittestResult* result) {
    json_value* obj = json_object_new(0);
    Task* actual = task_decode(obj);
    if (actual) {
        unittest_result_fail(result, "unexpected decoding");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(obj);
    task_destroy(actual);
}

void test_case_invalid(UnittestResult* result) {
    json_value* obj = json_object_new(0);
    json_object_push_string(obj, "name", "");
    Task* actual = task_decode(obj);
    if (actual) {
        unittest_result_fail(result, "unexpected decoding");
        goto cleanup;
    }
    unittest_result_ok(result);
cleanup:
    json_value_free(obj);
    task_destroy(actual);
}

Unittest* create_task_suite(const char* name) {
    Unittest* suite = unittest_create_suite(name);
    if (!suite) return NULL;

    Unittest* test = unittest_create_test("task_create", test_case_create);
    unittest_add(suite, test);
    
    test = unittest_create_test("task_destroy", test_case_destroy);
    unittest_add(suite, test);

    test = unittest_create_test("task_run", test_case_run);
    unittest_add(suite, test);

    test = unittest_create_test("task_run - with bug", test_case_bug);
    unittest_add(suite, test);
    
    test = unittest_create_test("task_run - write to directory", test_case_write);
    unittest_add(suite, test);

    test = unittest_create_test("task_encode", test_case_encode);
    unittest_add(suite, test);

    test = unittest_create_test("task_encode_status", test_case_encode_status);
    unittest_add(suite, test);

    test = unittest_create_test("task_encoce_status - with bug", test_case_encode_bug);
    unittest_add(suite, test);

    test = unittest_create_test("task_decode - missing name", test_case_missing);
    unittest_add(suite, test);

    test = unittest_create_test("task_decode - invalid type", test_case_invalid);
    unittest_add(suite, test);
    
    return suite;
}
