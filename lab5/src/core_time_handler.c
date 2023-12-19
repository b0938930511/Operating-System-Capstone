#include "core_time_handler.h"
#include "mini_uart.h"
void core_time_handler(void){

	unsigned long cntpct,cntfrq;
    int cnt = 0;

    asm volatile("mrs x0, cntfrq_el0	\n");
	asm volatile("add x0, x0, x0		\n");
	asm volatile("msr cntp_tval_el0, x0	\n");
	asm volatile("mrs %0, cntpct_el0	\n":"=r"(cntpct):);
	asm volatile("mrs %0, cntfrq_el0	\n":"=r"(cntfrq):);

	cnt=cntpct/cntfrq;
	uart_send_string("\r--------------------\n");
	uart_send_string("\rTime Elapsed: ");
    uart_printf("%d",cnt);
    uart_send_string("s\n");
	uart_send_string("\r--------------------\n");
}