// worker.h — Worker object responsible for executing Jobs.
//
// Worker owns an Engine instance and optionally an assigned Job. It exposes a
// small command-style API (get_status, assign, run) and a signal-style API
// (start, stop, restart).
//
// Thread safety:
// - All public functions are thread-safe.
// - Commands are synchronized: they serialize access and return a JSON result.
// - Signals are asynchronous: they request a state transition and return quickly.
//   Signals may take effect after the call returns.
//
// Ownership:
// - Worker owns its Engine for its lifetime.
// - Jobs passed to worker_assign/worker_run become owned by the Worker.
// - JSON values returned by worker_* commands are newly allocated and must be
//   freed by the caller (jsob_free/json_builder_free).

#ifndef WORKER_H
#define WORKER_H

#include "blueprint/job.h"
#include "shared/shared.h"

// Worker status codes.
enum
{
    WORKER_NOT_ASSIGNED,
    WORKER_ASSIGNED,
    WORKER_NOT_WORKING,
    WORKER_WORKING
};

// Worker is an opaque object that manages a single Engine and assigned Job.
// Created with worker_create() and destroyed with worker_destroy().
typedef struct Worker Worker;

// worker_create allocates a new Worker with the given id.
// Returns a new Worker on success; NULL on error.
Worker *worker_create(int id);

// worker_destroy stops any running work and frees all owned resources.
// Safe to call with NULL.
void worker_destroy(Worker *worker);

// worker_get_id gets the worker id and returns it.
int worker_get_id(Worker *worker);

// Commands (synchronized)

// worker_get_status returns a JSON object describing the worker state.
// The returned json_value* is caller-owned and must be freed.
json_value *worker_get_status(Worker *worker);

// worker_run starts executing the given job immediately.
// On success, Worker takes ownership of job and returns a JSON status object.
// Returns NULL on failure (e.g. worker already running).
json_value *worker_run(Worker *worker, Job *job);

// worker_assign assigns a job to the worker without starting the job.
// On success, Worker takes ownership of job and returns a JSON status object.
// Returns NULL on failure (e.g. worker is currently running).
json_value *worker_assign(Worker *worker, Job *job);

// Signals (asynchronous)

// worker_restart requests that the worker restart execution.
// This call is asynchronous and returns immediately.
void worker_restart(Worker *worker);

// worker_start requests that the worker start the assigned job.
// This call is asynchronous and returns immediately.
void worker_start(Worker *worker);

// worker_stop requests that the worker stop the currently running job.
// This call is asynchronous and returns immediately.
void worker_stop(Worker *worker);

// Helpers

// worker_status_map encodes a worker status code as a JSON string.
// The returned json_value* is caller-owned and must be freed.
json_value *worker_status_map(int status);

// worker_status_decode decodes a JSON string value into a worker status code.
// Returns the decoded status code; -1 if invalid/unknown.
int worker_status_decode(const json_value *value);

#endif
