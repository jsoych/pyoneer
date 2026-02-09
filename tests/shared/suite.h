#ifndef SUITE_H
#define SUITE_H

#include "unittest.h"

typedef struct Suite Suite;

Suite* suite_create(int capacity);
void suite_destroy(Suite* suite);

int suite_add(Suite* suite, Unittest* ut);
int suite_run(Suite* suite);

#endif
