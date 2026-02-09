#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <unistd.h>

#include "task.h"
#include "json-builder.h"
#include "json-helpers.h"

extern char** environ;

struct Task {
    int status;
    char* name;
};

struct TaskRunner {
    Task* task;
    Site* site;

    task_runner_info_t info;

    pthread_t task_thread;
    pid_t task_tid;
};

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
    task->name = strdup(name);
    return task;
}

/* task_destroy: Frees the memory allocated to the task. */
void task_destroy(Task *task) {
    if (!task) return;
    free(task->name);
    free(task);
}

/* task_get_name: Gets the name of the task. */
const char* task_get_name(Task* task) {
    if (!task) return "";
    return task->name;
}

/* task_get_status: Get the task status and returns it. */
int task_get_status(Task* task) { return task->status; }

/* task_set_status: Sets the task status. */
void task_set_status(Task* task, int status) { task->status = status; }

/* update_status: Updates the task status. */
static void update_status(TaskRunner* runner, int status) {
    if (WIFEXITED(status)) {
        if ((runner->info.exit_code = WEXITSTATUS(status)) == EXIT_SUCCESS) {
            task_set_status(runner->task, TASK_COMPLETED);
        } else {
            task_set_status(runner->task, TASK_NOT_READY);
        }
    }

    else if (WIFSIGNALED(status)) {
        if ((runner->info.signo = WTERMSIG(status)) == SIGINT) {
            task_set_status(runner->task, TASK_INCOMPLETE);
        } else {
            task_set_status(runner->task, TASK_READY);
        }
    }
}

static void task_runner_handler(void* arg) {
    TaskRunner* runner = (TaskRunner*)arg;
    int status;
    kill(runner->task_tid, SIGINT);
    waitpid(runner->task_tid, &status, 0);
    update_status(runner, status);
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

    // Change working directory
    posix_spawn_file_actions_addchdir_np(&actions, site->working_dir);

    // Redirect stdin/stdout/stderr to /dev/null
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO,  "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

    // Spawn the task process
    if (posix_spawn(&runner->task_tid, site->python, &actions, NULL,
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
    waitpid(runner->task_tid, &status, 0);
    update_status(runner, status);
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
    runner->info.exit_code = EXIT_SENTINAL;
    runner->info.signo = SIGSENT;
    
    // Create task thread
    int err = pthread_create(&runner->task_thread, NULL,
        task_runner_thread, runner);
    if (err != 0) {
        fprintf(stderr, "task_run: pthread_create: %s\n", strerror(err));
        free(runner);
        return NULL;
    }
    task_set_status(task, TASK_RUNNING);
    return runner;
}

/* task_runner_get_info: Gets the runner saved exit code and signal number. */
task_runner_info_t task_runner_get_info(TaskRunner* runner) {
    return runner->info;
}

/* task_runner_restart: Restarts the task runner. */
void task_runner_restart(TaskRunner* runner) {
    pthread_cancel(runner->task_thread);
    pthread_join(runner->task_thread, NULL);
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
