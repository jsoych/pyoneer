#ifndef TEST_PYONEERS_H
#define TEST_PYONEERS_H

#include "pyoneer.h"
#include <unittest/unittest.h>

#define BUFSIZE     4096
#define TASK_NAME   "task.py"
#define BUG_NAME    "bug.py"
#define JOB_ID      33
#define WORKER_ID   3

/* Creates a Suite of Enginer Tests. */
Unittest* suite_engine_create(const char* name);

/* Creates a Suite of Worker Tests. */
Unittest* suite_worker_create(const char* name);

/* Creates a Suite of Pyoneer Tests. */
Unittest* suite_pyoneer_create(const char* name);

#endif
