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


static void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

//static int loop(){
//    while (1){
//       outb(0xE9, '\165');
//
//    };
//    return 0xa5;
//}

void print_measures(){  outb(0xBE, 0);}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {

    long long unsigned *measures = (long long unsigned *)0x10000;
//    char *measures = (char *)0x10000;
    for(int i=0; i< NB_SAMPLES;i++){
//        *(measures+i) = 'G';
        *(measures++) = __rdtsc();
    }
    print_measures();

	*(long *) 0x400 = 42;

	for (;;)
		asm("hlt" : /* empty */ : "a" (42) : "memory");
//    loop();
}


/* rdtsc */
extern __inline uint64_t
__attribute__((__gnu_inline__, __always_inline__, __artificial__))
__rdtsc ()
{
    return __builtin_ia32_rdtsc ();
}
