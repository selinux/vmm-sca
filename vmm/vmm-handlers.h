#ifndef __VMM_HANDLERS_H_
#define __VMM_HANDLERS_H_

#include "../common/common.h"
#include "vmm.h"

void handle_mmio(vm *vm);
uint8_t handle_pmio(vm *vm, command_s* cmd, uint64_t * nb_timestamp);

#endif