#ifndef TIMER_H
#define TIMER_H

#include "kernel/INT.h"
#include "kernel/allocator.h"
#include "kernel/uart.h"

// timer queue made from double linked list
typedef struct task_timer{
    struct task_timer *prev;
    struct task_timer *next;
    void (*callback)(void *);
    void *data;
    unsigned int deadline;
}task_timer_t;

extern task_timer_t* head;
extern task_timer_t* tail;

int add_timer(void (*callback)(void *), void* data, int after);
void print_callback(void *str);
void settimeout(char *str, int second);

#endif