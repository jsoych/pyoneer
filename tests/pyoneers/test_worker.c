#include <stdio.h>

#include "test_pyoneers.h"
#include "json-helpers.h"

extern Site* SITE;

static result_t test_case_create(unittest_case* expected) {
    Worker* worker = worker_create(WORKER_ID);
    if (!worker) return UNITTEST_FAILURE;
    if (worker_get_id(worker) != WORKER_ID) {
        worker_destroy(worker);
        return UNITTEST_FAILURE;
    }
    worker_destroy(worker);
    return UNITTEST_SUCCESS;
}

static result_t test_case_destroy(unittest_case* expected) {
    worker_destroy(NULL);
    Worker* worker = worker_create(WORKER_ID);
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 1, NULL, 0);

    // Check not working
    worker_assign(worker, job);
    worker_destroy(worker);

    worker = worker_create(WORKER_ID);
    job = unittest_job_create(JOB_ID, TASK_NAME, 1, NULL, 0);

    // Check working
    worker_run(worker, job);
    worker_destroy(worker);
    return UNITTEST_SUCCESS;
}

static result_t test_case_get_status(unittest_case* expected) {
    Worker* worker = worker_create(WORKER_ID);
    json_value* actual = worker_get_status(worker);
    if (!actual) {
        worker_destroy(worker);
        return UNITTEST_ERROR;
    }
    int result = json_value_compare(actual, expected->as.json);
    json_value_free(actual);
    worker_destroy(worker);
    RETURN_RESULT(result);
}

static result_t test_case_run(unittest_case* expected) {
    Worker* worker = worker_create(WORKER_ID);
    Job* job = unittest_job_create(JOB_ID, TASK_NAME, 1, NULL, 0);
    json_value* status = worker_run(worker, job);
    if (!status) return UNITTEST_FAILURE;
    json_value_free(status);
    return UNITTEST_SUCCESS;
}

Unittest* test_worker_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "worker_create", test_case_create, CASE_NONE, NULL);

    unittest_add(ut, "worker_destroy", test_case_destroy, CASE_NONE, NULL);

    // Create expected status
    json_value* not_assigned = json_object_new(0);
    json_object_push(not_assigned, "status",
        worker_status_map(WORKER_NOT_ASSIGNED));
    json_object_push(not_assigned, "job", json_null_new());

    unittest_add(ut, "worker_get_status", test_case_get_status,
        CASE_JSON, not_assigned);

    unittest_add(ut, "worker_run", test_case_run, CASE_NONE, NULL);
    
    return ut;
}
