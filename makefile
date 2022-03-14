
help:
	@echo -e "Available targets:\n"
	@echo -e "all\t\t\t\t build the VMM, guest VMs and run tests\n"
	@echo -e "enable_ksm (need root privilege)\t run same VM twice to test KSM pages sharing\n"
	@echo -e "clean\t\t\t clean up everything\n"

.PHONY: all
all:
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

