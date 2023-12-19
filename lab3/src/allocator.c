#include "allocator.h"
#include "mini_uart.h"
#include "StringUtils.h"
#define MEM_SIZE 0x10000000 // 0.25G
#define MEM_START 0x10000000

unsigned long *cur_mem_point = MEM_START;

void* malloc(size_t size) {
  Align_4(&size);
  unsigned long *ret_mem_point = cur_mem_point;
  cur_mem_point+=(unsigned long)size;
  return ret_mem_point;
}