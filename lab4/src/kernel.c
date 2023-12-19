#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"
#include "interrupt.h"
#include "except_handler.h"
#include "page.h"
#include "allocator.h"

extern void *_dtb_ptr;
void kernel_main(void)
{
	uart_init();
	memory_init();
	init_reserve();
	uart_send_string("Hello, world!\r\n");
	uart_send_string("kernel Exception level: ");
    uart_hex(get_el());
	uart_send('\n');
	enable_interrupt();
	
	//core_timer_enable();
	shell();
}
