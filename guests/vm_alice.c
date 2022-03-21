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

//static inline uint8_t inb(uint16_t port)
//{
//    uint8_t ret;
//    asm volatile ( "inb %1, %0"
//    : "=a"(ret)
//    : "Nd"(port) );
//    return ret;
//}
//uint8_t inb(uint16_t port)
//{
//    uint8_t ret;
//    asm volatile ( "inb %1, %0"
//                   : "=a"(ret)
//                   : "Nd"(port) );
//    return ret;
//}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {



    /* MMIO */
    *(uint8_t *)(VM_MEM_MMIO_ADDR)  = 0xbe;
    *(uint16_t *)(VM_MEM_MMIO_ADDR) = 0xbeaf;
    *(uint32_t *)(VM_MEM_MMIO_ADDR) = 0xdeadbeef;
    *(uint64_t *)(VM_MEM_MMIO_ADDR) = 0xdeadbeefabdab00f;

    /* PMIO */
    uint8_t c = inb(PMIO_READ);
    uint16_t w = inw(PMIO_READ);
    uint32_t l = inl(PMIO_READ);
    outb(0xffaa, 0xab);
    outw(0xffab, 0xafaf);
    outl(0xffac, 0xafafbeef);

//    const char *p;
//    for (p = "Hello, world!\n"; *p; ++p)
//        outb(0xE9, *p);
    /* measure */
    static long long unsigned *measures = (void *)VM_MEM_MEASURES_ADDR;
    for(int i=0; i< NB_SAMPLES;i++){
        *(measures++) = __rdtsc();
    }
    print_measures();

    for(;;)
        exit_halt();
}


