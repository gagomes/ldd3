//-------------------------------------------------------------------
//	nicspy.cpp
//
//	This program will show all the ethernet packets received by 
//	the Intel Pro1000 network interface controller provided our 
//	'nicspy.c' device-driver module is installed in the kernel.
//
//		compile using:  $ g++ nicspy.cpp -o nicspy
//
//	programmer: ALLAN CRUSE
//	written on: 12 FEB 2008
//-------------------------------------------------------------------

#include <stdio.h>	// for printf(), perror() 
#include <fcntl.h>	// for open() 
#include <stdlib.h>	// for exit() 
#include <unistd.h>	// for read(), gethostname()  
#include <sys/ioctl.h>	// for ioctl()

char devname[] = "/dev/nic";
unsigned char	hostname[ 64 ], mac[ 6 ], buf[ 0x600 ]; 

int main( int argc, char **argv )
{
	int	fd = open( devname, O_RDONLY );
	if ( fd < 0 ) { perror( devname ); exit(1); }

	if ( ioctl( fd, 1, &mac ) < 0 ) { perror( "ioctl" ); exit(1); }
	gethostname( (char*)hostname, 63 );

	printf( "\n Monitoring packets for interface " ); 
	for (int i = 0; i < 6; i++) printf( "%02X%c", mac[i], (i<5)?':':' ' );
	printf( "on \'%s\' \n ----------\n", hostname );
	int	pkt_count = 0;
	for(;;)	{	
		int	nbytes = read( fd, buf, sizeof( buf ) );
		if ( nbytes < 0 ) { perror( "read" ); exit(1); }

		printf( "\n packet #%d ", ++pkt_count );
		for (int i = 0; i < nbytes; i+=16)
			{
			printf( "\n %04X: ", i );
			for (int j = 0; j < 16; j++) 
				{
				if ( i+j < nbytes ) 
					printf( "%02X ", buf[ i+j ] );
				else	printf( "   " );
				}
			for (int j = 0; j < 16; j++)
				{
				unsigned char	ch;
				ch = ( i+j < nbytes ) ? buf[ i+j ] : ' ';
				if (( ch < 0x20 )||( ch > 0x7E )) ch = '.';
				printf( "%c", ch );
				}
			}
		printf( "\n %d bytes transferred\n ----------\n", nbytes );
		}	
}
