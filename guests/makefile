CC=gcc
CFLAGS=-ffreestanding -fno-pic -fno-stack-protector -no-pie -O3
WFLAGS=-Wall -Wextra
DEBUG=-g
ARCH=-m64
OPTS=$(CFLAGS) $(WFLAGS) $(ARCH) $(DEBUG)

OUTPUT=payload.elf
COMMON_OBJ=vm_common.o
VMS=vm_alice.c vm_charlie.c vm_eve.c
VMS_BIN=$(VMS:.c=.bin)

all: payload.elf

payload.elf: $(VMS_BIN) ../common/common.h
	$(LD) -r -T payload.ld -o $@

%.bin: %.o.bin
	$(LD) -b binary -r $^ -o $@

vm_alice.o.bin: $(COMMON_OBJ) ../common/common.h

vm_charlie.o.bin: $(COMMON_OBJ) ../common/common.h

vm_eve.o.bin: $(COMMON_OBJ) ../common/common.h

%.o.bin: %.o
	$(LD) -T guest.ld $< $(COMMON_OBJ) -o $@

%.o: %.c
	$(CC) $(OPTS) -c -o $@ $<


.PHONY: clean
clean:
	@echo remove VMs payload
	$(RM) $(OUTPUT) *.o *.bin
