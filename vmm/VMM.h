#ifndef __VMM_H_
#define __VMM_H_


/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page.img directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)

/* 64-bit page.img * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)


extern const unsigned char vm_alice[], vm_alice_end[];
extern const unsigned char vm_charlie[], vm_charlie_end[];
extern const unsigned char vm_eve[], vm_eve_end[];

typedef enum {
    MEMRUN,
    MEMMMIO,
    MEMMEASURES,
    MEM_OWN_PAGES,
    MEM_SHARED_PAGES,
    NUMBEROFREGION
} MEM_REGION;

struct vcpu {
    struct kvm_run *kvm_run;
};

typedef struct _vm {
    char vm_name[256];
    ROLE vm_role;
    int fd_vm;
    int fd_vcpu;
    struct vcpu vcpu;
    struct kvm_sregs sregs;
    struct kvm_regs regs;
    char *mem_run;
   	struct kvm_userspace_memory_region mem_reg_run;
    char *mem_mmio;
    struct kvm_userspace_memory_region mem_reg_mmio;
    char *mem_measures;
    struct kvm_userspace_memory_region mem_reg_measures;
    char *mem_own;
    struct kvm_userspace_memory_region mem_reg_own;
    char *mem_shared;
    struct kvm_userspace_memory_region mem_reg_shared;
} vm;

void vm_init(vm* vm, int vcpu_mmap_size, const char * shared_pages);


#endif