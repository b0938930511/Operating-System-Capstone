#ifndef _SYSTEM_CALL_H
#define _SYSTEM_CALL_H

#include <stddef.h>

/* helper functions for user programs, not the real system calls */
int get_pid();
size_t uart_read(char buf[], size_t size);
size_t uart_write(const char buf[], size_t size);
int exec(const char *name, char *const argv[]);
int fork();
void exit();
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);

#endif