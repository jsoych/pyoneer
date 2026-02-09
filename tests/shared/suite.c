#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "suite.h"

#define EXIT_RUN_ERROR 254

struct Suite {
    int size;
    int capacity;
    Unittest** tests;
};

Suite* suite_create(int capacity) {
    if (capacity <= 0) return NULL;
    Suite* suite = malloc(sizeof(Suite));
    if (suite == NULL) return NULL;
    suite->tests = malloc(capacity*sizeof(Unittest*));
    if (suite->tests == NULL) {
        free(suite);
        return NULL;
    }
    suite->size = 0;
    suite->capacity = capacity;
    return suite;
}

void suite_destroy(Suite* suite) {
    if (suite == NULL) return;
    for (int i = 0; i < suite->size; i++)
        unittest_destroy(suite->tests[i]);
    free(suite->tests);
    free(suite);
}

int suite_add(Suite* suite, Unittest* ut) {
    if (suite->size == suite->capacity) {
        printf("suite_add: Error: Suite at capacity (%d)\n", suite->capacity);
        return -1;
    }
    suite->tests[suite->size++] = ut;
    return 0;
}

/* suite_run: Runs each test in the suite and returns the number of failed test
    cases. Otherwise, return -1. */
int suite_run(Suite* suite) {
    int total_failn = 0;

    // Run each unittest
    for (int i = 0; i < suite->size; i++) {
        const char* name = unittest_get_name(suite->tests[i]);

        // Create test process
        pid_t pid = fork();
        if (pid == -1) {
            perror("suite_run: fork");
            return -1;
        }

        if (pid == 0) {
            // Test process
            int failn = unittest_run(suite->tests[i]);
            if (failn == -1) exit(EXIT_RUN_ERROR);
            fflush(stdout);
            fflush(stderr);
            exit((failn < EXIT_RUN_ERROR) ? failn : EXIT_RUN_ERROR);
        }

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("suite_run: waitpid");
            return -1;
        }

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code == EXIT_RUN_ERROR) {
                fprintf(stderr, "suite_run: Error: Test %s failed to run\n", name);
                return -1;
            }
            
            if (code != 0) {
                printf("suite_run: Error: Test %s exited with code (%d)\n", name, code);
                total_failn += code;
            }
        
        } else if (WIFSIGNALED(status)) {
            int signo = WTERMSIG(status);
            printf("suite_run: Error: Test %s terminated by signal %d (%s)",
                name, signo, strsignal(signo));
            total_failn += 1;
            
        } else {
            fprintf(stderr, "suite_run: Error: Test %s unexpected status\n", name);
            return -1;
        }
    }

    return total_failn;
}
