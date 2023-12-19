#include "page.h"
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


frame_free_node *frame_free_lists[4];   // 4k,8k,16k,32k which page can be use
char frame_array[FRAME_ARRAY_SIZE];     // 0xBB: buddy, 0xAA: allocated
frame_free_node *free_node_table[FRAME_ARRAY_SIZE];  // 紀錄node位子
frame_free_node *node_pool_head;              //指向pool裡可用的node
frame_free_node node_pool[FRAME_ARRAY_SIZE]; //存可用的node,最多只會有 FRAME_ARRAY_SIZE 個 node . 



uint64_t getIndex(uint64_t addr) {
    uint64_t _addr = addr - MEMORY_BASE_ADDR;
    int page_size = 1 << 12; // 4096(4k)
    /*
    if (_addr % page_size != 0)
        uart_printf("[ERROR] getIndex: illegal address!\n");
    */
    return _addr / page_size;
}

uint64_t get_alloc_num(){
    uint64_t num = 0;
    for(uint64_t i =0 ; i < FRAME_ARRAY_SIZE ; i++)
        if(frame_array[i] == 0xAA)
            num++;
    return num;
}


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
        node_pool[i].node_size = 3;
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

/* remove the first free node from a list */
void pop_front(frame_free_node **list) {
    frame_free_node *free_node = *list;
    *list = (*list)->next;
    (*list)->prev = NULL;
    //將node還給node pool
    return_free_node(free_node);
}

/* add a free node holding specify index to a list */
void add_to_list(frame_free_node **list, uint64_t index) {
    if(node_pool_head == NULL){
        uart_printf("[ERROR] No pool node can use!\n");
        return;
    }
    frame_free_node *new_node = node_pool_head;
    node_pool_head = node_pool_head->next;
    free_node_table[index] = new_node;
    new_node->index = index;
    new_node->prev = NULL;
    new_node->next = *list;
    if (*list)
        (*list)->prev = new_node;
    (*list) = new_node;
}

/* remove the free node holding specify index from a list, if exits */
void remove_from_list(frame_free_node **list, uint64_t index) {
    frame_free_node *target = free_node_table[index];
    free_node_table[index] = NULL;
    if (target == *list) {            //當前list裡的第一個
        *list = (*list)->next;
        (*list)->prev = NULL;
    }
    else {
        target->next->prev = target->prev;
        target->prev->next = target->next;
    }
    return_free_node(target);
}

uint64_t request_page(int size) {
    if (size < 0 || size > 3) {                                   // 4k~32k
        uart_printf("[ERROR] request_page(%d): illegal argument!\n", size);
        return 0;
    }
    uint64_t *addr;
    frame_free_node *free_node = frame_free_lists[size];
    uint64_t index;
    if (free_node) {
        index = free_node->index;
        pop_front(&frame_free_lists[size]);
        frame_array[index] = 0xAA;
        addr = (uint64_t *)(MEMORY_BASE_ADDR + ( index << 12)); 
        for(uint64_t i = 0 ; i<64;i++){
            *(addr+i) = 0;  
        }
        uart_printf("[Page malloc] index: %ld, size: %dK\n", index, 4 << size);
    }
    else {
        index = request_page(size + 1);
        int end = index + (2 << size) - 1;
        int mid = (index + end) / 2;
        for (int i = mid + 2; i <= end; ++i)
            frame_array[i] = 0xBB;
        frame_array[mid + 1] = size;
        frame_array[index] = 0xAA;
        addr = (uint64_t *)(MEMORY_BASE_ADDR + ( index << 12)); 
        for(uint64_t i = 0 ; i< 64;i++){
            *(addr+i) = 0;  
        }
        /*
        if (!node_pool_head)
            uart_printf("[ERROR] request_page(%d): no more nodes!\n", size);
        */
        add_to_list(&frame_free_lists[size], mid + 1);
        uart_printf("[Page malloc] release redundant memory, index: %ld, size: %ldK\n", mid + 1, 4 * (1 << size));
    }

    return index;
}
//page malloc
uint64_t page_malloc() {
    uint64_t index = request_page(0);
    return MEMORY_BASE_ADDR + (index << 12);
}





void page_reserve(uint64_t addr , int size ) {   //size = {0,1,2,3}
    uint64_t index = getIndex(addr);    //index in 4k
    for(int i = 0 ; i<size+1;i++)
    {
        if(frame_array[index+i] == 0xAA){
            uart_printf("[page_reserve] addr: %ld has been allocated\n", addr);
            return ;
        }
    }

    if(size > 0){
        for(int i = 0 ; i < size + 1 ; i++)
            page_reserve(addr + (4096*i) , 0);
    }
    else{
            while(free_node_table[index] == NULL || (*free_node_table[index]).node_size > size){
                uint64_t sub_index = index;
                while(free_node_table[sub_index] == NULL)
                    sub_index-- ;
                if((*free_node_table[sub_index]).node_size > size)
                    split_node(sub_index , (*free_node_table[sub_index]).node_size);
                else if((*free_node_table[sub_index]).node_size < size){
                    uart_printf("[ERROR] page_reserve addr: %ld alloc size error \n", addr);
                    return ;
                }
            }
            // alloc memory
            remove_from_list(&frame_free_lists[size] , index);
            for(int i = index ;i<index+size+1;i++){
                frame_array[i] = 0xAA;
                uart_printf("[page_reserve] index: %ld has be reserved \n", i);
            }
        
    }
    
}

void split_node(uint64_t index , int node_size){
    int offset;
    if(node_size == 3)
      offset = 4;
    else if(node_size == 2)
      offset = 2;
    else if(node_size == 1)
      offset = 1;
    
    remove_from_list(&frame_free_lists[node_size] , index);
    add_to_list(&frame_free_lists[node_size-1] , index);
    add_to_list(&frame_free_lists[node_size-1] , index+offset);
    (*free_node_table[index]).node_size = node_size-1;
    (*free_node_table[index+offset]).node_size = node_size-1;
    frame_array[index] = node_size-1;
    frame_array[index+offset] = node_size-1;
    uart_printf("[split_page] split %ldk page into two %ldK page\n", 4 << node_size , 4 << (node_size-1));
}


void return_free_node(frame_free_node *node) {
    node->next = node_pool_head;
    node_pool_head = node;
}

void print_allocated_page() {
    for (uint64_t i = 0; i < FRAME_ARRAY_SIZE; ++i) {
        if (frame_array[i] == 0xAA)
            uart_printf("[ERROR] index: %d has been allocated\n", i);
    }
}


void print_frame_array() {
    uart_printf("frame_array: ");
    for (uint64_t i = 8; i < 16; ++i) {
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

void page_free(uint64_t addr, int size) {
    uint64_t index = getIndex(addr);
    for(int i = 0;i<size+1;i++){
        if(frame_array[index+i] != 0xAA)
            uart_printf("[ERROR] page_free: illegal index!\n");
    }

    if(size>0){
        for(int i = 0 ;i<size+1 ;i++)
            page_free(addr+(i*4096),0);
    }
    else{
        frame_array[index] = 0x0;
        uart_printf("[page_free] page index : %ld be free\n", index);
        add_to_list(&frame_free_lists[size], index);
        merge_page(index, size + 1);
    }

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

    uart_printf("[merge_page] merge into %ldK page\n", 4 << size);
    for (uint64_t i = start + 1; i <= end; ++i)
        frame_array[i] = 0xBB;
    frame_array[start] = size;

    for (uint64_t i = start; i <= end; ++i) {
        if (free_node_table[i])
            remove_from_list(&frame_free_lists[size - 1], i);
    }
    add_to_list(&frame_free_lists[size], start);
    (*free_node_table[start]).node_size = size ;

    merge_page(index, size + 1);
    
}
// ============================================================================
void test_page(){
        const int test_size = 3;
        const uint64_t test_address[] = {0x8001,0x9010,0xA100};    // covered index 8(4K), 9(4K), 10(8K)
        const int page_size[] = {0,0,1};
        //const uint64_t mask[] = {0x1fff, 0xfff, 0x1fff};
        for (int i = 0; i < test_size; ++i) {
            page_reserve( test_address[i],page_size[i]);  
            print_frame_array();
            uart_printf("\n");
        }
        for (int i = 0; i < test_size; ++i) {
            page_free(test_address[i] , page_size[i]);
            print_frame_array();
            uart_printf("\n");
        }
}

// ============================================================================