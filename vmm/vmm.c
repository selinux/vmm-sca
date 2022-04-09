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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/kvm.h>
#include <pthread.h>
#include <sys/random.h>
#include <argp.h>
#include "../common/common.h"

#include "vmm.h"
#include "vm.h"
#include "vmm-thread.h"
#include "ksm.h"
#include "measures_io.h"

#include "../version.h"


/* Program documentation */
static char doc[512]; // = "Argp example #3 -- a program with options and arguments using argp";

/* A description of the arguments */
static char args_doc[] = "INPUTFILE OUTPUTFILE";

/**
 * The options available
 */
static struct argp_option options[] = {
  {"verbose",  'v', 0,      0,  "Produce verbose output", 0},
  {"debug",    'd', 0,      0,  "Produce debug output", 0},
  {"quiet",    'q', 0,      0,  "Don't produce any output", 0},
  {0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key) {
        case 'v':
            arguments->verbose = 1;
            break;
        case 'd':
            arguments->debug = 1;
            break;
        case 'q':
            arguments->verbose = 0;
            break;

    case ARGP_KEY_END:
        if (state->arg_num < 2) /* Not enough arguments. */
            argp_usage (state);
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= 2) /* Too many arguments. */
            argp_usage (state);
        arguments->args[state->arg_num] = arg;
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

struct arguments arguments = {.debug=0, .verbose=0};


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
static int init_shared_pages(char** mem, const uint size){

    *mem = (char *) malloc(PAGESIZE*size);
    if (*mem == NULL) {perror("malloc error");return EXIT_FAILURE;}

    uint _s = 0;
    while(_s < PAGESIZE*size) {
        _s += getrandom(*mem, (PAGESIZE * size) - _s, GRND_NONBLOCK);
    }

    return EXIT_SUCCESS;
}

pthread_barrier_t   barrier; // wait until all VMs are started

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
    int vcpu_mmap_size;             // mmap size
    int err;

    /* Default values. */
    snprintf(doc, 512, "%s\nVMM side channel test bench : version (%s) - %s\n%s\n",
             "----------------------------------------------------------------------------------------",
             __KERN_VERSION__, __BUILD_TIME__,
             "----------------------------------------------------------------------------------------");
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if(arguments.debug) {
        printf ("INPUTFILE = %s\nOUTPUTFILE = %s\n"
            "VERBOSE = %s\nSILENT = %s\n", arguments.args[0], arguments.args[1],
          arguments.verbose ? "yes" : "no", arguments.debug ? "yes" : "no");
    }


    if(arguments.verbose) {
        printf("run (slot %d) : 0x%llx (0x%llx)\nmmio (slot %d) : 0x%llx (0x%llx)\nmeasures (slot %d) : 0x%llx (0x%llx)\nown (slot %d) : 0x%llx (0x%llx)\nshared (slot %d) : 0x%llx (0x%llx)\n",
               MEM_SLOT_0, VM_MEM_RUN_ADDR, VM_MEM_RUN_SIZE,
               MEM_SLOT_1, VM_MEM_MMIO_ADDR, VM_MEM_MMIO_SIZE, MEM_SLOT_2, VM_MEM_MEASURES_ADDR, VM_MEM_MEASURES_SIZE,
               MEM_SLOT_3, VM_MEM_OWNPAGES_ADDR, VM_MEM_OWNPAGES_SIZE, MEM_SLOT_4, VM_MEM_SHAREDPAGES_ADDR,
               VM_MEM_SHAREDPAGES_SIZE);
    }
    if(arguments.debug) {
        printf("run (slot %d)\t\t: 0x%llx\t   - 0x%llx\n", MEM_SLOT_0, VM_MEM_RUN_ADDR,
               VM_MEM_RUN_ADDR + VM_MEM_RUN_SIZE);
        printf("mmio (slot %d)\t\t: 0x%llx - 0x%llx\n", MEM_SLOT_1, VM_MEM_MMIO_ADDR,
               VM_MEM_MMIO_ADDR + VM_MEM_MMIO_SIZE);
        printf("pt (slot %d)\t\t\t: 0x%llx - 0x%llx\n", MEM_SLOT_2, VM_MEM_PT_ADDR, VM_MEM_PT_ADDR + VM_MEM_PT_SIZE);
        printf("measures (slot %d)\t: 0x%llx - 0x%llx\n", MEM_SLOT_3, VM_MEM_MEASURES_ADDR,
               VM_MEM_MEASURES_ADDR + VM_MEM_MEASURES_SIZE);
        printf("own (slot %d)\t\t: 0x%llx - 0x%llx\n", MEM_SLOT_4, VM_MEM_OWNPAGES_ADDR,
               VM_MEM_OWNPAGES_ADDR + VM_MEM_OWNPAGES_SIZE);
        printf("shared (slot %d)\t\t: 0x%llx - 0x%llx\n", MEM_SLOT_5, VM_MEM_SHAREDPAGES_ADDR,
               VM_MEM_SHAREDPAGES_ADDR + VM_MEM_SHAREDPAGES_SIZE);
    }
    /* init KSM file descriptors */
    if(ksm_init() != 0) { goto end_ksm;}

    /* test KSM capability */
    if(!ksm_enabled()){ perror("KSM is not enabled (required) - please run make enable_ksm manually to enable it"); exit(EXIT_FAILURE); }

    /* initialize KVM common settings */
    printf("VMM : initialize KVM\n");
    vmm_init(&vmm);
    vcpu_mmap_size = ioctl(vmm, KVM_GET_VCPU_MMAP_SIZE, 0);          // const and used by all VM
    if (vcpu_mmap_size <= 0) { perror("KVM_GET_VCPU_MMAP_SIZE"); exit(1);}
    if (vcpu_mmap_size < (int)sizeof(struct kvm_run)) { perror("KVM_GET_VCPU_MMAP_SIZE unexpectedly small"); exit(1);}

    /* init VMM pages to be transferred to VMs */
    if(arguments.verbose) printf("VMM : filled %lld pages with random data to be shared between VMs\n", NB_SHARED_PAGES);
    char * shared_page_1 = NULL;
    err = init_shared_pages(&shared_page_1, NB_SHARED_PAGES);
    if (err < 0 ) { perror("failed to init buffer"); exit(EXIT_FAILURE);}

    if(arguments.verbose) printf("VMM : %d VMs initialized...launch VMs threads\n\n", NUMBEROFROLE);

    /* pretty name VMs */
    strncpy(vm[VICTIM].vm_name, "VM alice",  256);
    strncpy(vm[ATTACKER].vm_name, "VM charlie",  256);
    strncpy(vm[DEFENDER].vm_name, "VM eve",  256);
    vm[VICTIM].vm_role = VICTIM;
    vm[ATTACKER].vm_role = ATTACKER;
    vm[DEFENDER].vm_role = DEFENDER;
    err = load_commands(arguments.args[0], vm);
    if (err < 0 ) { perror("no input file can't initialize commands"); exit(EXIT_FAILURE);}


    /* create a barrier */
    pthread_barrier_init (&barrier, NULL, NUMBEROFROLE+1);

    /* launch VMs threads */
    for( int i = 0; i < NUMBEROFROLE; i++) {
        vm[i].fd_vm = ioctl(vmm, KVM_CREATE_VM, 0);
        if (vm[i].fd_vm < 0) { perror("KVM_CREATE_VM"); exit(1);}
        vm[i].vcpu_mmap_size = vcpu_mmap_size;
        /* init VM, vcpu, registers and memory regions */
        vm_init((vm + i), shared_page_1);

        /* launch VM */
        err = pthread_create( &tid[i], NULL, run_vm, (void*) (vm+i));
        if (err != 0 ) { perror("failed to launch VM thread"); exit(EXIT_FAILURE);}

    }
    err = pthread_create( &tm, NULL, time_master, (void*) (vm));
    if (err != 0 ) { perror("failed to launch time master thread"); exit(EXIT_FAILURE);}

    /* join */
    for( int i = 0; i < NUMBEROFROLE; i++) {
        void *iret;
        pthread_join(tid[i], &iret);
        printf("%s : exit %d\n", vm[i].vm_name, *(int *)((void *)iret));
    }
    void *ret_tm = NULL;
    pthread_join(tm, ret_tm);
    if(arguments.verbose) printf("time master : exit %ld\n", (int64_t)((void *)ret_tm));

    save_measures(arguments.args[1], vm);
    timestamp_s ts = {.cmd=PRIMITIVE_READ, .wait=0, .vm_id=VICTIM, .id=1, .ts[0] = 1, .ts[1] = 3, .ts[2] = 3};

    printf("%lu - %lu - %lu - %lu - %lu\n", sizeof(ts.id), sizeof(ts.cmd), sizeof(ts.vm_id), sizeof(ts.wait), sizeof(ts.ts));
    if(arguments.verbose) printf("VMM : free shared pages buffers\n");
    free(shared_page_1);
    for( int i = 0; i < NUMBEROFROLE; i++) {
        vm_destroy((vm+i));
    }


 end_ksm:
    ksm_close();

    exit(0);
}
