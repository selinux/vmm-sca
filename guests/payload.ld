SECTIONS
{
        .alice 0 : {
                vm_alice = .;
                vm_alice.bin
                vm_alice_end = .;
        }
        .charlie 0 : AT(LOADADDR(.alice)+SIZEOF(.vm_alice)) {
                vm_charlie = .;
                vm_charlie.bin
                vm_charlie_end = .;
        }
        .eve 0 : AT(LOADADDR(.charlie)+SIZEOF(.vm_charlie)) {
                vm_eve = .;
                vm_eve.bin
                vm_eve_end = .;
        }
}
