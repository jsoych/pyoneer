#include "helpers.h"

Job *create_job(
    int id,
    const char *task,
    int ntask,
    const char *bug,
    int nbug)
{
    Job *job = job_create(id);
    if (!job)
    {
        return NULL;
    }

    /* Add tasks */
    for (int i = 0; i < ntask; i++)
    {
        if (job_add_task(job, task) == -1)
        {
            job_destroy(job);
            return NULL;
        }
    }

    /* Add bugs */
    for (int i = 0; i < nbug; i++)
    {
        if (job_add_task(job, bug) == -1)
        {
            job_destroy(job);
            return NULL;
        }
    }
    return job;
}
