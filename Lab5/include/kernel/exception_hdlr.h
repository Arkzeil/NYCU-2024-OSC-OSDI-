#ifndef EXCEPTION_HDLR_H
#define EXCEPTION_HDLR_H
// data structure that is used to represent the state of the CPU when an exception or interrupt occurs. 
// When an exception or interrupt is triggered, the OS needs to save the current state of the CPU (registers, program counter, etc.) 
// so that it can be restored later when the exception or interrupt handling is complete.
// including PC, PSR, general-purpose registers 
// and other relevant state information, such as the current execution mode (e.g., user mode vs. kernel mode), the CPU privilege level, and any additional CPU-specific state.

// So do I need to store all the general purpose registers?
typedef struct trap_frame{
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18; 
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;           // x29
    unsigned long lr;           // x30
    unsigned long sp;
    unsigned long elr_el1;      // the address of the instruction that caused the exception(load it when returning to user program from EL1 to EL0)
    unsigned long spsr_el1;     // origial process state
}trap_frame_t;

extern int test_NI;
void int_off(void);
void int_on(void);
void c_exception_handler();
void c_core_timer_handler();
void c_write_handler();
void c_recv_handler();
void c_timer_handler();

#endif