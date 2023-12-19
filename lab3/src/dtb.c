#include "dtb.h"
#include "mini_uart.h"
#include "StringUtils.h"

int free_space = 0;
extern char * cpio_addr;


typedef struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
}fdt_header;

// little endian to big endian
uint32_t get_be_int(void *ptr) {
    unsigned char *bytes = ptr;
    uint32_t ret = bytes[3];
    ret |= bytes[2] << 8;
    ret |= bytes[1] << 16;
    ret |= bytes[0] << 24;

    return ret;
}

void write_indent(int n) {
  while (n--) uart_send_string(" ");
}

static uint32_t align_up(uint32_t size, int alignment) {
  return (size + alignment - 1) & -alignment;
}

static int parse_struct(uintptr_t cur, uintptr_t strings, uint32_t totalsize, dtb_callback cb){
    uintptr_t end = cur + totalsize;
    while (cur < end) {
        uint32_t token = get_be_int((char *)cur);
        cur += 4;
        switch(token){
            case FDT_BEGIN_NODE:
                cb(FDT_BEGIN_NODE , (char *)cur , NULL ,0);
                cur += align_up(strlen((char *)cur)+1,4);
                break;
            case FDT_END_NODE:
                cb(FDT_END_NODE, NULL, NULL, 0);
                break;
            case FDT_NOP:
                cb(FDT_NOP, NULL, NULL, 0);
                break;
            case FDT_PROP:{
                uint32_t size = get_be_int((char *)cur);
                cur += 4;
                uint32_t nameoff = get_be_int((char *)cur);
                cur += 4;
                cb(FDT_PROP, (char *)(strings + nameoff), (void *)cur, size);
                cur += align_up(size,4);
                break;
            }
            case FDT_END:
                cb(FDT_END, NULL, NULL, 0);
                return 0;
            default:
                return -1;
        } 
    }
    return -1;
}

int traverse_device_tree(void *dtb, dtb_callback cb) {
  uintptr_t dtb_ptr = (uintptr_t) dtb;
  uart_send_string("\n\rdtb  at:");
  uart_hex(dtb_ptr);
  uart_send_string("\n");
  fdt_header *header = (fdt_header *)dtb_ptr;
  if (get_be_int(&header->magic) != 0xd00dfeed) {
    uart_send_string("header magic != 0xd00dfeed\n");
    return -1;
  }
  uint32_t totalsize = get_be_int(&header->totalsize);
  uintptr_t dt_struct_ptr = dtb_ptr + get_be_int(&header->off_dt_struct);
  uintptr_t dt_strings_ptr = dtb_ptr + get_be_int(&header->off_dt_strings);


  parse_struct(dt_struct_ptr, dt_strings_ptr, totalsize, cb);
}

void print_dtb(int type, const char *name, const void *data, uint32_t size)
{
    switch (type)
    {
    case FDT_BEGIN_NODE:
        uart_send_string("\n");
        write_indent(free_space);
        uart_send_string(name);
        uart_send_string("{\n ");
        free_space++;
        break;

    case FDT_END_NODE:
        uart_send_string("\n");
        free_space--;
        if (free_space>0) write_indent(free_space);
        
        uart_send_string("}\n");
        break;

    case FDT_NOP:
        break;

    case FDT_PROP:
        write_indent(free_space);
        uart_send_string(name);
        break;

    case FDT_END:
        break;
    }
}


void get_initramfs_addr(int type, const char *name, const void *data, uint32_t size)
{
    if(type==FDT_PROP&&!compareString(name,"linux,initrd-start")){
        cpio_addr=(char *)(uintptr_t)get_be_int(data);
        uart_send_string("initramfs_addr at ");
        uart_hex((uintptr_t)get_be_int(data));
        uart_send('\n');
    }
}