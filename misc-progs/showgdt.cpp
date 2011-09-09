//-------------------------------------------------------------------
//	showgdt.cpp
//
//	This program uses our 'dram.c' device-driver to display the 
//	current processor's Global Descriptor Table in hex format.
//
//		compile using:  $ g++ showgdt.cpp -o showgdt
//		execute using:  $ ./showgdt
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
unsigned short 	gdtr[3];
unsigned long	gdt_virt_address; 
unsigned long	gdt_phys_address;

int main( int argc, char **argv )
{
	// use inline assembly language to get GDTR register-image
	asm(" sgdt gdtr ");

	// extract GDT virtual-address from GDTR register-image
	gdt_virt_address = *(unsigned long*)(gdtr+1);

	// compute GDT physical-address using subtraction
	gdt_phys_address = gdt_virt_address - START_KERNEL_map;

	// extract GDT segment-limit and compute descriptor count
	int	n_elts = (1 + gdtr[0])/8;

	// report the GDT virtual and physical memory-addresses 
	printf( "\n               " );
	printf( "gdt_virt_address=%08lX ", gdt_virt_address );
	printf( "gdt_phys_address=%08lX ", gdt_phys_address );
	printf( "\n" );

	// find, read and display the GDT's entries in device-memory
	int	fd = open( devname, O_RDONLY );
	if ( fd < 0 ) { perror( devname ); return -1; }
	lseek( fd, gdt_phys_address, SEEK_SET );
	for (int i = 0; i < n_elts; i++)
		{
		if ( ( i % 4 ) == 0 ) printf( "\n %04X: ", i*8 );
		unsigned long long	descriptor;
		read( fd, &descriptor, sizeof( descriptor ) );
		printf( "%016llX ", descriptor );
		}
	printf( "\n\n" );
}
