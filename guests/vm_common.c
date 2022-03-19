#include "../common/common.h"
#include "vm_common.h"

long long unsigned *measures = (void *)VM_MEM_MEASURES_ADDR;

void exit_halt(){
    *(long *) VM_EXIT_RETURN_CODE_ADDR = VM_EXIT_RETURN_CODE;
    asm("hlt" : /* empty */ : "a" (VM_EXIT_RETURN_CODE) : "memory");
}

void exit_shutdown(){}

void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

void slow_vmm_printf(char *str){
    char* p;
    for (p = str; *p; ++p)
		outb(0xE9, *p);
}

void print_measures(){
    outb(PMIO_PRINT_MEASURES, 0);
}

int myloop(){
    while (1){
        if( *(long *)PRIMITIVE_CMD_ADDR == PRIMITIVE_EXIT){
//            slow_vmm_printf("Exit charlie Okyy\n");
            break;
        }
    }
    return 0xa5;
}

int wait_action(){
    while (1){
        if(*(long *)PRIMITIVE_CMD_ADDR != PRIMITIVE_WAIT){
            switch(*(long *)PRIMITIVE_CMD_ADDR){
                case PRIMITIVE_WAIT:
                    *(long *)PRIMITIVE_CMD_ADDR = 0;  // reset CMD
                    continue;
                case PRIMITIVE_READ:
                    *(long *)PRIMITIVE_CMD_ADDR = 0;  // reset CMD
                    continue;
                case PRIMITIVE_MEASURE:
                    *(long *)PRIMITIVE_CMD_ADDR = 0;  // reset CMD
                    *(measures++) = __builtin_ia32_rdtsc();
                    slow_vmm_printf("measure Okyyy from VM\n");
                    continue;
                case PRIMITIVE_PRINT_MEASURES:
                    *(long *)PRIMITIVE_CMD_ADDR = 0;  // reset CMD
                    print_measures();
                    continue;
                case PRIMITIVE_EXIT:
                    *(long *)PRIMITIVE_CMD_ADDR = 0;
                    goto end_loop;
                default:
                    continue;
            }
        }
    }
 end_loop:

    return 0xa5;
}

/* rdtsc */
//extern __inline long long unsigned
//__attribute__((__gnu_inline__, __always_inline__, __artificial__)) __rdtsc () {
//    return __builtin_ia32_rdtsc ();
//}
