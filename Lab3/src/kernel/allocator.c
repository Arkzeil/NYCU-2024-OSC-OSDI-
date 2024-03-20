#include "kernel/allocator.h"
// get the heap address
char *allocated = (char*)&__end;
int offset = 0;

void* simple_malloc(unsigned int size){
    // 64bits=8bytes
    size += align_offset(size, 8);

    if(offset + size > MAX_HEAP_SIZE)
        return 0;

    allocated += size;

    offset += size;
    // we need to return the head instead of tail of allocated space
    return (allocated - size);
}