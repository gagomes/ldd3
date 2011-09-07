//-------------------------------------------------------------------
//	seesetfs.c
//
//	This is a 'throw-away' module, quickly constructed in order 
//	to clarify the effects of the misleadingly-named 'set_fs()' 
//	macro-statements (used in our earlier 'tempcdev.c' module).  
//	It shows that there is no effect on the values in registers
//	FS and/or DS; instead, the value of a kernel-variable named
//	'addr_limit' is modified, as is discussed in Section 10.4.1
//	our "Understanding the Linux Kernel" textbook.
//
//	NOTE: Written and tested using Linux kernel version 2.6.22.
//
//	programmer: ALLAN CRUSE
//	written on: 31 AUG 2007
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <asm/uaccess.h>	// for get_ds(), set_fs()

static char modname[] = "seesetfs";
unsigned short	_ds, _fs;


static int __init my_init( void )
{
	struct thread_info	*ti = current_thread_info();
	unsigned long		address_limit;
	mm_segment_t		savedFS;
	
	printk( "<1>\nInstalling \'%s\' module\n", modname );

	address_limit = ti->addr_limit.seg;
	asm(" mov %ds, _ds \n mov %fs, _fs ");
	printk( "BEFORE:  ds=%04X fs=%04X ", _ds, _fs );
	printk( " addr_limit=%08lX \n", address_limit ); 

	savedFS = get_fs();
	set_fs( get_ds() );	

	address_limit = ti->addr_limit.seg;
	asm(" mov %ds, _ds \n mov %fs, _fs ");
	printk( " AFTER:  ds=%04X fs=%04X ", _ds, _fs );
	printk( " addr_limit=%08lX \n", address_limit ); 

	set_fs( savedFS );

	address_limit = ti->addr_limit.seg;
	asm(" mov %ds, _ds \n mov %fs, _fs ");
	printk( " FINAL:  ds=%04X fs=%04X ", _ds, _fs );
	printk( " addr_limit=%08lX \n", address_limit ); 

	return	0;  // SUCCESS
}


static void __exit my_exit( void )
{
	printk( "<1>Removing \'%s\' module\n", modname );
}

module_init( my_init );
module_exit( my_exit );
MODULE_LICENSE("GPL");

