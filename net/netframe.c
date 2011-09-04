//-------------------------------------------------------------------
//	netframe.c
//
//	Here's a basic framework for a Linux ethernet device-driver.
//
//	NOTE: Written and tested with Linux kernel version 2.6.22.5.
//
//	programmer: ALLAN CRUSE
//	written on: 28 JAN 2008
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/etherdevice.h>	// for alloc_etherdev()
#include <linux/proc_fs.h>
#include <linux/interrupt.h>	// for request_irq(), free_irq()
#include <linux/wait.h>		// for init_wait_queue_head()
#include <linux/sched.h>

#define irqID		0x06	// temporary -- an unused IRQ from /proc/interrupts
#define intID		0x36	// The interrupt vector ID (the lowest 8 bits) in the redirection table entry from /proc/ioapic


typedef struct	{
		struct tasklet_struct	my_rxtasklet;
		wait_queue_head_t  	my_waitqueue;
		spinlock_t		my_lock;
		} MY_DRIVERDATA;


int my_open( struct net_device * );
int my_stop( struct net_device * );
int my_hard_start_xmit( struct sk_buff *, struct net_device * );
irqreturn_t my_isr( int, void * );
void my_rx_handler( unsigned long );


char modname[] = "netframe";
struct net_device  *netdev;


int my_get_info( char *buf, char **start, off_t off, int count, int *eof, void *data )
{
	int	len = 0;

	*start = buf;
	if ( off == 0 )
		{
		len += sprintf( buf+len, "simulating an interrupt " );
		len += sprintf( buf+len, "by \'%s\' \n", netdev->name  );
		off += len;
		}
	else	asm(" int %0 " : : "i" (intID) );

        *eof = 1;
	return	len;
}


static void __exit my_exit(void )
{
	unregister_netdev( netdev );
	free_netdev( netdev );
	remove_proc_entry( modname, NULL );

	printk( "<1>Removing \'%s\' module\n", modname );
}

int my_open( struct net_device *dev ) 
{ 
   MY_DRIVERDATA	*priv = netdev_priv(dev);
	unsigned long	devaddr = (unsigned long)dev;

	printk( "opening the \'%s\' interface \n", dev->name );

	spin_lock_init( &priv->my_lock );
	init_waitqueue_head( &priv->my_waitqueue );
	tasklet_init( &priv->my_rxtasklet, my_rx_handler, devaddr );

	if ( request_irq( dev->irq, my_isr, IRQF_SHARED, 
			dev->name, dev ) < 0 ) return -EBUSY;

	netif_start_queue( dev );
	return 0; 
}

int my_stop( struct net_device *dev ) 
{ 
   MY_DRIVERDATA	*priv = netdev_priv(dev);

	printk( "stopping the \'%s\' interface \n", dev->name );

	free_irq( dev->irq, dev );

	tasklet_kill( &priv->my_rxtasklet );
	netif_stop_queue( dev );
	return 0; 
}

int my_hard_start_xmit( struct sk_buff *skb, struct net_device *dev )
{
   MY_DRIVERDATA	*priv = netdev_priv(dev);

	printk( "starting transmit on the \'%s\' interface \n", dev->name );

	dev->trans_start = jiffies;
	dev->stats.tx_packets += 1;
	dev->stats.tx_bytes += skb->len;

	wait_event_interruptible( priv->my_waitqueue, 1 );

	dev_kfree_skb( skb );
	return 0;
}


irqreturn_t my_isr( int irq, void *data )
{
	struct net_device	*dev = (struct net_device*)data;
   MY_DRIVERDATA	*priv = netdev_priv(dev);
   printk("Got an interrupt\n");

	tasklet_schedule( &priv->my_rxtasklet );
	wake_up_interruptible( &priv->my_waitqueue );
	return	IRQ_HANDLED;
}


void my_rx_handler( unsigned long data )
{
	struct net_device	*dev = (struct net_device *)data;
	struct sk_buff		*skb;
	int			rxbytes = 60;

	skb = dev_alloc_skb( rxbytes + 2 );
	skb->dev = dev;
	skb->protocol = eth_type_trans( skb, dev );
	skb->ip_summed = CHECKSUM_NONE;
	dev->stats.rx_packets += 1;
	dev->stats.rx_bytes += rxbytes;
	netif_rx( skb );
}


static const struct net_device_ops my_netdev_ops = {
	.ndo_open            = my_open,
	.ndo_stop            = my_stop,
	.ndo_start_xmit      = my_hard_start_xmit,
};

static int __init my_init( void )
{
	printk( "<1>\nInstalling \'%s\' module\n", modname );
	create_proc_read_entry( modname, 0, NULL, my_get_info, NULL );

	netdev = alloc_etherdev( sizeof( MY_DRIVERDATA ) );
	if ( !netdev ) return -ENOMEM;

	netdev->irq		= irqID;
	netdev->netdev_ops	= &my_netdev_ops;

	return	register_netdev( netdev );
}

module_init( my_init );
module_exit( my_exit );
MODULE_LICENSE("GPL"); 
