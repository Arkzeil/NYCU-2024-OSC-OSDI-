#ifndef MMU_H
#define MMU_H

#define PHYS_TO_VIRT(x)   (x + 0xffff000000000000)
#define VIRT_TO_PHYS(x)   (x - 0xffff000000000000)

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)
// ref:https://developer.arm.com/documentation/ihi0062/b/Stage-1-Translation-Context-Bank-Format/Memory-attribute-indirection
// https://blog.csdn.net/weixin_42135087/article/details/109351663
#define MAIR_DEVICE_nGnRnE      0b00000000 // Strongly-ordered
#define MAIR_NORMAL_NOCACHE     0b01000100 // Normal memory, Inner Non-cacheable
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1
// Bits[1:0] Specify the next level is a block/page, page table, or invalid.
#define PD_TABLE                0b11L
#define PD_BLOCK                0b01L
#define PD_ACCESS               (1 << 10)  // The access flag, a page fault is generated if not set.
#define PD_UNX                  (1L << 54)  // user(EL0) no-execute
#define PD_KNX                  (1L << 53)  // kernel(EL1) no-execute
#define PD_RDONLY               (1L << 7)   // 0 for read-write, 1 for read-only.
#define PD_UK_ACCESS            (1L << 6)   // 0 for only kernel access, 1 for user/kernel access.

#define BOOT_PGD_ATTR           PD_TABLE
// #define BOOT_PUD_ATTR           (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_PUD_ATTR           (PD_TABLE | PD_ACCESS)
#define BOOT_PTE_ATTR_NOCACHE   (PD_BLOCK | PD_ACCESS | (MAIR_NORMAL_NOCACHE << 2))
#define BOOT_PTE_ATTR_nGnRnE    (PD_BLOCK | PD_ACCESS | (MAIR_DEVICE_nGnRnE << 2) | PD_UNX | PD_KNX | PD_UK_ACCESS)

#define MMU_PGD_BASE            0x1000L
#define MMU_PGD_ADDR            (MMU_PGD_BASE + 0x0000L)
#define MMU_PUD_ADDR            (MMU_PGD_BASE + 0x1000L)
#define MMU_PTE_ADDR            (MMU_PGD_BASE + 0x2000L)
// Shouldn't not be 0x3f000000 to 0x3fffffff ?
#define PERIPHERAL_START        0x3c000000L
#define PERIPHERAL_END          0x3f000000L

#define USER_KERNEL_BASE        0x00000000L
#define USER_STACK_BASE         0xfffffffff000L
#define USER_SIGNAL_WRAPPER_VA  0xffffffff9000L

#define ENTRY_ADDR_MASK         0xfffffffff000L

// DDI0487A_a_armv8_arm.pdf-p.1901
#define MEMFAIL_DATA_ABORT_LOWER 0b100100
#define MEMFAIL_INST_ABORT_LOWER 0b100000

// Translation fault p.1525
#define TF_LEVEL0 0b000100
#define TF_LEVEL1 0b000101
#define TF_LEVEL2 0b000110
#define TF_LEVEL3 0b000111

#ifndef __ASSEMBLER__

#include "kernel/type.h"
#include "kernel/allocator.h"
#include "kernel/mem.h"
#include "kernel/list.h"
#include "kernel/process.h"
#include "kernel/thread.h"

typedef struct vm_area_struct
{
    list_head_t listhead;
    my_uint64_t virt_addr;
    my_uint64_t phys_addr;
    my_uint64_t area_size;
    my_uint64_t rwx;      // 1, 2, 4
    int is_alloced;
} vm_area_struct_t;

// DDI0487A_a_armv8_arm.pdf-p.1512
// used for mmu fault handling
typedef struct{
    unsigned int iss : 25, // Instruction specific 
                 il : 1,   // Instruction length bit 32bit(0) or 64bits(1)
                 ec : 6;   // Exception class
} esr_el1_t;

typedef struct task_struct task_struct_t;
typedef struct thread thread_t;

void *set_2M_kernel_mmu(void *x0);
void map_one_page(my_uint64_t *pgd, my_uint64_t va, my_uint64_t pa, my_uint64_t attr);
void mmu_add_vma(task_struct_t *tsk, my_uint64_t va, my_uint64_t pa, my_uint64_t size, my_uint64_t attr, int is_alloced);
void mmu_del_vma(task_struct_t *tsk);
void mmu_map_pages(my_uint64_t *virt_pgd_p, my_uint64_t va, my_uint64_t size, my_uint64_t pa, my_uint64_t flag);
void mmu_free_page_tables(my_uint64_t *page_table, int level);
void mmu_memfail_abort_handle(esr_el1_t* esr_el1);
#endif

#endif