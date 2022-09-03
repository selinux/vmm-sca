#  Memory Deduplication, Cache-based Side-Channel Attacks

My master thesis project.

## abstract

"For several decades, Side-Channel Attacks (SCA) have been considered a potential
threat to information systems security. These attacks operate through a
multitude of physical channels such as power consumption, execution or response
time with Timing Attacks (TA) or access times to cached data with Cache-based
Side-Channel Attacks (CSCA). With platform virtualization, several guests
co-resident on the same host. Virtualization is supposed to guarantee strict
partitioning between VMs. A potential risk has been described via two types of
attacks ; one exploiting processor cache and the other memory deduplication. The
KVM hypervisor (Kernel-based Virtual Machine) implements a memory optimization
called KSM (Kernel Same-page Merging). When several VMs use the same content, it
is merged by KSM and provided to VMs. This optimization, would allow a malicious
VM to infer the state of another VM thus violating the partitioning principle.

I developed this tool using the KVM API and bare-metal VMs in order to quantify
this risk in real conditions and assess the scope of memory deduplication and
CSCA attacks on information systems. Our study takes into account how VT-x
behave on the x86_64 architecture and implementation of virtualization under
GNU\Linux."


[Cache-Based Side-Channel Attacks](pics/cache-inference.eps)


## VMM

One VMM lauch two or three VMs 

[architecture](pics/vmm-sca.eps)


### simulate different timelines

[timeline](pics/VM_timeline.pdf)

[VMM-timeline](pics/VMM_VMs_timeline.pdf)


## VMs

[VMs memory](pics/VM_memory_map.pdf)


## some measurements 

### READ

[Read access cache/no cache](pics/exp1-read-own.png)

### WRITE (Copy-On-Write)

[Write access](pics/exp4-write-own_and_shared_(COW).png)

### RDTSC three VMs

[Qemu rdtsc](pics/02-vm_counter_point_of_view_qemu_high_activitiy.png)


### Shared pages

[Qemu BIOS/UEFI](pics/exp8-qemu_BIOS_EFI_shared_pages.png)
[Qemu Alpine](pics/exp7-twoVM_shared_page_in_time.png)


(sources : Memory Deduplication, Cache-based Side-Channel Attacks, une menace réelle
en environnement virtualisé? Sebastien Chassot's Master's Thesis - Geneva University - 08.2022 )