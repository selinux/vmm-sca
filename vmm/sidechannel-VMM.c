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
#include "../common/common.h"

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


extern unsigned char measures_start[], measures_end[];
extern const unsigned char vm_alice[], vm_alice_end[];
extern const unsigned char vm_charlie[], vm_charlie_end[];
extern const unsigned char vm_eve[], vm_eve_end[];


struct vcpu {
    int fd;
    struct kvm_run *kvm_run;
};

typedef struct _vm {
    char vm_name[256];
    int fd_vm;
    int fd_vcpu;
    struct vcpu vcpu;
    char *mem;
    ROLE vm_role;
    pthread_t runner;
} vm;



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

void vmm_init(int *vmm){

    int api_ver;
    *vmm = open("/dev/kvm", O_RDWR);
	if (vmm < 0) {
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

void vm_init(vm* vm, size_t mem_size, int vcpu_mmap_size)
{
	struct kvm_userspace_memory_region memreg;
	struct kvm_sregs sregs;
	struct kvm_regs regs;

    printf("init VM %s (%d) 64-bit mode\n", vm->vm_name, vm->vm_role);

    if (ioctl(vm->fd_vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
                perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

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

    /* */
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
    }

    /* time sampling storage */
    memset(vm->mem+VM_SAMPLES_ADDR, 0, sizeof(uint64_t)*NB_SAMPLES);

}

void *run_vm(void * ptr)
{
    vm *vm = (void *)ptr;
    int ret = 1;

    printf("running %s...\n\n", vm->vm_name);

	struct kvm_regs regs;
	uint64_t memval = 0;

	for (;;) {
		if (ioctl(vm->fd_vcpu, KVM_RUN, 0) < 0) {
			perror("KVM_RUN");
			ret = -1;
		}

        switch (vm->vcpu.kvm_run->exit_reason) {
            case KVM_EXIT_HLT:
                ret = 0;
                goto check;

            case KVM_EXIT_IO:
                if (vm->vcpu.kvm_run->io.direction == KVM_EXIT_IO_OUT
                       && vm->vcpu.kvm_run->io.port == 0xE9) {
                    char *p = (char *)vm->vcpu.kvm_run;
        		    fwrite(p + vm->vcpu.kvm_run->io.data_offset,
    			      vm->vcpu.kvm_run->io.size, 1, stdout);
	        	    fflush(stdout);
                    continue;}
                if (vm->vcpu.kvm_run->io.direction == KVM_EXIT_IO_OUT
                       && vm->vcpu.kvm_run->io.port == 0xBE) {
                    printf("%s - dump measurement from VMM (direct VM memory access)\n", vm->vm_name);

                    unsigned long long *m = (unsigned long long *)vm->mem+VMM_SAMPLES_ADDR;
                    for(int i=0; i< NB_SAMPLES; i++){
                        printf("%s (%04d) : %llu (Δ %llu)\n", vm->vm_name, i, *m, (*m-*(m-1)));
                        m++;
                   }
                   continue;}

            /* fall through */
            default:
                fprintf(stderr,	"Got exit_reason %d,"
                                   " expected KVM_EXIT_HLT (%d)\n",
        			vm->vcpu.kvm_run->exit_reason, KVM_EXIT_HLT);
		        ret = vm->vcpu.kvm_run->exit_reason;
        }
	}

 check:
	if (ioctl(vm->fd_vcpu, KVM_GET_REGS, &regs) < 0) {
		perror("KVM_GET_REGS");
		ret = -1;
	}

	if (regs.rax != 42) {
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		ret = -1;
	}

    memcpy(&memval, &vm->mem[0x400], sizeof(uint64_t));
	if (memval != 42) {
		printf("Wrong result: memory at 0x400 is %lld\n",
		       (unsigned long long)memval);
        ret = -1;
	}

    pthread_exit((void*)&ret);
}


int main(int argc, char **argv)
{
	int vmm;
    vm vm[NUMBEROFROLE];
    pthread_t tid[NUMBEROFROLE];
    void * iret[NUMBEROFROLE];
    int vcpu_mmap_size;

    vmm_init(&vmm);
    vcpu_mmap_size = ioctl(vmm, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        exit(1);
    }

    strncpy(vm[0].vm_name, "my alice",  256);
    strncpy(vm[1].vm_name, "my charlie",  256);
    strncpy(vm[2].vm_name, "my eve",  256);
    vm[0].vm_role = VICTIM;
    vm[1].vm_role = ATTACKER;
    vm[2].vm_role = DEFENDER;

    for( int i = 0; i < NUMBEROFROLE; i++) {
        vm[i].fd_vm = ioctl(vmm, KVM_CREATE_VM, 0);
        if (vm[i].fd_vm < 0) {
            perror("KVM_CREATE_VM");
            exit(1);
        }

        vm_init((vm + i), 0x200000, vcpu_mmap_size);
        int iret = pthread_create( &tid[i], NULL, run_vm, (void*) (vm+i));
    }

    for( int i = 0; i < NUMBEROFROLE; i++) {
        pthread_join(tid[i], &iret[i]);
        printf("VM %s - exit %d\n", vm[i].vm_name, *(int*)iret[i]);
    }


    exit(0);
}
