#include "mini_uart.h"
#include "shell.h"
#include "exception.h"
#include "timer.h"
#include "memory.h"
#include "allocator.h"
#include "task.h"


void kernel_main(void) {
	uart_init();
	memory_init();
	init_reserve();
	init_timer();
	enable_interrupt();
	create_root_thread();
	shell();
}
