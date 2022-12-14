/*
 * =====================================================================================
 *
 *       Filename:  vmm-handlers.c
 *
 *    Description:  vmm handlers for MMIO/PMIO
 *
 *        Version:  1.0
 *        Created:  21/03/22 11:02:06 AM CET
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sebastien Chassot (sinux), sebastien.chassot@etu.unige.ch
 *        Company:  Unige - Master in Computer Science
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>
#include <sys/random.h>
#include <assert.h>

#include "vmm-handlers.h"


void handle_mmio(vm *vm) {
    struct kvm_run *run = vm->vcpu.kvm_run;

    // Guest write to an MMIO address
    if (run->mmio.is_write) {

        uint64_t value;
        switch (run->mmio.len) {
            case 1:
                value = *((uint8_t *)run->mmio.data);
//                fprintf(stdout, "%s : MMIO write 1 byte\n", vm->vm_name);
                break;
            case 2:
                value = *((uint16_t *)run->mmio.data);
//                fprintf(stdout, "%s : MMIO write 2 bytes\n", vm->vm_name);
                break;
            case 4:
                value = *((uint32_t *)run->mmio.data);
//                fprintf(stdout, "%s : MMIO write 4 bytes\n", vm->vm_name);
                break;
            case 8:
                value = *((uint64_t *)run->mmio.data);
//                fprintf(stdout, "%s : MMIO write 8 bytes\n", vm->vm_name);
                break;
            default:
                fprintf(stderr, "VMM: Unsupported size in KVM_EXIT_MMIO\n");
                value = 0;
        }
        printf("VMM: MMIO %s write: len=%d addr=0x%lx value=0x%lx\n", vm->vm_name, run->mmio.len, (long int)run->mmio.phys_addr, value);
    }
}

uint8_t handle_pmio(vm *vm, command_s* cmd, uint64_t * nb_timestamp) {
    struct kvm_run *run = vm->vcpu.kvm_run;

    /* VM write (OUTB) */
    if (vm->vcpu.kvm_run->io.direction == KVM_EXIT_IO_OUT) {
        if (vm->vcpu.kvm_run->io.port == 0xE9) {
            char *p = (char *) run;
            fwrite(p + run->io.data_offset,
                   run->io.size, 1, stdout);
            fflush(stdout);
            return 0;
        }
        if (vm->vcpu.kvm_run->io.port == 0xffaa) {
            fprintf(stdout, "VMM : %s PMIO write on port 0x%x size %d\n", vm->vm_name, vm->vcpu.kvm_run->io.port, vm->vcpu.kvm_run->io.size);
        }
        if (vm->vcpu.kvm_run->io.port == 0xffab) {
            fprintf(stdout, "VMM : %s PMIO write on port 0x%x size %d\n", vm->vm_name, vm->vcpu.kvm_run->io.port, vm->vcpu.kvm_run->io.size);
        }
        if (vm->vcpu.kvm_run->io.port == 0xffac) {
            fprintf(stdout, "VMM : %s PMIO write on port 0x%x size %d\n", vm->vm_name, vm->vcpu.kvm_run->io.port, vm->vcpu.kvm_run->io.size);
        }
        return 0;
    }
    /* VM read (INB) */
    if (run->io.direction == KVM_EXIT_IO_IN) {
        if (run->io.port == PMIO_READ) {
            fprintf(stdout, "VMM : %s PMIO read on port 0x%x, size %d\n", vm->vm_name, vm->vcpu.kvm_run->io.port, vm->vcpu.kvm_run->io.size);
            uint8_t *addr = (uint8_t *)run + run->io.data_offset;
            *addr = 'g';
            return 0;
        }
        /* execute sca command */
        if (run->io.port == PMIO_READ_CMD) {
            if (cmd->cmd == PRIMITIVE_PRINT_MEASURES) {

                printf("%s - dump measurement from VMM (direct VM memory access)\n", vm->vm_name);
                uint64_t *m = (uint64_t *) vm->mem_measures;  // skip first measure
                for (uint64_t i = 0; i < *nb_timestamp; i++) {
                    uint64_t dm = *(uint64_t *) (m) - *(uint64_t *) (m - 1);
                    if(*(uint64_t *) (m - 1)==0){dm=0;}
                    printf("%s (%04ld) : %lu (%lu)\n", vm->vm_name, i, *(uint64_t *) (m++), dm);
                }
                uint8_t *write_back = (uint8_t *)run + run->io.data_offset;
                *write_back = cmd->cmd;
            } else {
                /* pass additional values */
                *(uint64_t *)(vm->mem_mmio+PRIMITIVE_TARGET_OFFSET) = (uint64_t )cmd->addr;
                *(uint64_t *)(vm->mem_mmio+PRIMITIVE_VALUE_OFFSET) = (uint64_t)cmd->value;
                *(uint64_t *)(vm->mem_mmio+PRIMITIVE_WAIT_OFFSET) = (uint64_t)cmd->wait;
                *(uint64_t *)(vm->mem_mmio+PRIMITIVE_REAPETE_OFFSET) = (uint64_t)cmd->repeat;
                /* write response back */
                uint8_t *write_back = (uint8_t *)run + run->io.data_offset;
                *write_back = cmd->cmd;
                switch (cmd->cmd) {
                    case PRIMITIVE_MEASURE:
                        *nb_timestamp += cmd->repeat;
                        break;
                    case PRIMITIVE_READ:
                        *nb_timestamp += 2*cmd->repeat;
                        break;
                    case PRIMITIVE_WRITE:
                        *nb_timestamp += 2*cmd->repeat;
                        break;
                    default:
                        break;
                }
                /* wait till release VM */
                usleep(cmd->wait);
            }
            return 1;
        }
    }
    return 0;
}
