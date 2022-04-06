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
#include <unistd.h>

#include "ksm.h"

FILE *ksm_run_fd = NULL;
FILE *ksm_sp_fd = NULL;
FILE *ksm_sharingp_fd = NULL;
FILE *ksm_msp_fd = NULL;
FILE *ksm_usp_fd = NULL;

int ksm_init(){

    printf("KSM : open KSM file descriptors\n");
    if((ksm_run_fd = fopen(KSM_RUN, "r")) == NULL){ perror("KSM run open error");}
    if((ksm_sp_fd = fopen(KSM_PSHARED, "r")) == NULL){ perror("KSM page shared open error");}
    if((ksm_sharingp_fd = fopen(KSM_PSHARING, "r")) == NULL){ perror("KSM page shared open error");}
    if((ksm_msp_fd = fopen(KSM_MAX_PSH, "r")) == NULL){ perror("KSM run open error");}
    if((ksm_usp_fd = fopen(KSM_PU, "r")) == NULL){ perror("KSM run open error");}

    return 0;
}

void ksm_close(){

    printf("KSM : close KSM file descriptors\n");
    if(ksm_run_fd != NULL) {fclose(ksm_run_fd);}
    if(ksm_sp_fd != NULL) {fclose(ksm_sp_fd);}
    if(ksm_sharingp_fd != NULL) {fclose(ksm_sharingp_fd);}
    if(ksm_msp_fd != NULL) {fclose(ksm_msp_fd);}
    if(ksm_usp_fd != NULL) {fclose(ksm_usp_fd);}
}


uint ksm_enabled(){

    char ksm_state = fgetc(ksm_run_fd);
    ksm_state == '1' ? printf("VMM : KSM enabled - continue\n") : printf("KSM not enabled\n");
    fseek(ksm_run_fd, 0, SEEK_SET);

    return ksm_state == '1' ? 1 : 0;
}


uint ksm_shared_pages(){
    char buffer[256];
    u_int64_t sp = 0;

    int count = fread(&buffer, sizeof(char), 20, ksm_sp_fd);
    if(count > 0)
        sp = atoi(buffer);

    fseek(ksm_sp_fd, 0, SEEK_SET);
    return sp;
}

uint ksm_sharing_pages(){
    char buffer[256];
    u_int64_t sp = 0;

    int count = fread(&buffer, sizeof(char), 20, ksm_sp_fd);
    if(count > 0)
        sp = atoi(buffer);

    fseek(ksm_sp_fd, 0, SEEK_SET);
    return sp;
}

uint ksm_ushared_pages(){
    char buffer[256];
    u_int64_t sp = 0;

    int count = fread(&buffer, sizeof(char), 20, ksm_usp_fd);
    if(count > 0)
        sp = atoi(buffer);

    fseek(ksm_usp_fd, 0, SEEK_SET);
    return sp;
}


uint ksm_max_shared_pages(){
    char buffer[256];
    u_int64_t sp = 0;

    int count = fread(&buffer, sizeof(char), 20, ksm_msp_fd);
    if(count > 0)
        sp = atoi(buffer);

    fseek(ksm_msp_fd, 0, SEEK_SET);

    return sp;
}

int ksm_wait(const uint shared_pages){
    int i = 0;
    uint64_t wait = 100*shared_pages;
    uint64_t nb_sp = ksm_shared_pages();
    uint64_t nb_sharingp = ksm_sharing_pages();
    uint64_t nb_usp = ksm_ushared_pages();
    printf("KSM : shared pages at beginning : %ld, sharing %ld (unshared %ld)\n", nb_sp, nb_sharingp, nb_usp);
#ifdef DEBUG
    printf("KSM : max pages sharing : %d\n", ksm_max_shared_pages());
#endif
    uint64_t last_val = nb_sp;
    while(nb_sp < shared_pages ) {
        i++;
        usleep(wait);
        nb_sp = ksm_shared_pages();
        nb_sharingp = ksm_sharing_pages();
        nb_usp = ksm_ushared_pages();
        if(last_val < nb_sp) {
            printf("KSM : shared pages after (%.2fs) : %ld, sharing %ld (unshared %ld)\n", (wait / 1000000.) * i, nb_sp, nb_sharingp, nb_usp);
            last_val = nb_sp;
        }
    }
    return 0;
}