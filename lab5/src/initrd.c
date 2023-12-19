#include "peripherals/gpio.h"
#include "mini_uart.h"
#include "StringUtils.h"
#include "shell.h"
#include "initrd.h"
#include "timer.h"

#include "sysreg.h"
#include "allocator.h"
#include "except_handler.h"

#define qemu_cpio 0x8000000 
#define rpi3_cpio 0x20000000 

/* convert hexadecimal string into decimal */
unsigned long hexToDex(char *s){   
    unsigned long r = 0;
    for(int i = 0 ;i < 8 ; ++i){
        if(s[i] >= '0' && s[i] <= '9'){
            r = r* 16 + s[i]-'0';
        }else{
            r = r * 16 + s[i]-'A'+10;
        }
    }
    return r;
}



char *findFile(char *name)
{
    char *addr = rpi3_cpio;
    while (compareString((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0)
    {
        if ((compareString((char *)(addr + sizeof(cpio_header)), name) == 0))
        {
            return addr;
        }
        cpio_header *header = (cpio_header *)addr;
        unsigned long pathname_size = hexToDex(header->namesize);
        unsigned long file_size = hexToDex(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        Align_4(&headerPathname_size); 
        Align_4(&file_size);           
        addr += (headerPathname_size + file_size);
    }
    return 0;
}
void initrd_ls()
{
    char *addr = rpi3_cpio;
    while (compareString((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0)
    {
        cpio_header *header = (cpio_header *)addr;
        unsigned long pathname_size = hexToDex(header->namesize);
        unsigned long file_size = hexToDex(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        Align_4(&headerPathname_size); 
        Align_4(&file_size);           
        uart_send_string("\r");
        uart_send_string(addr + sizeof(cpio_header)); // print the file name
        uart_send_string("\n");

        addr += (headerPathname_size + file_size);
    }
}

void initrd_cat(char *filename)
{
    char *target = findFile(filename);
    if (target)
    {
        cpio_header *header = (cpio_header *)target;
        unsigned long pathname_size = hexToDex(header->namesize);
        unsigned long file_size = hexToDex(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        Align_4(&headerPathname_size); 
        Align_4(&file_size);           
        uart_send_string("\r");
        char *file_content = target + headerPathname_size;
        for (unsigned int i = 0; i < file_size; i++)
        {
            uart_send(file_content[i]); // print the file content
        }
        uart_send_string("\n");
    }
    else
    {
        uart_send_string("\rNot found the file\n");
    }
}

void load_program(char *name)
{
    char *prog_addr = findFile(name);
    //unsigned long addr_hex = 0x200000;
    if(prog_addr){
        cpio_header *header = (cpio_header *)prog_addr;
        unsigned long pathname_size = hexToDex(header->namesize);
        unsigned long file_size = hexToDex(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;
        Align_4(&headerPathname_size); 
        Align_4(&file_size);  
        uart_send_string(prog_addr + sizeof(cpio_header)); // print the file name
        uart_send_string("\n\r");

        unsigned char *addr = (unsigned char *)USER_PROGRAM_ADDR;
        char *file_content = prog_addr + headerPathname_size;
        for (unsigned int i = 0; i < file_size; i++) {
            *addr = file_content[i];
            addr++;
        }
        uart_printf("[load_program] load program: %s\n", name);
    }
    else{
        uart_send_string("\rfile not found123\n");
        return;
    }
}