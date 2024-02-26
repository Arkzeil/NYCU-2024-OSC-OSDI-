#include "kernel/gpio.h"
#include "kernel/uart.h"

void main(void)
{
    uart_init();
    uart_puts("Hello, world!\r\n");
    /*show every char typed*/
    while (1) {
        uart_putc(uart_getc());
    }
}