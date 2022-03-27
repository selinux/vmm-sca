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
    if (!fp) { perror("fopen commands file"); return EXIT_FAILURE;}
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

//int save_measures(char* filename, uint64_t* measures, uint64_t nb_measures){
//    FILE *fp = fopen(filename, "rb");
//    if (!fp) { perror("fopen commands file"); return EXIT_FAILURE;}
//
//    return 0;
//}

int save_commands(char* filename, command_u *cmd, const uint64_t s){
    FILE *fp = fopen(filename, "w");
    if (!fp) { perror("fopen commands file"); return EXIT_FAILURE;}
    size_t ret = fwrite(cmd, sizeof(command_u), s, fp);
    if (ret != s) { perror("fwrite failed to write command file"); return -1;}
    printf("write commands to file %s\n", filename);
    fclose(fp);

    return 0;
}
