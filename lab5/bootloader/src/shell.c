#include "mini_uart.h"
#include "StringUtils.h"

#include <stddef.h>

#define BUFFER_MAX_SIZE 128u
extern char *_dtb;

static void printPrompt(void) {
    uart_send_string("\r");
    uart_send_string("> ");
}

void readCommand(char *buffer) {
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
    Enter2Null(buffer);
}


static void loadimg(){

	unsigned long k_addr=0,k_size=0;
    char *const kernel_addr = (char *)0x80000;
	char c;
	uart_send_string("\rPlease enter kernel load address (Hex): ");

    char buffer[BUFFER_MAX_SIZE];

    readCommand(buffer);
    k_addr = getHexFromString(buffer);
    uart_send_string("\n\r");
	uart_send_string("Please enter kernel size (Dec): ");
    readCommand(buffer);
    k_size = getIntegerFromString(buffer);
    uart_send_string("\n\r");

	uart_send_string("Please send kernel image now...\n");
		unsigned char* target=kernel_addr;
		while(k_size--){
			*target=uart_getb();
			target++;
			uart_send('.');
		}

	uart_send_string("\rloading...\n");
	((void (*)(char *))kernel_addr)(_dtb);
}

static void parseCommand(char *buffer) {
    // remove newline
    stripString(buffer);

    if (*buffer == '\0') {
        uart_send_string("\r");
        return;
    }

    if (compareString("loadimg", buffer) == 0) {
        loadimg();
    } else {
        uart_send_string("\rcommand not found: ");
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
