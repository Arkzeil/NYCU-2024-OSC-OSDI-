#include "kernel/gpio.h"
// inline should declare here instead of the header
inline void mmio_write(unsigned int* reg, uint32_t data){ //MMIO(Memory Mapped IO) : all interactions with hardware on the Raspberry Pi occur using MMIO.
    //vollatile: get the variable from memory directly, instead from register(which may resulted from compiler optimization)
    //see: https://ithelp.ithome.com.tw/articles/10308388
    *(volatile uint32_t*)reg = data;
}

inline uint32_t mmio_read(unsigned int* reg){
    return *(volatile uint32_t*)reg;
}