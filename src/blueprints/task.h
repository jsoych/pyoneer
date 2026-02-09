// task.h — Task object and TaskRunner execution handler.
//
// Task represents a single executable unit (typically a script).
// TaskRunner is a handler object that runs a Task asynchronously using a
// background thread and a spawned child process.
//
// Ownership:
// - Task owns its name.
// - TaskRunner does not own Task or Site; they must outlive the runner.
// - JSON values returned by encode/status functions are newly allocated and
//   must be freed by the caller (json_builder_free).
//
// Thread safety:
// - Task is not internally synchronized.
// - TaskRunner control functions (restart/stop/wait/destroy) are intended to
//   be called by the owning controller (e.g. Engine/Worker) with external
//   synchronization.

#ifndef TASK_H
#define TASK_H

#include "json.h"
#include "site.h"

// Task status codes.
enum {
    TASK_NOT_READY,
    TASK_READY,
    TASK_RUNNING,
    TASK_COMPLETED,
    TASK_INCOMPLETE
};

typedef struct Task Task;

// Exit/Signal sentinels used to represent "no exit status recorded yet".
#define EXIT_SENTINAL 256
#define SIGSENT 64

typedef struct {
    int exit_code;
    int signo;
} task_runner_info_t;

typedef struct TaskRunner TaskRunner;

// task_create allocates a Task from a task name.
// Returns a new Task on success; NULL on error.
Task* task_create(const char* name);

// task_decode decodes a Task from a JSON object.
// Returns a new Task on success; NULL on error.
Task* task_decode(const json_value* obj);

// task_destroy frees a Task and its owned resources.
// Safe to call with NULL.
void task_destroy(Task* task);

// task_get_name returns the name of the task.
const char* task_get_name(Task* task);

// task_get_status returns the current status of the task.
int task_get_status(Task* task);

// task_set_status sets the status of the task.
void task_set_status(Task* task, int status);

// task_encode encodes a Task into a JSON object.
// The returned json_value* is caller-owned and must be freed.
json_value* task_encode(const Task* task);

// task_encode_status encodes the Task's current status into a JSON object.
// The returned json_value* is caller-owned and must be freed.
json_value* task_encode_status(Task* task);

// task_run starts executing the task asynchronously at the given site.
// Returns a TaskRunner on success; NULL on error.
//
// Preconditions:
// - task and site must remain valid until the runner is destroyed.
TaskRunner* task_run(Task* task, Site* site);

// task_runner_get_info gets the task runner info and return it.
task_runner_info_t task_runner_get_info(TaskRunner* runner);

// task_runner_restart cancels the running execution (if any) and
// restarts it.
void task_runner_restart(TaskRunner* runner);

// task_runner_stop cancels the running execution (if any) and waits
// for cleanup.
void task_runner_stop(TaskRunner* runner);

// task_runner_wait blocks until execution completes.
void task_runner_wait(TaskRunner* runner);

// task_runner_destroy stops execution (if running) and frees runner resources.
// Safe to call with NULL.
void task_runner_destroy(TaskRunner* runner);

// task_status_map encodes a task status code as a JSON string.
json_value* task_status_map(const int status);

#endif
