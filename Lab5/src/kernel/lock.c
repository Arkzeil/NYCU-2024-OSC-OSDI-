#include "kernel/lock.h"

void lock(void){
    asm volatile(
        "msr daifset, 0xf;"
    );
}

void unlock(void){
    asm volatile(
        "msr daifclr, 0xf;"
    );
}