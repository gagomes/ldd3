//-------------------------------------------------------------------
//	smpwatch.c
//
//	This module creates a pseudo-file named '/proc/smpwatch'
//	that is both readable and writable by user applications.
//	That file consists of an array of counters corresponding
//	to the 256 possible interrupts on each of two processors
//	(on a dual-CPU platform), each counter being incremented
//	each time its associated interrupt-handler gets invoked.
//	The entire array gets reinitialized by any attempt to do
//	either 'open' or 'write' operations on this pseudo-file.
//
//	NOTE1: This module implicitly assumes that Linux has put
//	the same value into the IDTR register of each processor.
//
//	NOTE2: Written and tested with kernel version 2.6.27.10. 
//
//	programmer: ALLAN CRUSE
//	written on: 14 SEP 2009
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_entry()
#include <asm/uaccess.h>	// for copy_to_user()
#include <asm/io.h>		// for ioremap()

#define PROC_DIR	NULL	// for default directory
#define PROC_MODE	0666	// for access-permissions
#define LAPICBASE  0xFEE00000	// physical address of Local-APIC


typedef struct	{ unsigned int	lo, hi; } GATE_DESCRIPTOR;

char modname[] = "smpwatch";
unsigned short	oldidtr[3], newidtr[3];
GATE_DESCRIPTOR	*oldidt, *newidt;
void 		*kmem, *lapic;
unsigned int	original_isr[ 256 ];
unsigned int	n_interrupts[ 256 * 2 ];
const int filesize = sizeof( n_interrupts );

ssize_t
my_write( struct file *file, const char* buf, size_t len, loff_t *pos ),
my_read( struct file *file, char *buf, size_t len, loff_t *pos );
int my_open( struct inode *inode, struct file *file );
void isr_entry( void );

struct file_operations	
my_fops =	{
		owner:		THIS_MODULE,
		write:		my_write,
		read:		my_read,
		open:		my_open,
		};


void load_IDTR( void *regimage )
{
	asm(" lidt  %0 " : : "m" ( *(unsigned short*)regimage ) );
}


//---------  INTERRUPT SERVICE ROUTINES  --------//
asm("	.text					");
asm("	.type	isr_entry, @function		");
asm("	.align	16				");
asm("						");
asm("isr_entry:					");
asm("	i = 0					");
asm("	.rept	256				");
asm("	pushl	$i				");
asm("	jmp	ahead				");
asm("	i = i+1					");
asm("	.align	16				");
asm("	.endr					");
asm("ahead:					");		
asm("	push	%ebp				");
asm("	mov	%esp, %ebp			");
asm("	push	%eax				");
asm("	push	%ebx				");
asm("						");
asm("	mov	4(%ebp), %ebx			");
asm("	mov	original_isr(, %ebx, 4), %eax	");
asm("	mov	%eax, 4(%ebp)			");
asm("						");
asm("	mov	lapic, %eax			");
asm("	mov	0x23(%eax), %bh			");
asm("	and	$0x1FF, %ebx			");
asm("	incl	n_interrupts(, %ebx, 4)		");	
asm("						");
asm("	pop	%ebx				");
asm("	pop	%eax				");
asm("	pop	%ebp				");
asm("	ret					");
//-----------------------------------------------//


int my_open( struct inode *inode, struct file *file )
{
	memset( n_interrupts, 0, filesize );
	return	0;	// SUCCESS
}


ssize_t
my_write( struct file *file, const char* buf, size_t len, loff_t *pos )
{
	memset( n_interrupts, 0, filesize );
	return	len;
}


ssize_t
my_read( struct file *file, char *buf, size_t len, loff_t *pos )
{
	if ( *pos > filesize ) return -EINVAL;
	if ( *pos + len > filesize ) len = filesize - *pos;
	if ( copy_to_user( buf, n_interrupts, len ) ) return -EFAULT;
	*pos += len;
	return	len;
}


int init_module( void )
{
	struct proc_dir_entry	*proc_entry;
	unsigned int		i, isrlocn;

	printk( "<1>\nInstalling \'%s\' module\n", modname );

	// allocate a 'page' of kernel memory, to use for a new IDT
	kmem = kzalloc( PAGE_SIZE, GFP_KERNEL | GFP_DMA );
	if ( !kmem ) return -ENOMEM; 

	// initialize the register-images for our old and new IDTR 
	asm(" sidt oldidtr \n sidt newidtr ");
	memcpy( newidtr+1, &kmem, sizeof( unsigned int ) );

	// setup pointers to our old and new IDT arrays
	oldidt = (GATE_DESCRIPTOR*)(*(unsigned int*)(oldidtr+1));
	newidt = (GATE_DESCRIPTOR*)(*(unsigned int*)(newidtr+1));

	// build our table of 'indirect-jump' target-addresses
	for (i = 0; i < 256; i++)
		{
		GATE_DESCRIPTOR	gate = oldidt[ i ];
		gate.hi &= 0xFFFF0000;
		gate.lo &= 0x0000FFFF;
		original_isr[ i ] = (gate.lo | gate.hi);
		}

	// setup the gate-descriptor entries in our new IDT array
	isrlocn = (unsigned int)isr_entry;
	for (i = 0; i < 256; i++)
		{
		GATE_DESCRIPTOR	gate = oldidt[ i ];
		gate.hi &= 0x0000FFFF;
		gate.lo &= 0xFFFF0000;
		gate.hi |= (isrlocn & 0xFFFF0000);
		gate.lo |= (isrlocn & 0x0000FFFF);
		newidt[ i ] = gate;
		isrlocn += 16;
		}	

	// map in the processors' Local-APIC registers (so each
	// interrupt-handler can very quickly identify its CPU) 
	lapic = ioremap_nocache( LAPICBASE, PAGE_SIZE );
	if ( !lapic ) { kfree( kmem ); return -ENOSPC; } 

	// activate our new Interrupt Descriptor Table
	load_IDTR( newidtr );
	smp_call_function( load_IDTR, newidtr, 1 );	

	// create proc-file with read-and-write capabilities 
	proc_entry = create_proc_entry( modname, PROC_MODE, PROC_DIR );
	proc_entry->proc_fops = &my_fops;

	return	0;  // SUCCESS
}


void cleanup_module( void )
{
	// delete our module's proc-file
	remove_proc_entry( modname, PROC_DIR );

	// reactivate the system's original IDT
	smp_call_function( load_IDTR, oldidtr, 1 );
	load_IDTR( oldidtr );

	// unmap the Local-APIC registers
	iounmap( lapic );

	// release our kernel memory allocation
	kfree( kmem );

	printk( "<1>Removing \'%s\' module\n", modname );
}

MODULE_LICENSE("GPL");

