#include "kernel/allocator.h"

void* simple_malloc(unsigned int size){
    char *allocated = heap_end;
    heap_end += size;

    return allocated;
}