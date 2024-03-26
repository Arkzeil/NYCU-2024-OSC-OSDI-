#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "kernel/utils.h"
#include "kernel/uart.h"

#define MAX_HEAP_SIZE 8192
#define PAGE_SIZE   4096
#define MAX_ORDER   6
#define BUDDY_START 0x10000000
#define BUDDY_END   0x20000000
// Get the symbol __end from linker script
extern char* __end;
// make allocated variable global among all files
extern char* allocated;
extern int offset;

/*typedef struct buddy_block{
    unsigned int idx;           // the index of the block
    int val;                    // the order of the block
    struct buddy_block *prev;   // the previous block
    struct buddy_block *next;   // the next block
    void *addr;                 // the address of the memory block
}buddy_block_t;
// a block list for free blocks of same size
typedef struct buddy_block_list{
    buddy_block_t *block;                 // the address of the first block metadata
    int first_avail;                      // the index of the first available block
}buddy_block_list_t;*/

/*buddy->2d array with blocks metadata->usable memory*/

typedef struct buddy_block_list{
    unsigned int idx;           // the index of the block(used minimum size block as unit, so the index is the index of the block in the whole memory block)
    int val;                    // the order of the block, -2 indicate that it belongs to a larger contiguous memory block, -1 indicates that is already allocated
    struct buddy_block_list *prev;   // the previous block
    struct buddy_block_list *next;   // the next block
    void *addr;                 // the address of the memory block
} buddy_block_list_t;

// a buddy system for memory allocation
typedef struct buddy_system{
    //buddy_block_list_t *buddy_list;  // store the list of free blocks of different sizes
    buddy_block_list_t **buddy_list;
    void* first_avail[MAX_ORDER];      // the address of the first available block metadata
    void* list_addr[MAX_ORDER];        // record where list starts(actually is uunecssary(as it can be calculated by adding blocks' size), just for convenience)
}buddy_system_t;

extern buddy_system_t *buddy;

// return requested 'size' bytes which are continuous space
void* simple_malloc(unsigned int size);
void buddy_init(void);
void* buddy_malloc(unsigned int size);
void buddy_free(void *addr);

#endif