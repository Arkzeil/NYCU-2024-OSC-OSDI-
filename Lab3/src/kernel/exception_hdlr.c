#include "kernel/uart.h"

void c_exception_handler(){
    void *spsr1;
    void *elr1;
    void *esr1;
    void *el;

    uart_puts("Entering exception handler\n");

    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        "mrs %[var3], esr_el1;"
        "mrs %[var4], CurrentEL;"
        : [var1] "=r" (spsr1),[var2] "=r" (elr1),[var3] "=r" (esr1),[var4] "=r" (el)    // Output operands
    );

    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("SPSR_EL1:  ");
    uart_b2x_64((unsigned long long)spsr1);
    uart_putc('\n');

    uart_puts("ELR_EL1:   ");
    uart_b2x_64((unsigned long long)elr1);
    uart_putc('\n');

    uart_puts("ESR_EL1:  ");
    uart_b2x_64((unsigned long long)esr1);
    uart_putc('\n');

    uart_puts("Leaving exception handler\n");
    // It will keep printing as next line of boot.S is 'b exception_handler'
    /*while(1){
        asm volatile("nop");
    }*/
}

void c_core_timer_handler(){
    unsigned long long cur_cnt, cnt_freq;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

    uart_puts("Time after boots: ");
    uart_b2x_64(cur_cnt / cnt_freq);
    uart_puts(" sec.\n");

    cnt_freq *= 2;

    asm volatile(
        "msr cntp_tval_el0, %[var1];"
        :
        :[var1] "r" (cnt_freq)
        :
    );
}