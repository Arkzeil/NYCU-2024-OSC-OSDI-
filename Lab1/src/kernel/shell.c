#include "kernel/shell.h"

void my_shell(){
    char buf[MAX_BUF_LEN];
    int buf_index = 0;
    char input_char;

    while(1){
        uart_putc('#');

        while(1){
            input_char = uart_getc();
            buf[buf_index++] = input_char;
            // when receving ENTER
            if(input_char == '\n'){
                // add EOF after '\n'
                buf[buf_index] = '\0';
                break;
            }
        }

        
    }
}