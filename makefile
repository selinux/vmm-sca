QEMU=qemu-system-x86_64 -enable-kvm -m 128
ISO_NAME=tools/bare_metal_guest.iso
EXP=experiments/exp0.dat experiments/exp1.dat experiments/exp2.dat experiments/exp3.dat experiments/exp4.dat

help:
	@echo -e "Available targets:\n"
	@echo -e "all :\t\t\t\t\tbuild the VMM, guest VMs and run tests\n"
	@echo -e "enable_ksm :\t\t\t(need root privilege)\n"
	@echo -e "update ctags :\n"
	@echo -e "test_pages_sharing :\trun same VM twice with qemu to test KSM pages sharing\n"
	@echo -e "clean :\t\t\t\t\tclean up everything\n"

.PHONY: all
all: testbench.dat
	$(MAKE) -C guests all
	$(MAKE) -C vmm run
	@python tools/print_measures.py -f results/output.dat

testbench.dat: tools/gen_measures.py
	@echo generate test_bench commands
	@python tools/gen_measures.py -f $@

update_version:
	./gen_version.sh version.h

print_measures:
	@python tools/print_measures.py results/output.dat

experiments: $(EXP)

experiments/exp0.dat:
	@python tools/gen_measures.py $@ 0

experiments/exp1.dat:
	@python tools/gen_measures.py $@ 1

experiments/exp2.dat:
	@python tools/gen_measures.py $@ 2

experiments/exp3.dat:
	@python tools/gen_measures.py $@ 3

experiments/exp4.dat:
	@python tools/gen_measures.py $@ 4

experiments/exp5.dat:
	@python tools/gen_measures.py $@ 5

.PHONY: clean
clean:
	@rm -f test_bench.dat
	@rm -f $(EXP)
	$(MAKE) -C guests clean
	$(MAKE) -C vmm clean

tags:
	@ctags -R

enable_ksm:
	echo 1 | sudo tee /sys/kernel/mm/ksm/run

test_pages_sharing: $(ISO_NAME)
	@$(QEMU) -cdrom $< &
	@$(QEMU) -cdrom $< &
	@echo pages shared at beginning : `cat /sys/kernel/mm/ksm/pages_shared`
	@echo sleep 4s ; sleep 4
	@echo pages shared after 4s : `cat /sys/kernel/mm/ksm/pages_shared`
	@echo sleep 4s ; sleep 4
	@echo pages shared after 8s : `cat /sys/kernel/mm/ksm/pages_shared`

