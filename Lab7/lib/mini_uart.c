#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "utils.h"
#include "mini_uart.h"
#include "printf.h"
#include <stddef.h>


// The AUX_MU_LSR_REG register (8 bits) shows the data status.
// The AUX_MU_IO_REG register (8 bits) is primary used to write data to and read data from the UART FIFOs. 
void uart_send ( char c ) {	
	if (c == '\n')
    	uart_send('\r');
	while(!(*AUX_MU_LSR_REG & 0x20)) {} // Bit 5, if set to 1, tells us that the transmitter is empty.
	*AUX_MU_IO_REG = c;
}

char uart_recv ( void ) {
	while(!(*AUX_MU_LSR_REG & 0x01)) {} // Bit 0, if set to 1, indicates that the data is ready.
	char recv = *AUX_MU_IO_REG & 0xFF;
	return recv != '\r' ? recv : '\n';
}

void uart_recv_string(char *buffer) {
    int size = 0;
    while(size < MAX_BUFFER_SIZE){
        buffer[size] = uart_recv();
        uart_send(buffer[size]);
        if(buffer[size++] == '\n'){
            break;
        }
    }
    buffer[--size] = '\0';
}

void uart_send_string(char* str) {
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

unsigned int uart_printf(char* fmt,...) {
	char dst[100];
    //__builtin_va_start(args, fmt): "..." is pointed by args
    //__builtin_va_arg(args,int): ret=(int)*args;args++;return ret;
    __builtin_va_list args;
    __builtin_va_start(args,fmt);
    unsigned int ret=vsprintf(dst,fmt,args);
    uart_send_string(dst);
    return ret;
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        // get highest tetrad
        n=(d>>c)&0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

unsigned char uart_getb(){ //for data transfer
	unsigned char r;
	do{ asm volatile("nop"); } while (!(*AUX_MU_LSR_REG & 0x01));
	r = (unsigned char)(*AUX_MU_IO_REG);
	return r;
}

void delay(unsigned int clock) {
    while (clock--) {
        asm volatile("nop");
    }
}

// An alternative function is a number from 0 to 5 that can be set for each pin and configures which device is connected to the pin.
// GPFSEL1 register is used to control alternative functions for pins 10-19. 
// pin 14 -> TXD1: set bits 14-12 to 5
// pin 15 -> RXD1: set bits 17-15 to 5
void uart_init ( void ) {	
	// connect Mini UART to the GPIO pins
	unsigned int selector;
	selector = *GPFSEL1;
	selector &= ~(7u << 12);             // Clean gpio14
	selector |= 2u << 12;                // Set alt5 for gpio14
	selector &= ~(7u << 15);             // Clean gpio15
	selector |= 2u << 15;                // Set alt5 for gpio15
	*GPFSEL1 = selector;

	// Remove both the pull-up and pull-down states from a pin
	*GPPUD = 0;
	delay(150u);
	*GPPUDCLK0 = (1u << 14) | (1u << 15);
	delay(150u);
	*GPPUDCLK0 = 0;

	// Initializing the Mini UART
	*AUX_ENABLES = 1u;                   // Enable mini uart (this also enables access to its registers)
	*AUX_MU_CNTL_REG = 0u;               // Disable auto flow control and disable receiver and transmitter (for now)
	*AUX_MU_IER_REG = 1u;                // Disable receive and transmit interrupts
	*AUX_MU_LCR_REG = 3u;                // Enable 8 bit mode
	*AUX_MU_MCR_REG = 0u;                // Set RTS line to be always high
	*AUX_MU_BAUD_REG = 270u;             // Set baud rate to 115200
	*AUX_MU_CNTL_REG = 3u;               // Finally, enable transmitter and receiver

    *AUX_MU_IIR_REG = 6u;

    read_buf_start = read_buf_end = 0;
    write_buf_start = write_buf_end = 0;
}



/* async mini uart */
char read_buf[MAX_BUFFER_SIZE];
char write_buf[MAX_BUFFER_SIZE];
int read_buf_start, read_buf_end;
int write_buf_start, write_buf_end;

void enable_uart_interrupt() { *ENB_IRQS1 = AUX_IRQ; }
void disable_uart_interrupt() { *DISABLE_IRQS1 = AUX_IRQ; }
void set_transmit_interrupt() { *AUX_MU_IER_REG |= 0x2; }
void clear_transmit_interrupt() { *AUX_MU_IER_REG &= ~(0x2); }

void uart_handler() {
    disable_uart_interrupt();
    int RX = (*AUX_MU_IIR_REG & 0x4);
    int TX = (*AUX_MU_IIR_REG & 0x2);
    if (RX) {
        char c = (char)(*AUX_MU_IO_REG);
        read_buf[read_buf_end++] = c;
        if (read_buf_end == MAX_BUFFER_SIZE)
            read_buf_end = 0;
    }
    else if (TX) {
        while (*AUX_MU_LSR_REG & 0x20) {
            if (write_buf_start == write_buf_end) {
                clear_transmit_interrupt();
                break;
            }
            char c = write_buf[write_buf_start++];
            *AUX_MU_IO_REG = c;
            if (write_buf_start == MAX_BUFFER_SIZE)
                write_buf_start = 0;
        }
    }
    else {
        uart_send_string("[Error] uart_handler: should not reach here!\n");
        while (1) { }  
    }
    enable_uart_interrupt();
}

char uart_async_recv() {
    // wait until there are new data
    while (read_buf_start == read_buf_end)
        asm volatile("nop");
    char c = read_buf[read_buf_start++];
    if (read_buf_start == MAX_BUFFER_SIZE)
        read_buf_start = 0;
    return c == '\r' ? '\n' : c;
}

void uart_async_send_string(char *str) {

    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            write_buf[write_buf_end++] = '\r';
            write_buf[write_buf_end++] = '\n';
            continue;
        }
        write_buf[write_buf_end++] = str[i];
        if (write_buf_end == MAX_BUFFER_SIZE)
            write_buf_end = 0;
    }
    set_transmit_interrupt();
}

void test_uart_async() {
    uart_send_string("[Please type something]\n");
    enable_uart_interrupt();
    delay(15000);
    char buffer[MAX_BUFFER_SIZE];
    size_t index = 0;
    while (1) {
        buffer[index] = uart_async_recv();
        if (buffer[index] == '\n')
            break;
        index++;
    }
    buffer[index + 1] = '\0';
    uart_async_send_string(buffer);
    disable_uart_interrupt();
}
