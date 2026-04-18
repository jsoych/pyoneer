#ifndef BLUEPRINT_H
#define BLUEPRINT_H

#include "job.h"
#include "project.h"
#include "task.h"
#include "shared/shared.h"

typedef enum
{
    BLUEPRINT_JOB,
    BLUEPRINT_PROJECT
} blueprint_t;

typedef struct Blueprint
{
    union
    {
        Job *job;
        Project *project;
        Task *task;
    } as;
    blueprint_t kind;
} Blueprint;

void blueprint_destroy(Blueprint *blueprint);

Blueprint *blueprint_decode(blueprint_t kind, const json_value *obj);

#endif
