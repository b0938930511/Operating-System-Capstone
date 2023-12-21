#include "mini_uart.h"
#include "shell.h"
#include "utils.h"
#include "reboot.h"
#include "peripherals/mail_box.h"
#include "mail_box.h"
#include "cpio.h"
#include "timer.h"
#include "memory.h"
#include "allocator.h"
#include "printf.h"
#include "task.h"
#include "exception.h"


int debug_mode = 0;
char buffer[MAX_BUFFER_SIZE];

void get_command();
void parse_command();

void get_board_revision();

void shell(void) {
    while (1) {
        uart_send_string("> ");
        get_command();
        parse_command();
    }
}

void get_command() {
    unsigned int index = 0;
    char recv;
    while (1) {
        recv = uart_recv();
        if (recv == '\r')
            continue;
        uart_send(recv);
        buffer[index++] = recv;
        index = index < MAX_BUFFER_SIZE ? index : MAX_BUFFER_SIZE - 1;
        if (recv == '\n') {
            buffer[index - 1] = '\0';
            break;
        }
    }
}

unsigned int debug_printf(char* fmt,...) {
   if (debug_mode) {
       char dst[100];
        __builtin_va_list args;
        __builtin_va_start(args,fmt);
        unsigned int ret=vsprintf(dst,fmt,args);
        uart_send_string(dst);
        return ret;
   }
   return 0;
}

void parse_command() {
    if (compare_string(buffer, "\0") == 0) {}
    else if (compare_string(buffer, "hello") == 0)
        uart_send_string("Hello World!\n");
    else if (compare_string(buffer, "reboot") == 0) {
        uart_send_string("rebooting ...\n");
        reboot(100);
        while (1) {}
    }
    else if (compare_string(buffer, "info") == 0) {
        get_board_revision();
        get_arm_memory();
        get_serial_number();
    }
    else if (compare_string(buffer, "ls") == 0) {
        cpio_list();
    }
    else if (compare_string(buffer, "cat") == 0) {
        uart_send_string("Filename: \n");
        get_command();
        cpio_cat(buffer);
    }
    else if (compare_string(buffer, "async_uart") == 0) {
        test_uart_async();
    }
    else if (compare_string(buffer, "test_timer_lab3") == 0) {
        test_timer();
    }
    else if (compare_string(buffer, "test_page") == 0) {
        debug_mode = 1;
        const int test_size = 3;
        const uint64_t test_address[] = {0x2001, 0x3010, 0x6100};    // covered index 2(4K), 3(4K), 6(8K)
        const int page_size[] = {0, 0, 1};
        const uint64_t mask[] = {0xFFF, 0xFFF, 0x1FFF};
        for (int i = 0; i < test_size; ++i) {
            reserve_page(page_size[i], test_address[i]);  
            print_frame_array();
            uart_printf("\n");
        }
        for (int i = 0; i < test_size; ++i) {
            page_free(test_address[i] & ~mask[i], page_size[i]);
            print_frame_array();
            uart_printf("\n");
        }
        debug_mode = 0;
    }
    else if (compare_string(buffer, "test_dyn") == 0) {
        debug_mode = 1;
        const int test_size = 10;
        const int test_cases[] = {31, 2, 32, 63, 120, 256, 129, 155, 240, 250};
        void *addrs[test_size];
        for (int i = 0; i < test_size; ++i) {
            addrs[i] = kmalloc(test_cases[i]);
            print_slot_record(find_page(addrs[i]));
            uart_printf("\n");
        }
        for (int i = 0; i < test_size; ++i) {
            kfree(addrs[i]);
            print_slot_record(find_page(addrs[i]));
            uart_printf("\n");
        }
        debug_mode = 0;
    }
    else if (compare_string(buffer, "test_fork") == 0) {
        run_user_program("fork_test.img", NULL);
    }
    else if (compare_string(buffer, "test_exec") == 0) {
        run_user_program("exec_test.img", NULL);
    }
    else if (compare_string(buffer, "test_timer") == 0) {
        run_user_program("timer_test.img", NULL);
    }
    else if (compare_string(buffer, "test_mbox") == 0) {
        run_user_program("mbox_test.img", NULL);
    }
    else if (compare_string(buffer, "test_dummy") == 0) {
        run_user_program("dummy_test.img", NULL);
    }
    else if (compare_string(buffer, "test_vm") == 0) {
        run_user_program("vm.img", NULL);
    }
    else if (compare_string(buffer, "help") == 0) {
        uart_send_string("help               : print this help menu\n");
        uart_send_string("hello              : print Hello World!\n");
        uart_send_string("reboot             : reboot the device\n");
        uart_send_string("info               : print device info\n");
        uart_send_string("ls                 : print files in rootfs\n");
        uart_send_string("cat                : print file content\n");
        uart_send_string("async_uart         : test async uart\n");
        uart_send_string("test_timer_lab3    : test timer multiplexing\n");
        uart_send_string("test_page          : test buddy system\n");
        uart_send_string("test_dyn           : test dynamic allocator\n");
        uart_send_string("test_fork          : test user program\n");
    }
    else
        uart_send_string("\rcommand not found!\r\n");
}
