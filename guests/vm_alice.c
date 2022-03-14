/*
 * =====================================================================================
 *
 *       Filename:  vm_alice.c
 *
 *    Description:  alice do some every day activities (accessing/modifying data,...)
 *
 *
 *        Version:  1.0
 *        Created:  04/03/22 08:59:54 AM CET
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sebastien Chassot (sinux), sebastien.chassot@etu.unige.ch
 *        Company:  Unige - Master in Computer Science
 *
 * =====================================================================================
 */
#include <x86intrin.h>
#include <stdint.h>

#include "../common/common.h"
#include "vm_common.h"

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {

//    long long unsigned *measures = (void *)0x10000;
//    char *measures = (char *)0x10000;
//    for(int i=0; i< NB_SAMPLES;i++){
//        *(measures+i) = 'G';
//        *(measures++) = __rdtsc();
//    }

//    unsigned long* test_mmio = (void *)VM_MEM_MEASURES_ADDR+0x0;
    for(int i=0; i < VM_MEM_MEASURES_SIZE; i += 0x100) {
//        *(test_mmio + i) = 0xa5a5a5a5;
    }
//    print_measures();

    for(;;)
        exit_halt();
}


