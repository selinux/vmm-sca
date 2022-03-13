CFLAGS = -Wall -Wextra -Werror -fno-stack-protector -O3

help:
	@echo -e "Available targets:\n"
	@echo -e "run\t\t\t\t build the VMM, guest VMs and run tests\n"
	@echo -e "pages_shared\t run same VM twice to test KSM pages sharing\n"
	@echo -e "clean\t\t\t clean up everything\n"

.PHONY: all
all: update_version
	$(MAKE) -C guests all
	$(MAKE) -C vmm run

update_version:
	./gen_version.sh version.h

.PHONY: clean
clean:
	$(MAKE) -C guests clean
	$(MAKE) -C vmm clean

enable_ksm:
	echo 1 | sudo tee /sys/kernel/mm/ksm/run

