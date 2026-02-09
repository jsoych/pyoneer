#ifndef _TEST_PYONEERS_H
#define _TEST_PYONEERS_H

#include "pyoneer.h"
#include "suite.h"
#include "unittest.h"

#define BUFSIZE 4096
#define TASK_NAME "task.py"
#define BUG_NAME "bug.py"
#define JOB_ID 33
#define WORKER_ID 3

Unittest* test_engine_create(const char* name);
Unittest* test_worker_create(const char* name);
Unittest* test_pyoneer_create(const char* name);

#endif
