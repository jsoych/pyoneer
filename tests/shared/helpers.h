#ifndef HELPER_H
#define HELPER_H

#include "pyoneer.h"

/* Creates a job with the given id, task and bug names. */
Job* create_job(
    int id,
    const char* task,
    int ntask,  // number of dummy tasks
    const char* bug,
    int nbug    // number of dummy task with bug
);

#endif
