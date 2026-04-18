#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "engine.h"

struct Engine {
    JobRunner* job_runner;
};

/* engine_create: Creates a new engine. */
Engine* engine_create(void) {
    Engine* engine = malloc(sizeof(Engine));
    if (!engine) {
        perror("engine_create: malloc");
        return NULL;
    }  
    engine->job_runner = NULL;
    return engine;
}

/* engine_destroy: Destroys the engine and releases all of its resources. */
void engine_destroy(Engine* engine) {
    if (!engine) return;
    engine_stop(engine);
    free(engine);
}

/* engine_run: Runs the job at the engine job site. */
int engine_run(Engine* engine, Job* job, Site* site) {
    JobRunner* runner = job_run(job, site);
    if (!runner) return -1;

    // Destroy old job runner
    job_runner_destroy(engine->job_runner);
    engine->job_runner = runner;
    return 0;
}

/* engine_restart: Restarts the engine. */
void engine_restart(Engine* engine) {
    if (!engine->job_runner) return;
    job_runner_restart(engine->job_runner);
}

/* engine_stop: Stops the engine. */
void engine_stop(Engine* engine) {
    if (!engine->job_runner) return;
    job_runner_stop(engine->job_runner);
}

/* engine_wait: Waits for the job to complete. */
void engine_wait(Engine* engine) {
    if (!engine->job_runner) return;
    job_runner_wait(engine->job_runner);
}
