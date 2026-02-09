#ifndef TEST_API_H
#define TEST_API_H

#include "logger.h"
#include "pyoneer.h"
#include "server.h"
#include "suite.h"

#define NAME "API"
#define WORKER_ID 1

Unittest* test_logger_create(const char* name);
Unittest* test_server_create(const char* name);

#endif
