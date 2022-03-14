/*
 * =====================================================================================
 *
 *       Filename:  vm_charlie.c
 *
 *    Description:  charlie try to spy alice exploiting side channel by doing some cache
 *                  access measurement
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

//    uint64_t *measures = (uint64_t *)VM_MEM_MEASURES_ADDR;

//    for(int i=0; i< NB_SAMPLES/2;i++){
//        *(measures++) = __rdtsc();
//        outb(0xE9, ' ');
//    }

//    print_measures();
    myloop();

    for(;;)
        exit_halt();
}
