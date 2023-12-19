#include "peripherals/gpio.h"
#include "mini_uart.h"
#include "StringUtils.h"
#include "shell.h"
#include "initrd.h"



/* convert hexadecimal string into decimal */
unsigned long hexToDex(char *s){   
    unsigned long r = 0;
    for(int i = 0 ;i < 8 ; ++i){
        if(s[i] >= '0' && s[i] <= '9'){
            r = r* 16 + s[i]-'0';
        }else{
            r = r * 16 + s[i]-'a'+10;
        }
    }
    return r;
}



char *findFile(char *name)
{
    char *addr = cpio_addr;
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
    char *addr = cpio_addr;
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