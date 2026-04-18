#include <stdio.h>
#include <stdlib.h>
#include "crew.h"
#include "blueprint/job.h"
#include "worker.h"

#define CREW_MAXLEN 512

// job structure
typedef struct crew_job
{
    int id;
    int status;
} crew_job;

// worker structure
typedef struct
{
    int id;
    int status;
    crew_job job;
} crew_worker;

// crew node
typedef struct crew_node
{
    crew_worker worker;
    struct crew_node *next;
    struct crew_node *prev;
    struct crew_node *next_free;
    struct crew_node *prev_free;
    pthread_t tid;
} crew_node;

// crew list
typedef struct crew_list
{
    crew_node *head;
    crew_node *tail;
    int len;
} crew_list;

// Crew object
struct Crew
{
    crew_list workers[CREW_MAXLEN];
    crew_list freelist;
    int len;
    pthread_mutex_t lock;
    pthread_t tid;
};
