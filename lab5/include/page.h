#ifndef _PAGE_H
#define _PAGE_H
#include <stddef.h>
#include <stdint.h>
// 0x1000_0000 -> 0x2000_0000 meamory

#define MEMORY_BASE_ADDR 0x00
#define MEMORY_END_ADDR 0x3C000000
#define GET_PAGE_ADDR(index) (MEMORY_BASE_ADDR + (index << 12))

#define PAGE_SIZE_4K ((uint64_t)1 << 12) //4096
#define FRAME_ARRAY_SIZE ((MEMORY_END_ADDR - MEMORY_BASE_ADDR) / PAGE_SIZE_4K)
#define MAX_32K_NUM (FRAME_ARRAY_SIZE / 8)

typedef struct frame_free_node{
    uint64_t index;
    int node_size; 
    struct frame_free_node *next;
    struct frame_free_node *prev; 
}frame_free_node;

void* malloc(size_t size);
uint64_t getIndex(uint64_t addr);
void memory_init();
void pop_front(frame_free_node **list);
void add_to_list(frame_free_node **list, uint64_t index);
uint64_t request_page(int size);
uint64_t page_malloc(int sz);
void remove_from_list(frame_free_node **list, uint64_t index);
void split_node(uint64_t index , int node_size);
void return_free_node(frame_free_node *node);
void page_free(uint64_t addr, int size);
void merge_page(uint64_t index, int size);
void page_reserve(uint64_t addr , int size );

//demo method
void print_allocated_page();
void print_frame_array();
void print_frame_free_lists();
void test_page();
uint64_t get_alloc_num();
#endif
