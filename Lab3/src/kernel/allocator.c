#include "kernel/allocator.h"
// get the heap address
char *heap_start = (char*)&__end;
int offset = 0;

void* simple_malloc(unsigned int size){
    // 64bits=8bytes
    size += align_offset(size, 8);
    if (offset + size > MAX_HEAP_SIZE) {
        return 0; // Heap is full
    }
    
    char *allocated = heap_start + offset;
    offset += size; // Update offset for next allocation
    return allocated;
}