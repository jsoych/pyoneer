#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <unistd.h>

#include "task.h"
#include "json-builder.h"
#include "json-helpers.h"

extern char** environ;

/* task_create: Creates a new task. */
Task* task_create(const char* name) {
    int len = strlen(name);
    if (len == 0) return NULL;

    Task* task = malloc(sizeof(Task));
    if (!task) {
        perror("task_create: malloc");
        return NULL;
    }
    task->status = TASK_READY;
    task->exit_code = EXIT_SENTINAL;
    task->exit_signo = SIGSENT;
    task->name = strdup(name);
    return task;
}

/* task_destroy: Frees the memory allocated to the task. */
void task_destroy(Task *task) {
    if (!task) return;
    free(task->name);
    free(task);
}

/* update_status: Updates the task status. */
static void update_status(Task* task) {
    switch (task->exit_code) {
        case EXIT_SENTINAL:
            break;
        case EXIT_SYSCALL:
            task->status = TASK_READY;
            return;
        case EXIT_SUCCESS:
            task->status = TASK_COMPLETED;
            return;
        case EXIT_FAILURE:
            task->status = TASK_NOT_READY;
            return;
    }

    switch (task->exit_signo) {
        case SIGINT:
            task->status = TASK_INCOMPLETE;
            return;
    }
}

/* task_get_status: Get the task status and returns it. */
int task_get_status(Task* task) {
    update_status(task);
    return task->status;
}

static void task_runner_handler(void* arg) {
    TaskRunner* runner = (TaskRunner*)arg;
    int status;
    kill(runner->task_pid, SIGINT);
    waitpid(runner->task_pid, &status, 0);
    if (WIFEXITED(status)) {
        runner->task->exit_code = WEXITSTATUS(status);
        runner->task->exit_signo = SIGSENT;
    }
    else if (WIFSIGNALED(status)) {
        runner->task->exit_code = EXIT_SENTINAL;
        runner->task->exit_signo = WTERMSIG(status);
    }
}

/* task_runner_thread: Runs the task and waits for it to complete. */
static void* task_runner_thread(void* arg) {
    TaskRunner* runner = (TaskRunner*)arg;
    Task* task = runner->task;
    Site* site = runner->site;

    int len = strlen(site->task_dir) + strlen(task->name) + 1;
    char task_path[len + 1];
    sprintf(task_path, "%s/%s", site->task_dir, task->name);
    char* argv[] = {site->python, task_path, NULL};

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addchdir_np(&actions, site->working_dir);
    if (posix_spawn(&runner->task_pid, site->python, &actions, NULL,
        argv, environ) == -1) {
        perror("task_thread: posix_spawn");
        posix_spawn_file_actions_destroy(&actions);
        pthread_exit(NULL);
    }
    task->status = TASK_RUNNING;
    posix_spawn_file_actions_destroy(&actions);

    // Wait for task to complete
    pthread_cleanup_push(task_runner_handler, runner);
    int status;
    waitpid(runner->task_pid, &status, 0);
    if (WIFEXITED(status)) {
        runner->task->exit_code = WEXITSTATUS(status);
        runner->task->exit_signo = SIGSENT;
    }
    else if (WIFSIGNALED(status)) {
        runner->task->exit_code = EXIT_SENTINAL;
        runner->task->exit_signo = WTERMSIG(status);
    }
    pthread_cleanup_pop(0);
    pthread_exit(NULL);
}

/* task_run: Runs the task as a separated process and dedirects the task
    outputs to stdout and return its exit code. Otherwise, returns -1. */
TaskRunner* task_run(Task* task, Site* site) {
    if (!task || !site) return NULL;
    TaskRunner* runner = malloc(sizeof(TaskRunner));
    if (!runner) {
        perror("task_run: malloc");
        return NULL;
    }
    runner->task = task;
    runner->site = site;
    
    // Create task thread
    int err = pthread_create(&runner->task_thread, NULL,
        task_runner_thread, runner);
    if (err != 0) {
        fprintf(stderr, "task_run: pthread_create: %s\n", strerror(err));
        free(runner);
        return NULL;
    }
    return runner;
}

/* task_runner_start: Starts the task runner. */
void task_runner_start(TaskRunner* runner) {
    int err = pthread_create(&runner->task_thread, NULL,
        task_runner_thread, runner);
    if (err != 0) {
        fprintf(stderr, "task_start: pthread_create: %s\n", strerror(err));
        return;
    }
}

/* task_runner_stop: Stops the task runner. */
void task_runner_stop(TaskRunner* runner) {
    pthread_cancel(runner->task_thread);
    pthread_join(runner->task_thread, NULL);
}

/* task_runner_wait: Waits for a task runner to finish executing, cleans up, and
    sets its to exit code to the code of the exiting process. */
void task_runner_wait(TaskRunner* runner) {
    pthread_join(runner->task_thread, NULL);
}

/* task_runner_destroy: Frees the task resources. */
void task_runner_destroy(TaskRunner* runner) {
    if (!runner) return;
    task_runner_stop(runner);
    free(runner);
}

/* task_encode: Encodes the task into a JSON object. */
json_value* task_encode(const Task* task) {
    json_value* obj = json_object_new(0);
    if (!obj) return NULL;
    json_object_push_string(obj, "name", task->name);
    return obj;
}

/* task_status_map: Maps the task status code into a JSON value. */
json_value* task_status_map(const int status) {
    switch (status) {
        case TASK_NOT_READY:    return json_string_new("not_ready");
        case TASK_READY:        return json_string_new("ready");
        case TASK_RUNNING:      return json_string_new("running");
        case TASK_COMPLETED:    return json_string_new("completed");
        case TASK_INCOMPLETE:   return json_string_new("incomplete");
        default:                return json_null_new();
    }
}

/* task_encode_status: Encode the task status and returns it. If the task
    is not ready, it includes its exit code in addition.  */
json_value* task_encode_status(Task* task) {
    json_value* obj = json_object_new(0);
    if (!obj) return NULL;
    json_value* status = task_status_map(task_get_status(task));
    json_object_push(obj, "status", status);
    return obj;
}

/* task_decode: Decodes the object into a new task. */
Task* task_decode(const json_value* obj) {
    if (!obj) return NULL;
    if (obj->type != json_object) return NULL;
    json_value* name = json_object_get_value(obj, "name");
    if (name == NULL) return NULL;
    if (name->type != json_string) return NULL;
    return task_create(name->u.string.ptr);
}
