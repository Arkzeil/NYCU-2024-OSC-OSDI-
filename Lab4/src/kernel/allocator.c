#include "kernel/allocator.h"
// get the heap address
char *allocated = (char*)&__end;
int offset = 0;
buddy_system_t *buddy = 0;
void *buddy_mem_start = 0;

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
    buddy = (buddy_system_t*)BUDDY_START;
    buddy->buddy_list = (buddy_block_list_t**)(BUDDY_START + sizeof(buddy_system_t));

    //buddy_block_list_t buddy_list[MAX_ORDER];
    // the location where available memory starts(after all metadata)
    buddy_mem_start = (void*)(BUDDY_START + sizeof(buddy_system_t) + ((1 << MAX_ORDER) - 1) * sizeof(buddy_block_list_t));
    // the head of current size blocks
    buddy_block_list_t *list = (buddy_block_list_t *)(buddy->buddy_list);

    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int alloc_offset = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        
        buddy->first_avail[i] = -1;
        buddy->list_addr[i] = (void*)list;

        buddy_block_list_t *cur = list;

        for(j = 0; j < block_amount; j++){
            cur->idx = (j * (1 << i));
            // first, all block except largest one are belonged to the largest continuous memory block
            cur->val = -2;
            if(i == MAX_ORDER - 1)
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
            cur->size = PAGE_SIZE * (1 << i);
            // update the offset of buddy system memory
            alloc_offset += PAGE_SIZE * (1 << i);
            // get next block of block list
            cur += sizeof(buddy_block_list_t);
        }
        // goto next block list
        list += block_amount * sizeof(buddy_block_list_t);
    }

    // In the begining, only the largest block is available
    buddy->first_avail[MAX_ORDER - 1] = 0;
}
// get buddy of one level lower order
int find_buddy(int index, int order){
    return index ^ (1 << order);
}
// get the index of the next available block
int get_next_avail(int order){
    int i;
    buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[order];
    for(i = 0; i < (1 << (MAX_ORDER - 1)); i += (1 << order)){
        if(cur->val >= 0)
            return i;
        cur = cur->next;
    }
    return -1;
}
// get block index by using address
int get_index(void *addr){
    return (addr - buddy_mem_start) / PAGE_SIZE;
}
// get block metadata address by using index
buddy_block_list_t* get_block(int index){
    buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[0] + index * sizeof(buddy_block_list_t);
    buddy_block_list_t *last = cur;
    for(int i = 0; i < MAX_ORDER; i++){
        cur = (buddy_block_list_t *)buddy->list_addr[i] + (index / (1 << i)) * sizeof(buddy_block_list_t);
        // this block index is not legal in this level, indicate that it must belonged to lower levels
        if(index % (1 << i) != 0)
            return last;
        // if larger block is not -1, meaning that we reach the target block as larger block will also make smaller ones become -1
        if(cur->val != -1)
            return last;
        last = cur;
    }

    return cur;
}

buddy_block_list_t* buddy_split(int start_index, int end_index, int req_size, int order){
    int i;
    // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
    if((PAGE_SIZE * (1 << order) / 2) < req_size || order == 0){
        //buddy->first_avail[order] = get_next_avail(order);
        buddy_block_list_t *start = (buddy_block_list_t *)buddy->list_addr[order] + (start_index / (1 << order)) * sizeof(buddy_block_list_t);
        // mark it as allocated
        start->val = -1;
        return start;
    }
    // split the block until the smallest block that can be allocated
    for(i = order - 1; i >= 0; i--){
        int j = start_index;
        buddy_block_list_t *start = (buddy_block_list_t *)buddy->list_addr[i] + (start_index / (1 << i)) * sizeof(buddy_block_list_t);
        buddy_block_list_t *cur = (buddy_block_list_t *)start;

        for(; j < end_index && cur != 0; j += (1 << i)){
            // mark it as usable
            if(cur->val == -2){
                cur->val = i;
                if(cur != start){
                    uart_puts("Release redundent block:");
                    uart_b2x_64((unsigned long long)cur->size);
                    uart_putc('\n');
                }
                //buddy->first_avail[i] = get_next_avail(i);
            }
            cur = cur->next;
        }
        end_index /= 2;

        start->val = -3;
        // if the current block is already the smallest block that can be allocated or smaller block is not enough for the request
        if((PAGE_SIZE * (1 << i) / 2) < req_size || i == 0){
            start->val = -1;
            //buddy->first_avail[i] = get_next_avail(i);
            return start;
        }
        
        // mark current block as allocated(as its lower level block will be allocated in later iterations)
        //start->val = -1;
    }
    /*if(order == 0)
        return (buddy_block_list_t *)buddy->list_addr[0] + start_index * sizeof(buddy_block_list_t);*/

    return 0;
}
// mark the lower blocks within same range as allocated
void mark_allocated(buddy_block_list_t* start){
    int i;
    int order = simple_log(start->size / PAGE_SIZE, 2);
    int end_index = start->idx + (1 << order);
    for(i = order - 1; i >= 0; i--){
        int j = start->idx;
        buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[i] + (start->idx / (1 << i)) * sizeof(buddy_block_list_t);
        for(; j < end_index && cur != 0; j += (1 << i)){
            if(cur->val == -2)
                cur->val = -1;
            cur = cur->next;
        }
    }
}

void free_child(int block_index, int order){
    int i;

    for(i = order - 1; i >= 0; i--){
        int j;
        buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[i] + (block_index / (1 << i)) * sizeof(buddy_block_list_t);
        for(j = 0; j < (1 << order); j += (1 << i)){
            cur->val = -2;
            cur = cur->next;
        }
    }
}

// always allocate first block of smallest fitted size
void* buddy_malloc(unsigned int size){
    int i;

    if(buddy == 0)
        buddy_init();

    if(size > PAGE_SIZE * (1 << (MAX_ORDER - 1))){
        uart_puts("Requested size is too large\n");
        return 0;
    }


    for(i = 0; i < MAX_ORDER; i++){
        if(PAGE_SIZE * (1 << i) >= size){
            if(buddy->first_avail[i] >= 0){
                buddy_block_list_t *cur = (buddy_block_list_t *)buddy->list_addr[i] + (buddy->first_avail[i] / (1 << i)) * sizeof(buddy_block_list_t);
                //buddy_block_list_t *cur = (buddy_block_list_t *)buddy->first_avail[i];
                // meaning that the mechanism went wrong, this condition should not be met
                if(cur->val < 0){
                    uart_puts("Error: The first available block is already allocated\n");
                    return 0;
                }
                // first assuming that there's smaller block in lower layer
                cur->val = -3;

                int avail_index = buddy->first_avail[i];

                //split the block as small as possible
                buddy_block_list_t *buddy_allocated = buddy_split(avail_index, avail_index + (1 << i), size, i);
                mark_allocated(buddy_allocated);


                for(i = 0; i < MAX_ORDER; i++){
                    int j = 0;
                    int block_amount = (1 << (MAX_ORDER - i - 1));
                    buddy_block_list_t *list_cur = buddy->list_addr[i];
                    for(j = 0; j < block_amount; j++){

                        uart_itoa(list_cur->val);
                        uart_putc(' ');
                        list_cur = list_cur->next;
                    }
                    uart_putc('\n');
                }
                // update the first available block
                for(i = 0; i < MAX_ORDER; i++){
                    buddy->first_avail[i] = get_next_avail(i);
                    uart_itoa(buddy->first_avail[i]);
                    uart_putc(' ');
                }
                uart_putc('\n');

                uart_puts("Allocated block size:");
                uart_itoa(buddy_allocated->size);
                uart_puts(" at: ");
                uart_b2x_64((unsigned long long)buddy_allocated->addr);
                uart_putc('\n');
                return (void*)(buddy_allocated->addr);
            }
            else{
                continue;
            }
        }
    }

    return 0;
}

void buddy_free(void *addr){
    int block_index = get_index(addr);
    buddy_block_list_t *cur_block = get_block(block_index);
    int order = simple_log(cur_block->size / PAGE_SIZE, 2);
    int buddy_index = find_buddy(block_index, order);
    buddy_block_list_t *buddy_block = (buddy_block_list_t *)buddy->list_addr[order] + (buddy_index / (1 << order)) * sizeof(buddy_block_list_t);
    /*uart_itoa(cur_block->size);
    uart_putc(' ');
    uart_itoa(block_index);
    uart_putc(' ');
    uart_b2x_64((unsigned long long)cur_block->addr);
    uart_putc(' ');
    uart_itoa(order);
    uart_putc(' ');
    uart_itoa(buddy_index);
    uart_putc('\n');*/

    free_child(block_index, order);
    // First assuming no merge is needed
    cur_block->val = order;
    int i;
    for(i = order; i < MAX_ORDER; i++){
        // To prevet the case that block index is not a block in larger block level
        block_index = find_min(block_index, buddy_index);
        buddy_index = find_buddy(block_index, i);
        cur_block = (buddy_block_list_t *)buddy->list_addr[i] + (block_index / (1 << i)) * sizeof(buddy_block_list_t);
        buddy_block = (buddy_block_list_t *)buddy->list_addr[i] + (buddy_index / (1 << i)) * sizeof(buddy_block_list_t);

        // reaching largest block, no need to merge
        if(i == MAX_ORDER - 1){
            cur_block->val = i;;
            break;
        }

        // its buddy is also available, combine them into a larger block
        if(buddy_block->val >= 0){
            cur_block->val = -2;
            buddy_block->val = -2;
            uart_puts("Merge two blocks:");
            uart_itoa(block_index);
            uart_putc(' ');
            uart_itoa(buddy_index);
            uart_puts(" at order:");
            uart_itoa(i);
            uart_putc('\n');
        }
        else{
            cur_block->val = i;
            break;
        }
    }

    // update the first available block
    for(i = 0; i < MAX_ORDER; i++){
        buddy->first_avail[i] = get_next_avail(i);
        uart_itoa(buddy->first_avail[i]);
        uart_putc(' ');
    }
    uart_putc('\n');

    for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        buddy_block_list_t *list_cur = buddy->list_addr[i];
        for(j = 0; j < block_amount; j++){

            uart_itoa(list_cur->val);
            uart_putc(' ');
            list_cur = list_cur->next;
        }
        uart_putc('\n');
    }
}