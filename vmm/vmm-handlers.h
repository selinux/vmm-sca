#ifndef __VMM_HANDLERS_H_
#define __VMM_HANDLERS_H_

#include "../common/common.h"
#include "vmm.h"

void handle_mmio(vm *vm);
void handle_pmio(vm *vm);

#endif