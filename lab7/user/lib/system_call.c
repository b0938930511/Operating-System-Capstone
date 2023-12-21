#include "system_call.h"


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

int open(const char *pathname, int flags) {
    unsigned long ret;
    asm volatile("mov x8, 11\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int close(int fd) {
    unsigned long ret;
    asm volatile("mov x8, 12\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int write(int fd, const void *buf, int count) {
    unsigned long ret;
    asm volatile("mov x8, 13\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int read(int fd, void *buf, int count) {
    unsigned long ret;
    asm volatile("mov x8, 14\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

int mkdir(const char *pathname) {
    unsigned long ret;
    asm volatile("mov x8, 15\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

// you can ignore arguments other than target and filesystem
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data) {
    unsigned long ret;
    asm volatile("mov x8, 16\n");
	asm volatile("svc 0\n");
    asm volatile("mov %0, x0\n":"=r"(ret):);
	return ret;
}

/* utility functions */
unsigned int printf(char* fmt,...) {
    char dst[100];
    __builtin_va_list args;
    __builtin_va_start(args,fmt);
    unsigned int ret = vsprintf(dst,fmt,args);
    uart_write(dst, 100);
    return ret;
}

unsigned int vsprintf(char *dst, char* fmt, __builtin_va_list args) {
    long int arg;
    int len, sign, i;
    char *p, *orig=dst, tmpstr[19];

    // failsafes
    if(dst==(void*)0 || fmt==(void*)0) {
        return 0;
    }

    // main loop
    arg = 0;
    while(*fmt) {
        // argument access
        if(*fmt=='%') {
            fmt++;
            // literal %
            if(*fmt=='%') {
                goto put;
            }
            len=0;
            // size modifier
            while(*fmt>='0' && *fmt<='9') {
                len *= 10;
                len += *fmt-'0';
                fmt++;
            }
            // skip long modifier
            if(*fmt=='l') {
                fmt++;
            }
            // character
            if(*fmt=='c') {
                arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
                fmt++;
                continue;
            } else
            // decimal number
            if(*fmt=='d') {
                arg = __builtin_va_arg(args, int);
                // check input
                sign=0;
                if((int)arg<0) {
                    arg*=-1;
                    sign++;
                }
                if(arg>99999999999999999L) {
                    arg=99999999999999999L;
                }
                // convert to string
                i=18;
                tmpstr[i]=0;
                do {
                    tmpstr[--i]='0'+(arg%10);
                    arg/=10;
                } while(arg!=0 && i>0);
                if(sign) {
                    tmpstr[--i]='-';
                }
                // padding, only space
                if(len>0 && len<18) {
                    while(i>18-len) {
                        tmpstr[--i]=' ';
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            // hex number
            if(*fmt=='x') {
                arg = __builtin_va_arg(args, long int);
                // convert to string
                i=16;
                tmpstr[i]=0;
                do {
                    char n=arg & 0xf;
                    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
                    tmpstr[--i]=n+(n>9?0x37:0x30);
                    arg>>=4;
                } while(arg!=0 && i>0);
                // padding, only leading zeros
                if(len>0 && len<=16) {
                    while(i>16-len) {
                        tmpstr[--i]='0';
                    }
                }
                p=&tmpstr[i];
                goto copystring;
            } else
            // string
            if(*fmt=='s') {
                p = __builtin_va_arg(args, char*);
copystring:     if(p==(void*)0) {
                    p="(null)";
                }
                while(*p) {
                    *dst++ = *p++;
                }
            }
        } else {
put:        *dst++ = *fmt;
        }
        fmt++;
    }
    *dst=0;
    // number of bytes written
    return dst-orig;
}

void delay(unsigned int clock) {
    while (clock--) {
        asm volatile("nop");
    }
}