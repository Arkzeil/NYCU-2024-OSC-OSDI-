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
    unsigned int struct_size = BE2LE(fdt->size_dt_struct);
    // for manipulation of pointer
    char* struct_ptr = (char*)fdt;
    // goto struct block
    struct_ptr += BE2LE(fdt->off_dt_struct);
    //struct_ptr += align_mem_offset((void*)struct_ptr, 4);
    
    callback(cpio_addr);

    while(struct_ptr < ((char*)fdt + BE2LE(fdt->off_dt_struct) + struct_size)){
        uint32_t token = *(uint32_t*)struct_ptr;
        struct_ptr += 4;
        switch(BE2LE(token)){
            case FDT_NOP:
                //struct_ptr++;
                break;
            case FDT_BEGIN_NODE:
                uart_puts("node begin------------\n");
                unsigned int print_len = uart_puts(struct_ptr);
                uart_putc('\n');
                struct_ptr += print_len;
                // as there's a NULL, so add 1
                struct_ptr++;
                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_PROP:
                uart_puts("Property---------------\n");
                //while(*(++struct_ptr) == FDT_NOP){}
                // property value length 0 just indicate the property itself is sufficient(meaning that property name still exist)
                //if(*struct_ptr != 0x0){
                // property length
                unsigned int pro_len = BE2LE(*(unsigned int*)(struct_ptr));
                // 32bits
                struct_ptr += 4;
                // property name offset (at string block)
                uart_puts(struct_ptr + BE2LE(*(unsigned int*)struct_ptr));
                uart_putc('\n');
                // 32bits
                struct_ptr += 4;
                // property value
                if(pro_len > 0){
                    uart_puts_fixed(struct_ptr, pro_len);
                    uart_putc('\n');
                    struct_ptr += pro_len;
                }               
                //}
                /*uart_puts("len:");
                uart_b2x(*struct_ptr++);
                uart_putc('\n');
                uart_puts("nameoff:");
                uart_b2x(*struct_ptr);
                uart_putc('\n');*/
                struct_ptr += align_mem_offset((void*)struct_ptr, 4);
                break;
            case FDT_END_NODE:
                uart_puts("node end--------------\n");
                //struct_ptr++;
                break;
            case FDT_END:
                uart_puts("node all end----------\n");
                //struct_ptr++;
                break;
            default:
                uart_b2x((unsigned int)*struct_ptr);
                uart_putc('\n');
                //struct_ptr++;
        }
    }
    uart_b2x((unsigned int)*struct_ptr);
    uart_putc('\n');
}