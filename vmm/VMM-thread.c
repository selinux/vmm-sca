/*
 * =====================================================================================
 *
 *       Filename:  VMM-threads.c
 *
 *    Description:  threads VM and controller
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
#include <unistd.h>
#include <sys/ioctl.h>
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

//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#the-kvm-run-structure


	for (;;) {
		if (ioctl(vm->fd_vcpu, KVM_RUN, 0) < 0) { perror("KVM_RUN"); ret = -1; }
        printf("%s : exit reason %d\n", vm->vm_name, vm->vcpu.kvm_run->exit_reason);
        switch (vm->vcpu.kvm_run->exit_reason) {
            case KVM_EXIT_HLT:
                ret = 0;
                goto check;

            case KVM_EXIT_MMIO:
                if(vm->vcpu.kvm_run->mmio.is_write)
                    printf("at least !!\n");
                continue;

            case KVM_EXIT_IO:
                if (vm->vcpu.kvm_run->io.direction == KVM_EXIT_IO_OUT
                       && vm->vcpu.kvm_run->io.port == 0xE9) {
                    char *p = (char *)vm->vcpu.kvm_run;
        		    fwrite(p + vm->vcpu.kvm_run->io.data_offset,
    			      vm->vcpu.kvm_run->io.size, 1, stdout);
	        	    fflush(stdout);
                    continue;}
                if (vm->vcpu.kvm_run->io.direction == KVM_EXIT_IO_OUT
                       && vm->vcpu.kvm_run->io.port == PMIO_PRINT_MEASURES) {
                    printf("%s - dump measurement from VMM (direct VM memory access)\n", vm->vm_name);

                    long long unsigned *m = (long long unsigned *)vm->mem_run+(0x10000/8);
//                    char *m = (char *)vm->mem_run+0x10000;
                    for(int i=0; i< NB_SAMPLES; i++){
                        unsigned long long dm = *(unsigned long long *)m-*(unsigned long long *)(m-1);
                        printf("%s (%04d) : %llu (Δ %llu)\n", vm->vm_name, i, *(unsigned long long *)m, dm);
//                        printf("%s (%04d) : %llu\n", vm->vm_name, i, *(unsigned long long *)m);
//                        printf("%s (%04d) : %c\n", vm->vm_name, i, *m);
                        m++;
                   }
                }
                if(vm->vcpu.kvm_run->io.count)
                    printf("at least !!  %u \n", vm->vcpu.kvm_run->io.count);

                continue;

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

    memcpy(&memval, (void *)vm->mem_run+VM_EXIT_RETURN_CODE_ADDR, sizeof(uint64_t));
    if (memval != VM_EXIT_RETURN_CODE) {
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

    printf("time master : waiting KSM memory deduplication...\n");

    ksm_wait(NB_SHARED_PAGES);

    pthread_barrier_wait (&barrier);
    printf("time master : running...\n");

    usleep(100000);
    *(uint64_t *)(vms[ATTACKER].mem_run+0x500) = 1;   // test unlock VM victim

//    ioctl(vms[VICTIM].fd_vcpu, KVM_INTERRUPT, 20);

    ret = 0;
    pthread_exit((void*)&ret);
}