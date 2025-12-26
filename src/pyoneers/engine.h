#ifndef _ENGINE_H
#define _ENGINE_H

#include <pthread.h>

#include "job.h"

typedef struct _engine {
    JobRunner* job_runner;
} Engine;

Engine *engine_create(void);
void engine_destroy(Engine* engine);

int engine_run(Engine* engine, Job* job, Site* site);
void engine_restart(Engine* engine);
void engine_stop(Engine* engine);
void engine_wait(Engine* engine);

#endif
