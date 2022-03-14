CFLAGS=-ffreestanding -fno-pic -fno-stack-protector -no-pie
WFLAGS=-Wall -Wextra
DEBUG=-g
ARCH=-m64
OPTS=$(CFLAGS) $(WFLAGS) $(ARCH) $(DEBUG)

OUTPUT=payload.elf
VMS=$(wildcard *.c)
VMS_BIN=$(VMS:.c=.bin)
VMS_OBJ=$(VMS:.c=.o)
DEPS=payload.elf $(VMS_OBJ)

all: payload.elf

payload.elf: $(VMS_BIN) ../common/common.h
	$(LD) -r -T payload.ld -o $@

%.bin: %.o.bin
	$(LD) -b binary -r $^ -o $@

%.o.bin: %.o
	$(LD) -T guest.ld $< -o $@

%.o: %.c ../common/common.h
	$(CC) $(OPTS) -c -o $@ $<

$(VMS_OBJ): $(VMS)

.PHONY: clean
clean:
	@echo remove VMs payload
	$(RM) $(OUTPUT) *.o *.bin
