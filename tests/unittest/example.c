#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unittest.h"

struct task {
    char* name;
};

int task_init(struct task* t, const char* n) {
    t->name = strdup(n);
    if (!t->name) return -1;
    return 0;
}

const char* task_get_name(struct task* t) { return t->name; }

static void test_example(UnittestResult* result) {
    struct task task;
    if (task_init(&task, "task.py") == -1) {
        unittest_result_fail(result, "failed to initialize task");
        return;
    }
    if (strcmp(task_get_name(&task), "task.py") == 0) {
        unittest_result_ok(result);
        task_destroy(task);
        return;
    }
    unittest_result_fail(result, "unexpected name (%s)", task_get_name(&task));
    task_destroy(task);
    return;
}

int main() {
    Unittest* ut = unittest_create_test("example", test_example);
    if (!ut) {
        printf("%s: Error: Failed to create unittest\n");
        exit(EXIT_FAILURE);
    }

    UnittestResult* result;
    if (unittest_run(ut, result, NULL) == -1) {
        unittest_result_destroy(result);
        exit(EXIT_FAILURE);
    }

    unittest_print_result(ut, result, UNITTEST_QUIET);
    unittest_print_result(ut, result, UNITTEST_DEFAULT);
    unittest_print_result(ut, result, UNITTEST_VERBOSE);
    
    unittest_destroy(ut);
    unittest_result_destroy(result);
    exit(EXIT_SUCCESS);
}
