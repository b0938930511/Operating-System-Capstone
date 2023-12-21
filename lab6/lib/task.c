#include "task.h"
#include "allocator.h"
#include "switch.h"
#include "shell.h"
#include "utils.h"
#include "mini_uart.h"
#include "sysreg.h"
#include "cpio.h"
#include "timer.h"
#include "vm.h"


task_queue run_queue = {"run", NULL, NULL};
task_queue wait_queue = {"wait", NULL, NULL};
task_queue terminated_queue = {"terminated", NULL, NULL};
int run_queue_sz = 0;
char **_argv = NULL;
static int task_cnt = 0;

unsigned long user_addr;
unsigned long user_sp;
unsigned long user_page_table;


void run_user_program(const char* name, char *const argv[]) {
    task_struct *task = thread_create(switch_to_user_space);
    load_program((char*)name, task->page_table);
    _argv = (char**)argv;
    user_addr = USER_PROGRAM_VA;
    for (int i = 0; i < 4; ++i)
        map_pages(task->page_table, 0xffffffffb000 + i * 0x1000, 1, VA2PA(page_malloc(0)));
    user_sp = task->user_fp;
    user_page_table = (unsigned long)(task->page_table);
    add_timer(read_sysreg(cntfrq_el0) >> 5, normal_timer, NULL); // < 0.1s
    core_timer_enable();
    debug_mode = 0;
    idle();
}

void switch_to_user_space() {
    asm volatile("mov x0, 0x340   \n"::);
    asm volatile("msr spsr_el1, x0   \n"::);
    asm volatile("msr elr_el1,  %0   \n"::"r"(user_addr));
    asm volatile("msr sp_el0,   %0   \n"::"r"(user_sp));
    debug_printf("[DEBUG][switch_to_user_space]\n");
    asm volatile("eret  \n"::);
}

void create_root_thread() {
    task_struct* root_task = thread_create(idle);
    write_sysreg(tpidr_el1, root_task);
}

task_struct* thread_create(void *func) {
    task_struct* new_task = (task_struct*)page_malloc(0);
    initPT(&(new_task->page_table));
    new_task->context.fp = (unsigned long)new_task + PAGE_SIZE_4K - 16;
    new_task->context.lr = (unsigned long)func;
    new_task->context.sp = (unsigned long)new_task + PAGE_SIZE_4K - 16;  // kernel stack pointer for the thread
    new_task->user_fp = 0xfffffffff000 - 16;
    new_task->state = RUNNING;
    new_task->id = task_cnt++;
    new_task->handler = NULL;

    debug_printf("[DEBUG][thread_create] id: %d\n", new_task->id);

    push_task_to_queue(&run_queue, new_task);
    return new_task;
}

void thread_schedule() {
    task_struct *next_task = run_queue.begin;
    if (!next_task->id && !next_task->next) // escapes idle(), but the call stack will grow forever
        shell();
    
    pop_task_from_queue(&run_queue, next_task);
    push_task_to_queue(&run_queue, next_task);

    asm volatile("mov x0, %0 			\n"::"r"(next_task->page_table));
    asm volatile("dsb ish 	\n");  //ensure write has completed
	asm volatile("msr ttbr0_el1, x0 	\n"); //switch translation based address.
    asm volatile("tlbi vmalle1is 	\n");  //invalidates cached copies of translation table entries from L1 TLBs
    asm volatile("dsb ish 	\n");  //ensure completion of TLB invalidatation
    asm volatile("isb 	\n");  //clear pipeline

    task_struct *cur = get_current();
    debug_printf("[DEBUG][thread_schedule] switch from thread %d to %d\n", cur->id, next_task->id);

    asm volatile("\
			mov x0, %0\n\
			mov x1, %1\n\
			bl switch_to\n\
	"::"r"(cur), "r"(next_task));
}

void idle() {
    while(1) {
        kill_zombies();
        thread_schedule();
    }
}

void kill_zombies() {

}

void push_task_to_queue(task_queue *queue, task_struct *task) {
    if (queue->begin) {
        queue->end->next = task;
        task->prev = queue->end;
        task->next = NULL;
        queue->end = task;
    }
    else {
        queue->begin = queue->end = task;
        task->prev = task->next = NULL;
    }
    run_queue_sz += 1;
    debug_printf("[DEBUG][push_task_to_queue] push thread %d into %s queue\n", task->id, queue->name);
}

void pop_task_from_queue(task_queue *queue, task_struct *task) {
    task_struct *prev = task->prev;
    task_struct *next = task->next;
    if (prev && next) {
        prev->next = next;
        next->prev = prev;
    }
    else if (!prev && next) {
        next->prev = NULL;
        queue->begin = next;
    }
    else if (prev && !next) {
        prev->next = NULL;
        queue->end = prev;
    }
    else
        queue->begin = queue->end = NULL;
    run_queue_sz -= 1;
    debug_printf("[DEBUG][pop_task_from_queue] pop thread %d from %s queue\n", task->id, queue->name);
}

void dump_queue(task_queue *queue) {
    task_struct *task = queue->begin;
    uart_printf("%s queue: ", queue->name);
    while (task) {
        uart_printf("%d ", task->id, task);
        task = task->next;
    }
    uart_printf("\n");
}

/* Helper functions */
void put_args(char *const argv[]) {
    if (!argv)
        return;

    char **iter = (char**) argv;
    for (int i = 0; iter[i]; ++i)
        asm volatile("mov %0,   %1   \n"::"r"(i), "r"(iter[i]));
}

task_struct *find_task_by_id(task_queue *queue, int pid) {
    task_struct *task = queue->begin;
    while (task) {
        if (task->id == pid)
            break;
        task = task->next;
    }
    return task;
}