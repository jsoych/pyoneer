#ifndef _TEST_API_H
#define _TEST_API_H

#include "logger.h"
#include "pyoneer.h"
#include "server.h"
#include "suite.h"

#define FORMAT "test: %s\n"
#define WORKER_ID 1

Unittest* test_logger_create(const char* name);
Unittest* test_server_create(const char* name);

#endif
