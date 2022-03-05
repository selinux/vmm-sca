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


static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}


void print_measures(){  outb(0xBE, 0);}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {

    uint64_t *measures = (uint64_t *)VM_SAMPLES_ADDR;

    for(int i=0; i< NB_SAMPLES/2;i++){
        *(measures++) = __rdtsc();
//        outb(0xE9, ' ');
    }

    print_measures();

	*(long *) 0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
}

/* rdtsc */
extern __inline uint64_t
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
__rdtsc ()
{
    return __builtin_ia32_rdtsc ();
}
