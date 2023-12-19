#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include "mini_uart.h"
#include "StringUtils.h"
#include "printf.h"
#include "except_handler.h"


/* async uart declaration */
char read_buf[MAX_BUFFER_SIZE];
char write_buf[MAX_BUFFER_SIZE];
int read_buf_start, read_buf_end;
int write_buf_start, write_buf_end;



void delay(unsigned int clock) {
    while (clock--) {
        asm volatile("nop");
    }
}

void uart_init(void) {

    unsigned int selector;

    selector = *GPFSEL1;
    selector &= ~(7u << 12) | ~(7u << 15); // clean GPIO 14, 15
    selector |= 2u << 12 | 2u << 15;       // set alt5 (txd1, rxd1) for GPIO 14, 15
    *GPFSEL1 = selector;

    // check page 101 of the BCM2837 ARM Peripherals manual
    *GPPUD = 0;
    delay(150u);
    *GPPUDCLK0 = (1u << 14) | (1u << 15);
    delay(150u);
    *GPPUDCLK0 = 0u;

    *AUX_ENABLES = 1u;
    *AUX_MU_CNTL_REG = 0u;    // disable receiver and transmitter
    *AUX_MU_IER_REG = 1u;     // disable receive and transmit interrupts
    *AUX_MU_LCR_REG = 3u;     // enable 8-bit mode for data size
    *AUX_MU_MCR_REG = 0u;     // disable auto flow control
    *AUX_MU_BAUD_REG = 270u;  // set baud rate to 115200
    *AUX_MU_IIR_REG = 6u;     // clear FIFO
    *AUX_MU_CNTL_REG = 3u;    // enable rx, tx (receiver, transmitter)


    read_buf_start = read_buf_end = 0;
    write_buf_start = write_buf_end = 0;
}

void uart_send(char c) {
    if (c == '\n')
    	uart_send('\r');
    // wait until bit 5th set to 1
    while(!(*AUX_MU_LSR_REG & 0x20)) {}
    *AUX_MU_IO_REG = c;
}

char uart_recv(void) {
    // wait until bit 1st set to 1
    while(!(*AUX_MU_LSR_REG & 0x01)) {}

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

unsigned char uart_getb(){//for data transfer
	unsigned char r;
	do{ asm volatile("nop"); }while(!(*AUX_MU_LSR_REG&0x01));
	r=(unsigned char)(*AUX_MU_IO_REG);
	return r;
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

unsigned int uart_printf(char* fmt,...){
	char dst[100];
    //__builtin_va_start(args, fmt): "..." is pointed by args
    //__builtin_va_arg(args,int): ret=(int)*args;args++;return ret;
    __builtin_va_list args;
    __builtin_va_start(args,fmt);
    unsigned int ret=vsprintf(dst,fmt,args);
    uart_puts(dst);
    return ret;
}

void uart_puts(char *s) {
    while(*s) {
        /* convert newline to carrige return + newline */
        uart_send(*s++);
    }
}


/* async uart */


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