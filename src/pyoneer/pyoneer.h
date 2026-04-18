// pyoneer.h — Root Pyoneer object.
//
// Pyoneer is the top-level facade for the system. It represents either a Worker
// or a Manager instance (selected at creation time) and exposes a unified
// command/signal interface via function pointers.
//
// Design:
// - role selects the active backend: Worker or Manager.
// - the active backend is stored in a tagged union (pyoneer->as.*).
// - commands return JSON values; signals are asynchronous and return void.
//
// Thread safety:
// - Pyoneer methods are thread-safe if the underlying role implementation
//   (Worker/Manager) is thread-safe.
//
// Ownership:
// - Pyoneer owns the active Worker/Manager instance.
// - JSON values returned by commands are newly allocated and must be freed
//   by the caller (json_builder_free).

#ifndef PYONEER_H
#define PYONEER_H

#include "blueprint/blueprint.h"
#include "shared/shared.h"
#include "manager.h"
#include "worker.h"

#define PYONEER_MAX_ID 4096

// Role codes for selecting the active backend implementation.
enum
{
    PYONEER_WORKER,
    PYONEER_MANAGER
};

struct Pyoneer;

// Command function types.
// Commands return a JSON value describing status/results.
// Signals are asynchronous control actions (no JSON response).
typedef json_value *(*command)(struct Pyoneer *);
typedef json_value *(*command_blueprint)(struct Pyoneer *, Blueprint *);
typedef void (*command_signal)(struct Pyoneer *);

// Pyoneer is the root object for the system.
// Create with pyoneer_create() and destroy with pyoneer_destroy().
typedef struct Pyoneer
{
    int role;

    // Active backend implementation (selected by role).
    union
    {
        Worker *worker;
        Manager *manager;
    } as;

    // Commands (synchronized)
    command get_status;
    command_blueprint run;
    command_blueprint assign;

    // Signals (asynchronous)
    command_signal restart;
    command_signal start;
    command_signal stop;

} Pyoneer;

// pyoneer_create constructs a Pyoneer instance for the given role and id.
// Returns a new Pyoneer on success; NULL on error.
//
// Preconditions:
// - role must be one of PYONEER_WORKER or PYONEER_MANAGER.
// - id must be in the range [1, PYONEER_MAX_ID].
Pyoneer *pyoneer_create(int role, int id);

// pyoneer_destroy stops any running work and frees all owned resources.
// Safe to call with NULL.
void pyoneer_destroy(Pyoneer *pyoneer);

// pyoneer_blueprint_decode decodes a Blueprint from a JSON request object.
// Returns a Blueprint on success; NULL on error.
//
// Ownership:
// - Caller owns the returned Blueprint and must destroy it.
Blueprint *pyoneer_blueprint_decode(Pyoneer *pyoneer, const json_value *val);

#endif
