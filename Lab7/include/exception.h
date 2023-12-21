#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include "task.h"
#include "vfs.h"
#include <stddef.h>

void enable_interrupt();
void disable_interrupt();
void dumpState();
void lower_sync_handler();
void lower_iqr_handler();
void curr_sync_handler();
void curr_iqr_handler();
void error_handler();
void child_return_from_fork();

/* Implement system calls */
int sys_getpid();
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(const char buf[], size_t size);
int sys_exec(trap_frame *tf, const char *name, char *const argv[]);
void sys_fork(trap_frame *tf);
void sys_exit();
int sys_mbox_call(unsigned char ch, volatile unsigned int *mbox);
void sys_kill(int pid);
void sys_signal(int SIGNAL, void (*handler)());
void sys_signal_kill(int pid, int SIGNAL);

int sys_open(const char *pathname, int flags);
int sys_close(int fd);
int sys_write(int fd, const void *buf, int count);
int sys_read(int fd, void *buf, int count);
int sys_mkdir(const char *pathname);
// you can ignore arguments other than target and filesystem
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int sys_chdir(const char *path);

/* helper functions */
extern void (*_handler)();
extern int _pid;
void signal_handler_wrapper();

#endif
