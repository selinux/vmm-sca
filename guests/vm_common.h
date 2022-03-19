#ifndef __COMMON_VM_H_
#define __COMMON_VM_H_

#include <x86intrin.h>
#include <stdint.h>


void exit_halt();
void exit_shutdown();
void outb(uint16_t port, uint8_t value);
void print_measures();
int myloop();
void slow_vmm_printf(char *str);
int wait_action();



#endif