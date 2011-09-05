//-------------------------------------------------------------------
//	nicstate.c
//
//	This module illustrates the two mechanisms which the Intel 
//	Pro1000 Gigabit Ethernet controller provides for accessing 
//	its numerous internal 32-bit control and status registers.
//	Here the NIC's 'Device Status' register is accessed by the
//	two different mechanisms (so the results can be compared). 
//
//	NOTE: Written and tested for Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 05 FEB 2008
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/pci.h>		// for pci_get_device()
#include <linux/proc_fs.h>

#define VENDOR_ID	0x8086	// Intel Corporation
#define DEVICE_ID	0x104C

enum {	E1000_STATUS	= 0x0008, };

char modname[] = "nicstate";
struct pci_dev	*devp;
unsigned int	mmio_base;
unsigned int	mmio_size;
unsigned int	ioport;
void		*io;

int my_get_info( char *, char **, off_t, int, int *, void * );

static int __init nicstate_init( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );

	// detect the Intel Pro/1000 Network Controller
	devp = pci_get_device( VENDOR_ID, DEVICE_ID, NULL );
	if ( !devp ) return -ENODEV;

	// map the device's i/o-memory into kernel-space 
	mmio_base = pci_resource_start( devp, 0 );
	mmio_size = pci_resource_len( devp, 0 );
	io = ioremap_nocache( mmio_base, mmio_size );
	if ( !io ) return -ENOSPC;
	
	// obtain the device's base i/o-space port-address
	ioport = pci_resource_start( devp, 2 );

	// create our pseudo-file in the '/proc' directory
	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );
	return	0;  //SUCCESS
}


char	*link[] = { "DOWN", "UP" };
char	*duplex[] = { "HALF", "FULL" };
char	*speed[] = { "10Mbps", "100Mbps", "1000Mbps", "---" };

unsigned long long  stamp1;
unsigned long long  stamp2;
unsigned long long  cycles;

int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	int	status1, status2, len = 0;

	// disable interrupts and save TimeStamp Counter's initial value
	asm(" cli \n rdtsc ");
	asm(" mov %%eax, stamp1 \n mov %%edx, stamp1+4 " : : : "ax", "dx" );

	// input the current status via the i/o-space port-address
	outl( E1000_STATUS, ioport );	// select register
	status1 = inl( ioport + 4 );	// obtain contents

	// save TimeStamp Counter's final value and reenable interrupts
	asm(" rdtsc \n sti ");
	asm(" mov %%eax, stamp2 \n mov %%edx, stamp2+4 " : : : "ax", "dx" );

	// subtract to get the number of elapsed cpu clock-cycles
	cycles = stamp2 - stamp1;

	// display the number of elapsed cpu clock-cycles
	len += sprintf( buf+len, "\n Elapsed cpu-cycles = %llu\n", cycles );
	// display results based on mechanism #1
	len += sprintf( buf+len, " STATUS: 0x%08X  ", status1 );
	len += sprintf( buf+len, "speed=%s  ", speed[ (status1>>6)&3 ] );
	len += sprintf( buf+len, "duplex=%s  ", duplex[ (status1>>0)&1 ] );
	len += sprintf( buf+len, "link=%s  ", link[ (status1>>1)&1 ] );

	asm(" cli \n rdtsc ");
	asm(" mov %%eax, stamp1 \n mov %%edx, stamp1+4 " : : : "ax", "dx" );
	// input the current status via the i/o-memory mapping
	status2 = ioread32( io + E1000_STATUS );
	asm(" rdtsc \n sti ");
	asm(" mov %%eax, stamp2 \n mov %%edx, stamp2+4 " : : : "ax", "dx" );
	cycles = stamp2 - stamp1;
	len += sprintf( buf+len, "\n\n Elapsed cpu-cycles = %llu\n", cycles );

	// display results based on mechanism #2
	len += sprintf( buf+len, " STATUS: 0x%08X  ", status2 );
	len += sprintf( buf+len, "speed=%s  ", speed[ (status2>>6)&3 ] );
	len += sprintf( buf+len, "duplex=%s  ", duplex[ (status2>>0)&1 ] );
	len += sprintf( buf+len, "link=%s  ", link[ (status2>>1)&1 ] );

	len += sprintf( buf+len, "\n\n" );
	return	len;
}


static void __exit nicstate_exit(void )
{
	// delete our pseudo-file from the '/proc' directory
	remove_proc_entry( modname, NULL );

	// unmap the device's i/o-memory from kernel-space
	iounmap( io );

	printk( "<1>Removing \'%s\' module\n", modname );
}

module_init( nicstate_init );
module_exit( nicstate_exit );
MODULE_LICENSE("GPL"); 

