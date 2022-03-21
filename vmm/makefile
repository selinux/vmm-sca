CC=gcc
CFLAGS =-g -lpthread -fsanitize=address -O3
WFLAGS=-Wall -Wextra -Werror
DEBUG=-g
ARCH=-m64
OPTS=$(CFLAGS) $(WFLAGS) $(ARCH) $(DEBUG)

OUTPUT=vmm-sca
GUEST_DEPS=../common/common.h $(wildcard ../guests/*.c)

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
PAYLOAD=../guests/payload.elf
COMMON=../common/common.h

run: $(OUTPUT) $(PAYLOAD)
	@./$(OUTPUT)

vmm-sca: $(OBJ) $(PAYLOAD) $(COMMON)
	$(MAKE) -C ../guests payload.elf
	$(CC) $^ $(OPTS) -o $@

vm.o: vm.c vm.h $(PAYLOAD)
	$(CC) $(OPTS) $< -c

%.o: %.c ../common/common.h
	@$(CC) -o $@ $(OPTS) -c $<

$(OBJ): $(SRC)

../guests/payload.elf: $(GUEST_DEPS)
	$(MAKE) -C ../guests payload.elf

../common/common.h:


.PHONY: clean
clean:
	$(RM) *.o $(OUTPUT)
	$(MAKE) -C ../guests clean
