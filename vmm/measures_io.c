/*
 * =====================================================================================
 *
 *       Filename:  measures_io.c
 *
 *    Description:  import commands and export measurement
 *
 *        Version:  1.0
 *        Created:  08/03/22 10:18:54 AM CET
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sebastien Chassot (sinux), sebastien.chassot@etu.unige.ch
 *        Company:  Unige - Master in Computer Science
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <unistd.h>

#include "../common/common.h"
#include "measures_io.h"
#include "vm.h"


int load_commands(char * filename, vm *vm){
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("fopen commands file"); return -1;}
    command_u data[NUMBEROFROLE];
    command_s cmd;
    command_s * pos[NUMBEROFROLE];

    size_t ret = fread(&data, sizeof(command_u), NUMBEROFROLE, fp);
    if (ret != NUMBEROFROLE) { fprintf(stderr, "fread() failed: %zu\n", ret); return -1;}

    /* read n headers */
    for(int i =0; i < NUMBEROFROLE; i++){
        (vm+data[i].cmds_header.role)->nb_cmd = 0;
        (vm+data[i].cmds_header.role)->cmds = (command_s *) malloc(sizeof(command_s)*data[i].cmds_header.nb_measures);
        if ((vm+data[i].cmds_header.role)->cmds  == NULL) { fprintf(stderr, "malloc command failed: %s\n", vm_role(data[i].cmds_header.role)); exit(EXIT_FAILURE);}
        pos[data[i].cmds_header.role] = (vm+data[i].cmds_header.role)->cmds;
    }
    /* append commands to VMs */
    while (fread(&cmd, sizeof(command_s), 1, fp) == 1){
        switch (cmd.vm_role) {
            case VICTIM:
                *(pos[VICTIM]++) = (command_s)cmd;
                (vm+VICTIM)->nb_cmd += 1;
                continue;
            case ATTACKER:
                *(pos[ATTACKER]++) = (command_s)cmd;
                (vm+ATTACKER)->nb_cmd += 1;
                continue;
            case DEFENDER:
                *(pos[DEFENDER]++) = (command_s)cmd;
                (vm+DEFENDER)->nb_cmd += 1;
                continue;
            default:
                fprintf(stderr, "fread{} unknown VM command failed: %s\n", vm_role(cmd.vm_role));
                break;
        }
    }
    fclose(fp);

    return 0;
}


int save_measures(char* filename, vm* vm){
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("fopen commands file"); return -1;}
    size_t ret;

    for(int v=0; v < NUMBEROFROLE; v++) {
        uint64_t * _m = (vm+v)->mem_measures;
        command_s * _cmd = (vm+v)->cmds;
        for (uint64_t i = 0; i < (vm+v)->nb_cmd; i++) {
            timestamp_s t ={.id=i,.cmd = _cmd->cmd, .vm_id = v, .wait = _cmd->wait, .ts= {0,0,0,0,0}};
            switch (_cmd->cmd) {
                case PRIMITIVE_WAIT:
                case PRIMITIVE_EXIT:
                    ret = fwrite(&t, sizeof(timestamp_s), 1, fp);
                    if (ret != 1) { perror("fwrite failed to write timestamps file"); return -1;}
                    _cmd++;
                    continue;
                case PRIMITIVE_MEASURE:
                    t.ts[0] = *(_m++);
                    ret = fwrite(&t, sizeof(timestamp_s), 1, fp);
                    if (ret != 1) { perror("fwrite failed to write timestamps file"); return -1;}
                    _cmd++;
                    continue;
                case PRIMITIVE_READ:
                case PRIMITIVE_WRITE:
                    t.ts[0] = *(_m++);
                    t.ts[1] = *(_m++);
                    ret = fwrite(&t, sizeof(timestamp_s), 1, fp);
                    if (ret != 1) { perror("fwrite failed to write timestamps file"); return -1;}
                    _cmd++;
                    continue;
                default:
                    continue;
            }
        }
    }
    return 0;
}


int save_commands(char* filename, command_u *cmd, const uint64_t s){
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("fopen commands file"); return EXIT_FAILURE;}
    size_t ret = fwrite(cmd, sizeof(command_u), s, fp);
    if (ret != s) { perror("fwrite failed to write command file"); return -1;}
    printf("write commands to file %s\n", filename);
    fclose(fp);

    return 0;
}
