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

#define START_KERNEL_map 0xC0000000

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
	for (int i = 0; i < n_elts; i++)
		{
		if ( ( i % 4 ) == 0 ) printf( "\n %02X: ", i );
		unsigned long long	desc;
		read( fd, &desc, sizeof( desc ) );
		printf( " %016llX ", desc );
		}
	printf( "\n\n" );
}
