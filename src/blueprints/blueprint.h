#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include "job.h"
#include "project.h"

enum {
    BLUEPRINT_JOB,
    BLUEPRINT_PROJECT
};

// Blueprint
typedef struct _blueprint {
    int kind;
    union {
        Job* job;
        Project* project;
    } as;
} Blueprint;

void blueprint_destroy(Blueprint* blueprint);

Blueprint* blueprint_decode(int kind, const json_value* obj);

#endif
