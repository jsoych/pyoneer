// engine.h — Execution engine for running Jobs.
//
// Engine is responsible for executing a Job using the configured Site runtime.
// It manages the underlying execution mechanism (e.g., processes/threads),
// and exposes a small control API (run/restart/stop/wait).
//
// Thread safety:
// - All public functions are thread-safe.
// - Control functions (restart/stop) are asynchronous: they request a state
//   transition and may take effect after the call returns.
//
// Ownership:
// - Engine does not take ownership of Job or Site pointers passed to engine_run().
// - The caller must ensure Job and Site remain valid for the duration of execution.

#ifndef ENGINE_H
#define ENGINE_H

#include "blueprint/job.h"
#include "shared/site.h"

// Engine is an opaque object responsible for running a Job.
// Created with engine_create() and destroyed with engine_destroy().
typedef struct Engine Engine;

// engine_create allocates a new Engine instance.
// Returns a new Engine on success; NULL on error.
Engine *engine_create(void);

// engine_destroy stops any running execution and frees all owned resources.
// Safe to call with NULL.
void engine_destroy(Engine *engine);

// engine_run starts execution of job using the given site runtime.
// Returns 0 on success; -1 on failure.
//
// Preconditions:
// - job must be valid and remain alive until execution completes.
// - site must be valid and remain alive until execution completes.
// - Calling engine_run while a job is already running may fail.
int engine_run(Engine *engine, Job *job, Site *site);

// engine_restart requests that the Engine restart the current execution (if any).
// This call is asynchronous and returns immediately.
void engine_restart(Engine *engine);

// engine_stop requests that the Engine stop the current execution (if any).
// This call is asynchronous and returns immediately.
void engine_stop(Engine *engine);

// engine_wait blocks until the current execution completes (if any).
void engine_wait(Engine *engine);

#endif
