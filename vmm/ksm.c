/*
 * =====================================================================================
 *
 *       Filename:  ksm.c
 *
 *    Description:  main executable
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

#include "ksm.h"


uint ksm_init(){
    FILE *ksm_run = NULL;
    if((ksm_run = fopen(KSM_RUN, "r")) == NULL){ perror("KSM run open error");}

    char ksm_state = fgetc(ksm_run);
    ksm_state == '1' ? printf("VMM : KSM enabled - continue\n") : printf("KSM not enabled\n");
    fclose(ksm_run);

    return ksm_state == '1' ? 1 : 0;
}

uint ksm_shared_pages(){
    FILE *ksm_sp = NULL;
    char buffer[256];
    u_int64_t sp = 0;
    if((ksm_sp = fopen(KSM_PSHARED, "r")) == NULL){ perror("KSM shared pages open error");}

    int count = fread(&buffer, sizeof(char), 20, ksm_sp);

    if(count > 0)
        sp = atoi(buffer);

    // TODO : don't open and close at each call
    fclose(ksm_sp);
    return sp;
}

uint ksm_max_shared_pages(){
    FILE *ksm_sp = NULL;
    char buffer[256];
    u_int64_t sp = 0;
    if((ksm_sp = fopen(KSM_MAX_PSH, "r")) == NULL){ perror("KSM max sharing pages open error");}

    int count = fread(&buffer, sizeof(char), 20, ksm_sp);

    if(count > 0)
        sp = atoi(buffer);

    fclose(ksm_sp);
    return sp;
}
