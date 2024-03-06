#include "kernel/dtb.h"

void initramfs_callback(char *cpio_addr){
    uart_puts("callback function test\n");
    //uart_b2x((unsigned int)cpio_addr);
}

void fdt_traverse(void (*callback)(char *), void *_dtb){
    struct fdt_header* fdt = (struct fdt_header*)_dtb;

    if(BE2LE(fdt->magic) != 0xd00dfeed){
        uart_puts("dtb magic value error\n");
        uart_b2x(BE2LE(fdt->magic));
        uart_putc('\n');
        return;
    }
    //unsigned int total_size  = fdt->totalsize;
    //unsigned int string_size = fdt->size_dt_strings;
    unsigned int struct_size = fdt->size_dt_struct;
    // for manipulation of pointer
    char* struct_ptr = (char*)fdt;
    // goto struct block
    struct_ptr += fdt->off_dt_struct;
    
    callback(cpio_addr);

    while(struct_ptr < ((char*)fdt + struct_size)){
        switch(*struct_ptr){
            case FDT_NOP:
                break;
            case FDT_BEGIN_NODE:
                uart_puts("node begin------------\n");
                uart_puts((char*)(++struct_ptr));
                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_PROP:
                while(*(++struct_ptr) == FDT_NOP){}
                uart_puts("len:");
                uart_b2x(*struct_ptr++);
                uart_puts("nameoff:");
                uart_b2x(*struct_ptr);
                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_END_NODE:
                uart_puts("node end--------------\n");
                struct_ptr++;
                break;
            case FDT_END:
                uart_puts("node all end----------\n");
                struct_ptr++;
                break;
            default:
                uart_b2x((unsigned int)*struct_ptr);
                uart_putc('\n');
        }
    }
}