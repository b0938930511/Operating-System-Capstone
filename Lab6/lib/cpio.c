#include "peripherals/gpio.h"
#include "mini_uart.h"
#include "utils.h"
#include "shell.h"
#include "cpio.h"
#include "timer.h"
#include "sysreg.h"
#include <stddef.h>
#include "vm.h"
#include "memory.h"


void cpio_list() {
    /*
     cpio archive comprises a header record with basic numeric metadata followed by
     the full pathname of the entry and the file data.
    */
    char *addr = CPIO_ADDR;

    while (compare_string((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {   
        cpio_header *header = (cpio_header*)addr;
        
        unsigned long pathname_size = hexToDec(header->namesize);
        unsigned long file_size = hexToDec(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;
        
        align_4(&headerPathname_size); // The pathname is followed by NUL bytes so that the total size of the fixed header plus pathname is a multiple	of four.
        align_4(&file_size);           // Likewise, the	file data is padded to a multiple of four bytes.
        uart_send_string(addr + sizeof(cpio_header)); // print the fine name
        uart_printf("   %d\n", file_size);
        addr += (headerPathname_size + file_size);
    }
}

char *findFile(char *name) {
    char *addr = CPIO_ADDR;
    while (compare_string((char *)(addr + sizeof(cpio_header)), "TRAILER!!!") != 0) {
        if ((compare_string((char *)(addr + sizeof(cpio_header)), name) == 0)) {
            return addr;
        }
        cpio_header *header = (cpio_header *)addr;
        unsigned long pathname_size = hexToDec(header->namesize);
        unsigned long file_size = hexToDec(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        align_4(&headerPathname_size); 
        align_4(&file_size);           
        addr += (headerPathname_size + file_size);
    }
    return 0;
}

void cpio_cat(char *filename) {
    char *target = findFile(filename);
    if (target) {
        cpio_header *header = (cpio_header *)target;
        unsigned long pathname_size = hexToDec(header->namesize);
        unsigned long file_size = hexToDec(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        align_4(&headerPathname_size); 
        align_4(&file_size);           

        char *file_content = target + headerPathname_size;
        for (unsigned int i = 0; i < file_size; i++) {
            uart_send(file_content[i]); // print the file content
        }
        uart_send_string("\r\n");
    }
    else {
        uart_send_string("File not found!\n");
    }
}

void load_program(char *name, void *page_table) {
    unsigned char *target = (unsigned char*)findFile(name);
    if (target) {
        cpio_header *header = (cpio_header *)target;
        unsigned long pathname_size = hexToDec(header->namesize);
        unsigned long file_size = hexToDec(header->filesize);
        unsigned long headerPathname_size = sizeof(cpio_header) + pathname_size;

        align_4(&headerPathname_size); 
        align_4(&file_size);           

        asm volatile("mov x0, %0 			\n"::"r"(page_table));
        asm volatile("dsb ish 	\n");  //ensure write has completed
        asm volatile("msr ttbr0_el1, x0 	\n"); //switch translation based address.
        asm volatile("tlbi vmalle1is 	\n");  //invalidates cached copies of translation table entries from L1 TLBs
        asm volatile("dsb ish 	\n");  //ensure completion of TLB invalidatation
        asm volatile("isb 	\n");  //clear pipeline

        unsigned char *file_content = target + headerPathname_size;
        int sz = file_size / 4096 + (file_size % 4096 != 0);
        for (int i = 0; i < sz; ++i) {
            unsigned char *addr = (unsigned char*)page_malloc(0);
            map_pages(page_table, USER_PROGRAM_VA + 4096 * i, 1, (uint64_t)VA2PA(addr));
            for (int j = 0; j < 4096; ++j)
                *(addr + j) = file_content[i * 4096 + j];
        }

        debug_printf("[DEBUG][load_program] load program: %s\n", name);
    }
    else {
        uart_send_string("File not found!\n");
    }
}