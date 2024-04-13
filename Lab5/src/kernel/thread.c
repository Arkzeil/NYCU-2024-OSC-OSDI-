#include "kernel/thread.h"

thread_t *thread_list;
thread_t *cur_thread;
thread_t *run_queue;
thread_t *wait_queue;

void thread_init(void){
    lock();

    thread_list = 0;
    cur_thread = 0;
    run_queue = 0;
    wait_queue = 0;
    
    cur_thread = thread_create(idle_task, 0);
    unlock();
}

thread_t* thread_create(void (*cb)(void), void *arg){
    thread_t *new_thread = (thread_t*)pool_alloc(sizeof(thread_t));
    if(new_thread == 0)
        return 0;
        
    new_thread->cb = cb;
    new_thread->data = arg;
    //new_thread->pid = 0;
    new_thread->next = 0;
    new_thread->prev = 0;
    new_thread->status = 1; // consider this thread is running
    new_thread->sp = (void*)(pool_alloc(THREAD_STK_SIZE));
    if(new_thread->sp == 0){
        pool_free(new_thread);
        return 0;
    }

    lock();
    // add this thread to thread list
    if(thread_list == 0){
        thread_list = new_thread;
        new_thread->pid = 0;
    }
    else{
        thread_t *current = thread_list;
        while(current->next != 0)
            current = current->next;
        current->next = new_thread;
        new_thread->pid = current->pid + 1;
    }

    if(run_queue == 0) // if no thread is running, make this thread as running thread
        run_queue = new_thread;
    else{               // else put this thread to the end of run queue
        thread_t *current = run_queue;
        while(current->next != 0)
            current = current->next;
        current->next = new_thread;
    }

    unlock();

    return new_thread;
}

void schedule(void){
    if(run_queue == 0){
        uart_puts("No thread can be schedule(only idle)\n");
        return;
    }

    lock();

    if(cur_thread == 0)
        cur_thread = run_queue;
    else if(cur_thread->next != 0 && cur_thread->next->status != -1) // not zombie
        cur_thread = cur_thread->next;

    switch_to(get_current(), &cur_thread->context);

    unlock();
}

void idle_task(void){
    while(1){
        kill_zombies();
        schedule();
    }
}
// reclaim threads marked as zombie. In this exercise, all threads are consider the child of idle thread
void kill_zombies(void){
    thread_t *current = thread_list;
    while(current != 0){
        if(current->status == -1){
            thread_t *tmp = current;
            current = current->next;
            pool_free(tmp->sp);
            pool_free(tmp);
        }
        else
            current = current->next;
    }
}

void foo(void){
    for(int i = 0; i < 10; ++i) {
        uart_puts("Thread id: ");
        uart_itoa(cur_thread->pid);
        uart_putc(' ');
        uart_itoa(i);
        uart_putc('\n');
        delay(1000000);
        schedule();
    }
}