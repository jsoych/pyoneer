#ifndef PROJECT_H
#define PROJECT_H

#include "shared/shared.h"

enum
{
    PROJECT_NOT_READY,
    PROJECT_READY,
    PROJECT_RUNNING,
    PROJECT_COMPLETED,
    PROJECT_INCOMPLETE
};

typedef struct Project Project;
typedef struct ProjectRunner ProjectRunner;

// Construtor and destructor
Project *project_create(int id);
Project *project_decode(const json_value *obj);
void project_destroy(Project *project);

json_value *project_encode(Project *project);
json_value *project_encode_status(Project *project);
int project_get_status(Project *project);

ProjectRunner *project_run(Project *project);

void project_runner_restart(ProjectRunner *runner);
void project_runner_stop(ProjectRunner *runner);
void project_runner_wait(ProjectRunner *runner);

int project_add(Project *project, Job *job, int *deps, int ndeps);
int project_remove(Project *project, int id);

#endif
