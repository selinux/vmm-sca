#ifndef __COMMON_VM_H_
#define __COMMON_VM_H_

#include <x86intrin.h>
#include <stdint.h>


void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void exit_halt();
void print_measures();
void slow_vmm_printf(char *str);
int wait_action();



#endif