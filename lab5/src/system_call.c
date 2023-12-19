#include "system_call.h"
#include "switch.h"
#include "task.h"
#include "mini_uart.h"

#include "printf.h"

/* helper functions for user programs, not the real system calls */
int get_pid() {
    unsigned long ret;
	asm volatile("mov x8, 0\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

size_t uart_read(char buf[], size_t size) {
    unsigned long ret;
    asm volatile("mov x8, 1\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

size_t uart_write(const char buf[], size_t size) {
    unsigned long ret;
    asm volatile("mov x8, 2\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int exec(const char *name, char *const argv[]) {
    unsigned long ret;
    asm volatile("mov x8, 3\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int fork() {
    unsigned long ret;
    asm volatile("mov x8, 4\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

void exit() {
	asm volatile("mov x8, 5\n");
	asm volatile("svc 0\n");
}

int mbox_call(unsigned char ch, unsigned int *mbox) {
    unsigned long ret;
    asm volatile("mov x8, 6\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

void kill(int pid) {
    asm volatile("mov x8, 7\n");
    asm volatile("svc 0\n");
}

unsigned int printf(char* fmt,...) {
	char dst[100];
    __builtin_va_list args;
    __builtin_va_start(args,fmt);
    unsigned int ret = vsprintf(dst,fmt,args);
    uart_send_string(dst);
    return ret;

}