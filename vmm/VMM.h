#ifndef __VMM_H_
#define __VMM_H_

/*
#define KVM_EXIT_UNKNOWN          0
#define KVM_EXIT_EXCEPTION        1
#define KVM_EXIT_IO               2
#define KVM_EXIT_HYPERCALL        3
#define KVM_EXIT_DEBUG            4
#define KVM_EXIT_HLT              5
#define KVM_EXIT_MMIO             6
#define KVM_EXIT_IRQ_WINDOW_OPEN  7
#define KVM_EXIT_SHUTDOWN         8
#define KVM_EXIT_FAIL_ENTRY       9
#define KVM_EXIT_INTR             10
#define KVM_EXIT_SET_TPR          11
#define KVM_EXIT_TPR_ACCESS       12
#define KVM_EXIT_S390_SIEIC       13
#define KVM_EXIT_S390_RESET       14
#define KVM_EXIT_DCR              15
#define KVM_EXIT_NMI              16
#define KVM_EXIT_INTERNAL_ERROR   17
 */

struct vcpu {
    struct kvm_run *kvm_run;
};

typedef struct _vm {
    char vm_name[256];
    ROLE vm_role;
    int fd_vm;
    int fd_vcpu;
    struct vcpu vcpu;
    void *mem_run;
    void *mem_pages_tables;
    void *mem_mmio;
    void *mem_measures;
    void *mem_own;
    void *mem_shared;
} vm;


#endif