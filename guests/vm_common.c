#include "../common/common.h"
#include "vm_common.h"


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
    __asm__ ( "inl %1, %0" : "=eax"(ret) : "Nd"(port) );
    return ret;
}


void exit_halt(){
    *(long *) VM_EXIT_RETURN_CODE_ADDR = VM_EXIT_RETURN_CODE;
    __asm__ ("hlt" : /* empty */ : "a" (VM_EXIT_RETURN_CODE) : "memory");
}


void slow_vmm_printf(char *str){
    char* p;
    for (p = str; *p; ++p)
		outb(0xE9, *p);
}

void print_measures(){
    outb(PMIO_PRINT_MEASURES, 0);
}


int wait_action(){
    uint64_t *measures = (uint64_t *)VM_MEM_MEASURES_ADDR;
    uint64_t * addr = NULL;
    uint64_t tmp_value;
    uint64_t tmp_addr;
    while (1){
        switch(inb(PMIO_READ_CMD)){
            case PRIMITIVE_READ:
//                slow_vmm_printf("read\n");
                *(measures++) = __builtin_ia32_rdtsc();
                tmp_value = *addr;                          // read at
                *(measures++) = __builtin_ia32_rdtsc();

                tmp_addr = 0;       // reset address
                tmp_value = 0;      // reset read value
                continue;
            case PRIMITIVE_WRITE:
//                slow_vmm_printf("write\n");
                tmp_value = *(uint64_t *)(PRIMITIVE_WRITE_VALUE_ADDR);
                *(measures++) = __builtin_ia32_rdtsc();
                *(uint64_t *)(PRIMITIVE_TARGET_ADDR) = (uint64_t )tmp_value;   // write to
                *(measures++) = __builtin_ia32_rdtsc();

                *(uint64_t *)(PRIMITIVE_TARGET_ADDR) = 0;    // reset address
                continue;
            case PRIMITIVE_MEASURE:
//                slow_vmm_printf("measure\n");
                *(measures++) = __builtin_ia32_rdtsc();
                continue;
            case PRIMITIVE_PRINT_MEASURES:
//                slow_vmm_printf("print\n");
                print_measures();
                continue;
            case PRIMITIVE_WAIT:
                slow_vmm_printf("wait\n");
                continue;
            case PRIMITIVE_EXIT:
                slow_vmm_printf("exit\n");
                exit_halt();
                break;
            default:
                slow_vmm_printf("error unknown command\n");
//                outb(0xE9, *cmd);
                continue;
        }
    }
//    end_loop:

//    return 0xa5;
}

/* rdtsc */
//extern __inline long long unsigned
//__attribute__((__gnu_inline__, __always_inline__, __artificial__)) __rdtsc () {
//    return __builtin_ia32_rdtsc ();
//}

static idt64_entry_t idt_build_entry(uint16_t selector, uint64_t offset, uint8_t type, uint8_t dpl) {
    idt64_entry_t entry;
    entry.offset15_0 = offset & 0xffff;
    entry.selector = selector;
    entry.ist = 0;
    entry.reserved = 0;
    entry.type = type;
    entry.dpl = dpl;
    entry.p = 1;
    entry.offset31_16 = (offset >> 16) & 0xffff;
    entry.offset64_32 = (offset >> 32) & 0xffffffff;
    return entry;
}

void irq_0() {
    outb(0xffac, 0xaf);
    __asm__ ("iretq\n");

}


void idt_init(){

    idt64_entry_t *idt_table = (idt64_entry_t *)VM_IDT_ADDR;
    for (int i = 0; i < 20; i++) {
        idt_table[i] = idt_build_entry( 8, 0, TYPE_INTERRUPT_GATE, DPL_KERNEL);
    }
    idt_table[10] = idt_build_entry( 8, (uint64_t )&irq_0, TYPE_INTERRUPT_GATE, DPL_KERNEL);
     __asm__ volatile ("sti"); // set the interrupt flag
}