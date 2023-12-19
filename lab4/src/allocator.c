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

const int slot_type = 4;
const int slot[] = {32, 64, 128, 256};
const int slot_record_bits[] = {32, 15, 8, 4}; // page 內各大小slot數量
//==============================================================================================
const uint64_t slot_max[] = {UINT32_MAX, UINT16_MAX - 1, UINT8_MAX, 15};        // 
//================================================================================================
const uint64_t slot_masks[] = {UINT32_MAX, UINT16_MAX, UINT8_MAX, UINT8_MAX};  // the maximum number that can be stored in an unsigned x bit integer
const int slot_shift_amout[] = {32, 16, 8, 0};
const int slot_offset[] = {64, 64 + 32 * 32, 64 + 32 * 32 + 64 * 15, 64 + 32 * 32 + 64 * 15 + 128 * 8};  // 前64bits存slot狀態,有5bits沒用到

extern void *_dtb_ptr;

frame_free_node *allocated_pages = NULL; 
// =============================================================================================
void *kmalloc(size_t size) {
    int slot_size = round_to_smallest(size);
    frame_free_node * page = get_page_with_slot(slot_size);
    return allocate_slot(page, slot_size);
}

void *allocate_slot(frame_free_node *page, int size) {
    uint64_t record = get_slot_record(page, size);
    uint64_t mask = 1;
    int i;
    for (i = 0; i < slot_record_bits[size]; ++i) {
        if ((record & mask) == 0)
            break;
        mask <<= 1;
    }
    set_slot_record(page, size, i, 1);
    if (i == slot_record_bits[size])
        uart_printf("[ERROR] allocate_slot: should not reach here!\n");
    uint64_t addr = (MEMORY_BASE_ADDR + (page->index << 12)) + slot_offset[size] + slot[size] * i; 
    return (void*)addr;
}

void kfree(void *addr) {
    frame_free_node *page = find_page(addr);
    uint64_t _addr = (uint64_t)addr - (MEMORY_BASE_ADDR + (page->index << 12));
    int size;
    for (size = 3; size >= 0; --size) {
        if (_addr >= slot_offset[size])
            break;
    }
    _addr -= slot_offset[size];
    int which_slot = _addr / slot[size];
    set_slot_record(page, size, which_slot, 0);
    free_page_if_empty(page);
}



/* set/unset the bit in the record for a slot of a size of a page */

void set_slot_record(frame_free_node *page, int size, int which_slot, int value) {
    uint64_t record = get_slot_record(page, size);
    uint64_t mask = (uint64_t)1 << which_slot;
    if (value == 0) {
        mask = ~mask;
        record &= mask;
    }
    else
        record |= mask;
    uint64_t *full_record_ptr = (uint64_t *)(MEMORY_BASE_ADDR + (page->index << 12)); 
    mask = slot_masks[size] << slot_shift_amout[size];
    *full_record_ptr &= ~mask;
    *full_record_ptr |= (record << slot_shift_amout[size]);
    if (which_slot >= slot_record_bits[size])
        uart_printf("[ERROR] set_slot_record: invlaid slot index!\n");
    uart_printf("[set_slot_record] set %d'th of %d bytes slot to %d\n", which_slot, slot[size], value);
}

// =============================================================================================

uint64_t get_slot_record(frame_free_node *page, int size) {
    uint64_t page_addr = GET_PAGE_ADDR(page->index);
    uint64_t record = *(uint64_t *)page_addr;                    //看前64 bits
    return (record & (slot_masks[size] << slot_shift_amout[size])) >> slot_shift_amout[size];
}

frame_free_node *find_page(void *addr) {
    uint64_t _addr = (uint64_t)addr;
    uint64_t mask = (~(uint64_t)0) << 12;
    _addr &= mask;   // mask低位 
    frame_free_node *iter = allocated_pages;
    while (iter) {
        if ((MEMORY_BASE_ADDR + (iter->index << 12)) == _addr)
            break;
        iter = iter->next;
    }
    return iter;
}

int round_to_smallest(size_t size) {           //4捨5入找最相近的slot
    if (size <= 0)
        uart_printf("[ERROR] round_to_smallest: invalid size!\n");
    if (size > 256)
        uart_printf("[ERROR] round_to_smallest: too large!\n");
    int i;
    for (i = 0; i < slot_type; ++i) {
        if (slot[i] >= size)
            break;
    }

    uart_printf("[round_to_smallest] round %d to %d bytes\n", size, slot[i]);
    return i;
}

frame_free_node *get_page_with_slot(int size) {   // 從allocated_pages找有可用slot的page
    frame_free_node *page = allocated_pages;
    while (page) {
        if (!is_full(page, size))
            break;
        page = page->next;
    }   
    // 沒有已分配的page有可用的slot
    if (!page) {
        uint64_t addr = page_malloc();
        uint64_t index = ((addr - MEMORY_BASE_ADDR) >> 12);
        add_to_list(&allocated_pages, index);     
        page = allocated_pages;
        
    }
    return page;
}

int is_full(frame_free_node *page, int size) {
    return get_slot_record(page, size) >= slot_max[size];
}

int is_empty(frame_free_node *page, int size) {
    return get_slot_record(page, size) == 0;
}


void memory_reserve(uint64_t start, uint64_t end) {
    for (uint64_t i = start; i < end + PAGE_SIZE_4K; i += PAGE_SIZE_4K)
        page_reserve(i, 0);
}

void clear_page(frame_free_node *page) {
    char *addr = (char*)(MEMORY_BASE_ADDR + (page->index << 12));
    int sz = 1 << 12;
    for (int i = 0; i < sz; ++i)
        addr[i] = 0;
}

void free_page_if_empty(frame_free_node *page) {
    for (int i = 0; i < 4; ++i) {
        if (!is_empty(page, i))
            return;
    }
    remove_from_list(&allocated_pages, page->index);
    page_free((MEMORY_BASE_ADDR + (page->index << 12)), 0);
}

void print_slot_record(frame_free_node *page) {
    if (page) {
        uart_printf("[Slot record] (page index: %ld) ", page->index);
        uart_printf(" | ");
        for (int i = 0; i < 4; ++i) {
            uint64_t record = get_slot_record(page, i);
            uart_printf("%ld : %x", 1<<(5+i) , record);
            uart_printf(" | ");
        }
        uart_printf("\n");
    }
    else
        uart_printf("[Slot record] page already freed!\n");
}

//================================================================

void init_reserve() {
    memory_reserve(0x0000, 0x1000);     // spin tables for multicore boot
    memory_reserve(0x80000, 0x800000);  // kernel and heap/stack space
    traverse_device_tree(_dtb_ptr,get_initramfs_addr);
    memory_reserve((uintptr_t)initramfs_start, (uintptr_t)initramfs_end);  // reserve initramfs
    memory_reserve(dtb_start , dtb_end);  // reserve device tree

    uart_printf("\n[init_reserve] reserves %x 4K pages\n", get_alloc_num());

}
//==============================================================================================================

void test_dyn(){
    const int test_size = 10;
    const int test_cases[] = {200,200,230,240,250,60,70,80,90,200};
    //const int test_cases[] = {200};
    void *addrs[test_size];
    for (int i = 0; i < test_size; ++i) {
        addrs[i] = kmalloc(test_cases[i]);
        print_slot_record(find_page(addrs[i]));
        uart_printf("\n");
    }
    for (int i = test_size-1; i >= 0; --i) {
        kfree(addrs[i]);
        print_slot_record(find_page(addrs[i]));
        uart_printf("\n");
    }
}
