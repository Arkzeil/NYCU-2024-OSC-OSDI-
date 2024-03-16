#include "kernel/shell.h"
void gdb_break(){
    uart_puts("this is just for gdb\n");
}

void my_shell(){
    char buf[MAX_BUF_LEN];
    char *argv[5];
    int buf_index;
    //char input_char;
    for(buf_index = 0; buf_index < 5; buf_index++)
        argv[buf_index] = simple_malloc(MAX_ARGV_LEN);

    while(1){
        buf_index = 0;
        string_set(buf, 0, MAX_BUF_LEN);
        uart_puts("# ");

        buf_index = uart_gets(buf, argv);
        //buf_index = uart_get_fn(buf);
        // this one requires to tuen on interrupt in main function(can move to somewhere else in the future)
        //buf_index = uart_irq_gets(buf);

        if(buf_index >= MAX_BUF_LEN)
            uart_puts("Warning: buffer is full, command output may not correct\n");

        if(!string_comp(buf, "help")){
            uart_puts("help     :Print this help menu\n");
            uart_puts("hello    :Print Hello World!\n");
            uart_puts("info     :Get revision and memory\n");
            uart_puts("reboot   :Reboot the device\n");
            uart_puts("ls       :list all files in initramfs\n");
            uart_puts("cat      :show the content of file\n");
            uart_puts("el0      :execute programs in initramfs in El0\n");
        }
        else if(!string_comp(buf, "hello")){
            uart_puts("Hello World!\n");
        }
        else if(!string_comp(buf, "info")){
            //uart_puts("Mailbox function is still woring on\n");
            /*if(mailbox_call()){
                get_board_revision();
                uart_puts("My board revision is: ");
                uart_b2x(mailbox[5]);
                uart_puts("\r\n");
            }*/
            get_board_revision();
            get_arm_mem();
        }
        else if(!string_comp(buf, "reboot")){
            //uart_puts("Reboot function is still woring on\n");
            uart_puts("Rebooting...\n");
            // after 1000 ticks, start resetting
            reset(1000);
        }
        else if(!string_comp(buf, "ls")){
            cpio_ls();
        }
        else if(!string_comp(buf, "cat")){
            //uart_puts("cat is still working on\n");
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Filename: ");
            
            buf_index = uart_gets(buf, argv);
            if(buf_index >= MAX_BUF_LEN)
                uart_puts("Warning: buffer is full, command output may not correct\n");
        
            cpio_cat(buf);

            continue;
        }
        else if(!string_comp(buf, "el0")){
            void *file_addr;
            void *stack_addr;
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Program name: ");

            buf_index = uart_gets(buf, argv);
            if(buf_index >= MAX_BUF_LEN)
                uart_puts("Warning: buffer is full, command output may not correct\n");

            file_addr = cpio_find(buf);
            // indicating that the file is either a directory or not exist
            if(file_addr == 0)
                continue;

            stack_addr = simple_malloc(2048);
            // https://stackoverflow.com/questions/47516089/how-do-i-access-local-c-variable-in-arm-inline-assembly
            asm volatile(
                "mov x1, 0x3c0;"
                "msr spsr_el1, x1;"
                "mov x1, %[var1];"
                "mov x2, %[var2];"
                "msr elr_el1, x1;"
                "msr sp_el0, x2;"
                "eret"                         
                :                                                   // Output operand: empty here
                : [var1] "r" (file_addr),[var2] "r" (stack_addr)    // Input operand
                : "x1", "x2"                                        // clobbered registers(those are modified)
            );
            gdb_break();
        }
        else if(!string_comp(buf, "async")){
            char async_buf[MAX_BUF_LEN];

            uart_irq_puts("Async I/O test:");
            uart_irq_gets(async_buf);
            uart_irq_puts("You just typed:");
            uart_irq_puts(async_buf);
        }
        else if(!string_comp(buf, "settimeout")){
            uart_puts(argv[0]);
            uart_puts(argv[1]);

            add_timer(print_callback, h2i(argv[1], string_len(argv[1])));
        }
        /*else if(!string_comp(buf, "test")){
            buf_index = 0;
            string_set(buf, 0, MAX_BUF_LEN);
            
            uart_puts("Filename: ");

            while(1){
                input_char = uart_getc();
                // Get non ASCII code
                if(input_char > 127 || input_char < 0){
                    //uart_puts("\nwarning: Get non ASCII code\n");
                    continue;
                }
                if(buf_index < MAX_BUF_LEN)
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
            uart_puts(cpio_find(buf));
        }*/
        else{
            uart_puts("Unknown Command: ");
            uart_puts(buf);
        }
    }
}