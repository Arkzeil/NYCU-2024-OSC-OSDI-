#include "kernel/process.h"

task_struct_t init_task = INIT_TASK;
task_struct_t *current_task = &init_task;
task_struct_t *task[NR_TASKS] = {&init_task};

int nr_tasks = 1;

int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg){
    lock();
    // allocate a new task struct and trap frame for new process
    task_struct_t *np = (task_struct_t *)pool_alloc(sizeof(task_struct_t));
    // holds the complete register state of a process or thread at a specific point in time, usually when a system call, interrupt, or exception occurs.
    // this is used for load_all as load_all will load from sp
    //trap_frame_t  *tf = (trap_frame_t *)pool_alloc(sizeof(trap_frame_t));

    if(!np){
        unlock();
        return -1;
    }
    // zero out the process context and trap frame
    memzero(&(np->context), sizeof(process_context_t));
    memzero(&(np->tf), sizeof(trap_frame_t));

    // if it's kernel thread, we should set the function and argument
    if(clone_flags & PF_KTHREAD){
        np->context.x19 = fn;
        np->context.x20 = arg;
    }
    // if it's user thread, we just copy the trap frame from current task
    else{
        // 'copy' the state of the current task to the new task(by using pointer dereference)
        // this requires us to define 'memcpy' by ourself(as we didn't include stadard llibrary) 
        *(np->tf) = *(current_task->tf);
        // set the return value of the child process to 0
        np->tf->x0 = 0;
        np->tf->sp_el0 = (unsigned long)pool_alloc(THREAD_STK_SIZE);
        np->sp = np->tf->sp_el0;
    }

    np->status = TASK_RUNNING;
    np->pid = nr_tasks++;
    np->flag = clone_flags;
    // this is used for load_all as load_all will load from sp
    np->context.sp = (unsigned long)np->tf;
    np->context.lr = (unsigned long)ret_from_fork;

    task[nr_tasks] = np;
    // used to return pid
    int i = nr_tasks - 1;
    unlock();

    return i;
}

int to_el0(unsigned long fn){
    memzero(current_task->tf, sizeof(trap_frame_t));
    
    current_task->tf->elr_el1 = fn;
    current_task->tf->spsr_el1 = 0x00000000;
    current_task->tf->spsr_el1 |= (1 << 0); // set the M[0] bit to 1, which means the processor is in EL0
    current_task->tf->spsr_el1 |= (1 << 6); // set the DAIF[6] bit to 1, which means the processor is in EL0
    current_task->tf->sp_el0 = (unsigned long)pool_alloc(THREAD_STK_SIZE);
    if(!current_task->tf->sp_el0)
        return -1;
    current_task->sp = current_task->tf->sp_el0;
    
    return 0;
}