#include "timer.h"
#include "mini_uart.h"
#include "sysreg.h"
#include "allocator.h"
#include "exception.h"
#include "shell.h"


static struct Timer *timer_list = NULL;


void init_timer() {
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
}

void core_timer_enable() {
    write_sysreg(cntp_ctl_el0, 1); // enable
    *CORE0_TIMER_IRQ_CTRL = 2;     // unmask timer interrupt
}

void core_timer_disable() {
    write_sysreg(cntp_ctl_el0, 0); // disable
    *CORE0_TIMER_IRQ_CTRL = 0;     // mask timer interrupt
}

void add_timer(unsigned int time, timer_call_back call_back, void *args) {
    struct Timer *timer = (struct Timer*)kmalloc(sizeof(struct Timer));
    timer->expire_time = read_sysreg(cntpct_el0) + time;
    timer->call_back = call_back;
    timer->args = args;
    timer->next = NULL;
    int need_update = 0;
    //core_timer_disable();
    if (!timer_list) {
        timer_list = timer;
        need_update = 1;
    }
    else if (timer_list->expire_time > timer->expire_time) {
        timer->next = timer_list;
        timer_list = timer;
        need_update = 1;
    }
    else {
        struct Timer *pre = timer_list, *next = timer_list->next;
        while (next && pre->expire_time < timer->expire_time) {
            pre = next;
            next = next->next;
        }
        pre->next = timer;
        timer->next = next;
    }
    if (need_update)
        write_sysreg(cntp_tval_el0, time);
    //core_timer_enable();
}

void pop_timer() {
    //core_timer_disable();
    struct Timer *timer = timer_list;
    timer_list = timer_list->next;
    //core_timer_enable();
    timer->call_back(timer->args);
    kfree((void*)timer);
    if (!timer_list)
        core_timer_disable();
    else
        write_sysreg(cntp_tval_el0, timer_list->expire_time - read_sysreg(cntpct_el0));
}

void test_timer() {
    char* msg1 = (char *)kmalloc(20);
    char* msg2 = (char *)kmalloc(20);
    char* msg3 = (char *)kmalloc(20);
    msg1 = "\nThis is timer 1!\n";
    msg2 = "This is timer 2!\n";
    msg3 = "This is timer 3!\n";
    add_timer(3 * read_sysreg(cntfrq_el0), print_timer, msg3);
    add_timer(2 * read_sysreg(cntfrq_el0), print_timer, msg2);
    add_timer(1 * read_sysreg(cntfrq_el0), print_timer, msg1);
    core_timer_enable();
}

/* callbacks */
void show_time_elapsed(void *args) {
    unsigned long cntpct = read_sysreg(cntpct_el0);
	unsigned long cntfrq = read_sysreg(cntfrq_el0);
	unsigned long tmp = cntpct * 10 / cntfrq;
	uart_printf("--------------------\n");
	uart_printf("Time Elapsed: %d.%ds\n", tmp/10, tmp%10);
	uart_printf("--------------------\n");
    add_timer(cntfrq << 1, show_time_elapsed, NULL);
}

void print_timer(void *args) {
    char *msg = (char *)args;
    uart_send_string(msg);
}

void normal_timer() {
    unsigned long cntpct = read_sysreg(cntpct_el0);
	unsigned long cntfrq = read_sysreg(cntfrq_el0);
	unsigned long tmp = cntpct * 10 / cntfrq;
    debug_printf("[DEBUG][normal_timer] time elapsed: %d.%ds\n", tmp/10, tmp%10);
    add_timer(cntfrq >> 5, normal_timer, NULL);
    thread_schedule();
}