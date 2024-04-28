#include "kernel/process.h"

task_struct_t init_task = INIT_TASK;
task_struct_t *current_task = &init_task;
task_struct_t *task[NR_TASKS] = {&(init_task), };

int nr_tasks = 0;

void gdb(){

}

int copy_process(my_uint64_t clone_flags, my_uint64_t fn, my_uint64_t arg, my_uint64_t stack){
    lock();
    // allocate a new task struct and trap frame for new process
    task_struct_t *np = (task_struct_t *)pool_alloc(sizeof(task_struct_t));
    // holds the complete register state of a process or thread at a specific point in time, usually when a system call, interrupt, or exception occurs.
    // this is used for load_all as load_all will load from sp
    np->tf = (trap_frame_t *)pool_alloc(4096);
    //trap_frame_t  *tf = (trap_frame_t *)pool_alloc(sizeof(trap_frame_t));
    //show_mem_stat();
    //np->sp = (my_uint64_t)pool_alloc(THREAD_STK_SIZE);

    if(!np || !np->tf){
        unlock();
        return -1;
    }

    // zero out the process context and trap frame
    // memzero should pass the address of the first byte of the struct
    memzero(&(np->context), sizeof(process_context_t));
    memzero(np->tf, sizeof(trap_frame_t));

    // if it's kernel thread, we should set the function and argument
    if(clone_flags & PF_KTHREAD){
        np->context.x19 = fn;
        np->context.x20 = arg;
    }
    // if it's user thread, we just copy the trap frame from current task
    else{
        gdb();
        // 'copy' the state of the current task to the new task(by using pointer dereference)
        // this requires us to define 'memcpy' by ourself(as we didn't include stadard library) 
        *(np->tf) = *(current_task->tf);
        // set the return value of the child process to 0
        np->tf->x0 = 0;
        // user process got its own stack
        void *new_stack = pool_alloc(THREAD_STK_SIZE);
        np->tf->sp_el0 = (my_uint64_t)new_stack + THREAD_STK_SIZE;
        np->sp = (my_uint64_t)new_stack;
        //return 0;
    }
    uart_puts("context x19: ");
    uart_b2x_64(np->context.x19);
    uart_putc('\n');
    uart_puts("context x20: ");
    uart_b2x_64(np->context.x20);
    uart_putc('\n');

    np->status = TASK_WAITING;
    np->pid = nr_tasks;
    np->flag = clone_flags;
    // this is used for load_all as load_all will load from sp
    /* this one is crucial  */
    /*                      */
    /*                      */
    np->context.sp = (my_uint64_t)np->tf;
    np->context.lr = (my_uint64_t)ret_from_fork;
    uart_puts("context lr: ");
    uart_b2x_64(np->context.lr);
    uart_putc('\n');
    uart_puts("pid:");
    uart_itoa(np->pid);
    uart_putc('\n');

    task[nr_tasks] = np;
    // used to return pid
    int i = nr_tasks++;
    unlock();

    return i;
}
// this is achieved by changing current task's trap frame 
int to_el0(my_uint64_t fn){
    uart_puts("Starting moving to user mode\n");
    memzero(current_task->tf, sizeof(trap_frame_t));
    // since after the kernel process is finished, it will return to '1:' block of ret_from_work, which will then exexute load_all
    // and current sp is current_task->tf
    current_task->tf->elr_el1 = fn;
    current_task->tf->spsr_el1 = 0x00000000;
    //current_task->tf->spsr_el1 |= (1 << 0); // set the M[0] bit to 1, which means the processor is in EL0
    //current_task->tf->spsr_el1 |= (1 << 6); // set the DAIF[6] bit to 1, which means the processor is in EL0
    void *stack = pool_alloc(THREAD_STK_SIZE);
    if(!stack)
        return -1;
    
    current_task->tf->sp_el0 = (my_uint64_t)(stack + THREAD_STK_SIZE);
    current_task->sp = (my_uint64_t)stack;
    
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
            uart_puts("Current task: ");
            uart_itoa(current_task->status);
            uart_putc(' ');
            uart_puts("New task: ");
            uart_itoa(task[i]->status);
            uart_putc('\n');
            if(current_task->status != TASK_ZOMBIE)
                current_task->status = TASK_WAITING;
            task[i]->status = TASK_RUNNING;
            next = task[i];
            break;
        }
    }
    if(current_task == next || next == 0){
        uart_puts("No task to schedule\n");
        delay(1000000);
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
    /*my_uint64_t x19;
    uart_puts("Current x19:");
    asm volatile(
        "mov %[var1], x19;"
        : [var1] "=r" (x19)    // Output operands
        :
    );
    uart_b2x_64(x19);
    uart_putc('\n');*/
    unlock();
}

void exit_process(void){
    lock();
    uart_puts("Exit process\n");
    current_task->status = TASK_ZOMBIE;
    //if(current_task->sp)
      //  pool_free((void*)current_task->sp);
    unlock();
    process_schedule(); 
}

void idle_process(void){
    while(1){
        uart_puts("Idle process\n");
        process_schedule();
    }
}

void kernel_procsss(void){
    uart_puts("Kernel process started\n");
    void *el, *elr;
    asm volatile(
        "mrs %[var1], CurrentEL;"
        "mov %[var2], x30;"
        : [var1] "=r" (el), [var2] "=r" (elr)    // Output operands
    );

    uart_puts("Current EL:");
    uart_b2x_64((my_uint64_t)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');
    uart_puts("x30:");
    uart_b2x_64((my_uint64_t)elr);
    uart_putc('\n');
    
    int err = to_el0((my_uint64_t)&user_process);
    if(err < 0)
        uart_puts("Error while moving to user mode\n");
    
    uart_puts("Kernel process ended\n");
    //exit_process();
    asm volatile(
        "mov %[var1], x30;"
        : [var1] "=r" (elr)    // Output operands
    );
    uart_b2x_64((my_uint64_t)elr);
    uart_putc('\n');
    uart_b2x_64((my_uint64_t)current_task->tf->elr_el1);
    uart_putc('\n');
}

void user_process1(unsigned long arg){
    uart_puts("Testing user process1\n");
    uart_puts((char*)arg);
}

void user_process(void){
    // void *el;
    uart_puts("Entering user process\n");
    //call_fork();
    int pid = call_get_pid();
    uart_itoa(pid);
    uart_putc('\n');

    void *stack = pool_alloc(THREAD_STK_SIZE);

    int err = call_sys_clone((my_uint64_t)&user_process1, (unsigned long)"12345", (my_uint64_t)stack);
	if (err < 0){
		uart_puts("Error while clonning process 1\n");
		return;
	}
    // if err > 0, it's parent process
    uart_puts("Fork return value: ");
    uart_itoa(err);
    uart_putc('\n');
    // asm volatile(
    //     "mrs %[var1], CurrentEL;"
    //     :[var1] "=r" (el)    // Output operands
    // );

    // uart_puts("Current EL:");
    // uart_b2x_64((my_uint64_t)el>>2);     // bits [3:2] contain current El value
    // uart_putc('\n');
    call_exit();
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