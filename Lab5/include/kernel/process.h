#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/allocator.h"
#include "kernel/lock.h"
#include "kernel/exception_hdlr.h"
#include "kernel/mem.h"
#include "kernel/syscall.h"

#define NR_TASKS 64
#define TASK_RUNNING 0
#define TASK_WAITING 1
#define TASK_ZOMBIE -1
#define PF_KTHREAD   2

// calee saved registers
typedef struct process_context{
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
    unsigned long fp;   //x29, pointed to the bottom of the stack, which is the value of the stack pointer just before the function was called(should be immutable).
    unsigned long lr;   //x30, but it's refered as PC in some implementation
    unsigned long sp;
}process_context_t;

// this struct must match the format we defined in save_all and load_all
typedef struct task_struct{
    // this is used for saving the context of the process when it's scheduled out
    process_context_t context;
    unsigned long sp;
    trap_frame_t *tf;
    int status;
    int pid;
    int flag;
}task_struct_t;

extern task_struct_t *current_task;
extern task_struct_t *task[NR_TASKS];
extern int nr_tasks;
// this is the task of kernel shell
#define INIT_TASK { {0,0,0,0,0,0,0,0,0,0,0,0,0}, 0, 0, TASK_RUNNING, 0, PF_KTHREAD}

extern void ret_from_fork(void);
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg);
int to_el0(unsigned long fn);

#endif