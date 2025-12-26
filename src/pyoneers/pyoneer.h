#ifndef _PYONEER_H
#define _PYONEER_H

#include "json.h"
#include "worker.h"
#include "manager.h"
#include "blueprint.h"

#define PYONEER_MAX_ID 4096

enum {
    PYONEER_WORKER,
    PYONEER_MANAGER
};

struct _pyoneer;

typedef json_value* (*command)(struct _pyoneer*);
typedef json_value* (*command_blueprint)(struct _pyoneer*, Blueprint*);
typedef void (*command_signal)(struct _pyoneer*);

typedef struct _pyoneer {
    int role;
    union {
        Worker* worker;
        Manager* manager;
    } as;
    command get_status;
    command_blueprint run;
    command_blueprint assign;
    command_signal restart;
    command_signal start;
    command_signal stop;
} Pyoneer;

Pyoneer* pyoneer_create(int role, int id);
void pyoneer_destroy(Pyoneer* pyoneer);

Blueprint* pyoneer_blueprint_decode(Pyoneer* pyoneer, const json_value* val);

#endif
