# Lab4
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab4.html)
---
## Basic Exercises
### Basic Exercise 1 - Buddy System
+ background
+ Allocation
    + The idea will be first put whole metadata into 0x10000000, which starting with one ```buddy_system_t``` struct then ```(1 << MAX_ORDER) - 1``` ```buddy_block_list_t``` structs. The rest of the space will be used for memory allocation.
    + Defina a struct ```buddy_block_list_t``` as the metadata of every memory block
    + Define a struct ```buddy_system_t``` as the buddy system metadata and it contains
        1. ```buddy_block_list_t **buddy_list```: a 2D linked lists, which is the metadata of every memory block
        2. ```int first_avail[MAX_ORDER]```     : Store the first available block index of every list(the index amount is constant, so for 3rd list, its index of blocks will be 0->4->8, store 4 as first_avail if 0 is allocated)
        3. ```void* list_addr[MAX_ORDER];```    : Store the address of every list's metadata(i.e. the address of every ```*buddy_list```)
    + ```buddy_init``` should be used when the buddy system is first called(malloc) as it will initialize all metadata and allocate memory space to each block
+ Free
    
### Basic Exercise 2 - Dynamic Memory Allocator 
+ Background


### Basic Exercise 3 - Rpi3’s Peripheral Interrupt
+ Background
+ 
---
## Advanced Exercises
### Advanced Exercise 1 - Efficient Page Allocation
+ Background
+ 


### Advanced Exercise 2 - Reserved Memory 
+ 

### Advanced Exercise 3 - Startup Allocation
