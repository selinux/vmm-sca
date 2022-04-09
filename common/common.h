#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>
#include <unistd.h>


#define PAGESIZE                (0x1000LL)

#define VM_MEM_RUN_ADDR         (0x0LL)
#define VM_MEM_RUN_SIZE         (0x200000LL)

#define STACK_BOTTOM_ADDR       (0x100000LL)
#define STACK_TOP_ADDR          (VM_MEM_RUN_ADDR+VM_MEM_RUN_SIZE)

#define VM_GDT_ADDR             (0x10000)
#define VM_GDT_SIZE             (0x1000)    // 512 entries (8 bytes)
#define VM_IDT_ADDR             (0x11000)
#define VM_IDT_SIZE             (0x1000)    // 256 entries (16 bytes)
#define VM_TSS_ADDR             (0x12000)
#define VM_TSS_SIZE             (0x1000)    //

#define NB_MMIO_PAGES           (0x1LL)
#define VM_MEM_MMIO_ADDR        (VM_MEM_RUN_ADDR+VM_MEM_RUN_SIZE)
#define VM_MEM_MMIO_SIZE        (NB_MMIO_PAGES*PAGESIZE)

#define NB_PT_PML4_PAGES        (0x1LL)                               // CR3 entry
#define NB_PT_PDPT_PAGES        (0x1LL)                               // first Gb is enough
#define NB_PT_PD_PAGES          (0x4LL)                               // 2*512*2Mb = 2Gb
#define NB_PTE_PAGES            (NB_PT_PD_PAGES*512LL)                // 1024*2Mb
#define VM_MEM_PT_ADDR          (VM_MEM_MMIO_ADDR+VM_MEM_MMIO_SIZE)
#define VM_MEM_PT_SIZE          ((NB_PT_PML4_PAGES+NB_PT_PDPT_PAGES+NB_PT_PD_PAGES+NB_PTE_PAGES+1)*PAGESIZE)

#define NB_SAMPLES              (0x8000LL)
#define VM_MEM_MEASURES_ADDR    (VM_MEM_PT_ADDR+VM_MEM_PT_SIZE)
#define VM_MEM_MEASURES_SIZE    (((NB_SAMPLES+511LL)/512LL)*PAGESIZE)
//#define VM_MEM_MEASURES_SIZE    (((NB_SAMPLES+511LL)/512LL)*PAGESIZE)

#define NB_OWN_PAGES            (0x1LL)
#define VM_MEM_OWNPAGES_ADDR    (VM_MEM_MEASURES_ADDR+VM_MEM_MEASURES_SIZE)
#define VM_MEM_OWNPAGES_SIZE    (NB_OWN_PAGES*PAGESIZE)

#define NB_SHARED_PAGES         (0xf00LL)
#define VM_MEM_SHAREDPAGES_ADDR (VM_MEM_OWNPAGES_ADDR+VM_MEM_OWNPAGES_SIZE)
#define VM_MEM_SHAREDPAGES_SIZE (NB_SHARED_PAGES*PAGESIZE)


#define PMIO_PRINT_MEASURES         0xABBA
#define PMIO_READ                   0xABBB
#define PMIO_READ_CMD               0xABBC
#define VM_EXIT_RETURN_CODE         (42)
#define VM_EXIT_RETURN_CODE_ADDR    (0x400)

#define PRIMITIVE_TARGET_OFFSET     (0x0)
#define PRIMITIVE_VALUE_OFFSET      (0x8)
#define PRIMITIVE_WAIT_OFFSET       (0x10)
#define PRIMITIVE_REAPETE_OFFSET    (0x18)
#define PRIMITIVE_TARGET_ADDR       (VM_MEM_MMIO_ADDR+PRIMITIVE_TARGET_OFFSET)
#define PRIMITIVE_VALUE_ADDR        (VM_MEM_MMIO_ADDR+PRIMITIVE_VALUE_OFFSET)
#define PRIMITIVE_WAIT_ADDR         (VM_MEM_MMIO_ADDR+PRIMITIVE_WAIT_OFFSET)
#define PRIMITIVE_REAPETE_ADDR      (VM_MEM_MMIO_ADDR+PRIMITIVE_REAPETE_OFFSET)

typedef enum {
    PRIMITIVE_WAIT=1,
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


typedef struct __attribute__((__packed__)) _header_s {
    ROLE role;
    uint64_t nb_measures;
} header_s;

typedef struct __attribute__((__packed__)) _command_s {
    ROLE vm_role;
    uint8_t cmd;
    useconds_t wait;
    uint64_t * addr;
    uint64_t value;
    uint32_t repeat;
} command_s;

typedef struct __attribute__((__packed__)) _command_u {
    union {
        header_s cmds_header;
        command_s cmd;
    };
} command_u;

typedef struct __attribute__((__packed__)) _timestamp_s {
    uint32_t id;        // cmd idx
    uint8_t cmd;        // cmd type
    uint8_t vm_id;      // vm
    useconds_t wait;    // delay
    uint64_t ts[5];     // timestamps
} timestamp_s;

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | SEG_GRAN(1) | SEG_PRIV(0) | SEG_DATA_RDWR

// Privilege levels
#define DPL_USER    0x3
#define DPL_KERNEL  0x0


// Descriptor types for system segments and gates
#define TYPE_LDT               2
#define TYPE_TASK_GATE         5
#define TYPE_TSS               9
#define TYPE_CALL_GATE         12
#define TYPE_TRAP_GATE         15
#define TYPE_INTERRUPT_GATE    14
#define TYPE_DATA_RW_EXPAND_DOWN  6  // stack segment type

#define GDT_KERNEL_CODE_SELECTOR   (8 | DPL_KERNEL)
#define GDT_KERNEL_DATA_SELECTOR  (16 | DPL_KERNEL)


typedef struct idt64_entry_st {
	uint16_t offset15_0;   // only used by trap and interrupt gates
	uint16_t selector;     // segment selector for trap and interrupt gates; TSS segment selector for task gates
	uint16_t ist : 3;
	uint16_t reserved : 5;
    uint16_t type : 5;
	uint16_t dpl : 2;
	uint16_t p : 1;
	uint16_t offset31_16;  // only used by trap and interrupt gates
    uint32_t offset64_32;
    uint32_t reserved2;
} __attribute__((packed)) idt64_entry_t;

#endif
