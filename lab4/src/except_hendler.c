#include "except_handler.h"
#include "mini_uart.h"
#include "util_s.h"
#include "timer.h"
#include "peripherals/mini_uart.h"

void enable_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_interrupt() { asm volatile("msr DAIFSet, 0xf"); }



// dump interrupt state
void _except_handler(){
    unsigned long spsr, elr, esr;
    asm volatile("mrs %0 ,spsr_el1   \n":"=r"(spsr):);
    asm volatile("mrs %0 ,elr_el1     \n":"=r"(elr):);
    asm volatile("mrs %0 ,esr_el1    \n":"=r"(esr):);
    uart_send_string("spsr_el1: ");
    uart_hex(spsr);
    uart_send('\n');
    uart_send_string("elr_el1: ");
    uart_hex(elr);
    uart_send('\n');
    uart_send_string("esr_el1: ");
    uart_hex(esr);
    uart_send_string("\n\n");
}



void lower_sync_handler() {
	disable_interrupt();
	_except_handler();
	enable_interrupt();
}

void lower_iqr_handler() {
	disable_interrupt();
	pop_timer();
	enable_interrupt();
}

void curr_sync_handler() {
	disable_interrupt();
	error_handler();
	enable_interrupt();
}

void curr_iqr_handler() {
	disable_interrupt();
    if (*IRQ_PENDING_1 & AUX_IRQ)
		uart_handler();
	else
		pop_timer();
    enable_interrupt();
}

void error_handler() {
	disable_interrupt();
	uart_send_string("[ERROR] unknown exception...\n");
	_except_handler();
	while(1){}
}