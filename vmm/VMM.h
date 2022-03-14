#ifndef __VMM_H_
#define __VMM_H_


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
    void *mem_run;
   	struct kvm_userspace_memory_region mem_reg_run;
    void *mem_mmio;
    struct kvm_userspace_memory_region mem_reg_mmio;
    void *mem_measures;
    struct kvm_userspace_memory_region mem_reg_measures;
    void *mem_own;
    struct kvm_userspace_memory_region mem_reg_own;
    void *mem_shared;
    struct kvm_userspace_memory_region mem_reg_shared;
} vm;

void vm_init(vm* vm, int vcpu_mmap_size, const char * shared_pages);


#endif