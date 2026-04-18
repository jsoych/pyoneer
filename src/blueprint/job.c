#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "job.h"

struct Job
{
    int id;
    int status;
    int size;

    job_opts_t opts;

    job_node *head;
    job_node *tail;
};

struct JobRunner
{
    Job *job;
    Site *job_site;

    pthread_t job_tid;

    job_runner_info_t info;

    int task_count;
    int task_capacity;
    TaskRunner *task_runners[];
};

/* job_create: Creates a new job and returns it, if the id is positive
    (id > 0). Otherwise, returns NULL. */
Job *job_create(int id)
{
    if (id <= 0)
        return NULL;

    // Create new job
    Job *job = malloc(sizeof(Job));
    if (!job)
    {
        perror("job_create: malloc");
        return NULL;
    }

    job->id = id;
    job->status = JOB_READY;
    job->size = 0;
    job->opts = job_opts_default();
    job->head = NULL;
    job->tail = NULL;
    return job;
}

/* job_destroy: Frees the memory allocated to the job. */
void job_destroy(Job *job)
{
    if (!job)
        return;
    job_node *curr = job->head;
    while (curr)
    {
        job_node *prev = curr;
        curr = curr->next;
        task_destroy(prev->task);
        free(prev);
    }
    free(job);
}

/* update_status: Derives job status from the statuses of all its tasks.
 *
 * Priority order:
 *   RUNNING     if any task is running
 *   NOT_READY   if any task is not ready
 *   INCOMPLETE  if any task is incomplete
 *   READY       if any task is ready
 *   COMPLETED   otherwise (all tasks completed)
 */
static void update_status(Job *job)
{
    if (job->size == 0)
        return;

    // Count each task state
    int status[5] = {0};
    job_node *curr = job->head;
    while (curr)
    {
        switch (task_get_status(curr->task))
        {
        case TASK_NOT_READY:
            status[TASK_NOT_READY]++;
            break;
        case TASK_READY:
            status[TASK_READY]++;
            break;
        case TASK_RUNNING:
            status[TASK_RUNNING]++;
            break;
        case TASK_COMPLETED:
            status[TASK_COMPLETED]++;
            break;
        case TASK_INCOMPLETE:
            status[TASK_INCOMPLETE]++;
            break;
        }
        curr = curr->next;
    }

    job->status = (status[TASK_RUNNING] > 0) ? JOB_RUNNING : (status[TASK_NOT_READY] > 0) ? JOB_NOT_READY
                                                         : (status[TASK_INCOMPLETE] > 0)  ? JOB_INCOMPLETE
                                                         : (status[TASK_READY] > 0)       ? JOB_READY
                                                                                          :
                                                                                          /* all tasks are completed */ JOB_COMPLETED;
}

/* job_get_id: Gets the id and returns it. */
int job_get_id(Job *job) { return job->id; }

/* job_get_size: Gets the size and returns it. */
int job_get_size(Job *job) { return job->size; }

/* job_get_status: Updates the job status and returns it. */
int job_get_status(Job *job)
{
    update_status(job);
    return job->status;
}

/* job_get_tasks: Gets the tasks and returns the head of the linked-list. */
job_node *job_get_tasks(Job *job) { return job->head; }

/* job_set_opts: Sets the run and execute options. */
void job_set_opts(Job *job, const job_opts_t *opts)
{
    job_opts_t o = job_opts_default();

    if (opts)
    {
        size_t n = opts->size < sizeof(o) ? opts->size : sizeof(o);
        memcpy(&o, opts, n);
        o.size = sizeof(o);
    }

    job->opts = o;
}

/* job_add_task: Adds the task to the job and returns 0, if the task is
    successfully added to the job. Otherwise, returns -1. */
int job_add_task(Job *job, Task *task)
{
    job_node *node = malloc(sizeof(job_node));
    if (!node)
    {
        perror("job_add_task: malloc");
        return -1;
    }
    node->task = task;
    node->next = NULL;
    if (job->size++ == 0)
    {
        job->head = node;
        job->tail = node;
        return 0;
    }
    job->tail->next = node;
    job->tail = node;
    return 0;
}

/* job_status_map: Maps job status codes to its corresponding
    JSON value. */
json_value *job_status_map(int status)
{
    switch (status)
    {
    case JOB_NOT_READY:
        return json_string_new("not_ready");
    case JOB_READY:
        return json_string_new("ready");
    case JOB_RUNNING:
        return json_string_new("running");
    case JOB_COMPLETED:
        return json_string_new("completed");
    case JOB_INCOMPLETE:
        return json_string_new("incomplete");
    default:
        return json_null_new();
    }
}

/* job_exec_mode_map: Maps execution mode codes to a string. */
const char *job_exec_mode_map(job_exec_mode_t mode)
{
    switch (mode)
    {
    case JOB_EXEC_PARALLEL:
        return "parallel";
    case JOB_EXEC_SEQUENTIAL:
        return "sequential";
    default:
        return "";
    }
}

/* job_run_mode_map: Maps run mode codes to a string. */
const char *job_run_mode_map(job_run_mode_t mode)
{
    switch (mode)
    {
    case JOB_RUN_ONCE:
        return "once";
    case JOB_RUN_REPEAT:
        return "repeat";
    default:
        return "";
    }
}

/* job_encode_status: Encode the job status. */
json_value *job_encode_status(Job *job)
{
    json_value *obj = json_object_new(0);
    if (!obj)
        return NULL;
    json_value *status = job_status_map(job_get_status(job));
    json_object_push(obj, "status", status);
    json_value *tasks = json_array_new(job->size);
    job_node *curr = job->head;
    while (curr)
    {
        json_value *task_status = task_encode_status(curr->task);
        json_object_push_string(task_status, "name",
                                task_get_name(curr->task));
        json_array_push(tasks, task_status);
        curr = curr->next;
    }
    json_object_push(obj, "tasks", tasks);
    return obj;
}

/* job_encode: Encodes the job as a JSON object. */
json_value *job_encode(const Job *job)
{
    json_value *obj = json_object_new(0);
    if (!obj)
        return NULL;
    json_object_push_integer(obj, "id", job->id);

    // Add options
    const char *mode = job_exec_mode_map(job->opts.exec_mode);
    json_object_push_string(obj, "execMode", mode);
    mode = job_run_mode_map(job->opts.run_mode);
    json_object_push_string(obj, "runMode", mode);

    // Add tasks
    json_value *arr = json_array_new(0);
    job_node *curr = job->head;
    while (curr)
    {
        json_value *val = task_encode(curr->task);
        json_array_push(arr, val);
        curr = curr->next;
    }
    json_object_push(obj, "tasks", arr);

    return obj;
}

/* job_decode: Decodes the JSON object into a new job and returns it.
    Otherwise, returns NULL. */
Job *job_decode(const json_value *obj)
{
    if (!obj || obj->type != json_object)
        return NULL;

    json_value *id = json_object_get_value(obj, "id");
    json_value *tasks = json_object_get_value(obj, "tasks");
    if (!id || !tasks)
        return NULL;
    if (id->type != json_integer || tasks->type != json_array)
        return NULL;

    Job *job = job_create(id->u.integer);
    if (!job)
        return NULL;

    job_opts_t opts = job_opts_default();

    json_value *v = json_object_get_value(obj, "execMode");
    if (v && v->type == json_string)
    {
        const char *s = v->u.string.ptr;
        if (strcmp(s, "parallel") == 0)
            opts.exec_mode = JOB_EXEC_PARALLEL;
        else if (strcmp(s, "sequential") == 0)
            opts.exec_mode = JOB_EXEC_SEQUENTIAL;
    }

    v = json_object_get_value(obj, "runMode");
    if (v && v->type == json_string)
    {
        const char *s = v->u.string.ptr;
        if (strcmp(s, "repeat") == 0)
            opts.run_mode = JOB_RUN_REPEAT;
        else if (strcmp(s, "once") == 0)
            opts.run_mode = JOB_RUN_ONCE;
    }

    job_set_opts(job, &opts);

    for (unsigned int i = 0; i < tasks->u.array.length; i++)
    {
        Task *task = task_decode(tasks->u.array.values[i]);
        if (!task)
        {
            job_destroy(job);
            return NULL;
        }
        if (job_add_task(job, task) == -1)
        {
            task_destroy(task);
            job_destroy(job);
            return NULL;
        }
    }

    return job;
}

static void reset_status(Job *job)
{
    job_node *curr = job->head;
    while (curr)
    {
        task_set_status(curr->task, TASK_READY);
        curr = curr->next;
    }
}

static void job_runner_handler(void *arg)
{
    JobRunner *runner = (JobRunner *)arg;
    for (int i = 0; i < runner->task_count; i++)
        task_runner_destroy(runner->task_runners[i]);
}

static void *job_thread_parallel(void *arg)
{
    JobRunner *runner = (JobRunner *)arg;
    bool repeat = runner->job->opts.run_mode == JOB_RUN_REPEAT;

    do
    {
        runner->info.attempts++;
        runner->task_count = 0;
        bool completion = true;

        // Run Tasks
        pthread_cleanup_push(job_runner_handler, runner);
        for (job_node *curr = runner->job->head; curr; curr = curr->next)
        {
            int status = task_get_status(curr->task);
            if (status == TASK_READY || status == TASK_INCOMPLETE)
            {
                TaskRunner *task_runner = task_run(curr->task, runner->job_site);
                if (!task_runner)
                {
                    completion = false;
                    repeat = false;
                    break;
                }
                runner->task_runners[runner->task_count++] = task_runner;
            }

            // Inconsistent state
            if (status == TASK_NOT_READY || status == TASK_RUNNING)
            {
                completion = false;
                repeat = false;
                break;
            }
        }

        // Wait for tasks and cleanup task runners
        for (int i = 0; i < runner->task_count; i++)
            task_runner_wait(runner->task_runners[i]);
        pthread_cleanup_pop(1);

        // Update info
        if (completion)
            runner->info.completions++;

        if (completion && job_get_status(runner->job) == JOB_COMPLETED)
            runner->info.successes++;

        if (completion && repeat)
            reset_status(runner->job);
    } while (repeat);

    pthread_exit(NULL);
}

static void task_runner_handler(void *arg)
{
    TaskRunner *runner = (TaskRunner *)arg;
    task_runner_destroy(runner);
}

static void *job_thread_sequential(void *arg)
{
    JobRunner *runner = (JobRunner *)arg;
    bool repeat = runner->job->opts.run_mode == JOB_RUN_REPEAT;

    do
    {
        runner->info.attempts++;
        runner->task_count = 0;
        bool completion = true;

        // Run tasks
        for (job_node *curr = runner->job->head; curr; curr = curr->next)
        {
            int status = task_get_status(curr->task);
            if (status == TASK_READY || status == TASK_INCOMPLETE)
            {
                TaskRunner *task_runner = task_run(curr->task, runner->job_site);
                if (!task_runner)
                {
                    completion = false;
                    repeat = false;
                    break;
                }
                pthread_cleanup_push(task_runner_handler, task_runner);
                task_runner_wait(task_runner);
                pthread_cleanup_pop(1);
            }

            // Inconsistent state
            if (status == TASK_NOT_READY || status == TASK_RUNNING)
            {
                completion = false;
                repeat = false;
                break;
            }
        }

        if (completion)
            runner->info.completions++;

        if (completion && job_get_status(runner->job) == JOB_COMPLETED)
            runner->info.successes++;

        if (completion && repeat)
            reset_status(runner->job);
    } while (repeat);

    pthread_exit(NULL);
}

static int job_thread_create(JobRunner *runner)
{
    int err;
    int mode = runner->job->opts.exec_mode;

    switch (mode)
    {
    case JOB_EXEC_PARALLEL:
        err = pthread_create(&runner->job_tid, NULL,
                             job_thread_parallel, runner);
        break;
    case JOB_EXEC_SEQUENTIAL:
        err = pthread_create(&runner->job_tid, NULL,
                             job_thread_sequential, runner);
        break;
    default:
        fprintf(stderr,
                "job_thread_create: unexpected exec mode (%d)\n", mode);
        return EINVAL;
    }

    if (err != 0)
    {
        fprintf(stderr,
                "job_thread_create: pthread_create: %s\n",
                strerror(err));
    }

    return err;
}

/* job_run_opts_default: Returns the default run options. */
job_opts_t job_opts_default()
{
    return (job_opts_t){
        .size = sizeof(job_opts_t),
        .exec_mode = JOB_EXEC_SEQUENTIAL,
        .run_mode = JOB_RUN_ONCE};
}

/* job_runner_info_defaults: Returns the default runner info. */
static job_runner_info_t job_runner_info_defaults()
{
    return (job_runner_info_t){
        .size = sizeof(job_runner_info_t),
        .attempts = 0,
        .completions = 0,
        .successes = 0};
}

/* job_run: Runs the job and returns a new job runner. */
JobRunner *job_run(Job *job, Site *site)
{
    if (!site || job->size == 0)
        return NULL;
    JobRunner *runner = malloc(sizeof(JobRunner) +
                               (job->size) * sizeof(TaskRunner *));
    if (!runner)
    {
        perror("job_run: malloc");
        return NULL;
    }
    runner->job = job;
    runner->job_site = site;

    runner->info = job_runner_info_defaults();

    runner->task_count = 0;
    runner->task_capacity = job->size;

    // Create job runner
    int err = job_thread_create(runner);
    if (err != 0)
    {
        free(runner);
        return NULL;
    }
    return runner;
}

/* job_runner_get_info: Gets the runner info. */
job_runner_info_t job_runner_get_info(JobRunner *runner)
{
    return runner->info;
}

/* job_runner_start: Starts the job. */
void job_runner_restart(JobRunner *runner)
{
    pthread_cancel(runner->job_tid);
    pthread_join(runner->job_tid, NULL);
    (void)job_thread_create(runner);
}

/* job_runner_stop: Stops the job. */
void job_runner_stop(JobRunner *runner)
{
    pthread_cancel(runner->job_tid);
    pthread_join(runner->job_tid, NULL);
}

/* job_runner_wait: Waits for the job to finish. */
void job_runner_wait(JobRunner *runner)
{
    pthread_join(runner->job_tid, NULL);
}

/* job_runner_destroy: Destroys the job runner and releases all of its
    resources. */
void job_runner_destroy(JobRunner *runner)
{
    if (!runner)
        return;
    job_runner_stop(runner);
    free(runner);
}
