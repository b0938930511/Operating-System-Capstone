#ifndef _TASK_H
#define _TASK_H

#define TERMINATED 0
#define RUNNING 1
#define WAITING 2

typedef struct trap_frame{   //exception 發生時要保存的東西
  unsigned long regs[32];
  unsigned long sp_el0;
  unsigned long elr_el1;
  unsigned long spsr_el1;
}trap_frame;

typedef struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp; //x29: kernel frame pointer
    unsigned long lr; //x30: link register for function calls
    unsigned long sp; // kernel stack pointer
} cpu_context;

typedef struct task_struct {     //context switch 時要保存的東西
    cpu_context context;
    int id;
    int state;
    unsigned long user_fp;
    void (*handler)();
    struct task_struct *prev;
    struct task_struct *next;
} task_struct;

typedef struct task_queue {
    char name[10];
    task_struct *begin;
    task_struct *end;
} task_queue;

extern task_queue run_queue;
extern task_queue wait_queue;
extern task_queue terminated_queue;
extern int run_queue_sz;
extern char **_argv;
extern unsigned long user_addr;
extern unsigned long user_sp;

task_struct* thread_create(void* func);
void thread_schedule();
void kill_zombies();
void idle();
void create_root_thread();
void run_user_program(const char* name, char *const argv[]);
void switch_to_user_space();

void dump_queue(task_queue *queue);
void push_task_to_queue(task_queue *queue, task_struct *task);
void pop_task_from_queue(task_queue *queue, task_struct *task);


/* Helper functions */
void put_args(char *const argv[]);
task_struct *find_task_by_id(task_queue *queue, int pid);
#endif