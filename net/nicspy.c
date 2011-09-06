//-------------------------------------------------------------------
//	nicspy.c
//
//	This module implements character-mode device-driver services
//	used by our 'nicspy.cpp' application to show all the network
//	packets that the Intel Pro1000 ethernet controller receives.
//
//		compile using:  $ mmake nicspy.c
//		install using:  $ /sbin/insmod nicspy.ko
//
//	NOTE: Written and tested with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 12 FEB 2008
//	revised on: 14 FEB 2008 -- modified the 'read()' algorithm
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>
#include <linux/pci.h>		// for pci_get_device()
#include <linux/interrupt.h>	// for request_irq()
#include <asm/uaccess.h>	// for virt_to_phys()
#include <linux/sched.h>
#include <linux/delay.h>

#define VENDOR_ID	0x8086	// Intel Corporation
#define DEVICE_ID	0x104C
#define N_RX_DESC	    16	// number of Rx descriptors
#define KMEM_SIZE	0x8000	// memory allocation
#define INTR_MASK   0xFFFFFFFF	// interrupt mask

enum	{
	E1000_CTRL	= 0x0000,	// Device Control
	E1000_STATUS	= 0x0008,	// Device Status
	E1000_ICR	= 0x00C0,	// Interrupt Cause Read
	E1000_IMS	= 0x00D0,	// Interrupt Mask Set
	E1000_IMC	= 0x00D8,	// Interrupt Mask Clear
	E1000_RCTL	= 0x0100,	// Receive Control
	E1000_RDBAL	= 0x2800,	// Rx Descriptor Base-Address Low
	E1000_RDBAH	= 0x2804,	// Rx Descriptor Base-Address High
	E1000_RDLEN	= 0x2808,	// Rx Descriptor-queue Length
	E1000_RDH  	= 0x2810,	// Rx Descriptor-queue Head
	E1000_RDT  	= 0x2818,	// Rx Descriptor-queue Tail
	E1000_RXDCTL	= 0x2828,	// Rx Descriptor-queue Control
	E1000_TPR	= 0x40D0,	// Total Packets Received
	E1000_RA	= 0x5400,	// Receive-filter Array
	};	


typedef struct	{
		unsigned long long	base_address;
		unsigned short		packet_length;
		unsigned short		packet_chksum;
		unsigned char		desc_status;
		unsigned char		desc_errors;
		unsigned short		vlan_tag;
		} RX_DESCRIPTOR;

char modname[] = "nicspy";
char devname[] = "nic";
int	my_major = 97;
struct pci_dev	*devp;
unsigned int	mmio_base;
unsigned int	mmio_size;
void		*io, *kmem;
unsigned int	kmem_phys;
RX_DESCRIPTOR	*rxring;
unsigned int	rx_curr;
wait_queue_head_t wq_rx;
int		irq;
unsigned char	mac[ 6 ];
unsigned int	n_rx_pkts;


irqreturn_t my_isr( int irq, void *dev_id )
{
	static int	reps = 0;
	int		intr_cause = ioread32( io + E1000_ICR );

	if ( intr_cause == 0 ) return IRQ_NONE;

	printk( "NICSPY #%d: interrupt_cause=%08X ", ++reps, intr_cause );
	printk( "\n" );

	if ( intr_cause & (1<<7) ) wake_up_interruptible( &wq_rx );

	iowrite32( intr_cause, io + E1000_ICR );
	return	IRQ_HANDLED;
}



ssize_t my_read( struct file *file, char *buf, size_t len, loff_t *pos )
{
	void	*from = phys_to_virt( rxring[ rx_curr ].base_address );
	int	count;

	if ( rxring[ rx_curr ].desc_status == 0 )
		if ( wait_event_interruptible( wq_rx,
			rxring[ rx_curr ].desc_status != 0 ) ) return -EINTR;	

	count = rxring[ rx_curr ].packet_length;
	if ( copy_to_user( buf, from, count ) ) return -EFAULT;

	rxring[ rx_curr ].desc_status = 0;
	rx_curr = (1 + rx_curr) % N_RX_DESC;
	
	return	count;
}




long my_ioctl( struct file *filp, unsigned int request, unsigned long address )
{
	unsigned char	*where = (unsigned char *)address;

	switch ( request )
		{
		case 1:	// get network controller's hardware MAC-address
			if ( copy_to_user( where, mac, 6 ) ) return -EFAULT;
			return	0;  // SUCCESS
		}
	return	-EINVAL;
}


struct file_operations	my_fops = {
				  owner:	THIS_MODULE,
				  read:		my_read,
				  unlocked_ioctl:	my_ioctl,
				  };


int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	int	head, tail, i, len = 0;

	head = ioread32( io + E1000_RDH );
	tail = ioread32( io + E1000_RDT );
	n_rx_pkts += ioread32( io + E1000_TPR );

	len += sprintf( buf+len, "\n  Receive-Descriptor Buffer-Area " );
	len += sprintf( buf+len, "(head=%d, tail=%d) \n", head, tail );
	for (i = 0; i < N_RX_DESC; i++)
		{
		int	status = rxring[i].desc_status;
		len += sprintf( buf+len, "\n  #%-2d ", i );
		len += sprintf( buf+len, "%08lX: ", (long)(rxring + i) );
		len += sprintf( buf+len, "%016llX ", rxring[i].base_address );
		len += sprintf( buf+len, "%04X ", rxring[i].packet_length );
		len += sprintf( buf+len, "%04X ", rxring[i].packet_chksum );
		len += sprintf( buf+len, "%02X ", rxring[i].desc_status );
		len += sprintf( buf+len, "%02X ", rxring[i].desc_errors );
		len += sprintf( buf+len, "%04X ", rxring[i].vlan_tag );
		if ( status & (1<<0) ) len += sprintf( buf+len, "DD " );
		if ( status & (1<<1) ) len += sprintf( buf+len, "EOP " );
		if ( status & (1<<2) ) len += sprintf( buf+len, "IXSM " );
		if ( status & (1<<3) ) len += sprintf( buf+len, "VP " );
		if ( status & (1<<5) ) len += sprintf( buf+len, "TCPCS " );
		if ( status & (1<<6) ) len += sprintf( buf+len, "IPCS " );
		if ( status & (1<<7) ) len += sprintf( buf+len, "PIF " );
		}
	len += sprintf( buf+len, "\n\n" );
	len += sprintf( buf+len, "  Total packets received: %d ", n_rx_pkts );
	len += sprintf( buf+len, "\n\n" );
	return	len;
}


static int __init e1000spy_init( void )
{
	int		i, rx_control;
	u16		pci_cmd;

	devp = pci_get_device( VENDOR_ID, DEVICE_ID, NULL );
	if ( !devp ) return -ENODEV;

	mmio_base = pci_resource_start( devp, 0 );
	mmio_size = pci_resource_len( devp, 0 );
	io = ioremap_nocache( mmio_base, mmio_size );
	if ( !io ) return -ENOSPC;

	kmem = kzalloc( KMEM_SIZE, GFP_KERNEL );
	if ( !kmem ) { iounmap( io ); return -ENOMEM; }
	kmem_phys = virt_to_phys( kmem );

	init_waitqueue_head( &wq_rx );

	memcpy( mac, io + E1000_RA, 6 );

	pci_read_config_word( devp, 4, &pci_cmd );
	pci_cmd |= (1<<2);
	pci_write_config_word( devp, 4, pci_cmd );

	rxring = (RX_DESCRIPTOR *)phys_to_virt( kmem_phys );
	for (i = 0; i < N_RX_DESC; i++)
		{
		rxring[ i ].base_address = kmem_phys + 0x1000 + i*0x600;
		rxring[ i ].packet_length = 0;
		rxring[ i ].packet_chksum = 0; 
		rxring[ i ].desc_status = 0;
		rxring[ i ].desc_errors = 0;
		rxring[ i ].vlan_tag = 0;
		} 

	irq = devp->irq;
	printk( "<1>\nInstalling \'%s\' module ", modname );
	printk( "(major=%d, irq=%d)\n", my_major, irq );

	if ( request_irq( irq, my_isr, IRQF_SHARED, modname, &modname ) < 0 )
		{ kfree( kmem ); iounmap( io ); return -EBUSY; }	

	iowrite32( 0xFFFFFFFF, io + E1000_IMC );
	iowrite32( 0x00000000, io + E1000_STATUS );
	iowrite32( 0x040C0241, io + E1000_CTRL );
	iowrite32( 0x000C0241, io + E1000_CTRL );
	udelay( 10000 );

	rx_control = 0;
	rx_control |= (0<<1);	// EN-bit (Enable) 
	rx_control |= (1<<2);	// SBP-bit (Store Bad Packets)
	rx_control |= (1<<3);	// UPE-bit (Unicast Promiscuous Enable)
	rx_control |= (1<<4);	// MPE-bit (Multicast Promiscuous Enable)
	rx_control |= (1<<15);	// BAM-bit (Broadcast Address Mode)
	iowrite32( rx_control, io + E1000_RCTL );

	iowrite32(  kmem_phys, io + E1000_RDBAL );
	iowrite32( 0x00000000, io + E1000_RDBAH );
	iowrite32( N_RX_DESC * 16, io + E1000_RDLEN );
	iowrite32( 0x01000000, io + E1000_RXDCTL );

	iowrite32( N_RX_DESC, io + E1000_RDT );
	iowrite32( INTR_MASK, io + E1000_IMS );
	iowrite32( ioread32( io + E1000_RCTL ) | (1<<1), io + E1000_RCTL );

	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );
	return	register_chrdev( my_major, devname, &my_fops );
}


static void __exit e1000spy_exit(void )
{
	unregister_chrdev( my_major, devname );
	remove_proc_entry( modname, NULL );

	iowrite32( ioread32( io + E1000_RCTL ) & ~(1<<1), io + E1000_RCTL );
	iowrite32( 0xFFFFFFFF, io + E1000_IMC );
	free_irq( irq, modname );
	kfree( kmem );
	iounmap( io );

	printk( "<1>Removing \'%s\' module\n", modname );
}

module_init( e1000spy_init );
module_exit( e1000spy_exit );
MODULE_LICENSE("GPL"); 

