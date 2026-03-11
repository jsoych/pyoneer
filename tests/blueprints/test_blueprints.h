#ifndef TEST_BLUEPRINTS_H
#define TEST_BLUEPRINTS_H

#define TASK_NAME   "task.py"
#define BUFSIZE     1024
#define BUG_NAME    "bug.py"
#define WRITE_NAME  "write.py"
#define SHORT_NAME  "task.py"
#define LONG_NAME   "very_very_very_very_very_very_very_very_task_name.py"
#define VALID_ID    33
#define INVALID_ID  -13

#include "blueprint.h"
#include "shared.h"
#include <unittest/unittest.h>

/* Creates a suite of Task tests. */
Unittest* create_task_suite(const char* name);

/* Creates a suite of TaskRunner tests. */
Unittest* create_task_runner_suite(const char* name);

/* Creates a suite of Job tests. */
Unittest* create_job_suite(const char* name);

/* Creates a suite of JobRunner tests. */
Unittest* create_job_runner_suite(const char* name);

/* Creates a job with the given id, task and bug names. */
Job* create_job(
    int id,
    const char* task,
    int ntask, // number of dummy tasks
    const char* bug,
    int nbug // number of dummy task with bug
);

#endif
