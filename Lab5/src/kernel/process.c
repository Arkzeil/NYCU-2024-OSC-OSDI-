#include "kernel/process.h"

task_struct_t init_task = INIT_TASK;
task_struct_t *current_task = &init_task;
task_struct_t *task[NR_TASKS] = {&init_task};

int nr_tasks = 0;

int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg){
    lock();
    // allocate a new task struct and trap frame for new process
    task_struct_t *np = (task_struct_t *)pool_alloc(sizeof(task_struct_t));
    // holds the complete register state of a process or thread at a specific point in time, usually when a system call, interrupt, or exception occurs.
    // this is used for load_all as load_all will load from sp
    np->tf = (trap_frame_t *)pool_alloc(sizeof(trap_frame_t));
    //trap_frame_t  *tf = (trap_frame_t *)pool_alloc(sizeof(trap_frame_t));
    np->sp = (unsigned long)pool_alloc(THREAD_STK_SIZE);

    if(!np || !np->tf){
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

    uart_b2x_64(np->context.x19);
    uart_putc('\n');
    uart_b2x_64(np->context.x20);
    uart_putc('\n');

    np->status = TASK_WAITING;
    np->pid = nr_tasks;
    np->flag = clone_flags;
    // this is used for load_all as load_all will load from sp
    /* this one is crucial  */
    /*                      */
    /*                      */
    np->context.sp = (unsigned long)np->sp + THREAD_STK_SIZE;
    np->context.lr = (unsigned long)ret_from_fork;

    task[nr_tasks] = np;
    // used to return pid
    int i = nr_tasks++;
    unlock();

    return i;
}
// this is achieved by changing current task's trap frame 
int to_el0(unsigned long fn){
    memzero(current_task->tf, sizeof(trap_frame_t));
    
    current_task->tf->elr_el1 = fn;
    current_task->tf->spsr_el1 = 0x00000000;
    //current_task->tf->spsr_el1 |= (1 << 0); // set the M[0] bit to 1, which means the processor is in EL0
    //current_task->tf->spsr_el1 |= (1 << 6); // set the DAIF[6] bit to 1, which means the processor is in EL0
    current_task->tf->sp_el0 = (unsigned long)pool_alloc(THREAD_STK_SIZE);
    if(!current_task->tf->sp_el0)
        return -1;
    current_task->sp = current_task->tf->sp_el0;
    
    return 0;
}

void process_schedule(void){
    int i;
    task_struct_t *prev;
    task_struct_t *next = 0;
    uart_puts("Process schedule\n");
    lock();
    prev = current_task;
    for(i = 0; i < NR_TASKS; i++){
        if(task[i] && task[i]->status == TASK_WAITING){
            current_task->status = TASK_WAITING;
            task[i]->status = TASK_RUNNING;
            next = task[i];
            break;
        }
    }
    if(current_task == next || next == 0){
        uart_puts("No task to schedule\n");
        unlock();
        return;
    }

    uart_itoa(prev->pid);
    uart_puts(" ");
    uart_itoa(next->pid);
    uart_putc(' ');
    uart_b2x_64(next->context.x19);
    uart_putc(' ');
    uart_b2x_64(next->context.lr);
    uart_putc('\n');

    current_task = next;
    switch_to(get_current(), &next->context);
    unsigned long x19;
    uart_puts("Current x19:");
    asm volatile(
        "mov %[var1], x19;"
        : [var1] "=r" (x19)    // Output operands
        :
    );
    uart_b2x_64(x19);
    uart_putc('\n');
    unlock();
}

void exit_process(void){
    lock();
    current_task->status = TASK_ZOMBIE;
    if(current_task->sp)
        pool_free((void*)current_task->sp);
    unlock();
    process_schedule();
}

void kernel_procsss(void){
    uart_puts("Kernel process started\n");
    // int err = to_el0((unsigned long)user_process);
    // if(err < 0)
    //     uart_puts("Error while moving to user mode\n");
    exit_process();
}

void user_process(void){
    void *spsr1;
    void *elr1;
    void *esr1;
    void *el;
    uart_puts("Entering user process\n");

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
}

void pfoo(void){
    for(int i = 0; i < 10; ++i) {
        uart_puts("Thread id: ");
        uart_itoa(current_task->pid);
        uart_putc(' ');
        uart_itoa(i);
        uart_putc('\n');
        delay(1000000);
        process_schedule();
    }
}