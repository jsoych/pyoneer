#include <stdio.h>
#include <stdlib.h>

#include "blueprint.h"
#include "common.h"

typedef struct Blueprint {
    blueprint_t kind;
    union {
        Job* job;
        Project* project;
    } as;
} Blueprint;

/* blueprint_destroy: Destroys the blueprint and frees its resources. */
void blueprint_destroy(Blueprint* blueprint) {
    if (!blueprint) return;
    switch (blueprint->kind) {
        case BLUEPRINT_JOB:
            job_destroy(blueprint->as.job);
            break;
        default:
            fprintf(stderr, "blueprint_destroy: Warning: Unknown blueprint kind %d\n", blueprint->kind);
            break;
    }
}

/* blueprint_decode: Decodes the JSON object and returns a blueprint. */
Blueprint* blueprint_decode(blueprint_t kind, const json_value* obj) {
    Blueprint* blueprint = malloc(sizeof(Blueprint));
    if (!blueprint) {
        perror("blueprint_decode: malloc");
        return NULL;
    }

    // Create blueprint
    switch (kind) {
        case BLUEPRINT_JOB:
            blueprint->kind = kind;
            blueprint->as.job = job_decode(obj);
            if (!blueprint->as.job) {
                free(blueprint);
                return NULL;
            }
            break;
        default:
            free(blueprint);
            return NULL;
    }

    return blueprint;
}
