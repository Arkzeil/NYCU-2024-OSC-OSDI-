#ifndef THREAD_H
#define THREAD_H

#include "kernel/allocator.h"

typedef struct thread{
    unsigned int *sp;
    struct thread *next;
    void (*cb)(void);
    void *arg;
    int priority;
}thread_t;

thread_t *thread_list;

void thread_init(void);
thread_t* thread_create(void (*cb)(void), void *arg);
void thread_yield(void);
void thread_exit(void);
void schedule(void);

#endif