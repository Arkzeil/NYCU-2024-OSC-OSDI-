#include "kernel/syscall.h"

trap_frame_t *current_tf;

int getpid(){
    current_tf->x0 = cur_thread->pid; 
    return cur_thread->pid;
}

unsigned int uart_read(char buf[], unsigned int size){
    int i;
    // no overflow protection
    for(i = 0; i < size; i++){
        buf[i] = uart_getc();
    }

    current_tf->x0 = i;
    return i;
}

unsigned int uart_write(char buf[], unsigned int size){
    int i;
    // no overflow protection
    for(i = 0; i < size; i++){
        uart_putc(buf[i]);
    }

    current_tf->x0 = i;
    return i;
}
// to execute a new program, we should use elr_el1 to store the address of the new program and use eret to jump to that address(in boot.S).
// So we use current_tf to modified the user space register
// then use a thread to execute it
int exec(const char* name, char *const argv[]){
    char *file_addr = (char*)cpio_find((char*)name);
    // indicating that the file is either a directory or not exist
    if(file_addr == 0)
        return 0;

    cur_thread->data_size = cpio_get_size((char*)name);
    cur_thread->data = (char*)pool_alloc(cur_thread->data_size);
    
    for(int i = 0; i < cpio_get_size((char*)name); i++)
        cur_thread->data[i] = file_addr[i];
    
    current_tf->elr_el1 = (unsigned long)cur_thread->data;
    current_tf->sp_el0 = (unsigned long)cur_thread->context.sp;
    
    current_tf->x0 = 0;
    return 0;
}
// In C, fork will return 0 to the child process and return the child's pid to the parent process
int fork(){
    lock();
    int parent_pid = cur_thread->pid;
    thread_t *thrd = thread_create((void*)(cur_thread->context.lr), cur_thread->data);
    thrd->data_size = cur_thread->data_size;
    // set child process’s return value to 0 
    thrd->context.lr = 0;
    for(int i = 0; i < cur_thread->data_size; i++)
        thrd->sp[i] = cur_thread->sp[i];
    
    unlock();
    // from now on, the child process might be scheduled to run


    // the parent process get the child's pid as return value
    current_tf->lr = thrd->pid;
    // return the child's pid if we're parent process
    if(parent_pid == cur_thread->pid){
        current_tf->x0 = thrd->pid;
        return thrd->pid;
    }
    // return 0 if we're child process
    else{
        current_tf->x0 = 0;
        return 0;
    }
}
// mark the current thread as zombie and schedule another thread to run
void exit(){
    lock();
    cur_thread->status = -1; // indicate that this thread is zombie(dead, in this lab)
    unlock();
    schedule();
}

int mbox_call(unsigned char ch, unsigned int *mbox){
    lock();
    
    unsigned int mbox_ptr = ((unsigned int)((unsigned long)mbox) & ~0xF) | (ch & 0xF);

    // Wait until the mailbox is not full
    while((mmio_read((long)MAILBOX_STATUS) & MAILBOX_FULL)){
        asm volatile("nop");
    }
    // write our address containing message to mailbox address
    mmio_write((long)MAILBOX_WRITE, mbox_ptr);
    // Wait for response
    while(1){
        // until the mailbox is not empty
        while(mmio_read((long)MAILBOX_STATUS) & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        // if it's the response corresponded to our request
        if(mbox_ptr == mmio_read((long)MAILBOX_READ)){
            // if the response is successed
            current_tf->x0 = mailbox[1];
            unlock();
            return mbox[1] == REQUEST_SUCCEED;
        }
    }

    unlock();
    // failed to get from mailbox(should not reach here)
    current_tf->x0 = 0;
    return 0;
}

void kill(int pid){
    thread_t *current = run_queue;
    for(; current != 0; current = current->next){
        if(current->pid == pid){
            current->status = -1; // mark as zombie
            return;
        }
    }
    // if the pid is not found in run_queue, search in wait_queue
    current = wait_queue;
    for(; current != 0; current = current->next){
        if(current->pid == pid){
            current->status = -1; // mark as zombie
            return;
        }
    }
    // if the pid is not found in wait_queue, search in idle thread
    if(current->pid == pid){
        current->status = -1; // mark as zombie
        return;
    }
    // PID not found
    uart_puts("PID not found\n");
}