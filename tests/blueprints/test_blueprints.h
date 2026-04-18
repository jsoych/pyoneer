#ifndef TEST_BLUEPRINTS_H
#define TEST_BLUEPRINTS_H

#include "blueprint.h"
#include "shared.h"
#include <unittest/unittest.h>

/* Creates a suite of Task tests. */
Unittest *create_task_suite(const char *name);

/* Creates a suite of TaskRunner tests. */
Unittest *create_task_runner_suite(const char *name);

/* Creates a suite of Job tests. */
Unittest *create_job_suite(const char *name);

/* Creates a suite of JobRunner tests. */
Unittest *create_job_runner_suite(const char *name);

/* Creates a job with the given id, task and bug names. */
Job *create_job(
    int id,
    const char *task,
    int ntask, // number of dummy tasks
    const char *bug,
    int nbug // number of dummy task with bug
);

#endif
