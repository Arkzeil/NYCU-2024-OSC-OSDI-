#include "kernel/allocator.h"
// get the heap address
char *allocated = (char*)&__end;
int offset = 0;
buddy_system_t *buddy = 0;

void* simple_malloc(unsigned int size){
    // 64bits=8bytes
    size += align_offset(size, 8);

    if(offset + size > MAX_HEAP_SIZE)
        return 0;

    // allocate space
    allocated += size;
    // record accumulated allocated space
    offset += size;

    // we need to return the head instead of tail of allocated space
    return (allocated - size);
}

void buddy_init(void){
    int i;
    buddy_system_t *buddy = (buddy_system_t*)BUDDY_START;
    buddy->buddy_list = (buddy_block_list_t**)(BUDDY_START + sizeof(buddy_system_t));

    //buddy_block_list_t buddy_list[MAX_ORDER];
    // the location where available memory starts(after all metadata)
    void *buddy_mem_start = (void*)(BUDDY_START + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    // the head of current size blocks
    buddy_block_list_t *list = (buddy_block_list_t *)(buddy->buddy_list);

    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int alloc_offset = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        
        buddy->first_avail[i] = (void*)list;

        buddy_block_list_t *cur = list;

        for(j = 0; j < block_amount; j++){
            cur->idx = j;
            // first, all block are belonged to the largest continuous memory block
            cur->val = MAX_ORDER - 1;
            if(cur != buddy_mem_start){
                cur->prev = cur - sizeof(buddy_block_list_t);
                cur->prev->next = cur;
            }
            else
                cur->prev = 0;
            cur->next = 0;
            // the address of the memory of corresponding block
            cur->addr = buddy_mem_start + alloc_offset;
            uart_b2x_64((unsigned long long)cur->addr);
            uart_putc(' ');
            // update the offset of buddy system memory
            alloc_offset += PAGE_SIZE * (1 << i);
            // get next block of block list
            cur += sizeof(buddy_block_list_t);
        }
        uart_putc('\n');
        // goto next block list
        list = (buddy_block_list_t *)(buddy->buddy_list + block_amount * sizeof(buddy_block_list_t));
    }
}

void* buddy_malloc(unsigned int size){
    int i;

    if(buddy == 0)
        buddy_init();

    if(size > (1 << (MAX_ORDER - 1))){
        uart_puts("Requested size is too large\n");
        return 0;
    }

    buddy_block_list_t *list_head = buddy->buddy_list;

    for(i = 0; i < MAX_ORDER; i++){
        int block_amount = (1 << (MAX_ORDER - i - 1));

        if(PAGE_SIZE * (1 << i) >= size){
            if(buddy->first_avail[i] != 0){
                buddy_block_list_t *cur = (buddy_block_list_t *)buddy->first_avail[i];
                // meaning that the mechanism went wrong, this condition should not be met
                if(cur->val < 0){
                    return 0;
                }
                //
                if(cur != list_head)
                    cur->val = -1;
                else
                    cur->val = -2;
                uart_puts("Allocated block size:");
                uart_b2x_64((unsigned long long)(PAGE_SIZE * (1 << i) ) );
                uart_putc('\n');

                buddy->first_avail[i] = cur->next;

                return cur->addr;
            }
            else{
                list_head = (buddy_block_list_t *)(buddy->buddy_list + block_amount * sizeof(buddy_block_list_t));
                continue;
            }
        }
    }

    return 0;
}

void buddy_free(void *addr){

}