#ifndef _TEST_BLUEPRINTS_H
#define _TEST_BLUEPRINTS_H

#include "blueprint.h"
#include "unittest.h"

// Task names
#define TASK_NAME "task.py"
#define BUFSIZE 1024
#define BUG_NAME "bug.py"
#define WRITE_NAME "write.py"
#define SHORT_NAME "task.py"
#define LONG_NAME "very_very_very_very_very_very_very_very_task_name.py"

// Job ids
#define VALID_ID 33
#define INVALID_ID -13

Unittest* test_task_create(const char* name);
Unittest* test_task_runner_create(const char* name);
Unittest* test_job_create(const char* name);
Unittest* test_job_runner_create(const char* name);

#endif
