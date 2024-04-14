#include "kernel/syscall.h"
#include "kernel/exception_hdlr.h"

int getpid(){

}

unsigned int uart_read(char buf[], unsigned int size){
    return 0;
}

unsigned int uart_read(char buf[], unsigned int size){

}

int exec(const char* name, char *const argv[]){
    return 0;
}

int fork(){

}

void exit(){

}

int mbox_call(unsigned char ch, unsigned int *mbox){

}

void kill(int pid){

}