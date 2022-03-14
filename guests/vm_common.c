#include "../common/common.h"
#include "vm_common.h"

void exit_halt(){
    *(long *) VM_EXIT_RETURN_CODE_ADDR = VM_EXIT_RETURN_CODE;
    asm("hlt" : /* empty */ : "a" (VM_EXIT_RETURN_CODE) : "memory");
}

void exit_shutdown(){}

void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}


void print_measures(){
    outb(PMIO_PRINT_MEASURES, 0);
}

int myloop(){
    while (1){
        if( *(long *)0x500 == 1)
            break;
    };
    return 0xa5;
}

/* rdtsc */
extern __inline long long unsigned
__attribute__((__gnu_inline__, __always_inline__, __artificial__)) __rdtsc () {
    return __builtin_ia32_rdtsc ();
}
