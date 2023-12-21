#include "allocator.h"
#include "utils.h"
#include "mini_uart.h"
#include "shell.h"
#include "cpio.h"


/* record the usage of each slot */
const int slot_size = 4;
const int slot[] = {32, 64, 128, 256};
const uint64_t slot_max[] = {UINT32_MAX, UINT16_MAX - 1, UINT8_MAX, 15};
const int slot_record_bits[] = {32, 15, 8, 4};
const uint64_t slot_masks[] = {UINT32_MAX, UINT16_MAX, UINT8_MAX, UINT8_MAX};
const int slot_shift_amout[] = {32, 16, 8, 0};
const int slot_offset[] = {64, 64 + 32 * 32, 64 + 32 * 32 + 64 * 15, 64 + 32 * 32 + 64 * 15 + 128 * 8};

/* mantain pages */
frame_free_node *allocated_pages = NULL;

void *kmalloc(size_t size) {
    int sz = round_to_smallest(size);
    frame_free_node * page = get_page_with_slot(sz);
    return allocate_slot(page, sz);
}

void kfree(void *addr) {
    frame_free_node *page = find_page(addr);
    uint64_t _addr = (uint64_t)addr - GET_PAGE_ADDR(page->index);
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

/* get usage record of the slot size in the page */
uint64_t get_slot_record(frame_free_node *page, int size) {
    uint64_t page_addr = GET_PAGE_ADDR(page->index);
    uint64_t record = *(uint64_t *)page_addr;
    return (record & (slot_masks[size] << slot_shift_amout[size])) >> slot_shift_amout[size];
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
    uint64_t *full_record_ptr = (uint64_t *)GET_PAGE_ADDR(page->index);
    mask = slot_masks[size] << slot_shift_amout[size];
    *full_record_ptr &= ~mask;
    *full_record_ptr |= (record << slot_shift_amout[size]);
    if (which_slot >= slot_record_bits[size])
        uart_printf("[ERROR] set_slot_record: invlaid slot index!\n");
    debug_printf("[DEBUG][set_slot_record] set %d'th of %d bytes slot to %d\n", which_slot, slot[size], value);
}

/* check if there is any empty 2^size bytes slot is spedified page */
int is_full(frame_free_node *page, int size) {
    return get_slot_record(page, size) == slot_max[size];
}

int is_empty(frame_free_node *page, int size) {
    return get_slot_record(page, size) == 0;
}

int round_to_smallest(size_t size) {
    if (size <= 0)
        uart_printf("[ERROR] round_to_smallest: invalid size!\n");
    if (size > 256)
        uart_printf("[ERROR] round_to_smallest: too large!\n");
    int i;
    for (i = 0; i < 4; ++i) {
        if (slot[i] >= size)
            break;
    }
    if (i == 4)
        uart_printf("[ERROR][round_to_smallest] should not reach here!\n");
    debug_printf("[DEBUG][round_to_smallest] round %d to %d bytes\n", size, slot[i]);
    return i;
}

frame_free_node *get_page_with_slot(int size) {
    frame_free_node *page = allocated_pages;
    while (page) {
        if (!is_full(page, size))
            break;
        page = page->next;
    }   
    if (!page) {
        uint64_t addr = page_malloc(0);
        uint64_t index = GET_PAGE_INDEX(addr);
        add_to_list(&allocated_pages, index);
        page = allocated_pages;
        clear_4K_page(page->index);
    }
    return page;
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
    uint64_t addr = GET_PAGE_ADDR(page->index) + slot_offset[size] + slot[size] * i;
    return (void*)addr;
}

frame_free_node *find_page(void *addr) {
    uint64_t _addr = (uint64_t)addr;
    uint64_t mask = (~(uint64_t)0) << 12;
    _addr &= mask;
    frame_free_node *iter = allocated_pages;
    while (iter) {
        if (GET_PAGE_ADDR(iter->index) == _addr)
            break;
        iter = iter->next;
    }
    return iter;
}

void clear_page(frame_free_node *page) {
    char *addr = (char*)GET_PAGE_ADDR(page->index);
    int sz = 1 << 12;
    for (int i = 0; i < sz; ++i)
        addr[i] = 0;
}

void print_slot_record(frame_free_node *page) {
    if (page) {
        uart_printf("[Slot record] (page index: %ld) ", page->index);
        for (int i = 0; i < 4; ++i) {
            uint64_t record = get_slot_record(page, i);
            uart_printf("%x ", record);
        }
        uart_printf("\n");
    }
    else
        uart_printf("[Slot record] page already freed!\n");
}

void free_page_if_empty(frame_free_node *page) {
    for (int i = 0; i < 4; ++i) {
        if (!is_empty(page, i))
            return;
    }
    remove_from_list(&allocated_pages, page->index);
    page_free(GET_PAGE_ADDR(page->index), 0);
}


void memory_reserve(uint64_t start, uint64_t end) {
    for (uint64_t i = start; i < end + PAGE_SIZE_4K; i += PAGE_SIZE_4K)
        reserve_page(0, i);
}

void init_reserve() {
    // memory_reserve(KVA + 0x0000, KVA + 0x1000);     // spin tables for multicore boot
    // reserve_page(0, KVA + 0x2000);   // L2 page table for kernel
    // reserve_page(0, KVA + 0x3000);   // L2 page table for kernel
    // memory_reserve(KVA + 0x80000, KVA + 0x800000);  // kernel and heap/stack space
    debug_printf("[DEBUG][init_reserve] reserves 0X%x 4K pages\n", get_allocated_num());
}