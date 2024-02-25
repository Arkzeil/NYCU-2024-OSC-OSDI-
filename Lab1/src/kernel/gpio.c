#include "gpio.h"

static inline void mmio_write(uint32_t reg, uint32_t data){ //MMIO(Memory Mapped IO) : all interactions with hardware on the Raspberry Pi occur using MMIO.
    //vollatile: get the variable from memory directly, instead from register(which may resulted from compiler optimization)
    //see: https://ithelp.ithome.com.tw/articles/10308388
    *(volatile uint32_t*)reg = data; 
}

static inline uint32_t mmio_read(uint32_t reg){
    return *(volatile uint32_t*)reg;
}