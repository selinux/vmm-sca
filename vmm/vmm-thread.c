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
#include "measures_io.h"


extern pthread_barrier_t   barrier; // wait until all VMs are started

void *run_vm(void * ptr)
{
    vm *vm = (void *)ptr;
    int ret = 1;
    command_s *cmd = vm->cmds;
    uint64_t nb_cmd = vm->nb_cmd;
    uint64_t nb_timestamp = 0;

    printf("%s : waiting...\n", vm->vm_name);
    pthread_barrier_wait (&barrier);
    printf("%s : running...\n", vm->vm_name);

	struct kvm_regs regs;
	uint64_t memval = 0;

//    https://www.kernel.org/doc/html/latest/virt/kvm/api.html#the-kvm-run-structure


	for (;;) {
		if (ioctl(vm->fd_vcpu, KVM_RUN, 0) < 0) { perror("KVM_RUN"); ret = -1; }
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
                if(handle_pmio(vm, cmd, &nb_timestamp) == 1){ cmd++; nb_cmd--;}
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
    char filename[512];
    char time_buff[32];
    time_t timer = time(NULL);
    strftime (time_buff, 32, "%Y.%m.%d-%H:%M:%S", localtime (&timer));
    snprintf(filename, 512, "../results/%s-%s.dat", vm->vm_name, time_buff);
    save_measures(filename, vm->mem_measures, nb_timestamp);

    pthread_exit((void*)&ret);
}


void *time_master(void * ptr)
{
    vm *vms = (void *)ptr;
    int ret = 1;

    printf("time master : waiting KSM memory deduplication...\n");

    ksm_wait(NB_SHARED_PAGES);

    printf("pages unshared : %d\n", ksm_ushared_pages());

    pthread_barrier_wait (&barrier);
    printf("time master : running...\n");

    for(int i = 0; i < 30; i++) {
        printf("pages shared : %d, sharing %d, unshared %d\n", ksm_shared_pages(), ksm_sharing_pages(), ksm_ushared_pages());
        usleep(100000);
    }

    translate_vm_addr(&vms[0], VM_MEM_PT_ADDR);

    ret = 0;
    pthread_exit((void*)&ret);
}