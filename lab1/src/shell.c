#include "mini_uart.h"
#include "StringUtils.h"
#include "reboot.h"

#include <stddef.h>

#define BUFFER_MAX_SIZE 128u


static void printPrompt(void) {
    uart_send_string("\r");
    uart_send_string("> ");
}

static void readCommand(char *buffer) {
    size_t size = 0u;
    while (size < BUFFER_MAX_SIZE) {
        buffer[size] = uart_recv();
        
        // echo back
        uart_send(buffer[size]);

        if (buffer[size++] == '\n') {
            break;
        }
    }
    buffer[size] = '\0';
}

static void help(void) {
    uart_send_string("\rshell menu :\r\n");
    uart_send_string("help : print this help menu\r\n");
    uart_send_string("hello : print Hello World\r\n");
    uart_send_string("reboot : power off then on\r\n");
    uart_send_string("Info : use mailbox\r\n");
}

static void hello(void) {
    uart_send_string("Hello World!\r\n");
}

static void reboot(void) {
    reset(100);
    while(1){
        uart_send_string("...\r\n");
    }
}

static void Info(void) {
    get_board_revision();
    get_ARM_memory();
}

static void parseCommand(char *buffer) {
    // remove newline
    stripString(buffer);

    if (*buffer == '\0') {
        uart_send_string("\r");
        return;
    }

    if (compareString("help", buffer) == 0) {
        help();
    } else if (compareString("hello", buffer) == 0) {
        hello();
    } else if (compareString("reboot", buffer) == 0) {
        reboot();
    } else if (compareString("Info", buffer) == 0) {
        Info();
    } else {
        uart_send_string("command not found: ");
        uart_send_string(buffer);
        uart_send('\r\n');
    }
}

void shell(void) {
    char buffer[BUFFER_MAX_SIZE];
    while (1) {
        uart_send('\r\n');
        printPrompt();
        readCommand(buffer);
        stripString(buffer);
        parseCommand(buffer);
    }
}
