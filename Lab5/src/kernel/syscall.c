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

int fork(){
    lock();
    
    thread_t *thrd = thread_create((void*)(cur_thread->context.lr), cur_thread->data);
    thrd->data_size = cur_thread->data_size;


    return 0;
}
// mark the current thread as zombie and schedule another thread to run
void exit(){
    lock();
    cur_thread->status = -1; // indicate that this thread is zombie(dead, in this lab)
    unlock();
    schedule();
}

int mbox_call(unsigned char ch, unsigned int *mbox){
    return 0;
}

void kill(int pid){

}