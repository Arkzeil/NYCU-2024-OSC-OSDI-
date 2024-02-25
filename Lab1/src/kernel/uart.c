#include "kernel/uart.h"
void uart_init (void){
    // allocate an 32 bits register(if we don't assign resiter, it would be allocated in memory)
    register unsigned int reg;

    reg = mmio_read(GPFSEL1);
    reg &= ~((7<<12) | (7<<15));
    reg |= (2<<12) | (2<<15);

    mmio_write(GPFSEL1, reg);

    *AUX_ENABLE         |=  1;  // enable mini UART. Then mini UART register can be accessed.
    *AUX_MU_CNTL_REG    =   0;  // Disable transmitter and receiver during configuration.
    *AUX_MU_IER_REG     =   0;  // Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_LCR_REG     =   3;  // Set the data size to 8 bit.
    *AUX_MU_MCR_REG     =   0;  // Don’t need auto flow control.
    *AUX_MU_BAUD_REG    =   270;// Set baud rate to 115200
    *AUX_MU_IIR_REG     =   6;  // No FIFO
    *AUX_MU_CNTL_REG    =   3;  // Enable the transmitter and receiver.
}
void uart_putc(unsigned char c){

}
unsigned char uart_getc(){

}
void uart_puts(const char* str){

}