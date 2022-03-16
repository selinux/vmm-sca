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
#include "VM.h"
#include "VMM-thread.h"
#include "ksm.h"

#include "../version.h"


pthread_barrier_t   barrier; // wait until all VMs are started



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

/** initialize shared pages to be copied into VM memory shared region
 *
 * @param mem memory pointer
 * @param size number of 4K pages
 * @return status
 */
int init_shared_pages(char** mem, const uint size){

    *mem = (char *) malloc(PAGESIZE*size);
    if (*mem == NULL) {perror("malloc error");return EXIT_FAILURE;}

    uint _s = 0;
    while(_s < PAGESIZE*size) {
        _s += getrandom(*mem, (PAGESIZE * size) - _s, GRND_NONBLOCK);
    }

    return EXIT_SUCCESS;
}

/** main VMM
 *
 * @param argc unused
 * @param argv unused
 * @return status
 */
int main(int argc, char ** argv)
{
	int vmm;                        // VMM fd
    vm vm[NUMBEROFROLE];            // VMs
    pthread_t tid[NUMBEROFROLE];    // VMs thread controller
    pthread_t tm;                   // thread time master
    void * iret[NUMBEROFROLE];      // threads return satus
    int vcpu_mmap_size;             // mmap size
    int err;

    printf("running %s\n", argv[argc-argc]);
    printf("----------------------------------------------------------------------------------------\n");
    printf("VMM side channel test bench : version (%s) - %s\n", __KERN_VERSION__, __BUILD_TIME__);
    printf("----------------------------------------------------------------------------------------\n\n");

    /* init KSM file descriptors */
    if(ksm_init() != 0) { goto end_ksm;}

    /* test KSM capability */
    if(!ksm_enabled()){ perror("KSM is not enabled (required) - please run make enable_ksm manually to enable it"); exit(EXIT_FAILURE); }

    /* initialize KVM common settings */
    printf("VMM : initialize KVM\n");
    vmm_init(&vmm);
    vcpu_mmap_size = ioctl(vmm, KVM_GET_VCPU_MMAP_SIZE, 0);          // const and used by all VM
    if (vcpu_mmap_size <= 0) { perror("KVM_GET_VCPU_MMAP_SIZE"); exit(1);}

    /* init VMM pages to be transferred to VMs */
    printf("VMM : filled %d pages with random data to be shared between VMs\n", NB_SHARED_PAGES);
    char * shared_page_1 = NULL;
    err = init_shared_pages(&shared_page_1, NB_SHARED_PAGES);
    if (err < 0 ) { perror("failed to init buffer"); exit(EXIT_FAILURE);}

#ifdef DEBUG
    int nb_slot;
    nb_slot =  ioctl(vmm, KVM_CHECK_EXTENSION, KVM_CAP_NR_MEMSLOTS);
//    if (nb_slot < 0) {
//		printf("failed to get slot count (%s)\n", strerror(errno));
//	} else {
//        printf("bb of memory slots available : %u\n", nb_slot);
//    }
    printf("run (slot %d) : 0x%x (0x%x)\nmmio (slot %d) : 0x%x (0x%x)\nmeasures (slot %d) : 0x%x (0x%x)\nown (slot %d) : 0x%x (0x%x)\nshared (slot %d) : 0x%x (0x%x)\n", MEM_SLOT_0, VM_MEM_RUN_ADDR, VM_MEM_RUN_SIZE,
           MEM_SLOT_1, VM_MEM_MMIO_ADDR, VM_MEM_MMIO_SIZE, MEM_SLOT_2, VM_MEM_MEASURES_ADDR, VM_MEM_MEASURES_SIZE,
           MEM_SLOT_3, VM_MEM_OWNPAGES_ADDR, VM_MEM_OWNPAGES_SIZE, MEM_SLOT_4, VM_MEM_SHAREDPAGES_ADDR, VM_MEM_SHAREDPAGES_SIZE);
#endif

    printf("VMM : initialize %d VMs and launch VMs thread\n\n", NUMBEROFROLE);
    /* pretty name VMs */
    strncpy(vm[0].vm_name, "VM alice",  256);
    strncpy(vm[1].vm_name, "VM charlie",  256);
    strncpy(vm[2].vm_name, "VM eve",  256);
    vm[0].vm_role = VICTIM;
    vm[1].vm_role = ATTACKER;
    vm[2].vm_role = DEFENDER;

    /* create a barrier */
    pthread_barrier_init (&barrier, NULL, NUMBEROFROLE+1);

    /* launch VMs threads */
    for( int i = 0; i < NUMBEROFROLE; i++) {
        vm[i].fd_vm = ioctl(vmm, KVM_CREATE_VM, 0);
        if (vm[i].fd_vm < 0) {
            perror("KVM_CREATE_VM");
            exit(1);
        }

        /* init VM, vcpu, registers and memory regions */
        vm_init((vm + i), vcpu_mmap_size, shared_page_1);

        /* launch VM */
        err = pthread_create( &tid[i], NULL, run_vm, (void*) (vm+i));
        if (err != 0 ) { perror("failed to launch VM thread"); exit(EXIT_FAILURE);}

    }
    err = pthread_create( &tm, NULL, time_master, (void*) (vm));
    if (err != 0 ) { perror("failed to launch time master thread"); exit(EXIT_FAILURE);}

    /* join */
    for( int i = 0; i < NUMBEROFROLE; i++) {
        pthread_join(tid[i], &iret[i]);
        printf("%s : exit %d\n", vm[i].vm_name, *(int*)iret[i]);
    }
    void *ret_tm = NULL;
    pthread_join(tm, ret_tm);
    printf("time master : exit %ld\n", (int64_t)((void *)ret_tm));

    printf("VMM : free shared pages buffers\n");
    free(shared_page_1);

 end_ksm:
    ksm_close();

    exit(0);
}
