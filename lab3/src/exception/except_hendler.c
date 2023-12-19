#include "../../include/except.h"
#include "../../include/mini_uart.h"

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
    return ;
}