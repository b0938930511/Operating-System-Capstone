#include "mini_uart.h"
#include "utils.h"

void load_kernel() {
    char buffer[MAX_BUFFER_SIZE];

    while (compare_string(buffer, "[Load Kernel]") != 0) {
        uart_recv_string(buffer);
    }

    unsigned long k_addr=0,k_size=0;
	uart_send_string("Please enter kernel load address (Hex): ");

    uart_recv_string(buffer);
    k_addr = getHexFromString(buffer);
	uart_send_string("Please enter kernel size (Dec): ");
    uart_recv_string(buffer);
    k_size = getIntegerFromString(buffer);

	uart_send_string("Please send kernel image now...\n");
    unsigned char* target=(unsigned char*)k_addr;
    while(k_size--){
        *target=uart_getb();
        target++;
        uart_send('.');
    }

    uart_send_string("loading...\n");
    asm volatile("br %0\n"::"r"(k_addr)); // GCC inline assembly 
}

int main() {
    uart_init();
    load_kernel();
    return 0;   // should not reach here
}