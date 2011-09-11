//-------------------------------------------------------------------
//	ioapic.c
//
//	This module creates a pseudo-file (named '/proc/ioapic') 
//	which allows users to view the contents of the registers
//	that belong to the memory-mapped I/O APIC. 
//
//	NOTE: Written and tested with Linux kernel version 2.6.22.
//
//	programmer: ALLAN CRUSE
//	written on: 25 SEP 2007
//	revised on: 26 SEP 2007 -- to show title and register-names
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module(), printk()
#include <linux/proc_fs.h>
#include <asm/io.h>		// for ioremap(), iounmap()

#define IOAPIC_BASE 0xFEC00000

char modname[] = "ioapic";

int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	void	*io = ioremap_nocache( IOAPIC_BASE, PAGE_SIZE );
	void	*to = (void*)((long)io + 0x00);
	void	*fr = (void*)((long)io + 0x10);
	int	i, maxirq, ident, versn, len = 0;

	iowrite32( 0, to ); ident = ioread32( fr );
	iowrite32( 1, to ); versn = ioread32( fr );
	maxirq = ( versn >> 16 )&0x00FF;

	len += sprintf( buf+len, "\n  I/O APIC       " );
	len += sprintf( buf+len, " Identification: %08X  ", ident );
	len += sprintf( buf+len, "     Version: %08X   \n", versn );
	len += sprintf( buf+len, "\n%25s", " " );
	len += sprintf( buf+len, "%d Redirection-Table entries \n", 1+maxirq );

	for (i = 0; i <= maxirq; i++)
		{
		unsigned int	val_lo, val_hi;
		iowrite32( 0x10 + 2*i, to );
		val_lo = ioread32( fr );
		iowrite32( 0x10+2*i+1, to );
		val_hi = ioread32( fr );
		if ( ( i % 3 ) == 0 ) len += sprintf( buf+len, "\n" );
		len += sprintf( buf+len, "  0x%02X: ", i );
		len += sprintf( buf+len, "%08X%08X  ", val_hi, val_lo );
		}
	len += sprintf( buf+len, "\n\n" );
	
	iounmap( io );
        *eof = 1;
	return	len;  
}


static int __init ioapic_init( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );
	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );
	return	0; // SUCCESS
}


static void __exit ioapic_exit( void )
{
	printk( "<1>Removing \'%s\' module\n", modname );
	remove_proc_entry( modname, NULL );
}

module_init( ioapic_init );
module_exit( ioapic_exit );
MODULE_LICENSE("GPL");

