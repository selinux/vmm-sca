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
#include "VMM-thread.h"

#include "ksm.h"


extern pthread_barrier_t   barrier; // wait until all VMs are started

void *run_vm(void * ptr)
{
    vm *vm = (void *)ptr;
    int ret = 1;

    printf("%s : waiting to start...\n", vm->vm_name);
    pthread_barrier_wait (&barrier);
    printf("%s : running...\n", vm->vm_name);

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


void *time_master(void * ptr)
{
    vm *vms = (void *)ptr;
    int ret = 1;

    printf("time master : waiting to start...\n");
    pthread_barrier_wait (&barrier);
    printf("time master : running...\n");

    printf("KSM shared pages : %d\n",ksm_shared_pages());

    ioctl(vms[0].fd_vcpu, KVM_INTERRUPT, 20);
    usleep(1000000);
    ret = 0;
    pthread_exit((void*)&ret);
}