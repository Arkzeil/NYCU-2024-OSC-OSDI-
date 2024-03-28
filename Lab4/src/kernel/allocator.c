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
    buddy = (buddy_system_t*)BUDDY_START;
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
            /*uart_b2x_64((unsigned long long)cur->addr);
            uart_putc(' ');
            uart_b2x_64((unsigned long long)cur->val);
            uart_putc(' ');*/
            // update the offset of buddy system memory
            alloc_offset += PAGE_SIZE * (1 << i);
            // get next block of block list
            cur += sizeof(buddy_block_list_t);
        }
        //uart_putc('\n');
        // goto next block list
        list += block_amount * sizeof(buddy_block_list_t);
    }

    /*for(i = 0; i < MAX_ORDER; i++){
        int j = 0;
        int block_amount = (1 << (MAX_ORDER - i - 1));
        for(j = 0; j < block_amount; j++){
            buddy_block_list_t *cur = buddy->list_addr[i];

            uart_b2x_64((unsigned long long)cur->size);
            uart_putc(' ');
            cur += sizeof(buddy_block_list_t);
        }
        uart_putc('\n');
    }*/
    // In the begining, only the largest block is available
    buddy->first_avail[MAX_ORDER - 1] = 0;
}
// get buddy of one level lower order
int find_buddy(int index, int order){
    if(order == 0)
        return index + 1;
    return index ^ order;
}

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

buddy_block_list_t* buddy_split(int start_index, int end_index, int req_size, int order){
    int i;

    if((PAGE_SIZE * (1 << order) / 2) < req_size || order == 0){
        buddy->first_avail[order] = get_next_avail(order);
        return (buddy_block_list_t *)buddy->list_addr[0] + start_index * sizeof(buddy_block_list_t);
    }
    for(i = order - 1; i >= 0; i--){
        int j = start_index;
        buddy_block_list_t *start = (buddy_block_list_t *)buddy->list_addr[i] + (start_index / (1 << i)) * sizeof(buddy_block_list_t);
        buddy_block_list_t *cur = (buddy_block_list_t *)start;

        for(; j < end_index && cur != 0; j += (1 << i)){
            if(cur->val == -2){
                cur->val = i;

                /*if(buddy->first_avail[i] < 0 || buddy->first_avail[i] > j)
                    buddy->first_avail[i] = j;*/
                buddy->first_avail[i] = get_next_avail(i);
            }
            cur = cur->next;
        }
        end_index /= 2;
        /*if(cur->next != 0){
            if(cur->next->val == -2){
                cur->next->val = i;

                if(buddy->first_avail[i] < 0 || buddy->first_avail[i] > j)
                    buddy->first_avail[i] = cur->next->idx;
            }
        }*/

        if((PAGE_SIZE * (1 << i) / 2) < req_size || i == 0){
            start->val = -1;
            buddy->first_avail[i] = get_next_avail(i);
            return start;
        }
        
        // mark current block as allocated(as its lower level block will be allocated in later iterations)
        start->val = -1;
    }
    /*if(order == 0)
        return (buddy_block_list_t *)buddy->list_addr[0] + start_index * sizeof(buddy_block_list_t);*/

    return 0;
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
                // marked as allocated
                cur->val = -1;

                int avail_index = buddy->first_avail[i];
                //buddy->first_avail[i] = -1;

                //split the block as small as possible
                buddy_block_list_t *buddy_allocated = buddy_split(avail_index, avail_index + (1 << i), size, i);

                // find next available block
                /*buddy_block_list_t *avail = cur;
                while(avail->next != 0){
                    if(avail->next->val >= 0){
                        buddy->first_avail[i] = avail_index;
                        break;
                    }
                    avail = avail->next;
                    avail_index += (1 << i);
                }*/
                //buddy->first_avail[i] = get_next_avail(i);

                for(i = 0; i < MAX_ORDER; i++){
                    int j = 0;
                    int block_amount = (1 << (MAX_ORDER - i - 1));
                    buddy_block_list_t *list_cur = buddy->list_addr[i];
                    for(j = 0; j < block_amount; j++){

                        uart_b2x_64((unsigned long long)list_cur->val);
                        uart_putc(' ');
                        list_cur = list_cur->next;
                    }
                    uart_putc('\n');
                }
                for(i = 0; i < MAX_ORDER; i++){
                    buddy->first_avail[i] = get_next_avail(i);
                    uart_b2x_64((unsigned long long)buddy->first_avail[i]);
                    uart_putc(' ');
                }
                uart_putc('\n');

                uart_puts("Allocated block size:");
                uart_b2x_64((unsigned long long)buddy_allocated->size);
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

}