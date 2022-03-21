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

//    static long long unsigned *measures = (void *)VM_MEM_MEASURES_ADDR;
//    for(int i=0; i< NB_SAMPLES;i++){
//        *(measures++) = __rdtsc();
//    }
//    print_measures();

    for(;;)
        exit_halt();
}
