#include "memory.h"
#include "mini_uart.h"
#include "shell.h"


frame_free_node *frame_free_lists[4];   // 4K, 8K, 16K, 32K
char frame_array[FRAME_ARRAY_SIZE];     // 0xBB: buddy, 0xAA: allocated
frame_free_node *node_pool_head;        // only mantain the "next" pointer, should not use "prev"
frame_free_node node_pool[FRAME_ARRAY_SIZE];
frame_free_node *free_node_table[FRAME_ARRAY_SIZE];     // map index to node to enable O(1) search and removal


void memory_init() {
    for (int i = 0; i < FRAME_ARRAY_SIZE; ++i) {
        frame_array[i] = 0xBB;
        if (i % 8 == 0)
            frame_array[i] = 3;
        free_node_table[i] = NULL;
    }

    for (int i = 0; i < MAX_32K_NUM - 1; ++i) {
        node_pool[i].next = &node_pool[i + 1];
        node_pool[i + 1].prev = &node_pool[i];
    }
    node_pool[MAX_32K_NUM - 1].next = NULL;
    node_pool[0].prev = NULL;
    for (int i = 0; i < MAX_32K_NUM; ++i) {
        node_pool[i].index = i << 3;
        node_pool[i].list_addr = &frame_free_lists[3];
        free_node_table[i << 3] = &node_pool[i];
    }
    
    for (int i = MAX_32K_NUM; i < FRAME_ARRAY_SIZE - 1; ++i)
        node_pool[i].next = &node_pool[i + 1];
    node_pool[FRAME_ARRAY_SIZE - 1].next = NULL;
    
    frame_free_lists[0] = NULL;
    frame_free_lists[1] = NULL;
    frame_free_lists[2] = NULL;
    frame_free_lists[3] = &node_pool[0];
    node_pool_head = &node_pool[MAX_32K_NUM];
}

uint64_t page_malloc(int sz) {
    uint64_t index = request_page(sz);
    return GET_PAGE_ADDR(index);
}

/* currently support 4K page request only, the value of frame array may be wrong otherwise */
uint64_t request_page(int size) {
    // if (!get_free_num()) {
    //     uart_printf("[ERROR][request_page] run out of memory!\n");
    //     while (1) {}
    // }

    if (size < 0 || size > 3) {
        uart_printf("[ERROR][request_page] request_page(%d): illegal argument!\n", size);
        return 0;
    }

    frame_free_node *free_node = frame_free_lists[size];
    uint64_t index;
    if (free_node) {
        index = free_node->index;
        pop_front(&frame_free_lists[size]);
    }
    else {
        index = request_page(size + 1);
        int end = index + (2 << size) - 1;
        int mid = (index + end) / 2;
        for (int i = index + 1; i <= end; ++i)
            frame_array[i] = 0xBB;
        frame_array[mid + 1] = size;
        add_to_list(&frame_free_lists[size], mid + 1);
        debug_printf("[DEBUG][request_page] release redundant memory, index: %ld, size: %ldK\n", mid + 1, 4 * (1 << size));
    }
    for (int i = 0; i < (1 << size); ++i)
        frame_array[index + i] = 0xAA;
    debug_printf("[DEBUG][request_page] allocate memory, index: %ld, size: %dK\n", index, 4 << size);

    return index;
}

/* same as request_page but guarantees to cover specified address */
uint64_t reserve_page(int size, uint64_t addr) {
    if (size < 0 || size > 3) {
        uart_printf("[ERROR][reserve_page] reserve_page(%d): illegal argument!\n", size);
        return 0;
    }

    uint64_t index = GET_PAGE_INDEX(addr);
    uint64_t aligned_index = index & (~(uint64_t)0 << size);
    frame_free_node *free_node = free_node_table[aligned_index];
    if (free_node && free_node->list_addr == &frame_free_lists[size]) {
        remove_from_list(&frame_free_lists[size], aligned_index);
        debug_printf("[DEBUG][reserve_page] allocate memory, index: %ld, size: %dK\n", aligned_index, 4 << size);
    }
    else {
        aligned_index = reserve_page(size + 1, addr);
        int end = aligned_index + (2 << size) - 1;
        int mid = (aligned_index + end) / 2;
        for (int i = aligned_index + 1; i <= end; ++i)
                frame_array[i] = 0xBB;
        if (index <= mid) {     
            frame_array[mid + 1] = size;
            add_to_list(&frame_free_lists[size], mid + 1);
            debug_printf("[DEBUG][reserve_page] release redundant memory, index: %ld, size: %ldK\n", mid + 1, 4 * (1 << size));
        }
        else {
            frame_array[aligned_index] = size;
            add_to_list(&frame_free_lists[size], aligned_index);
            debug_printf("[DEBUG][reserve_page] release redundant memory, index: %ld, size: %ldK\n", aligned_index, 4 * (1 << size));
        }
        debug_printf("[DEBUG][reserve_page] allocate memory, index: %ld, size: %dK\n", index, 4 << size);
    }
    for (int i = 0; i < (1 << size); ++i)
            frame_array[index + i] = 0xAA;

    return aligned_index;
}

void page_free(uint64_t addr, int size) {
    uint64_t index = getIndex(addr, size);
    if (index >= FRAME_ARRAY_SIZE || frame_array[index] != 0xAA)
        uart_printf("[ERROR][page_free] page_free: illegal index!\n");
    
    debug_printf("[DEBUG][page_free] free %ldK page, index: %ld\n", 4 << size, index);
    frame_array[index] = size;
    for (int i = 1; i < (1 << size); ++i)
        frame_array[index + i] = 0xBB;
    add_to_list(&frame_free_lists[size], index);
    merge_page(index, size + 1);
}

void merge_page(uint64_t index, int size) {
    if (size > 3)
        return;

    uint64_t mask = (1 << size) - 1;
    uint64_t start = index & ~mask;
    uint64_t end = index | mask;
    int mergable = 1;
    for (uint64_t i = start; i <= end; ++i) {
        if (frame_array[i] == 0xAA) {
            mergable = 0;
            break;
        }
    }

    if (!mergable)
        return;

    debug_printf("[DEBUG][merge_page] merge into %ldK page\n", 4 << size);
    for (uint64_t i = start + 1; i <= end; ++i)
        frame_array[i] = 0xBB;
    frame_array[start] = size;
    for (uint64_t i = start; i <= end; ++i) {
        if (free_node_table[i])
            remove_from_list(&frame_free_lists[size - 1], i);
    }
    add_to_list(&frame_free_lists[size], start);

    merge_page(index, size + 1);
}

/* remove the first free node from a list */
void pop_front(frame_free_node **list) {
    frame_free_node *free_node = *list;
    *list = (*list)->next;
    if (*list)
        (*list)->prev = NULL;
    return_free_node(free_node);
}

/* remove the free node holding specify index from a list, if exits */
void remove_from_list(frame_free_node **list, uint64_t index) {
    frame_free_node *target = free_node_table[index];
    if (!target)
        uart_printf("[ERROR][remove_from_list] illegal index!\n");
    free_node_table[index] = NULL;
    if (target->list_addr != list)
        uart_printf("[ERROR][remove_from_list] list address not matched!\n");
    target->list_addr = NULL;
    if (target == *list) {
        *list = (*list)->next;
        (*list)->prev = NULL;
    }
    else {
        if (target->next)
            target->next->prev = target->prev;
        if (!target->prev)
            uart_printf("[ERROR][remove_from_list] should not reach here!\n");
        target->prev->next = target->next;
    }
    return_free_node(target);
}

/* add a free node holding specify index to a list */
void add_to_list(frame_free_node **list, uint64_t index) {
    frame_free_node *new_node = get_free_node();
    free_node_table[index] = new_node;
    new_node->list_addr = list;
    new_node->index = index;
    new_node->prev = NULL;
    new_node->next = *list;
    if (*list)
        (*list)->prev = new_node;
    (*list) = new_node;
}

uint64_t getIndex(uint64_t addr, int size) {
    uint64_t _addr = addr - MEMORY_BASE_ADDR;
    int page_size = (1 << size) << 12;
    if (_addr % page_size != 0)
        uart_printf("[ERROR][getIndex] getIndex: illegal address: %x\n", _addr);
    return _addr / PAGE_SIZE_4K;
}

frame_free_node *get_free_node() {
    frame_free_node *node = node_pool_head;
    if (!node)
        uart_printf("[ERROR][get_free_node] get_free_node: no more nodes!\n");
    node_pool_head = node_pool_head->next;
    return node;
}

void return_free_node(frame_free_node *node) {
    if (!node)
        uart_printf("[ERROR][return_free_node] null node!\n");
    node->next = node_pool_head;
    node_pool_head = node;
}

uint64_t get_allocated_num() {
    uint64_t cnt = 0;
    for (uint64_t i = 0; i < FRAME_ARRAY_SIZE; ++i) {
        if (frame_array[i] == 0xAA)
            ++cnt;
    }
    return cnt;
}

uint64_t get_free_num() {
    return (MEMORY_END_ADDR - MEMORY_BASE_ADDR) / 4096 - get_allocated_num();
}

/* clear first 64 bytes */
void clear_4K_page(uint64_t index) {
    uint64_t *addr = (uint64_t*)GET_PAGE_ADDR(index);
    addr[0] = 0;
}


void print_frame_array() {
    uart_printf("frame_array: ");
    for (uint64_t i = 0; i < 8; ++i) {
        uart_printf("(%ld, %x) ", i, frame_array[i]);
    }
    uart_printf("\n");
}

void print_frame_free_lists() {
    uart_printf("frame_free_lists: ");
    for (int i = 0; i < 4; ++i) {
        if (frame_free_lists[i])
            uart_printf("(%d, %ld) ", i, frame_free_lists[i]->index);
        else
            uart_printf("(%d, NULL) ", i);
    }
    uart_printf("\n");
}