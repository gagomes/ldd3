//-------------------------------------------------------------------
//	pgdir.c
//
//	This module implements a 'visualization' of the processor's 
//	4-GB virtual address space under Linux, based on attributes 
//	of the current page-directory's entries.  (Accordingly, the
//	granularity of the data depicted here is 4-MB.)  Frames not
//	presently 'mapped' are shown by '-', while frames which are
//	mapped are marked by a character ('3' or '7') based on bits
//	from the corresponding page-directory entry's attributes.        
//
//	NOTE: Written and tesed with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 04 OCT 2007
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>
#include <asm/io.h>		// for phys_to_virt()

char modname[] = "pgdir";

int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	unsigned long	ptdb, *pgdir;
	int		i, k, attr, len = 0;

	// inline assembly-language is used here to access register CR3
	asm(" mov %%cr3, %%eax \n mov %%eax, %0 " : "=m" (ptdb) :: "ax" );
	pgdir = phys_to_virt( ptdb );

	// title for the visualization
	len += sprintf( buf+len, "\n\n    A visualization of the CPU's" );
	len += sprintf( buf+len, " Virtual Address Space under Linux \n" );

	// loop through page-directory entries (from highest to lowest)
	for (i = 0; i < 1024; i++)
		{
		k = 1023 - i;
		attr = (pgdir[ k ] & 7) | '0';
		if ( attr == '0' ) attr = '-';
		if ( ( i % 64 ) == 0 ) len += sprintf( buf+len, "\n   " );
		len += sprintf( buf+len, "%c", attr );
		if ( ( i % 64 ) == 63 ) 
			len += sprintf( buf+len, "  %08X ", k << 22 );
		}

	// display explanatory legend (plus some extra information)
	len += sprintf( buf+len, "\n\n\n    Legend:  " );
	len += sprintf( buf+len, "'-'=unmapped  " ); 	
	len += sprintf( buf+len, "'3'=supervisor  " ); 	
	len += sprintf( buf+len, "'7'=user  " ); 
	len += sprintf( buf+len, "         (CR3=%08lX) \n\n", ptdb );
	return	len;
}


static int __init my_init( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );

	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );
	return	0;  //SUCCESS
}


static void __exit my_exit(void )
{
	remove_proc_entry( modname, NULL );

	printk( "<1>Removing \'%s\' module\n", modname );
}

module_init( my_init );
module_exit( my_exit );
MODULE_LICENSE("GPL"); 

