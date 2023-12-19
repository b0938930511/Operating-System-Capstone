#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void delay(unsigned int clock);
unsigned char uart_getb();
void uart_hex(unsigned int d) ;
unsigned int uart_printf(char* fmt,...);

extern char read_buf[];
extern char write_buf[];
extern int read_buf_start, read_buf_end;
extern int write_buf_start, write_buf_end;

void enable_uart_interrupt();
void disable_uart_interrupt();
void set_transmit_interrupt();
void clear_transmit_interrupt();
void uart_handler();
void test_uart_async();
char uart_async_recv();
void uart_async_send_string(char *str);
#endif  /*_MINI_UART_H */
