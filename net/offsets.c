//-------------------------------------------------------------------
//	offsets.c
//
//	This example shows how the 'offsetof' operator can be used
//	in a module to create a pseudo-file showing the offsets of
//	member-fields within a kernel data-structure, in this case
//	offsets for a few members of a 'struct net_device' object.  
//
//	NOTE: written and tested with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 28 JAN 2008
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_info_entry() 
#include <linux/netdevice.h>	// for 'struct net_device'


char modname[] = "offsets";


int my_get_info( char *buf, char **start, off_t off, int count )
{
	int	o_name	= offsetof( struct net_device, name );
	int	o_ifindex = offsetof( struct net_device, ifindex ); 
	int	o_irq 	= offsetof( struct net_device, irq );
	int	o_priv	= offsetof( struct net_device, priv );
	int	o_open	= offsetof( struct net_device, open );
	int	o_stop	= offsetof( struct net_device, stop );
	int	o_dev_addr = offsetof( struct net_device, dev_addr );
	int	o_hwxmit = offsetof( struct net_device, hard_start_xmit );
	int	len = 0;

	len += sprintf( buf+len, "\n" );
	len += sprintf( buf+len, " Structure-member offsets in " );
	len += sprintf( buf+len, "\'struct net_device' objects " );
	len += sprintf( buf+len, "\n" );

	len += sprintf( buf+len, "\n" );
	len += sprintf( buf+len, "  offset    member  \n" );
	len += sprintf( buf+len, " -------------------\n" );

	len += sprintf( buf+len, "  0x%04X    name \n", o_name );
	len += sprintf( buf+len, "  0x%04X    ifindex \n", o_ifindex );
	len += sprintf( buf+len, "  0x%04X    dev_addr \n", o_dev_addr );
	len += sprintf( buf+len, "  0x%04X    irq \n", o_irq );
	len += sprintf( buf+len, "  0x%04X    priv \n", o_priv );
	len += sprintf( buf+len, "  0x%04X    open \n", o_open );
	len += sprintf( buf+len, "  0x%04X    stop \n", o_stop );
	len += sprintf( buf+len, "  0x%04X    hard_start_xmit \n", o_hwxmit );
	len += sprintf( buf+len, "\n" );

	return	len;
}


static int __init my_init( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );

	create_proc_info_entry( modname, 0, NULL, my_get_info );
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

