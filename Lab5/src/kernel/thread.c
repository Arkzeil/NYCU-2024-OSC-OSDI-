#include "kernel/thread.h"

void thread_init(void){
    thread_list = NULL;
}

thread_t* thread_create(void (*cb)(void), void *arg){
    thread_t *new_thread = (thread_t*)pool_alloc(sizeof(thread_t));
    new_thread->cb = cb;
    new_thread->arg = arg;
    new_thread->priority = 0;
    new_thread->next = NULL;

    return new_thread;
}

void schedule(void){

}