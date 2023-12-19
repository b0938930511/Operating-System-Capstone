#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"
#include "interrupt.h"
#include "except_handler.h"
#include "page.h"
#include "allocator.h"
#include "task.h"
#include "timer.h"

// extern void *_dtb_ptr;
void kernel_main(void)
{
	uart_init();
	memory_init();
	init_reserve();
	init_timer();
	enable_interrupt();
	create_root_thread();
	//core_timer_enable();
	shell();
}
