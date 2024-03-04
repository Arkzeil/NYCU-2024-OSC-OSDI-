#ifndef ALLOCATOR_H
#define ALLOCATOR_H

extern char* heap_end;
// return requested 'size' bytes which are continuous space
void* simple_malloc(unsigned int size);

#endif