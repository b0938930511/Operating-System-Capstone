#ifndef	_MEMORY_H
#define	_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "peripherals/base.h"

#define MEMORY_BASE_ADDR (KVA + 0x10000000)
#define MEMORY_END_ADDR (KVA + 0x10000000 + 0x10000000)
#define PAGE_SIZE_4K ((uint64_t)1 << 12)
#define FRAME_ARRAY_SIZE ((MEMORY_END_ADDR - MEMORY_BASE_ADDR) / PAGE_SIZE_4K)
#define MAX_32K_NUM (FRAME_ARRAY_SIZE / 8)
#define GET_PAGE_ADDR(index) (MEMORY_BASE_ADDR + (index << 12))
#define GET_PAGE_INDEX(addr) ((addr - MEMORY_BASE_ADDR) >> 12)


typedef struct frame_free_node {
    uint64_t index;
    struct frame_free_node *next;
    struct frame_free_node *prev;          // double linked list to enable O(1) removal
    struct frame_free_node **list_addr;    // find out which list it belongs to in Q(1)
} frame_free_node;

void memory_init();
uint64_t page_malloc(int sz);
uint64_t request_page(int size);
uint64_t reserve_page(int size, uint64_t addr);
void page_free(uint64_t addr, int size);
void merge_page(uint64_t index, int size);
void pop_front(frame_free_node **list);
void remove_from_list(frame_free_node **list, uint64_t index);
void add_to_list(frame_free_node **list, uint64_t index);
uint64_t getIndex(uint64_t addr, int size);
frame_free_node *get_free_node();
void return_free_node(frame_free_node *node);
uint64_t get_allocated_num();
uint64_t get_free_num();
void clear_4K_page(uint64_t index);

void print_frame_array();
void print_frame_free_lists();

#endif
