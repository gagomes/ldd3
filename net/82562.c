//-------------------------------------------------------------------
//
//	This module creates a pseudo-file named '/proc/82562' which
//	lets users inspect the Intel NIC's PCI Configuration Space.
//
//	ADDENDUM: We have now added character device-driver methods 
//	which would allow an unprivileged application to access the 
//	ethernet controller's internal state in a 'read-only' mode;
//	in particular, our 'fileview' utility can be used for this.
//
//	NOTE: Written and tested with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 05 FEB 2008
//	revised on: 07 FEB 2008 -- to add character-driver methods
//	revised on: 04 AUG 2008 -- for Linux kernel version 2.6.26
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/pci.h>		// for pci_get_device()
#include <linux/proc_fs.h>	// for create_proc_read_entry() 
#include <asm/uaccess.h>	// for copy_to_user()

#define	VENDOR_ID 0x8086	// Intel Corporation
#define DEVICE_ID 0x104C


char modname[] = "82562";	// the filename for our pseudo-file
int 	my_major = 82;		// character device major ID-number 
struct pci_dev	*devp;		// Linux-initialized data-structure
unsigned int	mmio_base;	// physical address for i/o-memory
unsigned int	mmio_size;	// size of controller's i/o-memory
void		*io;		// remapped address for i/o-memory 


loff_t my_llseek( struct file *file, loff_t pos, int whence )
{
	loff_t	newpos = -1;

	switch ( whence )
		{
		case 0:	newpos = pos; break;			// SEEK_SET
		case 1:	newpos = file->f_pos + pos; break;	// SEEK_CUR
		case 2: newpos = mmio_size + pos; break;	// SEEK_END
		}

	if (( newpos < 0 )||( newpos > mmio_size )) return -EINVAL;
	file->f_pos = newpos;
	return	newpos;
}


ssize_t my_read( struct file *file, char *buf, size_t count, loff_t *pos )
{
	unsigned int	fpos = (unsigned int)(*pos);
	int		nbytes = 0;
	int		regval;

	//--------------------------------------------------------
	// sanity checks: the registers must be accessed as
	// 32-bit values that reside at 4-byte aligned addresses
	//--------------------------------------------------------

	// we cannot return fewer than 4 bytes (unless at end-of-file)
	if ( count < 4 ) return -EINVAL;

	// we cannot read from a misaligned address
	if ( ( fpos % 4 ) != 0 ) return -EINVAL;

	// we cannot read anything past the end of the i/o-memory
	if ( fpos >= mmio_size ) return 0;

	// ok, transfer the integer at the current file-position
	regval = ioread32( io + fpos );
	if ( copy_to_user( buf, &regval, 4 ) ) return -EFAULT;
	 
	// advance file-pointer and notify kernel of bytes transferred
	nbytes += 4;
	*pos += nbytes;
	return	nbytes;
}


struct file_operations	my_fops = {
				  llseek:	my_llseek,
				  read:		my_read,
				  };



char legend1[] = "Intel 82562V Ethernet Interface";
char legend2[] = "PCI Configuration Space Header";

int my_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	int	i, len = 0;

	len += sprintf( buf+len, "\n  %s -- %s\n", legend1, legend2 );
	for (i = 0; i < 0x100; i+=4)
		{
		u32	datum;
		if ( (i % 32)==0 ) len += sprintf( buf+len, "\n %02X: ", i );
		pci_read_config_dword( devp, i, &datum );
		len += sprintf( buf+len, "%08X ", datum );
		}
	len += sprintf( buf+len, "\n\n" );
	return	len;
}


static int __init my_init( void )
{
	// write confirmation-message to kernel's log-file
	printk( "<1>\nInstalling \'%s\' module ", modname );
	printk( "(major=%d) \n", my_major );

	// detect the network interface device
	devp = pci_get_device( VENDOR_ID, DEVICE_ID, devp );
	if ( !devp ) return -ENODEV; 

	// remap the controller's i/o-memory into kernel-apace
	mmio_base = pci_resource_start( devp, 0 );
	mmio_size = pci_resource_len( devp, 0 );
	io = ioremap_nocache( mmio_base, mmio_size );
	if ( !io ) return -ENOSPC;

	// create our pseudo-file in the '/proc' directory
	create_proc_read_entry( modname, 0, NULL, my_info, NULL );
	return	register_chrdev( my_major, modname, &my_fops );	
}


static void __exit my_exit( void )
{
	// unregister our character device-driver
	unregister_chrdev( my_major, modname );

	// delete our pseudo-file from the '/proc' directory
	remove_proc_entry( modname, NULL );

	// write confirmation-message to kernel's log-file
	printk( "<1>Removing \'%s\' module\n", modname );
}

MODULE_LICENSE("GPL"); 
module_init( my_init );
module_exit( my_exit );
