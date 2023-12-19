#ifndef __PRINT_H
#define __PRINT_H

unsigned int sprintf(char *dst, char* fmt, ...);
unsigned int vsprintf(char *dst,char* fmt, __builtin_va_list args);
unsigned int log_printf(char* fmt,...);
int log_mode;
#endif