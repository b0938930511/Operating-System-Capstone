#include "system_call.h"

void timer_test() {
    fork();

    for (int i = 1; i <= 10000000; ++i) {
        for (int j = 0; j < 1000000; ++j) {}
        printf("Thread: %d, counts to %dM\n", get_pid(), i);
    } 

    exit();
}