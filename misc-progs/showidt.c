//-------------------------------------------------------------------
//	showidt.cpp
//
//	This program uses our 'dram.c' device-driver to display all
//	the entries in this processor's Interrupt Descriptor Table.
//
//		compile using:  $ g++ showidt.cpp -o showidt
//		execute using:  $ ./showidt
//
//	NOTE: Our 'dram.ko' module must be installed in the kernel.
//
//	programmer: ALLAN CRUSE
//	written on: 18 MAR 2006
//-------------------------------------------------------------------

#include <stdio.h>	// for printf(), perror() 
#include <fcntl.h>	// for open() 
#include <unistd.h>	// for read()
#include <stdint.h> 

#define START_KERNEL_map 0xC0000000

// http://wiki.osdev.org/Interrupt_Descriptor_Table
// Intel's internal faults are initialized in arch/x86/kernel/traps.c,
// and external IRQs are initialized in irqinit.c, with the entry stubs set up in entry_32.S.
struct IDTDescr{
   uint16_t offset_1; // offset bits 0..15
   uint16_t selector; // a code segment selector in GDT or LDT
   uint8_t zero;      // unused, set to 0
   uint8_t type_attr; // type and attributes, see below
   uint16_t offset_2; // offset bits 16..31
};

char devname[] = "/dev/dram";
unsigned short 	idtr[3];
unsigned long	idt_virt_address;
unsigned long	idt_phys_address;

int main( int argc, char **argv )
{
	// use inline assembly language to get IDTR register-image
	asm(" sidt idtr ");

	// extract IDT virtual-address from IDTR register-image
	idt_virt_address = *(unsigned long*)(idtr+1);

	// compute IDT physical-address using subtraction
	idt_phys_address = idt_virt_address - START_KERNEL_map;

	// extract IDT segment-limit and compute descriptor-count
	int	n_elts = (1 + idtr[0])/8;

	// report the IDT virtual and physical memory-addresses
	printf( "\n               " );
	printf( "idt_virt_address=%08lX  ", idt_virt_address );
	printf( "idt_phys_address=%08lX  ", idt_phys_address );
	printf( "\n" );

	// find, read, and display the IDT's entries in device-memory
	int	fd = open( devname, O_RDONLY );
	if ( fd < 0 ) { perror( devname ); return -1; }
	lseek( fd, idt_phys_address, SEEK_SET );
        int i = 0;
	for (; i < n_elts; i++)
		{
		printf( "\n%02X: ", i );
		struct IDTDescr desc;
		read( fd, &desc, sizeof( desc ) );
                uint32_t offset = (desc.offset_2 << 16) | desc.offset_1;
                // __KERNEL_CS selector is set to 0x60 in asm/segment.h.
                // Look up offset in /proc/kallsyms we can find trap handlers and irq_entries_start.
		printf( " %08X %X %X", offset, desc.selector, desc.type_attr );
		}
	printf( "\n\n" );
        return 0;
}
