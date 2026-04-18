#ifndef CREW_H
#define CREW_H

#include <pthread.h>
#include "blueprint/job.h"
#include "shared/shared.h"
#include "worker.h"

typedef struct Crew Crew;

// Constructor and destructor
Crew *crew_create(void);
void crew_destroy(Crew *crew);

// Methods
int crew_add(Crew *crew, int id);
int crew_remove(Crew *crew, int id);
int crew_get_status(Crew *crew, int id);
int crew_get_job_status(Crew *crew, int id);
int crew_assign_job(Crew *crew, Job *job);
int crew_unassign(Crew *crew, int id);

void crew_send_command(Crew *crew, int id, const char *command);
void crew_broadcast(Crew *crew, const char *command);

#endif
