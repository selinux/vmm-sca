/*
 * =====================================================================================
 *
 *       Filename:  VMM.c
 *
 *    Description:  main executable
 *
 *        Version:  1.0
 *        Created:  08/03/22 10:18:54 AM CET
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <pthread.h>
#include <sys/random.h>
#include "../common/common.h"

#include "VMM.h"
#include "VMM-thread.h"
#include "../version.h"


pthread_barrier_t   barrier; // wait until all VMs are started

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
            .type = 11, /* Code: execute, read, accessed */
            .dpl = 0,
            .db = 0,
            .s = 1, /* Code/data */
            .l = 1,
            .g = 1, /* 4KB granularity */
    };

    sregs->cs = seg;

    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 2 << 3;
    sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void setup_long_mode(vm *vm, struct kvm_sregs *sregs)
{
    uint64_t pml4_addr = 0x2000;
    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = 0x3000;
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    uint64_t pd_addr = 0x4000;
    uint64_t *pd = (void *)(vm->mem + pd_addr);

    pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
    pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
    pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

    sregs->cr3 = pml4_addr;
    sregs->cr4 = CR4_PAE;
    sregs->cr0
            = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs->efer = EFER_LME | EFER_LMA;

    setup_64bit_code_segment(sregs);
}

/** initialize KVM
 *
 * @param vmm fd
 */
void vmm_init(int *vmm){

    int api_ver;
	if ((*vmm = open("/dev/kvm", O_RDWR)) < 0) {
		perror("open /dev/kvm");
		exit(1);
	}
    api_ver = ioctl(*vmm, KVM_GET_API_VERSION, 0);
    if (api_ver < 0) {
        perror("KVM_GET_API_VERSION");
        exit(1);
    }

    if (api_ver != KVM_API_VERSION) {
        fprintf(stderr, "Got KVM api version %d, expected %d\n",
                api_ver, KVM_API_VERSION);
        exit(1);
    }
}

/** Initialize a VM according to his role
 *
 * @param vm VM fd
 * @param mem_size
 * @param vcpu_mmap_size
 */
void vm_init(vm* vm, size_t mem_size, int vcpu_mmap_size)
{
	struct kvm_userspace_memory_region memreg;
	struct kvm_sregs sregs;
	struct kvm_regs regs;

    printf("%s (%s) : init 64-bit mode\n", vm->vm_name, vm_role(vm->vm_role));

    if (ioctl(vm->fd_vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
        perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

//    if(ioctl(vm->fd_vm, KVM_CREATE_IRQCHIP, 0) < 0){
//        perror("KVM_CREATE_IRQCHIP");
//        exit(1);
//    }

    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (vm->mem == MAP_FAILED) {
		perror("mmap mem");
		exit(1);
	}

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = 0;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
    if (ioctl(vm->fd_vm, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
		perror("KVM_SET_USER_MEMORY_REGION");
                exit(1);
	}

    /* init vcpu */
    vm->fd_vcpu = ioctl(vm->fd_vm, KVM_CREATE_VCPU, 0);
    if (vm->fd_vcpu < 0) {
        perror("KVM_CREATE_VCPU");
        exit(1);
    }

    vm->vcpu.kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, vm->fd_vcpu, 0);
    if (vm->vcpu.kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        exit(1);
    }

    /* set special register */
    if (ioctl(vm->fd_vcpu, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

    setup_long_mode(vm, &sregs);

    if (ioctl(vm->fd_vcpu, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }
   	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	/* Create stack at top of 2 MB page.img and grow down. */
	regs.rsp = 2 << 20;

	if (ioctl(vm->fd_vcpu, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

    switch(vm->vm_role){
        case VICTIM:
        	memcpy(vm->mem, vm_alice, vm_alice_end-vm_alice);
            break;
        case ATTACKER:
            memcpy(vm->mem, vm_charlie, vm_charlie_end-vm_charlie);
            break;
        case DEFENDER:
            memcpy(vm->mem, vm_eve, vm_eve_end-vm_eve);
            break;
        case NUMBEROFROLE:
            break;
    }

    /* time sampling storage */
    memset(vm->mem+VM_SAMPLES_ADDR, 0, sizeof(uint64_t)*NB_SAMPLES);

    int tsc_freq = ioctl(vm->fd_vcpu, KVM_GET_TSC_KHZ, 0);
    printf("%s : TSC %d KHz\n", vm->vm_name, tsc_freq);


}

/** initialize pages to be copied into VM mem
 *
 * @param mem memory pointer
 * @param size number of 4K pages
 * @return status
 */
int init_pages(char** mem, const uint size){
    *mem = (char *) malloc(PAGESIZE*size);
    if (*mem == NULL) {perror("malloc error");return EXIT_FAILURE;}

    uint _s = 0;
    while(_s < PAGESIZE*size) {
        _s += getrandom(*mem, (PAGESIZE * size) - _s, GRND_NONBLOCK);
    }
    printf("\nFill %d pages with random data\n", size);

    return EXIT_SUCCESS;
}

/** main VMM
 *
 * @param argc unused
 * @param argv unused
 * @return status
 */
int main()
{
	int vmm;                        // VMM fd
    vm vm[NUMBEROFROLE];            // VMs
    pthread_t tid[NUMBEROFROLE];    // VMs thread controller
    pthread_t tm;                   // thread time master
    void * iret[NUMBEROFROLE];      // threads return satus
    int vcpu_mmap_size;             // mmap size
    int err = 0;

    /* init VMM pages to be transferred to VMs */
    char * page_group1 = NULL;
    char * page_group2 = NULL;
    err = init_pages(&page_group1, 100);
  	if (err < 0 ) {
		perror("failed to init buffer");
        exit(EXIT_FAILURE);
	}
    err = init_pages(&page_group2, 10);
    if (err < 0 ) {
        perror("failed to init buffer");
        exit(EXIT_FAILURE);
    }

    /* initialize KVM common settings */
    vmm_init(&vmm);
    vcpu_mmap_size = ioctl(vmm, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        exit(1);
    }

    /* pretty name VMs */
    strncpy(vm[0].vm_name, "my alice",  256);
    strncpy(vm[1].vm_name, "my charlie",  256);
    strncpy(vm[2].vm_name, "my eve",  256);
    vm[0].vm_role = VICTIM;
    vm[1].vm_role = ATTACKER;
    vm[2].vm_role = DEFENDER;

    /*
     * launch VMs threads
     * */
    // create a barrier
    pthread_barrier_init (&barrier, NULL, NUMBEROFROLE+1);

    for( int i = 0; i < NUMBEROFROLE; i++) {
        vm[i].fd_vm = ioctl(vmm, KVM_CREATE_VM, 0);
        if (vm[i].fd_vm < 0) {
            perror("KVM_CREATE_VM");
            exit(1);
        }

        // init VM cpu and mem
        vm_init((vm + i), 0x200000, vcpu_mmap_size);
        // launch VM
        err = pthread_create( &tid[i], NULL, run_vm, (void*) (vm+i));
        if (err != 0 ) {
            perror("failed to launch VM thread");
            exit(EXIT_FAILURE);
        }
    }
    err = pthread_create( &tm, NULL, time_master, (void*) (vm));
    if (err != 0 ) {
        perror("failed to launch time master thread");
        exit(EXIT_FAILURE);
    }

    /* join */
    for( int i = 0; i < NUMBEROFROLE; i++) {
        pthread_join(tid[i], &iret[i]);
        printf("VM %s - exit %d\n", vm[i].vm_name, *(int*)iret[i]);
    }
    void *ret_tm = NULL;
    pthread_join(tm, ret_tm);
    printf("time master - exit %ld\n", (int64_t)((void *)ret_tm));

    printf("free VMM buffers\n");
    free(page_group2);
    free(page_group1);

    exit(0);
}
