#include "kernel/mmu.h"

void *set_2M_kernel_mmu(void *x0){
    my_uint64_t* pud_table = (my_uint64_t*)MMU_PUD_ADDR;

    my_uint64_t* pud_table1 = (my_uint64_t*)MMU_PTE_ADDR;
    // Use one page(4096) to store the page table for 2M block, so there are 512 entries(4096/8=512)
    my_uint64_t* pud_table2 = (my_uint64_t*)(MMU_PTE_ADDR + 0x1000L);
    // set 2M block for kernel, 
    for(int i = 0; i < 512; i++){
        // 0x200000 is 2M
        my_uint64_t addr = 0x200000L * i;
        
        if(addr >= PERIPHERAL_END){
            // device memory
            // + and | should be the same here
            pud_table1[i] = (0x00000000 + addr) + BOOT_PTE_ATTR_nGnRnE;
            continue;   
        }

        // device memory
        pud_table1[i] = (0x00000000 + addr) | BOOT_PTE_ATTR_NOCACHE;    // [0, 511] 2M blocks
        pud_table2[i] = (0x40000000 + addr) | BOOT_PTE_ATTR_NOCACHE;    // [512, 1023] 2M blocks
    }
    // set the pud table
    pud_table[0] = ((my_uint64_t)pud_table1) | BOOT_PUD_ATTR;
    pud_table[1] = ((my_uint64_t)pud_table2) | BOOT_PUD_ATTR;

    return x0;
}
// map a single page from user space to a physical address
void map_one_page(my_uint64_t *pgd, my_uint64_t va, my_uint64_t pa, my_uint64_t attr){
    my_uint64_t *table = pgd; // pgd is the virtual address of the page table
    // PGD->PUD->PTE->PA
    for(int i = 0; i < 4; i++){
        // get the index of the entry in the page table
        unsigned int idx = (va >> (39 - 9 * i)) & 0x1ff;
        // PTE
        if(i == 3){
            // set the attribute of the page
            // Bits[4:2] is for MAIR
            table[idx] = pa;
            table[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | attr;
            return;
        }
        // get the address of the next level page table
        //my_uint64_t *next_table = (my_uint64_t*)(table[idx] & 0x0000fffffffff000);
        // if the next level page table is not present, allocate a page for it
        if(table[idx] == 0){
            my_uint64_t *next_table = (my_uint64_t*)pool_alloc(4096);
            memzero((my_uint64_t)next_table, 4096);
            // set the attribute of the page table
            table[idx] = VIRT_TO_PHYS((my_uint64_t)next_table);
            table[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }
        table = (my_uint64_t *)PHYS_TO_VIRT(((my_uint64_t)(table[idx] & ENTRY_ADDR_MASK)));
    }
}

void mmu_add_vma(task_struct_t *tsk, my_uint64_t va, my_uint64_t pa, my_uint64_t size, my_uint64_t attr, int is_alloced){
    // align the size to 4KB. If it's larger than 4KB, it will be aligned to the next 4KB
    //size = (size + 0xfff) & ~0xfff;
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;
    // allocate a vm_area_struct
    vm_area_struct_t *vma = (vm_area_struct_t*)pool_alloc(sizeof(vm_area_struct_t));
    // set the fields of the vm_area_struct
    vma->virt_addr = va;
    vma->phys_addr = pa;
    vma->area_size = size;
    vma->rwx = attr;
    vma->is_alloced = is_alloced;
    // add the vm_area_struct to the vma list of the task
    // this should be equivalent to list_add_tail((list_head_t *)vma, &tsk->vma_list); ?
    list_add_tail((list_head_t *)vma, &tsk->vma_list);
}

void mmu_del_vma(task_struct_t *tsk){
    // get the first vm_area_struct in the vma list
    list_head_t *cur = tsk->vma_list.next;
    vm_area_struct_t *vma;
    // delete all the vm_area_struct in the vma list
    while(cur != &tsk->vma_list){
        vma = (vm_area_struct_t *)cur;
        // delete the current vm_area_struct
        //list_del_entry(&vma->listhead);
        if(vma->is_alloced){
            // free the memory of the page table
            pool_free((void*)PHYS_TO_VIRT(vma->phys_addr));
        }
        // get the next vm_area_struct
        list_head_t *next = cur->next;
        // free the memory of the vm_area_struct
        pool_free(cur);
        cur = next;
    }
}

void mmu_map_pages(my_uint64_t *virt_pgd_p, my_uint64_t va, my_uint64_t size, my_uint64_t pa, my_uint64_t flag){
    // align the size to 4KB. 
    //pa = pa & ~0xfff;
    pa = pa - (pa % 0x1000); // align
    for(my_uint64_t i = 0; i < size; i+=0x1000)
        map_one_page(virt_pgd_p, (va + i), (pa + i), flag);
}

void mmu_free_page_tables(my_uint64_t *page_table, int level){
    my_uint64_t *virt_table = (my_uint64_t*)PHYS_TO_VIRT((char*)page_table);

    if(level < 0)
        return;
    // free the page table of the next level
    for(int i = 0; i < 512; i++){
        if(virt_table[i] != 0){
            my_uint64_t *next_table = (my_uint64_t*)(virt_table[i] & ENTRY_ADDR_MASK);
            if (virt_table[i] & PD_TABLE){
                if(level != 2) // if it's not the last level, free the page table of the next level
                    mmu_free_page_tables(next_table, level + 1);
                virt_table[i] = 0L;
                pool_free((void*)PHYS_TO_VIRT((char*)next_table));
            }
        }
    }
}

void mmu_memfail_abort_handle(esr_el1_t* esr_el1){
    // Fault Address Register: contains the virtual address that caused a memory access fault
    unsigned long long far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t": "=r"(far_el1));

    list_head_t *pos;
    vm_area_struct_t *vma;
    vm_area_struct_t *the_area_ptr = 0;
    list_for_each(pos, &current_task->vma_list){
        vma = (vm_area_struct_t *)pos;
        if (vma->virt_addr <= far_el1 && vma->virt_addr + vma->area_size >= far_el1){
            the_area_ptr = vma;
            break;
        }
    }

    // no VMA contains the address
    if (!the_area_ptr){
        uart_puts("[Segmentation fault]: Kill Process ");
        uart_b2x_64(far_el1);
        uart_putc('\n');
        exit_process();
        return;
    }

    // For translation fault
    if ((esr_el1->iss & 0x3f) == TF_LEVEL0 || (esr_el1->iss & 0x3f) == TF_LEVEL1 || (esr_el1->iss & 0x3f) == TF_LEVEL2 || (esr_el1->iss & 0x3f) == TF_LEVEL3){
        uart_puts("[Translation fault]: ");
        uart_b2x_64(far_el1);
        uart_putc('\n');

        my_uint64_t addr_offset = (far_el1 - the_area_ptr->virt_addr);
        addr_offset = (addr_offset % 0x1000) == 0 ? addr_offset : addr_offset - (addr_offset % 0x1000);

        my_uint64_t flag = 0;
        if(!(the_area_ptr->rwx & (0b1 << 2))) flag |= PD_UNX;         // 4: executable
        if(!(the_area_ptr->rwx & (0b1 << 1))) flag |= PD_RDONLY;      // 2: writable
        if(  the_area_ptr->rwx & (0b1 << 0) ) flag |= PD_UK_ACCESS;   // 1: readable / accessible
        map_one_page(PHYS_TO_VIRT(current_task->context.pgd), the_area_ptr->virt_addr + addr_offset, the_area_ptr->phys_addr + addr_offset, flag);
        uart_puts("[Translation fault]: Fixed\n");
    }
    else{
        // For other Fault (permisson ...etc)
        uart_puts("[Segmentation fault]: Kill Process\n");
        exit_process();
    }
}