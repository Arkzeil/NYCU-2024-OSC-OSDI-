#include "kernel/shell.h"
#include "kernel/utils.h"
#include "kernel/mailbox.h"

void my_shell(){
    char buf[MAX_BUF_LEN];
    int buf_index;
    char input_char;

    while(1){
        buf_index = 0;
        uart_puts("# ");

        while(1){
            input_char = uart_getc();
            buf[buf_index++] = parse(input_char);
            // should replace with parsed char
            uart_putc(input_char);
            // when receving ENTER
            if(input_char == '\n'){
                // add EOF after '\n'
                buf[buf_index] = '\0';
                break;
            }
        }

        if(!string_comp(buf, "help")){
            uart_puts("help     :print this help menu\n");
            uart_puts("hello    :print Hello World!\n");
            uart_puts("reboot   :reboot the device\n");
        }
        else if(!string_comp(buf, "hello")){
            uart_puts("Hello World!\n");
        }
        else if(!string_comp(buf, "info")){
            uart_puts("Mailbox function is still woring on\n");
            if(mailbox_call()){
                get_board_revision();
                uart_puts("My board revision is: ");
                
            }
        }
        else if(!string_comp(buf, "reboot")){
            uart_puts("Reboot function is still woring on\n");
        }
        else{
            uart_puts("Unknown Command: ");
            uart_puts(buf);
        }
    }
}