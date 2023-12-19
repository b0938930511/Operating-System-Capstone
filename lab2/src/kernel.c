#include "mini_uart.h"
#include "shell.h"
#include "dtb.h"

extern void *_dtb_ptr;
void kernel_main(void)
{
	uart_init();
	uart_send_string("Hello, world!\r\n");
	traverse_device_tree(_dtb_ptr,get_initramfs_addr);
	shell();
}
