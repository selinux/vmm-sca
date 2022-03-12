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

// Function to swap two numbers
void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}

// Function to reverse `buffer[i…j]`
char* reverse(char *buffer, int i, int j)
{
    while (i < j) {
        swap(&buffer[i++], &buffer[j--]);
    }

    return buffer;
}

// Iterative function to implement `itoa()` function in C
char* itoa(int value, char* buffer, int base)
{
    // invalid input
    if (base < 2 || base > 32) {
        return buffer;
    }

    // consider the absolute value of the number
    int n = value;

    int i = 0;
    while (n)
    {
        int r = n % base;

        if (r >= 10) {
            buffer[i++] = 65 + (r - 10);
        }
        else {
            buffer[i++] = 48 + r;
        }

        n = n / base;
    }

    // if the number is 0
    if (i == 0) {
        buffer[i++] = '0';
    }

    // If the base is 10 and the value is negative, the resulting string
    // is preceded with a minus sign (-)
    // With any other base, value is always considered unsigned
    if (value < 0 && base == 10) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // null terminate string

    // reverse the string and return it
    return reverse(buffer, 0, i - 1);
}

void print_measures(){  outb(0xBE, 0);}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {

//    uint64_t *measures = (uint64_t *)VM_MEM_MEASURES_ADDR;
//    *(measures++) = __rdtsc();
//    __rdtsc();
//    for(int i=0; i< NB_SAMPLES;i++){
//        *(measures++) = __rdtsc();
//        outb(0xE9, ' ');
//    }

    uint64_t mes = __rdtsc();      // this instruction work

    *(long *) 0x100000 = 123456;   // write somewhere in memory slot 0
    mes = *(long *) 0x100000;      // it works
    char p[256];
//    mes = *(long *) 0x200500;      // this access don't : exit(8) + exit(17)

    itoa(mes, p, 10);
    uint i = 0; // printf
    while(p[i]){
        outb(0xE9, p[i]);
        i++;
    }
    outb(0xE9, '\n');

//    while (1);

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
