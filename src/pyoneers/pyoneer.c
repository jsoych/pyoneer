#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pyoneer.h"
#include "json-helpers.h"

/* pyoneer worker wrappers */
static json_value* pyoneer_worker_get_status(Pyoneer* pyoneer) {
    json_value* status = worker_get_status(pyoneer->as.worker);
    json_object_push_string(status, "role", "worker");
    json_object_push_integer(status, "id", pyoneer->as.worker->id);
    return status;
}

static json_value* pyoneer_run_job(Pyoneer* pyoneer, Blueprint* blueprint) {
    json_value* status = worker_run(pyoneer->as.worker, blueprint->as.job);
    json_object_push_string(status, "role", "worker");
    json_object_push_integer(status, "id", pyoneer->as.worker->id);
    return status;
}

static json_value* pyoneer_assign_job(Pyoneer* pyoneer, Blueprint* blueprint) {
    json_value* status = worker_assign(pyoneer->as.worker, blueprint->as.job);
    json_object_push_string(status, "role", "worker");
    json_object_push_integer(status, "id", pyoneer->as.worker->id);
    return status;
}

static void pyoneer_worker_restart(Pyoneer* pyoneer) {
    worker_restart(pyoneer->as.worker);
}

static void pyoneer_worker_stop(Pyoneer* pyoneer) {
    worker_stop(pyoneer->as.worker);
}

static void pyoneer_worker_start(Pyoneer* pyoneer) {
    worker_start(pyoneer->as.worker);
}

/* pyoneer manager wrappers */

/* pyoneer_create: Creates a new pyoneer. */
Pyoneer* pyoneer_create(int role, int id) {
    Pyoneer *pyoneer = malloc(sizeof(Pyoneer));
    if (pyoneer == NULL) {
        perror("pyoneer: pyoneer_create: malloc");
        return NULL;
    }
    switch (role) {
        case PYONEER_WORKER:
            pyoneer->role = PYONEER_WORKER;
            pyoneer->as.worker = worker_create(id);
            pyoneer->run = pyoneer_run_job;
            pyoneer->get_status = pyoneer_worker_get_status;
            pyoneer->assign = pyoneer_assign_job;
            pyoneer->restart = pyoneer_worker_restart;
            pyoneer->start = pyoneer_worker_start;
            pyoneer->stop = pyoneer_worker_stop;
            break;
    }
    return pyoneer;
}

/* pyoneer_destroy: Destroys the pyoneer. */
void pyoneer_destroy(Pyoneer* pyoneer) {
    if (!pyoneer) return;
    switch (pyoneer->role) {
        case PYONEER_WORKER:
            worker_destroy(pyoneer->as.worker);
            break;
    }
    free(pyoneer);
}

Blueprint* pyoneer_blueprint_decode(Pyoneer* pyoneer, const json_value* val) {
    if (!val) return NULL;
    if (val->type != json_object) return NULL;
    switch (pyoneer->role) {
        case PYONEER_WORKER: {
            json_value* obj = json_object_get_value(val, "job");
            if (!obj) return NULL;
            return blueprint_decode(BLUEPRINT_JOB, obj);
        }
    }
    return NULL;
}
