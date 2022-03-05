#ifndef __COMMON_H_
#define __COMMON_H_



#define NB_SAMPLES        (0x10)


#define VMM_SAMPLES_ADDR  (0x10000)
#define VM_SAMPLES_ADDR   (0x80000)


typedef enum {
    VICTIM,
    ATTACKER,
    DEFENDER,
    NUMBEROFROLE
} ROLE;

typedef struct _vm {
    char vm_name[256];
    int fd;
    char *mem;
    ROLE vm_role;
} vm;


#endif
