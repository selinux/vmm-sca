CFLAGS = -Wall -Wextra -Werror -fno-stack-protector -O3



payload.o: payload.ld vm_alice.img.o vm_charlie.img.o vm_eve.img.o
	$(LD) -r -T $< -o $@


%.o: %.c
	$(CC) $(CFLAGS) -m64 -ffreestanding -fno-pic -fno-stack-protector -c -o $@ $<

%.img: %.o
	$(LD) -T guest.ld $< -o $@

%.img.o: %.img
	$(LD) -b binary -r $^ -o $@


.PHONY: clean
clean:
	$(RM) *.o
