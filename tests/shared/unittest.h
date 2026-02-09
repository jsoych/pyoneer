#ifndef UNITTEST_H
#define UNITTEST_H

#include "blueprint.h"
#include "json.h"

// Result macro
#define RETURN_RESULT(result) switch((result)) { \
    case 0: return UNITTEST_SUCCESS; \
    case 1: return UNITTEST_FAILURE; \
} \
return UNITTEST_ERROR

typedef enum {
    UNITTEST_SUCCESS,
    UNITTEST_FAILURE,
    UNITTEST_ERROR
} result_t;

typedef enum {
    CASE_NONE,
    CASE_INT,
    CASE_NUM,
    CASE_STR,
    CASE_JSON,
    CASE_TASK,
    CASE_JOB
} case_t;

typedef struct {
    int type;
    union {
        int integer;
        double number;
        char* string;
        json_value* json;
        Task* task;
        Job* job;
    } as;
    char name[];
} unittest_case;

typedef result_t (*unittest_test)(unittest_case*);

typedef struct Unittest Unittest;

Unittest* unittest_create(const char* name);
void unittest_destroy(Unittest* ut);

const char* unittest_get_name(Unittest* ut);

int unittest_add(Unittest* ut, const char* name, unittest_test tc, 
    case_t type, void* expected);
int unittest_run(Unittest* ut);

int unittest_compare_task(const Task* a, const Task* b);
int unittest_compare_job(const Job* a, const Job* b);

Job* unittest_job_create(int id, const char* task_name, int ntasks, 
    const char* bug_name, int nbugs);

#endif
