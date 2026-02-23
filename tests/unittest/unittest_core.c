#include "unittest_core.h"

unittest_opts_t unittest_opts_default() {
    return (unittest_opts_t){
        .timeout_ms = -1,
        .level = UNITTEST_DEFAULT
    };
}