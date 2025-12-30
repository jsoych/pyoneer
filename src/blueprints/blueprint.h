#ifndef BLUEPRINT_H
#define BLUEPRINT_H

#include "job.h"
#include "project.h"

typedef enum {
    BLUEPRINT_JOB,
    BLUEPRINT_PROJECT
} blueprint_t;

// Blueprint
typedef struct Blueprint Blueprint;

void blueprint_destroy(Blueprint* blueprint);

Blueprint* blueprint_decode(blueprint_t kind, const json_value* obj);

#endif
