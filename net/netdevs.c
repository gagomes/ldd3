//-------------------------------------------------------------------
//	netdevs.c
//
//	This module create a pseudo-file named '/proc/netdevs' that 
//	displays a list of the current 'struct net_device' objects.
//
//	NOTE: Written and tested for Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 16 JAN 2008
//	revised on: 25 JAN 2008 -- to also show size of 'net_device'  
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_info_entry() 
#include <linux/netdevice.h>	// for 'struct net_device'
#include <asm/io.h>		// for virt_to_phys()

char modname[] = "netdevs";
char legend[] = "List of the kernel's current 'struct net_device' objects";
char header[] = "MemAddress   HardwareAddress  ifindex  Name";

int my_get_info( char *buf, char **start, off_t off, int count )
{
	struct net_device	*dev;
	int			i, len = 0;
	
	len += sprintf( buf+len, "\n %s\n\n %s\n", legend, header );
	for_each_netdev( dev ) 
		{
		unsigned long	phys_address = virt_to_phys( dev );
		unsigned char	*mac = dev->dev_addr;
		len += sprintf( buf+len, " 0x%08lX  ", phys_address );
		for (i = 0; i < 6; i++) 
		    len += sprintf( buf+len, "%02X%c", mac[i], (i<5)?':':' ' );
		len += sprintf( buf+len, " %3d   ", dev->ifindex ); 
		len += sprintf( buf+len, "  %s ", dev->name );
		len += sprintf( buf+len, "\n" );
		}
	len += sprintf( buf+len, "\n" );
	len += sprintf( buf+len, " sizeof( struct net_device )=" );
	len += sprintf( buf+len, "%d bytes\n\n", sizeof( struct net_device ) );
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

