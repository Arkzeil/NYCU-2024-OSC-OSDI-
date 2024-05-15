#include "kernel/mmu.h"

void *set_2M_kernel_mmu(void *x0){
    my_uint64_t* pud_table = (my_uint64_t*)MMU_PUD_ADDR;

    my_uint64_t* pud_table1 = (my_uint64_t*)MMU_PTE_ADDR;
    // Use one page(4096) to store the page table for 2M block, so there are 512 entries(4096/8=512)
    my_uint64_t* pud_table2 = (my_uint64_t*)(MMU_PTE_ADDR + 0x1000);
    // set 2M block for kernel, 
    for(int i = 0; i < 512; i++){
        // 0x200000 is 2M
        my_uint64_t addr = 0x200000 * i;
        
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
void map_one_page(void *pgd, void *va, void *pa, my_uint64_t attr){
    my_uint64_t *table = (my_uint64_t*)pgd; // pgd is the virtual address of the page table
    // PGD->PUD->PTE->PA
    for(int i = 0; i < 4; i++){
        // get the index of the entry in the page table
        int idx = ((my_uint64_t)va >> (39 - 9 * i)) & 0x1ff;
        // PTE
        if(i == 3){
            // set the attribute of the page
            // Bits[4:2] is for MAIR
            table[idx] = ((my_uint64_t)pa) | PD_ACCESS | PD_TABLE | (MAIR_NORMAL_NOCACHE << 2) | PD_KNX | attr;
            return;
        }
        // get the address of the next level page table
        //my_uint64_t *next_table = (my_uint64_t*)(table[idx] & 0x0000fffffffff000);
        // if the next level page table is not present, allocate a page for it
        if(table[idx] == 0){
            my_uint64_t *next_table = (my_uint64_t*)pool_alloc(4096);
            memzero(next_table, 4096);
            // set the attribute of the page table
            table[idx] = VIRT_TO_PHYS((my_uint64_t)next_table);
            table[idx] |= PD_ACCESS | PD_TABLE | (MAIR_NORMAL_NOCACHE << 2);
        }
        table = PHYS_TO_VIRT(table[idx] & 0x0000fffffffff000);
    }
}

void mmu_add_vma(task_struct_t *tsk, void *va, void *pa, my_uint64_t size, my_uint64_t attr, my_uint64_t is_alloced){
    // align the size to 4KB. If it's larger than 4KB, it will be aligned to the next 4KB
    size = (size + 0xfff) & ~0xfff;
    // allocate a vm_area_struct
    vm_area_struct_t *vma = (vm_area_struct_t*)pool_alloc(sizeof(vm_area_struct_t));
    // set the fields of the vm_area_struct
    vma->virt_addr = (my_uint64_t)va;
    vma->phys_addr = (my_uint64_t)pa;
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
        // get the next vm_area_struct
        vm_area_struct_t *next = (vm_area_struct_t *)cur->next;
        // delete the current vm_area_struct
        //list_del_entry(&vma->listhead);
        if(vma->is_alloced){
            // free the memory of the page table
            pool_free((void*)PHYS_TO_VIRT(vma->phys_addr));
        }
        // free the memory of the vm_area_struct
        pool_free(cur);
        cur = next;
    }
}

void mmu_map_pages(my_uint64_t *virt_pgd_p, my_uint64_t va, my_uint64_t size, my_uint64_t pa, my_uint64_t flag){
    // align the size to 4KB. 
    pa = pa & ~0xfff;
    for(my_uint64_t i = 0; i < size; i+=0x1000)
        map_one_page(virt_pgd_p, (void*)(va + i), (void*)(pa + i), flag);
}