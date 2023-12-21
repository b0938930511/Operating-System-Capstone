#include "system_call.h"

void exec_test() {
    printf("\nExec Test, pid %d\n", get_pid());
    exec("dummy_test.img", NULL);
}