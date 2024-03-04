#include "kernel/dtb.h"

void initramfs_callback(){

}

void fdt_traverse(void *callback, void *_dtb){
    struct fdt_header* fdt = (struct fdt_header*)_dtb;

    if(fdt->magic != 0xd00dfeed){
        uart_puts("dtb magic value error\n");
        return;
    }
    unsigned int total_size  = fdt->totalsize;
    unsigned int string_size = fdt->size_dt_strings;
    unsigned int struct_size = fdt->size_dt_struct;

    uint32_t* struct_ptr = (uint32_t*)(fdt + fdt->off_dt_struct);
    
    while(struct_ptr < fdt + struct_size){
        switch(*struct_ptr){
            case FDT_NOP:
                break;
            case FDT_BEGIN_NODE:
                uart_puts("node begin------------\n");
                uart_puts((char*)(++struct_ptr));
                struct_ptr += align_offset(struct_ptr, 4);
                break;
            case FDT_PROP:
                while(*(++struct_ptr) == FDT_NOP){}
                uart_puts("len:");
                uart_b2x(*struct_ptr++);
                uart_puts("nameoff:");
                uart_b2x(*struct_ptr);
                struct_ptr += align_offset(struct_ptr, 4);
                break;
            case FDT_END_NODE:
                uart_puts("node end--------------\n");
                struct_ptr++;
                break;
            case FDT_END:
                uart_puts("node all end----------\n");
                struct_ptr++;
                break;
        }
    }
}