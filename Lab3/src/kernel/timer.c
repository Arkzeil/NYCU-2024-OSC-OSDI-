#include "kernel/timer.h"

task_timer_t* head = 0;
task_timer_t* tail = 0;

int add_timer(void (*callback)(void *), void* data, int after){
    // This is for "If the timeout is earlier than the previous programed expired time, the kernel reprograms the hardware timer to the earlier one."
    int timer_set_flag = 0;
    unsigned long long cur_cnt, cnt_freq;

    // empty queue
    /*if(head == tail){
        head = simple_malloc(sizeof(task_timer_t));
        if(head == 0)
            return 0;

        tail = head;
    }*/
    // Invalid setting
    if(after == 0)
        return 0;
    // disable interrupt to protect critical section
    asm volatile(
        "msr daifset, 0xf;"
    );

    task_timer_t *cur = head;
    task_timer_t *temp = simple_malloc(sizeof(task_timer_t));
    // malloc fail
    if(temp == 0)
        return 0;
    // preserve data
    char *copy;
    string_copy(copy, data);

    temp->callback = callback;
    temp->data = copy;
    temp->deadline = after;
    temp->next = 0;
    temp->prev = 0;

    while(1){
        // this is the first timer inserted into queue
        if(cur == 0){
            head = temp;
            tail = temp;
            timer_set_flag = 1;
            break;
        }
        // insert into appropiate location based on increase-order
        if(after <= cur->deadline){
            temp->prev = cur->prev;
            if(cur->prev != 0)
                cur->prev->next = temp;
            cur->prev = temp;
            temp->next = cur;
            // if it is inserted into the head of queue
            if(cur == head){
                head = temp;
                timer_set_flag = 1;
            }
            break;
        }
        // traverse to last element
        if(cur == tail){
            tail->next = temp;
            temp->prev = tail;

            tail = temp;
            
            break;
        }    

        cur = cur->next;
    }

    if(timer_set_flag){
        asm volatile(
            "mrs %[var1], cntpct_el0;"
            "mrs %[var2], cntfrq_el0;"
            :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
        );
        // Get current_time + waiting seconds
        cnt_freq *= after;
        //cnt_freq += cur_cnt;
        // setting tval leads to cval = cur_time + tval
        
        asm volatile(
            "msr cntp_tval_el0, %[var1];"
            :
            :[var1] "r" (cnt_freq)
            :
        );
    }
    // activate core0 timer interrupt
    mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);

    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
    
    return 1;
}

void print_callback(void *str){
    unsigned long long cur_cnt, cnt_freq;
    uart_puts("The message is: ");
    uart_puts(str);

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq)
    );

    uart_puts("Timeout: ");
    uart_b2x_64(cur_cnt / cnt_freq);
    uart_putc('\n');
}

void settimeout(char *str, int second){
    //char *copy;
    //string_copy(copy, str);
    add_timer(print_callback, (void*)str, second);
}