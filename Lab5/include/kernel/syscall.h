#ifndef SYSCALL_H
#define SYSCALL_H

#include "kernel/syscall.h"
#include "kernel/exception_hdlr.h"
#include "kernel/thread.h"
#include "kernel/uart.h"
#include "kernel/cpio.h"
#include "kernel/mailbox.h"
#include "kernel/process.h"

extern trap_frame_t *current_tf;

int getpid();
// read user input from uart into buf
unsigned int uart_read(char buf[], unsigned int size);
// write buf to uart
unsigned int uart_write(char buf[], unsigned int size);
int exec(const char* name, char *const argv[]);
int fork(my_uint64_t stack);
void exit();
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);

#endif