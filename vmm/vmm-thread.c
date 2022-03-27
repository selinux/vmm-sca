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
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/kvm.h>
#include <pthread.h>
#include <sys/random.h>
#include "../common/common.h"
#include "vmm-thread.h"

#include "ksm.h"
#include "vm.h"
#include "vmm-handlers.h"


extern pthread_barrier_t   barrier; // wait until all VMs are started

void *run_vm(void * ptr)
{
    vm *vm = (void *)ptr;
    int ret = 1;
    command_s *cmd = vm->cmds;
    uint64_t nb_cmd = vm->nb_cmd;

    printf("%s : waiting...\n", vm->vm_name);
    pthread_barrier_wait (&barrier);
    printf("%s : running...\n", vm->vm_name);

	struct kvm_regs regs;
	uint64_t memval = 0;

//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#the-kvm-run-structure


	for (;;) {
		if (ioctl(vm->fd_vcpu, KVM_RUN, 0) < 0) { perror("KVM_RUN"); ret = -1; }
//        printf("%s : exit reason %d\n", vm->vm_name, vm->vcpu.kvm_run->exit_reason);
        switch (vm->vcpu.kvm_run->exit_reason) {
            case KVM_EXIT_HLT:
                ret = 0;
                goto check;
            case KVM_EXIT_MMIO:
                handle_mmio(vm);
                continue;
            case KVM_EXIT_SHUTDOWN:
                ret = 0;
                goto check;
            case KVM_EXIT_IO:
                if(handle_pmio(vm, cmd) == 1){ cmd++; nb_cmd--;}
                if(nb_cmd == 0) { usleep(500); goto check;}
                continue;
            /* fall through */
            default:
                fprintf(stderr,	"Got exit_reason %d, expected KVM_EXIT_HLT (%d)\n", vm->vcpu.kvm_run->exit_reason, KVM_EXIT_HLT);
                ret = (int)vm->vcpu.kvm_run->exit_reason;
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

    translate_vm_addr(&vms[0], VM_MEM_PT_ADDR);
//    usleep(100000);
//    *(uint64_t *)(vms[ATTACKER].mem_run+PRIMITIVE_CMD_ADDR) = PRIMITIVE_EXIT;   // test unlock VM victim

//    ioctl(vms[VICTIM].fd_vcpu, KVM_INTERRUPT, 20);
//    ioctl(vms[VICTIM].fd_vcpu, KVM_GET_TSC_KHZ, 20);

//    *(uint64_t *)(vms[VICTIM].mem_run+PRIMITIVE_CMD_ADDR) = PRIMITIVE_MEASURE;
//    *(uint64_t *)(vms[VICTIM].mem_run+PRIMITIVE_CMD_ADDR) = PRIMITIVE_PRINT_MEASURES;
//    *(uint64_t *)(vms[VICTIM].mem_run+PRIMITIVE_CMD_ADDR) = PRIMITIVE_EXIT;
//    usleep(1000000);
    ret = 0;
    pthread_exit((void*)&ret);
}