CFLAGS = -Wall -Wextra -Werror -fno-stack-protector -O3

help:
	@echo "Available targets:"
	@echo "run      build the VMM, guest VMs and run tests"
	@echo "tools    build the tools only (unused)"
	@echo "debug    build the OS ISO image (+ filsystem) and run it in QEMU for debugging"
	@echo "deploy   build the OS ISO image (+ filsystem) and deploy it to the specified device"
	@echo "         Requires DEV to be defined (eg. DEV=/dev/sdb)"
	@echo "clean    clean up everything"
	@echo ""

.PHONY: run
run: vmm/sidechannel-VMM
	$(MAKE) -C guest
	$(MAKE) -C vmm run


.PHONY: clean
clean:
	$(MAKE) -C guests clean
	$(MAKE) -C vmm clean



