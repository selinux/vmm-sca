#ifndef __COMMON_H_
#define __COMMON_H_



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

#define PMIO_PRINT_MEASURES         0xABBA
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

#endif
