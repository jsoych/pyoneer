#ifndef BLUEPRINT_H
#define BLUEPRINT_H

#include "job.h"
#include "project.h"

typedef enum {
    BLUEPRINT_JOB,
    BLUEPRINT_PROJECT
} blueprint_t;

typedef struct Blueprint {
    blueprint_t kind;
    union {
        Job* job;
        Project* project;
    } as;
} Blueprint;

void blueprint_destroy(Blueprint* blueprint);

Blueprint* blueprint_decode(blueprint_t kind, const json_value* obj);

#endif
