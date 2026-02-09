// job.h — Job object and JobRunner execution handler.
//
// Job represents a collection of Tasks that may be executed either serially
// or in parallel. JobRunner is a handler object that executes a Job
// asynchronously using the configured Site runtime.
//
// Design:
// - Job owns a linked list of Tasks.
// - If parallel is true, runnable tasks may execute concurrently.
// - Status is derived from the statuses of the underlying tasks.
//
// Ownership:
// - Job owns all Tasks added via job_add_task() and destroys them in job_destroy().
// - JobRunner does not own Job or Site; they must outlive the runner.
// - JSON values returned by encode/status functions are newly allocated and must
//   be freed by the caller (json_builder_free).
//
// Thread safety:
// - Job and job_node are not internally synchronized.
// - JobRunner control functions are intended to be called by the owning
//   controller (e.g. Engine/Worker) with external synchronization.
// - Jobs are expected to be immutable once execution begins.

#ifndef JOB_H
#define JOB_H

#include "json.h"
#include "site.h"
#include "task.h"

typedef struct Job Job;

// Job status codes.
enum {
    JOB_NOT_READY,
    JOB_READY,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_INCOMPLETE
};

// job_node is the internal linked-list node type used to store tasks.
typedef struct job_node {
    Task* task;
    struct job_node* next;
} job_node;

typedef enum {
    JOB_EXEC_SEQUENTIAL,
    JOB_EXEC_PARALLEL
} job_exec_mode_t;

typedef enum {
    JOB_RUN_ONCE,
    JOB_RUN_REPEAT
} job_run_mode_t;

typedef struct {
    size_t size;
    job_exec_mode_t exec_mode;
    job_run_mode_t  run_mode;
} job_opts_t;

typedef struct JobRunner JobRunner;

typedef struct {
    size_t size;
    int attempts;
    int completions;
    int successes;
} job_runner_info_t;

// Constructors and destructor

// job_create allocates a new Job with the given id.
// If parallel is true, tasks may execute concurrently.
// Returns a new Job on success; NULL on error.
Job* job_create(int id);

// job_decode decodes a Job from a JSON object.
// Returns a new Job on success; NULL on error.
Job* job_decode(const json_value* obj);

// job_destroy frees a Job and all owned Tasks.
// Safe to call with NULL.
void job_destroy(Job* job);

// Methods

// job_get_id returns the job id.
int job_get_id(Job* job);

// job_get_status returns the current job status.
// The status is derived from task statuses and may change over time.
int job_get_status(Job* job);

// job_get_size returns the size for job (number of job tasks).
int job_get_size(Job* job);

// job_get_tasks returns the head to the list of job tasks.
job_node* job_get_tasks(Job* job);

// job_set_opts sets the job options. If opts is NULL, then the default
// job options are set.
void job_set_opts(Job* job, const job_opts_t* opts);

// job_add_task appends a Task to the Job.
// On success, Job takes ownership of task.
// Returns 0 on success; -1 on failure.
int job_add_task(Job* job, Task* task);

// job_encode encodes the Job into a JSON object.
// The returned json_value* is caller-owned and must be freed.
json_value* job_encode(const Job* job);

// job_encode_status encodes the current Job status into a JSON object,
// including per-task status information.
// The returned json_value* is caller-owned and must be freed.
json_value* job_encode_status(Job* job);

// job_run starts executing the Job asynchronously using the given site.
// Returns a JobRunner on success; NULL on error.
//
// Preconditions:
// - job and site must remain valid until the runner is destroyed.
// - job must contain at least one Task.
JobRunner* job_run(Job* job, Site* site);

// job_runner_get_info gets the JobRunner info.
job_runner_info_t job_runner_get_info(JobRunner* runner);

// JobRunner control (asynchronous)

// job_runner_restart cancels the running execution (if any) and restarts it.
void job_runner_restart(JobRunner* runner);

// job_runner_stop cancels the running execution (if any) and waits for cleanup.
void job_runner_stop(JobRunner* runner);

// job_runner_wait blocks until execution completes.
void job_runner_wait(JobRunner* runner);

// job_runner_destroy stops execution (if running) and frees runner resources.
// Safe to call with NULL.
void job_runner_destroy(JobRunner* runner);

// Helpers

// job_status_map encodes a job status code as a JSON string.
// The returned json_value* is caller-owned and must be freed.
json_value* job_status_map(int status);

// job_run_opts_default returns the default run options.
job_opts_t job_opts_default();

#endif
