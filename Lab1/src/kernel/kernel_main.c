#include "kernel/kernel_main.h"

void main(void)
{
    uart_init();
    uart_puts("Hello, world!\r\n");

    while (1) {
        uart_putc(uart_getc());
    }
}