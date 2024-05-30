#include "kernel/process.h"

task_struct_t init_task = INIT_TASK;
task_struct_t *current_task = &init_task;
task_struct_t *PCB[NR_TASKS] = {&(init_task), };

int nr_tasks = 0;

void gdb(){

}

char* _memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

int copy_process(my_uint64_t clone_flags, my_uint64_t fn, my_uint64_t arg, my_uint64_t stack){
    lock();
    // allocate a new task struct and trap frame for new process
    task_struct_t *np = (task_struct_t *)pool_alloc(sizeof(task_struct_t));
    // holds the complete register state of a process or thread at a specific point in time, usually when a system call, interrupt, or exception occurs.
    // this is used for load_all as load_all will load from sp
    //np->tf = (trap_frame_t *)pool_alloc(THREAD_STK_SIZE);
    //trap_frame_t  *tf = (trap_frame_t *)pool_alloc(sizeof(trap_frame_t));
    //show_mem_stat();
    //np->sp = (my_uint64_t)pool_alloc(THREAD_STK_SIZE);
    //trap_frame_t *tf = get_task_tf(np);

    if(!np){
        unlock();
        return -1;
    }

    // zero out the process context and trap frame
    // memzero should pass the address of the first byte of the struct
    memset((my_uint64_t)&(np->context), 0, sizeof(process_context_t));
    memset((my_uint64_t)&np->tf, 0, sizeof(trap_frame_t));

    // initialize the vma of new task struct
    INIT_LIST_HEAD(&(np->vma_list));
    np->context.pgd = pool_alloc(4096);
    memset((my_uint64_t)np->context.pgd, 0, 4096);

    // if it's kernel thread, we should set the function and argument
    if(clone_flags & PF_KTHREAD){
        np->context.x19 = fn;
        np->context.x20 = arg;
        np->sp = (my_uint64_t)pool_alloc(THREAD_STK_SIZE);
        
        np->signal_is_checking = 0;
        for(int i = 0; i <= NR_SIGNALS; i++){
            np->signal_handler[i] = signal_default_handler;  // set all signal handler to default
            np->sigcount[i] = 0;        // set all signal count to 0
        }

    }
    // if it's user thread, we just copy the trap frame from current task
    else{
        // copy the vma list of the parent process
        list_head_t *pos;
        vm_area_struct_t *vma;
        list_for_each(pos, &current_task->vma_list){
            // ignore device and signal wrapper
            vma = (vm_area_struct_t *)pos;
            if(vma->virt_addr == USER_SIGNAL_WRAPPER_VA || vma->virt_addr == PERIPHERAL_START)
                continue;

            char *new_alloc = (char*)pool_alloc(vma->area_size);
            memset((my_uint64_t)new_alloc, 0, vma->area_size);

            mmu_add_vma(np, vma->virt_addr, (my_uint64_t)VIRT_TO_PHYS(new_alloc), vma->area_size, vma->rwx, 1);
            if(vma->virt_addr == USER_STACK_BASE - THREAD_STK_SIZE){
                continue;
            }
            _memcpy(new_alloc, (void*)PHYS_TO_VIRT(vma->phys_addr), vma->area_size);
            for(int i = 0; i < vma->area_size; i++){
                *((char*)new_alloc + i) = *((char*)PHYS_TO_VIRT(vma->phys_addr) + i);
            }
        }
        // reserve periphiaral space
        mmu_add_vma(np, PERIPHERAL_START, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, 0b011, 0);
        // reserve user signal wrapper space
        mmu_add_vma(np, USER_SIGNAL_WRAPPER_VA, (my_uint64_t)VIRT_TO_PHYS(signal_handler_wrapper), 0x2000, 0b101, 0);
        
        // 'copy' the state of the current task to the new task(by using pointer dereference)
        // this requires us to define 'memcpy' by ourself(as we didn't include stadard library) 
        //*(np->tf) = *(current_task->tf);
        np->tf.x0 = current_task->tf.x0;
        np->tf.x1 = current_task->tf.x1;
        np->tf.x2 = current_task->tf.x2;
        np->tf.x3 = current_task->tf.x3;
        np->tf.x4 = current_task->tf.x4;
        np->tf.x5 = current_task->tf.x5;
        np->tf.x6 = current_task->tf.x6;
        np->tf.x7 = current_task->tf.x7;
        np->tf.x8 = current_task->tf.x8;
        np->tf.x9 = current_task->tf.x9;
        np->tf.x10 = current_task->tf.x10;
        np->tf.x11 = current_task->tf.x11;
        np->tf.x12 = current_task->tf.x12;
        np->tf.x13 = current_task->tf.x13;
        np->tf.x14 = current_task->tf.x14;
        np->tf.x15 = current_task->tf.x15;
        np->tf.x16 = current_task->tf.x16;
        np->tf.x17 = current_task->tf.x17;
        np->tf.x18 = current_task->tf.x18;
        np->tf.x19 = current_task->tf.x19;
        np->tf.x20 = current_task->tf.x20;
        np->tf.x21 = current_task->tf.x21;
        np->tf.x22 = current_task->tf.x22;
        np->tf.x23 = current_task->tf.x23;
        np->tf.x24 = current_task->tf.x24;
        np->tf.x25 = current_task->tf.x25;
        np->tf.x26 = current_task->tf.x26;
        np->tf.x27 = current_task->tf.x27;
        np->tf.x28 = current_task->tf.x28;
        np->tf.fp = current_task->tf.fp;
        np->tf.lr = current_task->tf.lr;
        np->tf.spsr_el1 = current_task->tf.spsr_el1;
        np->tf.elr_el1 = current_task->tf.elr_el1;
        np->tf.sp_el0 = current_task->tf.sp_el0;
        //memcpy(&np->context, &current_task->context, sizeof(process_context_t));
        uart_b2x_64(np->tf.sp_el0);
        uart_putc(' ');
        uart_b2x_64(np->tf.elr_el1);
        uart_putc('\n');
        // set the return value of the child process to 0
        np->tf.x0 = 0;

        // user process got its own stack
        void *new_stack = pool_alloc(THREAD_STK_SIZE);
        // The compiler will automatically copy the local variable when newly created child process return to a function
        // So we must reserve the space of local variables for the child process, e.g. parent:0x194000->0x193FD0, child's stack start from 0x195000(0x194000 + 0x1000)
        // If we don't adjust it, it will save the local variables of parent process to the child process's stack at 0x195000 to 0x195030
        // So we must adjust the stack pointer of the child process to 0x195000 - 0x30(i.e. 0x195000 - (0x194000 - 0x193FD0) ), 

        np->sp = (my_uint64_t)new_stack;
        // np->tf.sp_el0 = (my_uint64_t)(np->tf.sp_el0 + (np->sp - current_task->sp));
        // np->tf.fp = (np->tf.sp_el0);

        np->signal_is_checking = 0;
        for(int i = 0; i <= NR_SIGNALS; i++){
            np->signal_handler[i] = current_task->signal_handler[i];  // copy signal handler
            np->sigcount[i] = 0;        // set all signal count to 0
        }

        //mmu_add_vma(np, np->sp, (my_uint64_t)VIRT_TO_PHYS(np->sp), THREAD_STK_SIZE, 0b111, 1);

        // copy the stack from parent process to child process
        // for(int i = 0; i < 925; i++)
        //     np->space[i] = current_task->space[i];
        for(int i = 0; i < THREAD_STK_SIZE; i++){
            *((char*)np->sp + i) = *((char*)current_task->sp + i);
        }
        for(int i = 0; i < 688; i++){
            np->space[i] = current_task->space[i];
        }

        np->context.pgd = VIRT_TO_PHYS(np->context.pgd);

        uart_puts("New SP: ");
        uart_b2x_64(np->tf.sp_el0);
        uart_putc('\n');
        uart_puts("Parent SP: ");
        uart_b2x_64((current_task->tf.sp_el0 - current_task->sp));
        uart_putc('\n');
        uart_puts("Parent SP_B: ");
        uart_b2x_64(current_task->sp);
        uart_putc('\n');
        gdb();
        /*asm volatile(
            "msr ttbr0_el1, %[var1];"
            :
            : [var1] "r" (np->context.pgd)
        );*/
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
    // use the space between metadata and trap frame as stack
    np->context.sp = (my_uint64_t)&(np->tf);
    np->context.lr = (my_uint64_t)ret_from_fork;
    // uart_b2x_64((my_uint64_t)np->context.pgd);
    // uart_putc('\n');

    // uart_puts("context lr: ");
    // uart_b2x_64(np->context.lr);
    // uart_putc('\n');
    uart_puts("pid:");
    uart_itoa(np->pid);
    uart_putc('\n');
    // this will replace init_pcb when first idle process created
    PCB[nr_tasks] = np;
    // used to return pid
    int i = nr_tasks++;
    unlock();

    return i;
}
// this is achieved by changing current task's trap frame 
int to_el0(my_uint64_t fn){
    uart_puts("Starting moving to user mode\n");
    memset((my_uint64_t)&current_task->tf, 0, sizeof(trap_frame_t));
    // since after the kernel process is finished, it will return to '1:' block of ret_from_work, which will then exexute load_all
    // and current sp is current_task->tf
    current_task->tf.elr_el1 = fn;
    current_task->tf.spsr_el1 = 0x00000000;
    //current_task->tf.spsr_el1 |= (1 << 0); // set the M[0] bit to 1, which means the processor is in EL0
    //current_task->tf.spsr_el1 |= (1 << 6); // set the DAIF[6] bit to 1, which means the processor is in EL0
    // void *stack = pool_alloc(THREAD_STK_SIZE);
    // if(!stack)
    //     return -1;
    // gdb();
    // current_task->tf.sp_el0 = (my_uint64_t)(stack + THREAD_STK_SIZE);
    // current_task->tf.fp = (my_uint64_t)(stack + THREAD_STK_SIZE);
    // current_task->sp = (my_uint64_t)stack;
    
    return 0;
}

void ini_process_schedule(void){
    int i;
    task_struct_t *prev;
    task_struct_t *next = 0;
    //uart_puts("Process schedule\n");
    lock();
    prev = current_task;
    int temp;
    for(i = 0; i < NR_TASKS; i++){
        // to iterate the pid larger than current task, round back if not found
        temp = (current_task->pid + i) % NR_TASKS;
        if(PCB[temp] && PCB[temp]->status == TASK_WAITING){
            /*uart_puts("Current task: ");
            uart_itoa(current_task->status);
            uart_putc(' ');
            uart_puts("New task: ");
            uart_itoa(PCB[i]->status);
            uart_putc('\n');*/
            if(temp == 0)
                continue;
            if(current_task->status != TASK_ZOMBIE)
                current_task->status = TASK_WAITING;
            PCB[temp]->status = TASK_RUNNING;
            next = PCB[temp];
            
            break;
        }
    }
    if(current_task == next || next == 0){
        if(current_task->status != TASK_ZOMBIE){
            unlock();
            return;
        }
        else{
            next = PCB[0];
            next->status = TASK_RUNNING;
        }
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
    delay(150);
    // uart_b2x_64(get_current());
    // uart_putc('\n');
    ini_switch_to(get_current(), &(next->context));
    /*my_uint64_t x19;
    uart_puts("Current x19:");
    asm volatile(
        "mov %[var1], x19;"
        : [var1] "=r" (x19)    // Output operands
        :
    );
    uart_b2x_64(x19);
    uart_putc('\n');*/
    // uart_puts("Switched\n");
    unlock();
}

void process_schedule(void){
    int i;
    task_struct_t *prev;
    task_struct_t *next = 0;
    //uart_puts("Process schedule\n");
    lock();
    prev = current_task;
    int temp;
    for(i = 0; i < NR_TASKS; i++){
        // to iterate the pid larger than current task, round back if not found
        temp = (current_task->pid + i) % NR_TASKS;
        if(PCB[temp] && PCB[temp]->status == TASK_WAITING){
            /*uart_puts("Current task: ");
            uart_itoa(current_task->status);
            uart_putc(' ');
            uart_puts("New task: ");
            uart_itoa(PCB[i]->status);
            uart_putc('\n');*/
            if(temp == 0)
                continue;
            if(current_task->status != TASK_ZOMBIE)
                current_task->status = TASK_WAITING;
            PCB[temp]->status = TASK_RUNNING;
            next = PCB[temp];
            
            break;
        }
    }
    if(current_task == next || next == 0){
        if(current_task->status != TASK_ZOMBIE){
            unlock();
            return;
        }
        else{
            next = PCB[0];
            next->status = TASK_RUNNING;
        }
    }
    my_uint64_t lr;
    asm volatile(
        "mov %[var1], lr;"
        : [var1] "=r" (lr)    // Output operands
        :
    );
    uart_b2x_64(lr);
    uart_putc(' ');
    uart_itoa(prev->pid);
    uart_putc(' ');
    uart_itoa(next->pid);
    uart_putc(' ');
    uart_b2x_64(prev->tf.elr_el1);
    uart_putc(' ');
    uart_b2x_64(next->tf.elr_el1);
    uart_putc(' ');
    uart_b2x_64(next->tf.sp_el0);
    uart_putc('\n');

    current_task = next;
    delay(150);
    // uart_b2x_64(get_current());
    // uart_putc('\n');
    switch_to(get_current(), &(next->context));
    /*my_uint64_t x19;
    uart_puts("Current x19:");
    asm volatile(
        "mov %[var1], x19;"
        : [var1] "=r" (x19)    // Output operands
        :
    );
    uart_b2x_64(x19);
    uart_putc('\n');*/
    // uart_puts("Switched\n");
    unlock();
}

void exit_process(void){
    lock();
    uart_puts("Exit process ");
    uart_itoa(current_task->pid);
    uart_putc('\n');
    current_task->status = TASK_ZOMBIE;
    //if(current_task->sp)
      //  pool_free((void*)current_task->sp);
    unlock();
    process_schedule(); 
}

void kill_zombie_process(void){
    lock();
    for(int i = 0; i < NR_TASKS; i++){
        if(PCB[i] && PCB[i]->status == TASK_ZOMBIE){
            mmu_free_page_tables(PCB[i]->context.pgd, 0);
            mmu_del_vma(PCB[i]);
            pool_free((void*)PCB[i]->sp);
            pool_free((void*)PCB[i]);
            PCB[i] = 0;
        }
    }
    unlock();
}

void idle_process(void){
    while(1){
        //uart_puts("Idle process\n");
        kill_zombie_process();
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
    
    //int err = to_el0((my_uint64_t)&user_process2);
    int err = to_el0((my_uint64_t)&fork_test);
    if(err < 0)
        uart_puts("Error while moving to user mode\n");
    
    uart_puts("Kernel process ended\n");
    //mmu_add_vma(current_task, USER_KERNEL_BASE, (my_uint64_t)VIRT_TO_PHYS(&fork_test), 0x2000, 0b111, 0);
    mmu_add_vma(current_task, USER_KERNEL_BASE, VIRT_TO_PHYS((my_uint64_t)&fork_test), 0x1000, 0b111, 0);    
    mmu_add_vma(current_task, (USER_STACK_BASE - THREAD_STK_SIZE), VIRT_TO_PHYS(current_task->sp), 0x1000, 0b111, 0);
    // reserve periphiaral space
    mmu_add_vma(current_task, PERIPHERAL_START, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, 0b011, 0);
    // reserve user signal wrapper space
    mmu_add_vma(current_task, USER_SIGNAL_WRAPPER_VA, (my_uint64_t)VIRT_TO_PHYS(signal_handler_wrapper), 0x2000, 0b101, 0);

    current_task->context.pgd = VIRT_TO_PHYS(current_task->context.pgd);
    current_task->context.sp = USER_STACK_BASE;
    current_task->context.fp = USER_STACK_BASE;
    current_task->context.lr = USER_KERNEL_BASE;       
    current_task->tf.elr_el1 = current_task->context.lr; 
    current_task->tf.sp_el0 = current_task->context.sp;
    current_task->tf.fp = current_task->context.fp;
    current_task->sp = (USER_STACK_BASE - THREAD_STK_SIZE);
    asm volatile(
        "msr tpidr_el1, %[var1];"
        :
        : [var1] "r" (&current_task->context)
    );
    asm volatile(
        "dsb ish\r\n"
        "msr ttbr0_el1, %[var1]\r\n"
        "tlbi vmalle1\r\n"
        "dsb ish\r\n"
        "isb\r\n"
        :
        : [var1] "r" (current_task->context.pgd)
    );
}

void user_process1(unsigned long arg){
    uart_puts("Testing user process1\n");
    uart_puts((char*)arg);
}
void user_process2(void){
    unsigned int mailbox[32];

    mailbox[0] = 7 * 4;               // buffer size in bytes (size of the message in bytes)
    mailbox[1] = REQUEST_CODE;        // MBOX_REQUEST magic value, indicates request message
    // tags begin
    mailbox[2] = GET_BOARD_REVISION;  // tag identifier
    mailbox[3] = 4;                   // maximum of request and response value buffer's length.(value buffer size in bytes)
    mailbox[4] = TAG_REQUEST_CODE;    // must be zero
    mailbox[5] = 0;                   // (optional) value buffer
    // tags end
    mailbox[6] = END_TAG;

    call_mbox((unsigned char)8, mailbox);

    uart_puts("My board revision: ");
    uart_b2x(mailbox[5]);
    uart_puts("\r\n");

    uart_puts("-------------------------\n");
    call_uart_write("Process start, pid is: ", 24);
    //call_uart_write(call_get_pid(), 1);

    char async_buf[50];
    int ticks = 150;

    call_uart_write("# Enter a string: ", 16);
    while(ticks--);

    int received = call_uart_read(async_buf, 5);

    uart_puts("Bytes received: ");
    uart_itoa(received);
    uart_putc('\n');

    int printed =  call_uart_write(async_buf, 5);
    ticks = 150;
    while(ticks--);

    uart_puts("Bytes printed: ");
    uart_itoa(printed);
    uart_putc('\n');

    call_exit();
}

void fork_test(void){
    uart_puts("\nFork Test, pid: ");
    uart_itoa(call_get_pid());
    uart_putc('\n');
    gdb();
    int cnt = 1;
    int ret = 0;
    void *stack;
    stack = pool_alloc(THREAD_STK_SIZE);

    if ((ret = call_sys_clone(0, 0, (my_uint64_t)stack)) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        gdb();
        uart_puts("first child pid: ");
        uart_itoa(call_get_pid());
        uart_puts(", cnt: ");
        uart_itoa(cnt);
        uart_puts(", ptr: ");
        uart_b2x_64((unsigned long long)&cnt);
        uart_puts(", sp : ");
        uart_b2x_64(cur_sp);
        uart_putc('\n');

        ++cnt;
        stack = pool_alloc(THREAD_STK_SIZE);

        if ((ret = call_sys_clone(0, 0, (my_uint64_t)stack) ) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            
            uart_puts("first(2) child pid: ");
            uart_itoa(call_get_pid());
            uart_puts(", cnt: ");
            uart_itoa(cnt);
            uart_puts(", ptr: ");
            uart_b2x_64((unsigned long long)&cnt);
            uart_puts(", sp : ");
            uart_b2x_64(cur_sp);
            uart_putc('\n');
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                
                uart_puts("second child pid: ");
                uart_itoa(call_get_pid());
                uart_puts(", cnt: ");
                uart_itoa(cnt);
                uart_puts(", ptr: ");
                uart_b2x_64((unsigned long long)&cnt);
                uart_puts(", sp : ");
                uart_b2x_64(cur_sp);
                uart_putc('\n');

                delay(1000000);
                ++cnt;
            }
        }
        call_exit();
    }
    else {
        gdb();
        uart_puts("parent here, pid ");
        uart_itoa(call_get_pid());
        uart_puts(", child ");
        uart_itoa(ret);
        uart_putc('\n');
        call_exit();
    }
}

void process_schedule_timer(void *nouse){
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); //tick frequency
    if(add_timer_NA(process_schedule_timer, cntfrq_el0 >> 5) == 0)
        uart_puts("Failed to add timer\n");
}
char *file_data;
void file_process(my_uint64_t file_addr){
    uart_puts("File process\n");
    uart_b2x_64(file_addr);
    uart_putc(' ');
    uart_itoa(cpio_file_size);
    uart_putc('\n');
    // char *temp;
    // uart_get_fn(temp);

    //file_data = (char*)pool_alloc(cpio_file_size);
    current_task->data = pool_alloc(cpio_file_size);

    mmu_add_vma(current_task, USER_KERNEL_BASE, (my_uint64_t)VIRT_TO_PHYS(current_task->data), cpio_file_size, 0b111, 0);
    //mmu_add_vma(current_task, USER_KERNEL_BASE, VIRT_TO_PHYS(ret_from_fork), 0x1000, 0b111, 0);    
    mmu_add_vma(current_task, (USER_STACK_BASE - THREAD_STK_SIZE), VIRT_TO_PHYS(current_task->sp), 0x4000, 0b111, 0);
    // reserve periphiaral space
    mmu_add_vma(current_task, PERIPHERAL_START, PERIPHERAL_START, PERIPHERAL_END - PERIPHERAL_START, 0b011, 0);
    // reserve user signal wrapper space
    mmu_add_vma(current_task, USER_SIGNAL_WRAPPER_VA, (my_uint64_t)VIRT_TO_PHYS(signal_handler_wrapper), 0x2000, 0b101, 0);

    // for(int i = 0; i < cpio_file_size; i++)
    //     file_data[i] = ((char*)file_addr)[i];

    for(int i = 0; i < cpio_file_size; i++)
        current_task->data[i] = ((char*)file_addr)[i];
    
    to_el0((my_uint64_t)current_task->data);

    current_task->context.pgd = VIRT_TO_PHYS(current_task->context.pgd);
    // current_task->context.sp = USER_STACK_BASE;
    // current_task->context.fp = USER_STACK_BASE;
    // current_task->context.lr = USER_KERNEL_BASE;       
    current_task->tf.elr_el1 = USER_KERNEL_BASE; 
    current_task->tf.sp_el0 = USER_STACK_BASE;;
    current_task->tf.fp = USER_STACK_BASE;
    // This leads to not scheduling the process?
    //current_task->sp = (USER_STACK_BASE - THREAD_STK_SIZE);

    //memcpy(file_data, (char*)file_addr, 0x3D000);

    asm volatile(
        "msr tpidr_el1, %[var1];"
        :
        : [var1] "r" (&current_task->context)
    );

    boot_timer_flag = 2;   

    mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);
    if(add_timer_NA(process_schedule_timer, 100000000) == 0)
        uart_puts("Failed to add timer\n");

    uart_puts("PGD: ");
    asm volatile(
        "dsb ish\r\n"
        "msr ttbr0_el1, %[var1]\r\n"
        "tlbi vmalle1\r\n"
        "dsb ish\r\n"
        "isb\r\n"
        :
        : [var1] "r" (current_task->context.pgd)
    );
    
    uart_b2x_64((my_uint64_t)current_task->context.pgd);
    uart_putc('\n');

    //call_exit();
}

void user_process(void){
    // void *el;
    uart_puts("Entering user process\n");
    //call_fork();
    int pid = call_get_pid();
    int var = 1;
    uart_itoa(pid);
    uart_putc('\n');

    long long cur_sp;
    asm volatile("mov %0, sp" : "=r"(cur_sp));

    uart_puts("Current sp:");
    uart_b2x_64(cur_sp);
    uart_putc('\n');

    uart_puts("Mbox: ");
    uart_itoa(call_mbox(0, 0));

    void *stack = pool_alloc(THREAD_STK_SIZE);
    // int err = call_sys_clone((my_uint64_t)&user_process1, (unsigned long)"12345", (my_uint64_t)stack);
    int err = call_sys_clone(0, 0, (my_uint64_t)stack);
	if (err < 0){
		uart_puts("Error while clonning process 1\n");
		return;
	}
    // if err > 0, it's parent process
    uart_puts("Fork return value: ");
    uart_itoa(err);
    uart_putc('\n');
    uart_puts("Var: ");
    uart_itoa(var);
    uart_putc('\n');
    // asm volatile(
    //     "mrs %[var1], CurrentEL;"
    //     :[var1] "=r" (el)    // Output operands
    // );

    // uart_puts("Current EL:");
    // uart_b2x_64((my_uint64_t)el>>2);     // bits [3:2] contain current El value
    // uart_putc('\n');
    uart_puts("Exiting user process\n");
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

trap_frame_t *get_task_tf(task_struct_t *task){
    my_uint64_t p = (my_uint64_t)task + THREAD_STK_SIZE - sizeof(trap_frame_t);
    return (trap_frame_t *)p;
}