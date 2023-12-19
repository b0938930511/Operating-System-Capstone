#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

#define CORE0_TIMER_IRQ_CTRL ((volatile unsigned int *)(0x40000040))
#define MAX_TIMER_QUEUE_SIZE 10

typedef void (*timer_call_back)(void *);
struct Timer {
    struct Timer *next;
    uint64_t expire_time;
    timer_call_back call_back;
    void *args;
};

void core_timer_enable();
void core_timer_handler();
void add_timer(unsigned int time, timer_call_back, void *);
void pop_timer();
void test_timer();

/* callbacks */
void show_time_elapsed(void *);
void print_timer(void *);
void clear_timer();
void normal_timer();
void init_timer();

#endif