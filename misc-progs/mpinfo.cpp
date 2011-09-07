//-------------------------------------------------------------------
//	mpinfo.cpp
//
//	This Linux utility uses our 'dram.c' device-driver to access
//	the bottom one-megabyte of system memory, in order to search 
//	for the MP Floating Pointer Structure and, upon locating it,
//	to display the various records in the MP Configuration Table
//	that it references, thus letting us know how many processors 
//	are installed in our system (plus other system information).
//	The search scans three system memory-regions, in this order:	
//	
//		a. The first kilobyte of the EBDA
//		b. The final kilobyte of base memory
//		c. The ROM-BIOS address space arenas
//
//	Reference: Intel Multiprocessor Specification (Version 1.4)
//
//	     to compile-and-link:  $ g++ mpinfo.cpp -o mpinfo
//	     and then to execute:  $ ./mpinfo
//
//	NOTE: Our 'dram.ko' kernel-object needs to be pre-installed.
//
//	programmer: ALLAN CRUSE
//	written on: 08 AUG 2008
//-------------------------------------------------------------------

#include <stdio.h>		// for printf(), perror() 
#include <fcntl.h>		// for open() 
#include <stdlib.h>		// for exit() 
#include <unistd.h>		// for read(), write(), close() 
#include <string.h>		// for strncmp()
#include <sys/utsname.h>	// for utsname()

struct zone_struct { unsigned short  begin, reach; };
struct zone_struct zone[ 3 ] = { { 0, 64 }, { 0x9FC0, 64 }, { 0xF000, 4096 } };

char devname[] = "/dev/dram";
struct utsname	uts;

int main( int argc, char **argv )
{
	// get this station's hostname for display (in case we want
	// to print the machine's information for future reference)
	char	hostname[ 64 ];
	if ( gethostname( hostname, sizeof( hostname ) - 1 ) < 0 )
		{ perror( "gethostname" ); exit(1); }
	else	strtok( hostname, "." );

	// open our device-file for this machine's system-memory 
	int	fd = open( devname, O_RDONLY );
	if ( fd < 0 ) { perror( devname ); exit(1); }

	// read from ROM-BIOS DATA-AREA (0x400-0x500) to get EBDA
	lseek( fd, 0x40E, SEEK_SET );
	read( fd, &zone[0].begin, sizeof( short ) );

	// loop to search for the MP Floating Pointer Structure
	printf( "\n Looking for the MP Floating Pointer Structure " );
	printf( "on station \'%s\' ...\n\n", hostname );
	char	buf[ 16 ];
	long	mpfp = 0;
	for (int i = 0; i < 3; i++)
		{
		int	base = zone[i].begin * 16;
		int	size = zone[i].reach * 16;

		printf( "\t searching in zone %d: ", i+1 );
		printf( "0x%08X - 0x%08X \n", base, base+size );
		for (int p = 0; p < size; p += 16)
			{
			lseek( fd, base+p, SEEK_SET );
			read( fd, buf, sizeof( buf ) );
			if ( strncmp( buf, "_MP_", 4 ) ) continue;
			else	{ mpfp = (base+p); break; }
			}	
		if ( mpfp ) break;
		}
	// exit if no MP Floating Pointer Structure is present
	if ( !mpfp ) 
		{
		fprintf( stderr, "MP Floating Pointer Structure not found\n" );	
		exit(1);
		}

	// compute the paragraph checksum	
	char	cksum = 0;
	for (int i = 0; i < 16; i++) cksum += buf[i];
	if ( cksum ) 
		{ 
		fprintf( stderr, "MP Floating Pointer Structure is invalid\n" );
		exit(1);
		}

	// report location of the MP Floating Pointer Structure
	printf( "\n MP Floating Pointer Structure found " );
	printf( "at 0x%08lX (checksum=%d) \n", mpfp, cksum );

	printf( "\n 0x%08lX: ", mpfp );
	for (int i = 0; i < 16; i++) printf( "%02X ", (buf[i] & 0xFF) );
	for (int i = 0; i < 16; i++)
		{
		char	ch = buf[i];
		if (( ch < 0x20 )||( ch > 0x7E )) ch = '.';
		printf( "%c", ch );
		}
	printf( "\n" );

	int	ptr = *(int*)(buf + 4);
	int	len = (buf[8] & 0xFF);
	int	ver = (buf[9] & 0xFF);

	printf( "\n Multiprocessor Specification version 1.%X \n", ver );
	printf( "\n MP Configuration Table located at 0x%08X  \n", ptr );

	// read the MP Configuration Table Header
	char	mpcthdr[ 44 ];
	lseek( fd, ptr, SEEK_SET );
	read( fd, mpcthdr, sizeof( mpcthdr ) );

	if ( strncmp( mpcthdr, "PCMP", 4 ) )
		{
		fprintf( stderr, " MP Configuration Table is not valid \n" );
		exit(1);
		}  

	for (int p = 0; p < 44; p += 16)
		{
		printf( "\n 0x%08X: ", ptr + p );
		for (int j = 0; j < 16; j++)
			{
			if ( p+j < 44 ) printf( "%02X ", mpcthdr[p+j] & 0xFF );
			else	printf( "   " );
			}
		for (int j = 0; j < 16; j++)
			{
			char	ch = (p+j < 44) ? mpcthdr[p+j] : ' ';
			if (( ch < 0x20 )||( ch > 0x7E )) ch = '.';
			printf( "%c", ch );
			}
		}
	printf( "\n\n" );			

	int	base_table_len = *(unsigned short*)( mpcthdr + 4 );
	int	entry_count = *(unsigned short*)( mpcthdr + 34 );

	int	ext_table_len = *(unsigned short*)( mpcthdr + 40 );
	int	total_table_len = base_table_len + ext_table_len;

	char	table[ 4096 ] = { 0 };
	lseek( fd, ptr, SEEK_SET );
	read( fd, table, total_table_len );
	
	char	checksum1 = 0;
	for (int i = 0; i < base_table_len; i++) checksum1 += table[i];

	printf( " Base Table Length = %d bytes  ", base_table_len );
	printf( " Base Table Entry Count = %d  ", entry_count ); 
	printf( " checksum = %d \n", checksum1 );

	char	*entptr = table + 44;
	for (int i = 0; i < entry_count; i++)
		{
		printf( "\n " );
		int	entlen = entptr[0] ? 8 : 20;
		for (int j = 0; j < entlen; j++)
			printf( "%02X ", entptr[ j ] & 0xFF );
		entptr += entlen;
		}
	printf( "\n\n" );	

	char	checksum2 = 0;
	for (int i = base_table_len; i < total_table_len; i++) 
			checksum2 += table[i];

	printf( " Extended Table Length = %d bytes \n", ext_table_len );

	entptr = table + base_table_len;
	while ( entptr < table + total_table_len )
		{
		int	entlen = (unsigned char)entptr[ 1 ];
		printf( "\n " );
		for (int j = 0; j < entlen; j++) 
			printf( "%02X ", entptr[ j ] & 0xFF );
		entptr += entlen;
		}
	if ( ext_table_len == 0 ) printf( "\n" );
	else printf( "\n\n" );
		
	int	oem_table_size = *(unsigned short*)( mpcthdr + 32 );	
	printf( " OEM Table Length = %d bytes \n", oem_table_size );
	printf( "\n" );
}
