#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H
#include <stdint.h>
#include <stddef.h>
#include "initrd.h"
#include "dtb.h"
#include "allocator.h"
#include "StringUtils.h"
#include "mini_uart.h"
#include "shell.h"
#include "initrd.h"
#include "page.h"

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

//demo
void memory_reserve(uint64_t start, uint64_t end);
void init_reserve();
void test_dyn();
#endif