#include "../common/common.h"
#include "vm_common.h"

long long unsigned *measures = (void *)VM_MEM_MEASURES_ADDR;

/** write to port : oub 1 byte, outw 2 bytes, outl 4 bytes
 *
 * @param port
 * @param value
 */
void outb(uint16_t port, uint8_t value) {
    asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

void outw(uint16_t port, uint16_t value) {
    asm("outw %0,%1" : /* empty */ : "ax" (value), "Nd" (port) : "memory");
}

void outl(uint16_t port, uint32_t value) {
    asm("outl %0,%1" : /* empty */ : "eax" (value), "Nd" (port) : "memory");
}

/** read from port : oub 1 byte, inw 2 bytes, inl 4 bytes
 *
 * @param port
 * @return
 */
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=ax"(ret) : "Nd"(port) );
    return ret;
}

uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0" : "=eax"(ret) : "Nd"(port) );
    return ret;
}


void exit_halt(){
    *(long *) VM_EXIT_RETURN_CODE_ADDR = VM_EXIT_RETURN_CODE;
    asm("hlt" : /* empty */ : "a" (VM_EXIT_RETURN_CODE) : "memory");
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
