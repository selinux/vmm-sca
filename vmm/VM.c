/*
 * =====================================================================================
 *
 *       Filename:  VM.c
 *
 *    Description:  VM
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <sys/random.h>

#include "../common/common.h"
#include "VM.h"


char * vm_role(ROLE r){
    switch (r) {
        case VICTIM:
            return "victime";
        case ATTACKER:
            return "attacker";
        case DEFENDER:
            return "defender";
        case NUMBEROFROLE:
            return "";
        default:
            return "";
    }
}

static void setup_64bit_code_segment(struct kvm_sregs *sregs)
{
    struct kvm_segment seg = {
            .base = 0,
            .limit = 0xffffffff,
            .selector = 1 << 3,
            .present = 1,
            .type = 11,     /* Code: execute, read, accessed */
            .dpl = 0,        /* ring 0 */
            .db = 0,
            .s = 1,           /* Code/data */
            .l = 1,
            .g = 1,           /* 4KB granularity */
    };

    sregs->cs = seg;

    seg.type = 3;                /* Data: read/write, accessed */
    seg.selector = 2 << 3;
    sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void setup_long_mode(vm *vm, struct kvm_sregs *sregs)
{
    uint64_t pml4_addr = 0x2000;
    uint64_t *pml4 = (void *)(vm->mem_run + pml4_addr);

    uint64_t pdpt_addr = 0x3000;
    uint64_t *pdpt = (void *)(vm->mem_run + pdpt_addr);

    uint64_t pd_addr = 0x4000;
    uint64_t *pd = (void *)(vm->mem_run + pd_addr);

    pml4[0] = PDE64_PRESENT | PDE64_RW | pdpt_addr;
    pdpt[0] = PDE64_PRESENT | PDE64_RW | pd_addr;
    for(uint64_t i = 0; i < 0x200; i++)
        pd[i] = PDE64_PRESENT | PDE64_RW | PDE64_PS | (i * PAGESIZE);

    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;  // CR0 : https://en.wikipedia.org/wiki/Control_register#CR0
    sregs->cr3 = pml4_addr;                                                     // page table entry
    sregs->cr4 = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT | CR4_TSD | CR4_PCE;     // TSD : If set, RDTSC instruction can only be executed when in ring 0
    sregs->efer = EFER_LME | EFER_LMA;                                          // PCE : Performance-Monitoring Counter enable
                                                                                // PGE : If set, address translations (PDE or PTE records) may be shared between address spaces
    setup_64bit_code_segment(sregs);
}

/** Initialize a VM according to his role
 *
 * @param vm VM fd
 * @param mem_size
 * @param vcpu_mmap_size
 */
void vm_init(vm* vm, int vcpu_mmap_size, const char * shared_pages)
{

    printf("%s (%s) : init 64-bit mode\n", vm->vm_name, vm_role(vm->vm_role));

    if (ioctl(vm->fd_vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) { perror("KVM_SET_TSS_ADDR"); exit(1);}

//    if(ioctl(vm->fd_vm, KVM_CREATE_IRQCHIP, 0) < 0){
//        perror("KVM_CREATE_IRQCHIP");
//        exit(1);
//    }

    vm->mem_run = mmap(NULL, VM_MEM_RUN_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_run == MAP_FAILED) { perror("mmap run region"); exit(1);}

    vm->mem_mmio = mmap(NULL, VM_MEM_MMIO_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_mmio == MAP_FAILED) { perror("mmap mmio region"); exit(1);}

    vm->mem_measures = mmap(NULL, VM_MEM_MEASURES_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_measures == MAP_FAILED) { perror("mmap measures region"); exit(1);}

    vm->mem_own = mmap(NULL, VM_MEM_OWNPAGES_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_own == MAP_FAILED) { perror("mmap own pages region"); exit(1);}

    vm->mem_shared = mmap(NULL, VM_MEM_SHAREDPAGES_SIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_shared == MAP_FAILED) { perror("mmap shared pages region"); exit(1);}

    /* host kernel memory regions advise */
    madvise(vm->mem_run, VM_MEM_RUN_SIZE, MADV_UNMERGEABLE);
    madvise(vm->mem_mmio, VM_MEM_MMIO_SIZE, MADV_UNMERGEABLE);
    madvise(vm->mem_measures, VM_MEM_MEASURES_SIZE, MADV_UNMERGEABLE);
    madvise(vm->mem_own, VM_MEM_OWNPAGES_SIZE, MADV_UNMERGEABLE);      // avoid merging
    madvise(vm->mem_shared, VM_MEM_SHAREDPAGES_SIZE, MADV_MERGEABLE);  // advise merge

    /* VM code,data,stack */
    vm->mem_reg_run.slot = MEM_SLOT_0;
    vm->mem_reg_run.flags = 0;
    vm->mem_reg_run.guest_phys_addr = VM_MEM_RUN_ADDR;
    vm->mem_reg_run.memory_size = VM_MEM_RUN_SIZE;
    vm->mem_reg_run.userspace_addr = (unsigned long)vm->mem_run;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &vm->mem_reg_run) < 0) { perror("KVM_SET_USER_MEMORY_REGION VMRUN"); exit(1);}

    /* MMIO (unused yet) */
    vm->mem_reg_mmio.slot = MEM_SLOT_1;
    vm->mem_reg_mmio.flags = KVM_MEM_READONLY;             // VM_EXIT
    vm->mem_reg_mmio.guest_phys_addr = VM_MEM_MMIO_ADDR;
    vm->mem_reg_mmio.memory_size = VM_MEM_MMIO_SIZE;
    vm->mem_reg_mmio.userspace_addr = (unsigned long)vm->mem_mmio;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &vm->mem_reg_mmio) < 0) { perror("KVM_SET_USER_MEMORY_REGION MMIO"); exit(1);}

    /* time measurement region */
    vm->mem_reg_measures.slot = MEM_SLOT_2;
    vm->mem_reg_measures.flags = 0;
    vm->mem_reg_measures.guest_phys_addr = VM_MEM_MEASURES_ADDR;
    vm->mem_reg_measures.memory_size = VM_MEM_MEASURES_SIZE;
    vm->mem_reg_measures.userspace_addr = (unsigned long)vm->mem_measures;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &vm->mem_reg_measures) < 0) { perror("KVM_SET_USER_MEMORY_REGION MEASURES"); exit(1);}

    /* VM own pages (not shared) for read/write access tests */
    vm->mem_reg_own.slot = MEM_SLOT_3;
    vm->mem_reg_own.flags = 0;
    vm->mem_reg_own.guest_phys_addr = VM_MEM_OWNPAGES_ADDR;
    vm->mem_reg_own.memory_size = VM_MEM_OWNPAGES_SIZE;
    vm->mem_reg_own.userspace_addr = (unsigned long)vm->mem_own;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &vm->mem_reg_own) < 0) { perror("KVM_SET_USER_MEMORY_REGION OWN PAGES"); exit(1);}

    /* VMs shared pages for read access, flush, COW,... tests (attacker) */
    vm->mem_reg_shared.slot = MEM_SLOT_4;
    vm->mem_reg_shared.flags = 0;
    vm->mem_reg_shared.guest_phys_addr = VM_MEM_SHAREDPAGES_ADDR;
    vm->mem_reg_shared.memory_size = VM_MEM_SHAREDPAGES_SIZE;
    vm->mem_reg_shared.userspace_addr = (unsigned long)vm->mem_shared;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &vm->mem_reg_shared) < 0) { perror("KVM_SET_USER_MEMORY_REGION SHARED PAGES"); exit(1);}

    /* init vcpu */
    vm->fd_vcpu = ioctl(vm->fd_vm, KVM_CREATE_VCPU, 0);
    if (vm->fd_vcpu < 0) { perror("KVM_CREATE_VCPU"); exit(1);}

    vm->vcpu.kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, vm->fd_vcpu, 0);
    if (vm->vcpu.kvm_run == MAP_FAILED) { perror("mmap kvm_run"); exit(1); }

    /* set special register */
    if (ioctl(vm->fd_vcpu, KVM_GET_SREGS, &vm->sregs) < 0) { perror("KVM_GET_SREGS");	exit(1);}

    setup_long_mode(vm, &vm->sregs);
    if (ioctl(vm->fd_vcpu, KVM_SET_SREGS, &vm->sregs) < 0) { perror("KVM_SET_SREGS"); exit(1);}

    memset(&vm->regs, 0, sizeof(vm->regs));
    /* Clear all FLAGS bits, except bit 1 which is always set. */
    vm->regs.rflags = 2;
    vm->regs.rip = 0;
    /* Create stack at top of 2 MB page.img and grow down. */
    vm->regs.rsp = STACK_ADDR;

    if (ioctl(vm->fd_vcpu, KVM_SET_REGS, &vm->regs) < 0) { perror("KVM_SET_REGS"); exit(1);}

    switch(vm->vm_role){
        case VICTIM:
            memcpy(vm->mem_run, vm_alice, vm_alice_end - vm_alice);
            break;
        case ATTACKER:
            memcpy(vm->mem_run, vm_charlie, vm_charlie_end - vm_charlie);
            break;
        case DEFENDER:
            memcpy(vm->mem_run, vm_eve, vm_eve_end - vm_eve);
            break;
        case NUMBEROFROLE:
            break;
    }

    /* time sampling storage */
    memset(vm->mem_mmio, 'A', VM_MEM_MMIO_SIZE);
    memset(vm->mem_measures, 'B', VM_MEM_MEASURES_SIZE);
    memset(vm->mem_own, 'C', VM_MEM_OWNPAGES_SIZE);
    memset(vm->mem_shared, 'D', VM_MEM_SHAREDPAGES_SIZE);

    /* own pages */
    uint _s = 0;
    while(_s < NB_OWN_PAGES*PAGESIZE) {
        _s += getrandom(vm->mem_own, (NB_OWN_PAGES*PAGESIZE) - _s, GRND_NONBLOCK);
    }
    printf("%s (%s) : fill %d pages with (own) random data\n", vm->vm_name, vm_role(vm->vm_role), NB_OWN_PAGES);

    /* shared pages */
    char *_c = vm->mem_shared;
    for(int i = 0; i< NB_SHARED_PAGES*PAGESIZE; i++){
        *(_c++) = shared_pages[i];
    }
    printf("%s (%s) : transferred %d shared pages with (common) random data\n", vm->vm_name, vm_role(vm->vm_role), NB_SHARED_PAGES);
#ifdef DEBUG
    /* print TSC frequency from guest pov */
    int tsc_freq = ioctl(vm->fd_vcpu, KVM_GET_TSC_KHZ, 0);
    printf("%s : TSC %d KHz\n", vm->vm_name, tsc_freq);
#endif

}
