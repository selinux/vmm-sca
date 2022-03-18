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


/** Initialize vcpu long mode (64bits)
 *
 * @param vm
 * @param sregs vcpu control registers
 */
static void init_long_mode(vm *vm, struct kvm_sregs *sregs)
{
    /* init virtual addressing tables */

    /* page map level 4 */
    uint64_t pml4_addr = 0x2000;
    uint64_t *pml4 = (void *)(vm->mem_run + pml4_addr);

    /* page directory page table */
    uint64_t pdpt_addr = 0x3000;
    uint64_t *pdpt = (void *)(vm->mem_run + pdpt_addr);

    /* page directory table */
    uint64_t pd_addr = 0x4000;
    uint64_t *pd = (void *)(vm->mem_run + pd_addr);

    /* link them */
    pml4[0] = PDE64_PRESENT | PDE64_RW | pdpt_addr;
    pdpt[0] = PDE64_PRESENT | PDE64_RW | pd_addr;

    /* page entry */
    for(uint64_t i = 0; i < 0x200; i++)
        pd[i] = PDE64_PRESENT | PDE64_RW | PDE64_PS | (i * PAGESIZE);

    /* init long mode (ref : https://en.wikipedia.org/wiki/Control_register#CR0) */
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;  // CR0 : flags
    sregs->cr3 = pml4_addr;                                                     // point to page table entry
    sregs->cr4 = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT | CR4_TSD | CR4_PCE;     // TSD : If set, RDTSC instruction can only be executed when in ring 0
    sregs->efer = EFER_LME | EFER_LMA;                                          // PCE : Performance-Monitoring Counter enable
                                                                                // PGE : If set, address translations (PDE or PTE records) may be shared between address spaces
}

/** Initialize segmentation
 *
 * @param sregs vcpu control registers
 */
static void init_segments(struct kvm_sregs *sregs)
{
    /* In 64 bit, segemntation is not used for addressing but only used to know if
     * something is in user mode (ring 3) or kernel mode(ring 0). This is later been used for paging also.
     *
     * Only CS and SS segments are define because all type of code access is checked through
     * CS register RPL and all data access is checked through SS register.
     *
     * src : https://nixhacker.com/segmentation-in-intel-64-bit/
     * */
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


/** Initialize vcpu registers
 *
 * @param regs vcpu registers
 */
static void init_registers(struct kvm_regs * regs){

    /* clear all FLAGS bits, except bit 1 which is always set. */
    memset(regs, 0, sizeof(struct kvm_regs));
    regs->rflags = 2;
    /* instruction pointer at bottom of run slot */
    regs->rip = 0;
    /* create stack at top of run slot */
    regs->rsp = STACK_END_ADDR;
}


/** Initialize a VM according to his role
 *
 * @param vm VM fd
 * @param mem_size
 * @param vcpu_mmap_size
 */
void vm_init(vm* vm, const int vcpu_mmap_size, const char * shared_pages)
{

//    if (ioctl(vm->fd_vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) { perror("KVM_SET_TSS_ADDR"); exit(1);}
//    if(ioctl(vm->fd_vm, KVM_CREATE_IRQCHIP, 0) < 0){ perror("KVM_CREATE_IRQCHIP"); exit(1);}
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-irq-line
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-create-pit2
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-irqfd            Allows setting an eventfd to directly trigger a guest interrupt.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-create-device    Creates an emulated device in the kernel.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-get-vcpu-events
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-enable-cap
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-set-identity-map-addr  This ioctl is required on Intel-based hosts.
//                                                                                        This ioctl defines the physical address of a one-page region in the guest physical address space.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-set-gsi-routing  Sets the GSI routing table entries, overwriting any previously set entries.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-get-sregs2       Reads special registers from the vcpu. This ioctl (when supported) replaces the KVM_GET_SREGS.


//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-get-clock
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-set-clock
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-set-tsc-khz      Specifies the tsc frequency for the virtual machine. The unit of the frequency is KHz.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-dirty-tlb        This must be called whenever userspace has changed an entry in the shared TLB, prior to calling KVM_RUN on the associated vcpu.
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-kvmclock-ctrl    This ioctl sets a flag accessible to the guest indicating that the specified vCPU has been paused by the host userspace.

//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#capabilities-that-can-be-enabled-on-vcpus
//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#kvm-cap-sw-tlb       Configures the virtual CPU’s TLB array, establishing a shared memory area between userspace and KVM.

//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#capabilities-that-can-be-enabled-on-vms

//    MMU info ? :
//    https://01.org/linuxgraphics/gfx-docs/drm/virt/kvm/mmu.html


    printf("%s (%s) : mmap memory slots\n", vm->vm_name, vm_role(vm->vm_role));
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

    printf("%s (%s) : init memory slots\n", vm->vm_name, vm_role(vm->vm_role));
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
    printf("%s (%s) : create vcpu\n", vm->vm_name, vm_role(vm->vm_role));
    vm->fd_vcpu = ioctl(vm->fd_vm, KVM_CREATE_VCPU, 0);
    if (vm->fd_vcpu < 0) { perror("KVM_CREATE_VCPU"); exit(1);}

    vm->vcpu.kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, vm->fd_vcpu, 0);
    if (vm->vcpu.kvm_run == MAP_FAILED) { perror("mmap kvm_run"); exit(1); }

    /* get special registers */
    if (ioctl(vm->fd_vcpu, KVM_GET_SREGS2, &vm->sregs) < 0) { perror("KVM_GET_SREGS");	exit(1);}

    /* init 64bits mode */
    printf("%s (%s) : init vcpu long mode (64-bit)\n", vm->vm_name, vm_role(vm->vm_role));
    init_long_mode(vm, &vm->sregs);
    printf("%s (%s) : init vcpu segments\n", vm->vm_name, vm_role(vm->vm_role));
    init_segments(&vm->sregs);
    if (ioctl(vm->fd_vcpu, KVM_SET_SREGS2, &vm->sregs) < 0) { perror("KVM_SET_SREGS"); exit(1);}

    /* init vcpu registers instruction pointer and stack pointer */
    init_registers(&vm->regs);
    printf("%s (%s) : init vcpu registers (rip,rsp)\n", vm->vm_name, vm_role(vm->vm_role));
    if (ioctl(vm->fd_vcpu, KVM_SET_REGS, &vm->regs) < 0) { perror("KVM_SET_REGS"); exit(1);}

    /* personalize VM role */
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

    /* init mmio and time sampling storage */
//    memset(vm->mem_mmio, 'A', VM_MEM_MMIO_SIZE);            // test
//    memset(vm->mem_mmio, 0, VM_MEM_MMIO_SIZE);
//    memset(vm->mem_measures, 0, VM_MEM_MEASURES_SIZE);
//    memset(vm->mem_mmio, 'A', VM_MEM_MMIO_SIZE);            // test
//    memset(vm->mem_measures, 'B', VM_MEM_MEASURES_SIZE);    // test
//    memset(vm->mem_own, 'C', VM_MEM_OWNPAGES_SIZE);         // test
//    memset(vm->mem_shared, 'D', VM_MEM_SHAREDPAGES_SIZE);   // test

#ifdef DEBUG
    translate_vm_addr(vm, (long long unsigned int) VM_MEM_MMIO_ADDR);
    translate_vm_addr(vm, (long long unsigned int) vm->mem_mmio);
    translate_vm_addr(vm, (long long unsigned int) VM_MEM_MEASURES_ADDR);
    translate_vm_addr(vm, (long long unsigned int) vm->mem_measures);
    translate_vm_addr(vm, (long long unsigned int) VM_MEM_OWNPAGES_ADDR);
    translate_vm_addr(vm, (long long unsigned int) vm->mem_own);
    translate_vm_addr(vm, (long long unsigned int) VM_MEM_SHAREDPAGES_ADDR);
    translate_vm_addr(vm, (long long unsigned int) vm->mem_shared);
#endif

    /* generate own pages */
    uint _s = 0;
    while(_s < NB_OWN_PAGES*PAGESIZE) {
        _s += getrandom(vm->mem_own, (NB_OWN_PAGES*PAGESIZE) - _s, GRND_NONBLOCK);
    }
    printf("%s (%s) : fill %d pages with (own) random data\n", vm->vm_name, vm_role(vm->vm_role), NB_OWN_PAGES);

    /* transfer shared pages */
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


/** translate a virtual address according to the vcpu’s current address translation mode.
 *
 * @param vm
 * @param addr
 */
void translate_vm_addr(vm* vm, const long long unsigned int addr) {
    struct kvm_translation trans_addr;
    trans_addr.linear_address = addr;
    if (ioctl(vm->fd_vcpu, KVM_TRANSLATE, &trans_addr) < 0) { perror("KVM_TRANSLATION_ADDR"); exit(1);}

    printf("linear_addr : 0x%llx\ntranslated_addr : 0x%llx\n", addr, trans_addr.physical_address);
    printf("valide :%hhx\nwritable : %hhx\nusermode : %hhx\n", trans_addr.valid, trans_addr.writeable, trans_addr.usermode);
}


/** pretty print VM name
 *
 * @param r VM role
 * @return VM role string
 */
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
