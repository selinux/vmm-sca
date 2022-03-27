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
#include <assert.h>

#include "../common/common.h"
#include "vm.h"






uint64_t gdt_create_descriptor(uint32_t base, uint32_t limit, uint16_t flag)
{
    uint64_t descriptor;

    // Create the high 32 bit segment
    descriptor  =  limit       & 0x000F0000;         // set limit bits 19:16
    descriptor |= (flag <<  8) & 0x00F0FF00;         // set type, p, dpl, s, g, d/b, l and avl fields
    descriptor |= (base >> 16) & 0x000000FF;         // set base bits 23:16
    descriptor |=  base        & 0xFF000000;         // set base bits 31:24

    // Shift by 32 to allow for low part of segment
    descriptor <<= 32;

    // Create the low 32 bit segment
    descriptor |= base  << 16;                       // set base bits 15:0
    descriptor |= limit  & 0x0000FFFF;               // set limit bits 15:0

    return descriptor;
}

/** Initialize paging PML4 pages tables in VM memory
 *
 * @param vm
 */
static void init_pages_tables(vm* vm){

    /* page table ref : https://wiki.osdev.org/Paging */
    printf("%s (%s) : init memory paging  (64-bit)\n", vm->vm_name, vm_role(vm->vm_role));

    /* page map level 4 */
    uint64_t *pml4 = (uint64_t *)vm->mem_pages_tables;

    /* page directory page table (second level, 1 is enough) */
    uint64_t *pdpt = (uint64_t *)(vm->mem_pages_tables + 0x1000);

    /* page directory table (512*2Mb = 1Gb) */
    uint64_t *pd = (uint64_t *)(vm->mem_pages_tables + 0x2000);
    uint64_t *pte = (uint64_t *)(vm->mem_pages_tables+0x3000+(NB_PT_PD_PAGES*PAGESIZE));

    /* PML4 entry (1 PDPT) */
    pml4[0] = PDE64_PRESENT | PDE64_RW | (VM_MEM_PT_ADDR+0x1000LL);
    /* PDPT entries (4 PD) */
    for(uint64_t i = 0; i < NB_PT_PD_PAGES; i++) {
        pdpt[i] = PDE64_PRESENT | PDE64_RW | (VM_MEM_PT_ADDR+0x2000LL+(i*PAGESIZE));
    }
    /* PD entries (4*512 PT) */
    for(uint64_t i = 0; i < NB_PT_PD_PAGES*512; i++) {
        pd[i] = PDE64_PRESENT | PDE64_RW | (VM_MEM_PT_ADDR+0x3000LL+(NB_PT_PD_PAGES*PAGESIZE)+(i*PAGESIZE));
    }
    /* PT entries identity mapping */
    for(uint64_t i = 0; i < NB_PT_PD_PAGES*512*512; i++){
        pte[i] = PDE64_PRESENT | PDE64_RW | (i * PAGESIZE);
    }
}


/** Initialize vcpu long mode (64bits) with paging and PML4 in CR3
 *
 * @param vm
 * @param sregs vcpu control registers
 */
static void init_long_mode(struct kvm_sregs *sregs)
{

    /* init long mode (ref : https://en.wikipedia.org/wiki/Control_register#CR0) */

    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM |CR0_PG;   // CR0 : flags
    sregs->cr3 = VM_MEM_PT_ADDR;                                                // point to pml4 table
    sregs->cr4 = CR4_PAE | CR4_PGE | CR4_PCE | CR4_OSFXSR | CR4_OSXMMEXCPT;     // TSD : If set, RDTSC instruction can only be executed when in ring 0
    sregs->efer = EFER_LME | EFER_LMA;                                          // PCE : Performance-Monitoring Counter enable
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
            .avl = 0,
    };

    sregs->cs = seg;

    seg.type = 3;                /* Data: read/write, accessed */
    seg.selector = 2 << 3;
    sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}


///** Initialize GDT and IDT without handler VM will attach them later
// *
// * @param vm
// * @param sregs
// */
//static void init_gdt_idt(vm * vm, struct kvm_sregs *sregs){
//    struct kvm_dtable _gdt = {.base=VM_GDT_ADDR, .limit= VM_GDT_SIZE-1};
//    struct kvm_dtable _idt = {.base = VM_IDT_ADDR, .limit = VM_IDT_SIZE-1};
//    sregs->gdt = _gdt;
//    sregs->idt = _idt;
//
//    /* populate GDT with two entries */
//    uint64_t *gdt_table = vm->mem_run + VM_GDT_ADDR;
//    gdt_table[0] = gdt_create_descriptor(0, 0, 0);
//    gdt_table[1] = gdt_create_descriptor(0, 0x000FFFFF, (GDT_CODE_PL0));
//    gdt_table[2] = gdt_create_descriptor(0, 0x000FFFFF, (GDT_DATA_PL0));
//
//    /* populate IDT without functions handler (guest will do) */
////    idt64_entry_t *idt_table = vm->mem_run + VM_IDT_ADDR;
////    for (int i = 0; i < 20; i++) {
////        idt_table[i] = idt_build_entry( 8, 0, TYPE_INTERRUPT_GATE, DPL_KERNEL);
////    }
////    for (int i = 0; i < 16; i++) {
////		idt_table[32+i] = idt_build_entry( 8, 0, TYPE_INTERRUPT_GATE, DPL_KERNEL);
////	}
//
//}


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
    regs->rsp = STACK_TOP_ADDR;
}


/** Initialize vcpu
 *
 * @param vm
 */
static void init_vcpu(vm * vm){

    vm->fd_vcpu = ioctl(vm->fd_vm, KVM_CREATE_VCPU, 0);
    if (vm->fd_vcpu < 0) { perror("KVM_CREATE_VCPU"); exit(1);}
    vm->vcpu.kvm_run = mmap(NULL, (size_t)vm->vcpu_mmap_size, PROT_READ | PROT_WRITE,MAP_SHARED, vm->fd_vcpu, 0);
    if (vm->vcpu.kvm_run == MAP_FAILED) { perror("mmap kvm_run"); exit(1); }
}


/** init a memory region
 *
 * @param vm vm fd
 * @param mem mem addr
 * @param slot
 * @param flags
 * @param guest_pa guest physical address
 * @param size
 * @return 0 or error
 */
static int init_memory_slot(vm* vm, void* mem, MEM_REGION slot, __u32 flags, __u64 guest_pa, __u64 size){

#ifdef DEBUG
    printf("%s (%s) : init memory slots (%d) - flag %d, gla 0x%llx, size 0x%llx - hva %p\n", vm->vm_name, vm_role(vm->vm_role), slot, flags, guest_pa, size, mem);
#endif
   	struct kvm_userspace_memory_region memreg;
    memreg.slot = slot;
    memreg.flags = flags;
    memreg.guest_phys_addr = guest_pa;
    memreg.memory_size = size;
    memreg.userspace_addr = (unsigned long)mem;

    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) { perror("KVM_SET_USER_MEMORY_REGION"); return -1;}

    return 0;
}

/** Initialize a VM according to his role
 *
 * @param vm VM fd
 * @param mem_size
 */
void vm_init(vm* vm, const char * shared_pages)
{

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


//    if (ioctl(vm->fd_vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) { perror("KVM_SET_TSS_ADDR"); exit(1);}
//    if(ioctl(vm->fd_vm, KVM_CREATE_IRQCHIP, 0) < 0){ perror("KVM_CREATE_IRQCHIP"); exit(1);}


    /* VM code,data,stack */
    vm->mem_run = mmap(NULL, VM_MEM_RUN_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_run == MAP_FAILED) { perror("mmap run region"); exit(1);}
    /* load VM code */
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

    /* MMIO */
    vm->mem_mmio = mmap(NULL, VM_MEM_MMIO_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_mmio == MAP_FAILED) { perror("mmap mmio region"); exit(1);}
    memset(vm->mem_mmio, 0, VM_MEM_MMIO_SIZE);

    /* Pages tables */
    vm->mem_pages_tables = mmap(NULL, VM_MEM_PT_SIZE, PROT_READ | PROT_WRITE,
                                MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_pages_tables == MAP_FAILED) { perror("mmap pages tables region"); exit(1);}
    memset(vm->mem_pages_tables, 0, VM_MEM_PT_SIZE);
    /* init 4-level paging  */
    init_pages_tables(vm);

    /* time measurement region */
    vm->mem_measures = mmap(NULL, VM_MEM_MEASURES_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_measures == MAP_FAILED) { perror("mmap measures region"); exit(1);}
    memset(vm->mem_measures, 0, VM_MEM_MEASURES_SIZE);

    /* VM own pages (not shared) for read/write access tests */
    vm->mem_own = mmap(NULL, VM_MEM_OWNPAGES_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_own == MAP_FAILED) { perror("mmap own pages region"); exit(1);}
    /* generate own pages data */
    uint _s = 0;
    while(_s < NB_OWN_PAGES*PAGESIZE) {
        _s += getrandom(vm->mem_own, (NB_OWN_PAGES*PAGESIZE) - _s, GRND_NONBLOCK);
    }
    printf("%s (%s) : fill %lld pages with (own) random data\n", vm->vm_name, vm_role(vm->vm_role), NB_OWN_PAGES);

    /* VMs shared pages for read access, flush, COW,... tests */
    /* shared pages */
    vm->mem_shared = mmap(NULL, VM_MEM_SHAREDPAGES_SIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem_shared == MAP_FAILED) { perror("mmap shared pages region"); exit(1);}
    /* transfer shared pages data */
    memcpy(vm->mem_shared, shared_pages, NB_SHARED_PAGES*PAGESIZE);
    printf("%s (%s) : transferred %lld shared pages with (common) random data\n", vm->vm_name, vm_role(vm->vm_role), NB_SHARED_PAGES);

    /* initialize and attach memory region to VM */
    madvise(vm->mem_run, VM_MEM_RUN_SIZE, MADV_UNMERGEABLE);
    if (init_memory_slot(vm, vm->mem_run, MEM_SLOT_0, 0, VM_MEM_RUN_ADDR, VM_MEM_RUN_SIZE) != 0){ perror("VM init memory region run KVM_SET_USER_MEMORY_REGION");}

    madvise(vm->mem_mmio, VM_MEM_MMIO_SIZE, MADV_UNMERGEABLE);
    if (init_memory_slot(vm, vm->mem_mmio, MEM_SLOT_1, KVM_MEM_READONLY, VM_MEM_MMIO_ADDR, VM_MEM_MMIO_SIZE) != 0){ perror("VM init memory region MMIO KVM_SET_USER_MEMORY_REGION");}

    madvise(vm->mem_pages_tables, VM_MEM_PT_SIZE, MADV_UNMERGEABLE);
    if (init_memory_slot(vm, vm->mem_pages_tables, MEM_SLOT_2, 0, VM_MEM_PT_ADDR, VM_MEM_PT_SIZE) != 0){ perror("VM init memory region pages tables KVM_SET_USER_MEMORY_REGION");}

    madvise(vm->mem_measures, VM_MEM_MEASURES_SIZE, MADV_UNMERGEABLE);
    if (init_memory_slot(vm, vm->mem_measures, MEM_SLOT_3, 0, VM_MEM_MEASURES_ADDR, VM_MEM_MEASURES_SIZE) != 0){ perror("VM init memory region measures KVM_SET_USER_MEMORY_REGION");}

    madvise(vm->mem_own, VM_MEM_OWNPAGES_SIZE, MADV_UNMERGEABLE);      // avoid merging
    if (init_memory_slot(vm, vm->mem_own, MEM_SLOT_4, 0, VM_MEM_OWNPAGES_ADDR, VM_MEM_OWNPAGES_SIZE) != 0){ perror("VM init memory region own pages KVM_SET_USER_MEMORY_REGION");}

    madvise(vm->mem_shared, VM_MEM_SHAREDPAGES_SIZE, MADV_MERGEABLE);  // advise merge
    if (init_memory_slot(vm, vm->mem_shared, MEM_SLOT_5, 0, VM_MEM_SHAREDPAGES_ADDR, VM_MEM_SHAREDPAGES_SIZE) != 0){ perror("VM init memory region own pages KVM_SET_USER_MEMORY_REGION");}
    printf("%s (%s) : mmap memory slots\n", vm->vm_name, vm_role(vm->vm_role));

    /* init vcpu */
    init_vcpu(vm);

    /* get special registers */
    struct kvm_sregs sregs;
    if (ioctl(vm->fd_vcpu, KVM_GET_SREGS2, &sregs) < 0) { perror("KVM_GET_SREGS");	exit(1);}

    /* init 64bits long mode */
    init_long_mode(&sregs);
    printf("%s (%s) : init vcpu long mode (64-bit) with paging (4Gb identity mapped)\n", vm->vm_name, vm_role(vm->vm_role));

    /* init segments */
    init_segments(&sregs);
    printf("%s (%s) : init vcpu segments\n", vm->vm_name, vm_role(vm->vm_role));

    /* init IDT GDT */
//    init_gdt_idt(vm, &sregs);

    /* set special registers */
    if (ioctl(vm->fd_vcpu, KVM_SET_SREGS, &sregs) < 0) { perror("KVM_SET_SREGS"); exit(1);}

    /* init vcpu initial state (instruction and stack pointer) */
    struct kvm_regs regs;
    init_registers(&regs);
    if (ioctl(vm->fd_vcpu, KVM_SET_REGS, &regs) < 0) { perror("KVM_SET_REGS"); exit(1);}
    printf("%s (%s) : init vcpu registers (rip,rsp)\n", vm->vm_name, vm_role(vm->vm_role));

    struct kvm_mp_state mp_state;
    if (ioctl(vm->fd_vcpu, KVM_GET_MP_STATE, &mp_state) < 0) { perror("KVM_GET_MP_STATE"); exit(1);}


    assert(VM_MEM_MEASURES_ADDR == translate_vm_addr(vm, (uint64_t) VM_MEM_MEASURES_ADDR).physical_address);
    assert(VM_MEM_OWNPAGES_ADDR == translate_vm_addr(vm, (uint64_t) VM_MEM_OWNPAGES_ADDR).physical_address);
    assert(VM_MEM_SHAREDPAGES_ADDR == translate_vm_addr(vm, (uint64_t) VM_MEM_SHAREDPAGES_ADDR).physical_address);
    assert(1 != translate_vm_addr(vm, 0x100000000LL).valid);   // memory over 4Gb is not valid


    /* print TSC frequency from guest pov */
    int tsc_freq = ioctl(vm->fd_vcpu, KVM_GET_TSC_KHZ, 0);
    printf("%s : TSC %d MHz\n", vm->vm_name, tsc_freq/1000);
}


/** translate a virtual address according to the vcpu’s current address translation mode.
 *
 * @param vm
 * @param addr
 * @return kvm_translation
 */
struct kvm_translation translate_vm_addr(vm* vm, const long long unsigned int addr) {
    struct kvm_translation trans_addr;
    trans_addr.linear_address = addr;
    if (ioctl(vm->fd_vcpu, KVM_TRANSLATE, &trans_addr) < 0) { perror("KVM_TRANSLATION_ADDR"); exit(1);}
#ifdef DEBUG
    printf("linear_addr : 0x%llx\ntranslated_addr : 0x%llx\n", addr, trans_addr.physical_address);
    printf("valid\t : %hhx\nwritable : %hhx\nusermode : %hhx\n", trans_addr.valid, trans_addr.writeable, trans_addr.usermode);
#endif
    return trans_addr;
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
        default:
            return "";
    }
}
