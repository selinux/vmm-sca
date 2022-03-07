CFLAGS = -Wall -Wextra -Werror -fno-stack-protector -O3

help:
	@echo "Available targets:"
	@echo "run      build the VMM, guest VMs and run tests"
	@echo "clean    clean up everything"
	@echo ""

.PHONY: all
all:
	$(MAKE) -C guests all
	$(MAKE) -C vmm run


.PHONY: clean
clean:
	$(MAKE) -C guests clean
	$(MAKE) -C vmm clean



