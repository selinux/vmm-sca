/*
 * =====================================================================================
 *
 *       Filename:  VM.h
 *
 *    Description:  VM header
 *
 *        Version:  1.0
 *        Created:  14/03/22 09:37:00 AM CET
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sebastien Chassot (sinux), sebastien.chassot@etu.unige.ch
 *        Company:  Unige - Master in Computer Science
 *
 * =====================================================================================
 */
#ifndef __VM_H_
#define __VM_H_


#include "vmm.h"
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
#define EFER_SVME (1U << 12)
#define EFER_LMSLE (1U << 13)
#define EFER_TCE (1U << 15)


/* 64-bit page.img * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_PWT (1U << 3)
#define PDE64_PCD (1U << 4)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)
#define PDE64_R (1U << 11)

/*
 * kvm define from git://git.kernel.org/pub/scm/virt/kvm/kvm.git
 */
// #define KVM_EXIT_TRACE "kvm:kvm_exit"
// #define KVM_EXIT_REASON "exit_reason"
// #define KVM_EXIT_TRACE "kvm:kvm_s390_sie_exit"
// #define KVM_EXIT_REASON "icptcode"
// #define KVM_EXIT_HYPERV_SYNIC          1
// #define KVM_EXIT_HYPERV_HCALL          2
// #define KVM_EXIT_HYPERV_SYNDBG         3
// #define KVM_EXIT_XEN_HCALL          1
// #define KVM_EXIT_UNKNOWN          0
// #define KVM_EXIT_EXCEPTION        1
// #define KVM_EXIT_IO               2
// #define KVM_EXIT_HYPERCALL        3
// #define KVM_EXIT_DEBUG            4
// #define KVM_EXIT_HLT              5
// #define KVM_EXIT_MMIO             6
// #define KVM_EXIT_IRQ_WINDOW_OPEN  7
// #define KVM_EXIT_SHUTDOWN         8
// #define KVM_EXIT_FAIL_ENTRY       9
// #define KVM_EXIT_INTR             10
// #define KVM_EXIT_SET_TPR          11
// #define KVM_EXIT_TPR_ACCESS       12
// #define KVM_EXIT_S390_SIEIC       13
// #define KVM_EXIT_S390_RESET       14
// #define KVM_EXIT_DCR              15 /* deprecated */
// #define KVM_EXIT_NMI              16
// #define KVM_EXIT_INTERNAL_ERROR   17
// #define KVM_EXIT_OSI              18
// #define KVM_EXIT_PAPR_HCALL       19
// #define KVM_EXIT_S390_UCONTROL    20
// #define KVM_EXIT_WATCHDOG         21
// #define KVM_EXIT_S390_TSCH        22
// #define KVM_EXIT_EPR              23
// #define KVM_EXIT_SYSTEM_EVENT     24
// #define KVM_EXIT_S390_STSI        25
// #define KVM_EXIT_IOAPIC_EOI       26
// #define KVM_EXIT_HYPERV           27
// #define KVM_EXIT_ARM_NISV         28
// #define KVM_EXIT_X86_RDMSR        29
// #define KVM_EXIT_X86_WRMSR        30
// #define KVM_EXIT_DIRTY_RING_FULL  31
// #define KVM_EXIT_AP_RESET_HOLD    32
// #define KVM_EXIT_X86_BUS_LOCK     33
// #define KVM_EXIT_XEN              34
// #define KVM_EXIT_RISCV_SBI        35
// #define KVM_EXIT_IO_IN  0
// #define KVM_EXIT_IO_OUT 1


extern const unsigned char vm_alice[], vm_alice_end[];
extern const unsigned char vm_charlie[], vm_charlie_end[];
extern const unsigned char vm_eve[], vm_eve_end[];

typedef enum {
    MEM_SLOT_0,
    MEM_SLOT_1,
    MEM_SLOT_2,
    MEM_SLOT_3,
    MEM_SLOT_4,
    MEM_SLOT_5,
    NUMBEROFSLOT
} MEM_REGION;


void vm_init(vm* vm, const char * shared_pages);
struct kvm_translation translate_vm_addr(vm* vm, long long unsigned int addr);
char * vm_role(ROLE r);

#endif
