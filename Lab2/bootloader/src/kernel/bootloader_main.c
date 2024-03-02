#include "kernel/uart.h"

void bootloader_main(){
    int kernel_size = 0;
    // a char pointer has the same alignment requirement as a void pointer.
    char *kernel_addr = (char *)0x80000;
    uart_init();
    uart_puts("Hello, world! 312552025\r\n");
    uart_puts("Start bootloading\n");

    // since the data is little endian, LSB will at lowest address(first received)
    kernel_size = uart_getc();
    kernel_size += (uart_getc() << 8);
    kernel_size += (uart_getc() << 16);
    kernel_size += (uart_getc() << 24);

    for(;kernel_size>0; kernel_size--){
        *kernel_addr = uart_getc();
        kernel_addr++;
    }

    uart_puts("Kernel transmission completed\n");
    // x30 is link register(lr), record return address(usuallt used when bl is called)
    // here we use it as our path to our shell kernel as 'ret' will return to return address
    asm volatile(
        "mov x0, x10;"
        "mov x1, x11;"
        "mov x2, x12;"
        "mov x3, x13;"
        "mov x30, 0x80000;"
        "ret;"
    );
}