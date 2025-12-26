#include <stdio.h>
#include <string.h>

#include "test_blueprints.h"
#include "json-builder.h"
#include "json-helpers.h"

extern Site* SITE;

const char* JOB_JSON = "{\"id\": 33, \"tasks\": [{\"name\": \"task.py\"}, {\"name\": \"task.py\"}, {\"name\": \"task.py\"}]}";
const char* JOB_JSON_INVALID_TASK = "{\"id\": 42, \"tasks\": [{\"key\": \"value\"}]}";

static result_t test_case_create(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    if (job == NULL) return UNITTEST_FAILURE;
    
    // Check properties
    int result = 0;
    if (job->id != VALID_ID ||
        job->status != JOB_READY ||
        job->size != 0 ||
        job->head != NULL) result = 1;
                
    job_destroy(job);
    if (result == 0) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_invalid_id(unittest_case* expected) {
    Job* job = job_create(INVALID_ID, false);
    if (job == NULL) return UNITTEST_SUCCESS;
    job_destroy(job);
    return UNITTEST_FAILURE;
}

static result_t test_case_destroy(unittest_case* expected) {
    // Check NULL
    job_destroy(NULL);

    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 10; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    job_destroy(job);
    return UNITTEST_SUCCESS;
}

static result_t test_case_get_status(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    // Check ready
    if (job_get_status(job) != JOB_READY) {
        job_node* curr = job->head;
        while (curr) {
            printf("task status: %d\n", curr->task->status);
            curr = curr->next;
        }
        job_destroy(job);
        printf("failed ready check");
        return UNITTEST_FAILURE;
    }

    // Check running
    job->head->next->task->status = TASK_RUNNING;
    if (job_get_status(job) != JOB_RUNNING) {
        job_destroy(job);
        printf("failed running check");
        return UNITTEST_FAILURE;
    }

    // Check not ready
    job->head->next->task->status = TASK_NOT_READY;
    if (job_get_status(job) != JOB_NOT_READY) {
        job_destroy(job);
        printf("failed not ready check");
        return UNITTEST_FAILURE;
    }

    // Check completed
    job_node* curr = job->head;
    while (curr) {
        curr->task->status = TASK_COMPLETED;
        curr = curr->next;
    }
    
    if (job_get_status(job) != JOB_COMPLETED) {
        job_destroy(job);
        printf("failed completed check");
        return UNITTEST_FAILURE;
    }

    // Check incomplete
    job->head->next->next->task->status = TASK_INCOMPLETE;
    if (job_get_status(job) != JOB_INCOMPLETE) {
        job_destroy(job);
        printf("failed incomplete check");
        return UNITTEST_FAILURE;
    }

    job_destroy(job);
    return UNITTEST_SUCCESS;
}

static result_t test_case_run(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    JobRunner* runner = job_run(job, SITE);
    if (runner) {
        job_destroy(job);
        job_runner_destroy(runner);
        return UNITTEST_FAILURE;
    }

    // Add tasks to job
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    runner = job_run(job, SITE);
    if (!runner) {
        job_destroy(job);
        return UNITTEST_FAILURE;
    }
    job_runner_wait(runner);

    // Clean up and return result
    int actual = job_get_status(job);
    job_destroy(job);
    job_runner_destroy(runner);
    if (actual == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_bug(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 4; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }
    Task* bug = task_create(BUG_NAME);
    job_add_task(job, bug);

    JobRunner* runner = job_run(job, SITE);
    job_runner_wait(runner);

    int actaul = job_get_status(job);
    job_destroy(job);
    job_runner_destroy(runner);
    if (actaul == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_parallel(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 3, BUG_NAME, 2);
    job->parallel = true;
    JobRunner* runner = job_run(job, SITE);
    job_runner_wait(runner);
    job_destroy(job);
    job_runner_destroy(runner);
    return UNITTEST_SUCCESS;
}

static result_t test_case_many_bugs(unittest_case* expected) {
    Job* job = unittest_job_create(VALID_ID, TASK_NAME, 1, BUG_NAME, 5);
    JobRunner* runner = job_run(job, SITE);
    job_runner_wait(runner);
    job_destroy(job);
    job_runner_destroy(runner);
    return UNITTEST_SUCCESS;
}

static result_t test_case_stop(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }
    JobRunner* runner = job_run(job, SITE);
    if (!runner) {
        job_destroy(job);
        return UNITTEST_ERROR;
    }
    job_runner_stop(runner);
    int actual = job_get_status(job);
    job_runner_destroy(runner);
    job_destroy(job);
    if (actual == expected->as.integer) return UNITTEST_SUCCESS;
    return UNITTEST_FAILURE;
}

static result_t test_case_add(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    if (job->size != 0) {
        job_destroy(job);
        return UNITTEST_FAILURE;
    }

    // Add 3 tasks
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    if (job->size != 3) {
        job_destroy(job);
        return UNITTEST_FAILURE;
    }

    // Add 7 more tasks
    for (int i = 0; i < 7; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    if (job->size != 10) {
        job_destroy(job);
        return UNITTEST_FAILURE;
    }

    job_destroy(job);
    return UNITTEST_SUCCESS;
}

static result_t test_case_encode(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }

    json_value* actual = job_encode(job);
    if (!actual) {
        printf("failed to encode");
        job_destroy(job);
        return UNITTEST_FAILURE;
    }

    int result = json_value_compare(actual, expected->as.json);
    if (result == -1) {
        printf("failed to compare\n");
        job_destroy(job);
        json_value_free(actual);
        return UNITTEST_ERROR;
    }

    char buf[BUFSIZE] = {0};
    printf("actual %s\n", buf);

    json_serialize(buf, expected->as.json);
    printf("expect %s\n", buf);

    job_destroy(job);
    json_value_free(actual);
    RETURN_RESULT(result);
}

static result_t test_case_encode_status(unittest_case* expected) {
    Job* job = job_create(VALID_ID, false);
    Task* task = task_create(TASK_NAME);
    job_add_task(job, task);
    json_value* actual = job_encode_status(job);
    char buf[BUFSIZE];
    json_serialize(buf, actual);
    printf("actual: %s\n", buf);
    job_destroy(job);
    RETURN_RESULT(json_value_compare(actual, expected->as.json));
}

static result_t test_case_decode(unittest_case* expected) {
    json_value* obj = json_parse(JOB_JSON, strlen(JOB_JSON));
    if (!obj) return UNITTEST_ERROR;
    Job* actual = job_decode(obj);
    if (!actual) {
        printf("failed to decode\n");
        json_value_free(obj);
        return UNITTEST_FAILURE;
    }
    int result = unittest_compare_job(actual, expected->as.job);
    job_destroy(actual);
    RETURN_RESULT(result);
}

static result_t test_case_invalid_task(unittest_case* expected) {
    json_value* obj = json_parse(JOB_JSON_INVALID_TASK,
        strlen(JOB_JSON_INVALID_TASK));
    if (!obj) return UNITTEST_ERROR;
    Job* actual = job_decode(obj);
    if (!actual) return UNITTEST_SUCCESS;
    job_destroy(actual);
    return UNITTEST_FAILURE;
}

Unittest* test_job_create(const char* name) {
    Unittest* ut = unittest_create(name);

    unittest_add(ut, "job_create", test_case_create,
        CASE_NONE, NULL);

    unittest_add(ut, "job_create - invalid id", test_case_invalid_id,
        CASE_NONE, NULL);

    unittest_add(ut, "job_destroy", test_case_destroy,
        CASE_NONE, NULL);

    unittest_add(ut, "job_get_status", test_case_get_status,
        CASE_NONE, NULL);

    unittest_add(ut, "job_add_task", test_case_add, CASE_NONE, NULL);

    int completed = JOB_COMPLETED;
    unittest_add(ut, "job_run", test_case_run, CASE_INT, &completed);

    int not_ready = JOB_NOT_READY;
    unittest_add(ut, "job_run - with bug", test_case_bug,
        CASE_INT, &not_ready);
    
    unittest_add(ut, "job_run - parrallel", test_case_parallel,
        CASE_NONE, NULL);
    
    unittest_add(ut, "job_run - many bugs", 
        test_case_many_bugs, CASE_NONE, NULL);
    
    int incomplete = JOB_INCOMPLETE;
    unittest_add(ut, "job_run - stop", test_case_stop,
        CASE_INT, &incomplete);
    
    // Create expected job encoding
    json_value* obj = json_object_new(0);
    json_object_push_integer(obj, "id", VALID_ID);
    json_value* arr = json_array_new(3);
    for (int i = 0; i < 3; i++) {
        json_value* task = json_object_new(0);
        json_object_push_string(task, "name", TASK_NAME);
        json_array_push(arr, task);
    }
    json_object_push(obj, "tasks", arr);

    unittest_add(ut, "job_encode", test_case_encode, CASE_JSON, obj);

    // Create expected job status
    json_value* ready = json_object_new(0);
    json_object_push_string(ready, "status", "ready");
    json_value* tasks = json_array_new(1);
    json_value* task_status = json_object_new(0);
    json_object_push_string(task_status, "status", "ready");
    json_object_push_string(task_status, "name", TASK_NAME);
    json_array_push(tasks, task_status);
    json_object_push(ready, "tasks", tasks);
    
    unittest_add(ut, "job_encode_status", test_case_encode_status,
        CASE_JSON, ready);
    
    // Create expected job
    Job* job = job_create(VALID_ID, false);
    for (int i = 0; i < 3; i++) {
        Task* task = task_create(TASK_NAME);
        job_add_task(job, task);
    }
    
    unittest_add(ut, "job_decode", test_case_decode, CASE_JOB, job);

    unittest_add(ut, "job_decode - invalid task",
        test_case_invalid_task, CASE_NONE, NULL);

    return ut;
}
