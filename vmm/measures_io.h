#ifndef __MEASURES_IO__
#define __MEASURES_IO__

#include <stdlib.h>
#include <stdlib.h>
#include <stdint.h>

#include "../common/common.h"
#include "vmm.h"

int load_commands(char * filename, vm *vm);
int save_commands(char* filename, command_u *cmd, uint64_t size);
int save_measures(char* filename, uint64_t* measures, uint64_t nb_measures);

#endif