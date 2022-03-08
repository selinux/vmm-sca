CFLAGS = -Wall -Wextra -Werror -fno-stack-protector -O3

help:
	@echo "Available targets:"
	@echo "run      build the VMM, guest VMs and run tests"
	@echo "clean    clean up everything"
	@echo ""

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



