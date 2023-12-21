#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include "memory.h"
#include <stddef.h>

/*
    Partition of a 4K page: (size, amount)
    (32, 32) (64, 15) (128, 8) (256, 4)

    First 64 byte is used to record usage of each slot, shown below:
    uint32_t usage32;
    uint16_t usage64; // upper 1 bit is not used
    uint8_t usage128;
    uint8_t usage258; // upper 4 bits reserve for future use
    
    [Future use] 4 bits of record of 256-bytes slot can be used to represent fused mode to get larger slot
*/

void *kmalloc(size_t size);
void kfree(void *addr);
uint64_t get_slot_record(frame_free_node *page, int size);
void set_slot_record(frame_free_node *page, int size, int which_slot, int value);
int is_full(frame_free_node *page, int size);
int is_empty(frame_free_node *page, int size);
int round_to_smallest(size_t size);
frame_free_node *get_page_with_slot(int size);
void *allocate_slot(frame_free_node *page, int size);
frame_free_node *find_page(void *addr);
void clear_page(frame_free_node *page);
void print_slot_record(frame_free_node *page);
void free_page_if_empty(frame_free_node *page);

/*
    memory reservation
*/
void memory_reserve(uint64_t start, uint64_t end);
void init_reserve();

#endif