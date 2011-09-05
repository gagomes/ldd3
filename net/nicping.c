//-------------------------------------------------------------------
//	nicping.c
//
//	This module shows a sequence of programming steps that allow
//	our Pro1000 network controller to transmit ethernet packets;
//	transmission is triggered each time the pseudo-file is read.
//
//	NOTE: Written and tested with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 06 FEB 2008
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_info_entry() 
#include <linux/utsname.h>	// for utsname()
#include <linux/pci.h>		// for pci_get_device()
#include <linux/delay.h>

#define VENDOR_ID	0x8086	// Intel Corporation
#define DEVICE_ID	0x104C
#define KMEM_SIZE	0x1000	// size of allocation

enum	{
	E1000_CTRL 	= 0x0000,	// Device Control
	E1000_STATUS	= 0x0008,	// Device Status
	E1000_IMC	= 0x00D8,	// Interrupt Mask Clear
	E1000_TCTL	= 0x0400,	// Transmit Control
	E1000_TDBAL	= 0x3800,	// Tx-Descriptor Base-Address Low
	E1000_TDBAH	= 0x3804,	// Tx-Descriptor Base-Address High
	E1000_TDLEN	= 0x3808,	// Tx-Descriptor queue Length
	E1000_TDH	= 0x3810,	// Tx-Descriptor queue Head
	E1000_TDT	= 0x3818,	// Tx-Descriptor queue Tail
	E1000_RA	= 0x5400,	// Receive-address Array
	};

typedef struct 	{
		unsigned long long	base_address;
		unsigned short		packet_length;
		unsigned char		cksum_offset;
		unsigned char		desc_command;
		unsigned char		desc_status;
		unsigned char		cksum_origin;
		unsigned short		special_info;
		} TX_DESCRIPTOR;

char modname[] = "nicping";
unsigned char	cmos[ 10 ];
unsigned char	mac[ 6 ];
struct pci_dev		*devp;
struct new_utsname	*uts;
unsigned int	mmio_base;
unsigned int	mmio_size;
void		*io, *kmem;
unsigned int	kmem_phys;
TX_DESCRIPTOR	*txring;


char	*month[ 13 ] = { " ", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	unsigned char	*cp = (unsigned char *)kmem;
	int	i, ss, mm, hh, dd, mh, yy, tail, len = 0;

	// we do not want re-entry to trigger a second transmission  
	*start = buf; if ( off == 0 ) ++off; else return 0;

	// input the current date and time from the Real-Time Clock
	for (i = 0; i < 10; i++) 
		{
		outb( i, 0x70 );
		cmos[ i ] = inb( 0x71 );
		}

	// convert the Real-Time Clock's entries from BDC to binary	
	ss = (cmos[ 0 ] & 0x0F) + 10 * ((cmos[ 0 ]>>4) & 0x0F);
	mm = (cmos[ 2 ] & 0x0F) + 10 * ((cmos[ 2 ]>>4) & 0x0F);
	hh = (cmos[ 4 ] & 0x0F) + 10 * ((cmos[ 4 ]>>4) & 0x0F);
	mh = (cmos[ 8 ] & 0x0F) + 10 * ((cmos[ 8 ]>>4) & 0x0F);
	dd = (cmos[ 7 ] & 0x0F) + 10 * ((cmos[ 7 ]>>4) & 0x0F);
	yy = (cmos[ 9 ] & 0x0F) + 10 * ((cmos[ 9 ]>>4) & 0x0F);

	// form a verification message we can easily recognize as unique
        len += sprintf( buf+len, " MAC: %pM\n", mac);
	len += sprintf( buf+len, " Hello from station \'%s\'", uts->nodename );
	len += sprintf( buf+len, " at %d:%02d:%02d", hh, mm, ss );
	len += sprintf( buf+len, " on %02d %s 20%02d", dd, month[ mh ], yy );
	len += sprintf( buf+len, "\n" );

	// setup an ethernet packet in our Transmit Buffer
	memset( cp+0, 0xFF, 6 );	// broadcast address
	memcpy( cp+6, mac, 6 );		// source HW-address
	cp[12] = 8;			// type-code LSB
	cp[13] = 0;			// type-code MSB
	*(short*)(cp+14) = len;		// message's length
	memcpy( cp+16, buf, len );	// copy message to packet-buffer

	// start a packet-transmission by giving descriptor-ownership to NIC
	tail = ioread32( io + E1000_TDT );	// current tail-descriptor
	txring[ tail ].packet_length = 16+len; 	// setup packet's length
	txring[ tail ].desc_status = 0;		// clear descriptor-status
	tail = (1 + tail) % 8;			// next ring-buffer index
	iowrite32( tail, io + E1000_TDT );	// give descriptor to NIC

	return	len;			// length of pseudo-file's contents
}



static int __init nicping_init( void )
{
	int	i, tx_control;
	u16	pci_cmd;

	printk( "<1>\nInstalling \'%s\' module\n", modname );

	// detect the Intel Pro1000 gigabit ethernet controller
	devp = pci_get_device( VENDOR_ID, DEVICE_ID, NULL );
	if ( !devp ) return -ENODEV;

	// remap the controller's i/o-memory into kernel-space
	mmio_base = pci_resource_start( devp, 0 );
	mmio_size = pci_resource_len( devp, 0 );
	io = ioremap_nocache( mmio_base, mmio_size );
	if ( !io ) return  -ENOSPC;

	// allocate kernel memory for the network controller to use    
	kmem = kzalloc( KMEM_SIZE, GFP_KERNEL );
	if ( !kmem ) { iounmap( io ); return -ENOMEM; }
	kmem_phys = virt_to_phys( kmem );

	// get this station's node-name for insertion in our pseudo-file
	uts = utsname();

	// copy the controller's unique Hardware MAC-address to 'mac[]' 
	memcpy( mac, io + E1000_RA, 6 );

	// reset the network controller's internal state
	iowrite32( 0x00000000, io + E1000_STATUS );
	iowrite32( 0xFFFFFFFF, io + E1000_IMC );
	iowrite32( 0x040C0241, io + E1000_CTRL );
	iowrite32( 0x000C0241, io + E1000_CTRL );
	udelay( 10000 );

	// initialize the controller's queue of Tx-descriptors
	txring = phys_to_virt( kmem_phys + 0x600 );
	for (i = 0; i < 8; i++)
		{
		txring[ i ].base_address = kmem_phys;
		txring[ i ].packet_length = 0;
		txring[ i ].cksum_offset = 0;
		txring[ i ].desc_command = (1<<0)|(1<<1)|(1<<3); // EOP,IFCS,RS
		txring[ i ].desc_status = 0;
		txring[ i ].cksum_origin = 0;
		txring[ i ].special_info = 0;
		}

	// configure the controller's 'Transmit' engine
	tx_control = 0;
	tx_control |= (0<<1);	// EN-bit (Enable)
	tx_control |= (1<<3);	// PSP-bit (Pad Short Packets)
	tx_control |= (15<<4);	// CT (Collision Threshold)
	tx_control |= (63<<12);	// COLD (Collision Distance)
	tx_control |= (1<<24);	// RTLC (Re-Transmit on Late Collisions)
	iowrite32( tx_control, io + E1000_TCTL );	

	// tell the controller where to find its Tx-Descriptors
	iowrite32( kmem_phys + 0x600, io + E1000_TDBAL );
	iowrite32( 0x00000000, io + E1000_TDBAH );
	iowrite32( 0x00000080, io + E1000_TDLEN );
	
	// insure that the controller's Bus Master capability is enabled
	pci_read_config_word( devp, 4, &pci_cmd );
	pci_cmd |= (1<<2);
	pci_write_config_word( devp, 4, pci_cmd );

	// start the controller's 'Transmit' engine
	tx_control |= (1<<1);	// EN-bit
	iowrite32( tx_control, io + E1000_TCTL );

	// install our pseudo-file in the '/proc' directory
	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );
	return	0;  //SUCCESS
}


static void __exit nicping_exit(void )
{
	// delete pseudo-file from the '/proc' directory
	remove_proc_entry( modname, NULL );

	// release this module's allocation of kernel memory
	kfree( kmem );

	// unmap the controller's i/o-memory from kernel space
	iounmap( io );

	printk( "<1>Removing \'%s\' module\n", modname );
}

module_init( nicping_init );
module_exit( nicping_exit );
MODULE_LICENSE("GPL"); 

