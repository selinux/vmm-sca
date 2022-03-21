#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>


#define PAGESIZE                (0x1000LL)
#define SAMPLES2PAGES(A)        (((A+511)/512)*PAGESIZE)
#define PAGES2SIZE(A)           (A*PAGESIZE)

#define VM_MEM_RUN_ADDR         (0x0LL)
#define VM_MEM_RUN_SIZE         (0x200000LL)
#define STACK_BOTTOM_ADDR       (0x100000LL)
#define STACK_TOP_ADDR          (VM_MEM_RUN_ADDR+VM_MEM_RUN_SIZE)

#define NB_MMIO_PAGES           (0x1LL)
#define VM_MEM_MMIO_ADDR        (VM_MEM_RUN_ADDR+VM_MEM_RUN_SIZE)
#define VM_MEM_MMIO_SIZE        (PAGES2SIZE(NB_MMIO_PAGES))

#define NB_PT_PML4_PAGES        (0x1LL)                               // CR3 entry
#define NB_PT_PDPT_PAGES        (0x1LL)                               // first Gb is enough
#define NB_PT_PD_PAGES          (0x4LL)                               // 2*512*2Mb = 2Gb
#define NB_PTE_PAGES            (NB_PT_PD_PAGES*512LL)                // 1024*2Mb
#define VM_MEM_PT_ADDR          (VM_MEM_MMIO_ADDR+VM_MEM_MMIO_SIZE)
#define VM_MEM_PT_SIZE          ((NB_PT_PML4_PAGES+NB_PT_PDPT_PAGES+NB_PT_PD_PAGES+NB_PTE_PAGES+1)*PAGESIZE)

#define NB_SAMPLES              (0x10LL)
#define VM_MEM_MEASURES_ADDR    (VM_MEM_PT_ADDR+VM_MEM_PT_SIZE)
#define VM_MEM_MEASURES_SIZE    (SAMPLES2PAGES(NB_SAMPLES))

#define NB_OWN_PAGES            (0x20LL)
#define VM_MEM_OWNPAGES_ADDR    (VM_MEM_MEASURES_ADDR+VM_MEM_MEASURES_SIZE)
#define VM_MEM_OWNPAGES_SIZE    (PAGES2SIZE(NB_OWN_PAGES))

#define NB_SHARED_PAGES         (0x40LL)
#define VM_MEM_SHAREDPAGES_ADDR (VM_MEM_OWNPAGES_ADDR+VM_MEM_OWNPAGES_SIZE)
#define VM_MEM_SHAREDPAGES_SIZE (PAGES2SIZE(NB_SHARED_PAGES))

#define VM_GDT_ADDR                 (0x4000)
#define VM_GDT_SIZE                 (0x1000)    // 512 entries (8 bytes)
#define VM_IDT_ADDR                 (0x5000)
#define VM_IDT_SIZE                 (0x1000)    // 256 entries (16 bytes)
#define PMIO_PRINT_MEASURES         0xABBA
#define PMIO_READ                   0xABCA
#define VM_EXIT_RETURN_CODE         42
#define VM_EXIT_RETURN_CODE_ADDR    (0x400)

#define PRIMITIVE_CMD_ADDR          (0x500)

typedef enum {
    PRIMITIVE_WAIT=0,
    PRIMITIVE_MEASURE,
    PRIMITIVE_READ,
    PRIMITIVE_WRITE,
    PRIMITIVE_PRINT_MEASURES,
    PRIMITIVE_EXIT
} SCA_PRIMITIVE;


typedef enum {
    VICTIM,
    ATTACKER,
    DEFENDER,
    NUMBEROFROLE,
} ROLE;

// Privilege levels
#define DPL_USER    0x3
#define DPL_KERNEL  0x0

// Descriptor types for code and data segments
#define TYPE_DATA_RO    1   // read-only
#define TYPE_DATA_RW    3   // read-write

// Descriptor types for system segments and gates
#define TYPE_LDT               2
#define TYPE_TASK_GATE         5
#define TYPE_TSS               9
#define TYPE_CALL_GATE         12
#define TYPE_TRAP_GATE         15
#define TYPE_INTERRUPT_GATE    14
#define TYPE_DATA_RW_EXPAND_DOWN  6  // stack segment type

#define TYPE_CODE_EXECONLY     9
#define TYPE_CODE_EXECREAD    11

#define GDT_KERNEL_CODE_SELECTOR   (8 | DPL_KERNEL)
#define GDT_KERNEL_DATA_SELECTOR  (16 | DPL_KERNEL)

typedef struct gdt_entry_st {
	uint16_t lim15_0;
	uint16_t base15_0;
	uint8_t base23_16;

	uint8_t type : 4;
	uint8_t s : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;

	uint8_t lim19_16 : 4;
	uint8_t avl : 1;
	uint8_t l : 1;
	uint8_t db : 1;
	uint8_t granularity : 1;

	uint8_t base31_24;
} __attribute__((packed)) gdt_entry_t;

typedef struct idt_entry_st {
	uint16_t offset15_0;   // only used by trap and interrupt gates
	uint16_t selector;     // segment selector for trap and interrupt gates; TSS segment selector for task gates
	uint16_t reserved : 8;
	uint16_t type : 5;
	uint16_t dpl : 2;
	uint16_t p : 1;

	uint16_t offset31_16;  // only used by trap and interrupt gates
} __attribute__((packed)) idt_entry_t;

#endif
