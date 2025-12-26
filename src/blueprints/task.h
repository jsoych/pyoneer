#ifndef _TASK_H
#define _TASK_H

#define EXIT_SYSCALL 255
#define EXIT_SENTINAL 256
#define SIGSENT 64

#include <pthread.h>
#include <unistd.h>
#include "json.h"
#include "site.h"

enum {
    TASK_NOT_READY,
    TASK_READY,
    TASK_RUNNING,
    TASK_COMPLETED,
    TASK_INCOMPLETE
};

typedef struct _task {
    int status;
    int exit_code;
    int exit_signo;
    char* name;
} Task;

typedef struct _task_runner {
    Task* task;
    Site* site;
    pthread_t task_thread;
    pid_t task_pid;
} TaskRunner;

// Construtor and destructor
Task* task_create(const char* name);
Task* task_decode(const json_value* obj);
void task_destroy(Task* task);

int task_get_status(Task* task);
json_value* task_encode(const Task* task);
json_value* task_encode_status(Task* task);
TaskRunner* task_run(Task* task, Site* site);

void task_runner_restart(TaskRunner* runner);
void task_runner_stop(TaskRunner* runner);
void task_runner_wait(TaskRunner* runner);
void task_runner_destroy(TaskRunner* runner);

// Helpers
json_value* task_status_map(const int status);

#endif
