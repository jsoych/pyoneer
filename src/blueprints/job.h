#ifndef JOB_H
#define JOB_H

#include <stdbool.h>
#include <pthread.h>

#include "json.h"
#include "site.h"
#include "task.h"

enum {
    JOB_NOT_READY,
    JOB_READY,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_INCOMPLETE
};

typedef struct job_node {
    Task* task;
    struct _job_node *next;
} job_node;

typedef struct Job Job;

typedef struct JobRunner JobRunner;

// Constructors and destructor
Job* job_create(int id, bool parallel);
Job* job_decode(const json_value* obj);
void job_destroy(Job* job);

// Methods
int job_get_id(Job* job);
int job_get_status(Job* job);
json_value* job_encode(const Job* job);
json_value* job_encode_status(Job* job);
JobRunner* job_run(Job* job, Site* site);

void job_runner_restart(JobRunner* runner);
void job_runner_stop(JobRunner* runner);
void job_runner_wait(JobRunner* runner);
void job_runner_destroy(JobRunner* runner);

// Helpers
int job_add_task(Job* job, Task* task);
json_value* job_status_map(int status);

#endif
