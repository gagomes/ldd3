//-------------------------------------------------------------------
//	smpwatch.cpp
//
//	This application will continually reread the volatile kernel 
//	data from the pseudo-file (named '/proc/smpwatch') which our
//	'smpwatch,c' kernel module has created, and will display its
//	formatted contents onscreen (until the <ESCAPE>-key is hit).
//	The 'data' consists of an array of 512 counters representing
//	invocations of the Linux kernel's interrupt service routines
//	on each of two logical processors in an SMP-enabled system.
//
//	       compile using:  $ g++ smpwatch.cpp -o smpwatch
//	       execute using:  $ ./smpwatch
//
//	NOTE: Our 'smpwatch.c' kernel module has to be preinstalled.
//
//	programmer: ALLAN CRUSE
//	written on: 14 SEP 2009
//-------------------------------------------------------------------

#include <stdio.h>	// for printf(), perror() 
#include <fcntl.h>	// for open() 
#include <stdlib.h>	// for exit() 
#include <unistd.h>	// for read(), write(), close() 
#include <termios.h>	// for tcgetattr(), tcsetattr()

#define KEY_ESCAPE  27	// ASCII-code for the <ESCAPE>-key

char filename[] = "/proc/smpwatch";

int main( int argc, char **argv )
{
	// open the pseudo-file for reading
	int	fd = open( filename, O_RDONLY );
	if ( fd < 0 ) { perror( filename ); exit(1); }

	// enable non-canonical terminal input
	struct termios	tty_orig;
	tcgetattr( STDIN_FILENO, &tty_orig );
	struct termios	tty_work = tty_orig;
	tty_work.c_lflag &= ~( ECHO | ICANON | ISIG );
	tty_work.c_cc[ VMIN ] = 1;
	tty_work.c_cc[ VTIME ] = 0;
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &tty_work );

	// initialize a file-descriptor bitmap for select()
	fd_set	permset;
	FD_ZERO( &permset );
	FD_SET( STDIN_FILENO, &permset );
	FD_SET( fd, &permset );
	
	// initialize the screen-display
	printf( "\e[H\e[J" );		// erase the screen
	printf( "\e[?25l" );		// hide the cursor
	
	// draw the screen's title, headline and sideline
	int	i, ndev = 1+fd, row = 2, col = 25;
	printf( "\e[%d;%dH SMP INTERRUPT ACTIVITY MONITOR ", row, col );
	for (i = 0; i < 16; i++)
		{
		// draw a sideline for each processor
		row = i+6;
		col = 2;
		printf( "\e[%d;%dH %X0:", row, col, i );
		col = 40+2;
		printf( "\e[%d;%dH %X0:", row, col, i );
		// draw a headline for each processor
		row = 5;
		col = i*2 + 6;
		printf( "\e[%d;%dH %X", row, col );
		col += 40;
		printf( "\e[%d;%dH %X", row, col );
		}
	// draw a footline for each processor
	for (i = 0; i < 2; i++)
		{
		char	banner[ 40 ] = { 0 };
		int	len = 0;
		len += sprintf( banner+len, "==========" );
		len += sprintf( banner+len, "  processor %d  ", i );
		len += sprintf( banner+len, "==========" );
		row = 22;
		col = 40*i + 2;
		printf( "\e[%d;%dH %s", row, col, banner );
		}
	fflush( stdout );

	// main loop: continually responds to multiplexed input
	for(;;)	{
		// sleep until some new data is ready to be read
		fd_set	readset = permset;
		if ( select( ndev, &readset, NULL, NULL, NULL ) < 0 ) break;
		
		// process any new data read from the pseudo-file
		if ( FD_ISSET( fd, &readset ) )
			{
			unsigned int	rx, counter[ 256 * 2 ] = { 0 };
			lseek( fd, 0, SEEK_SET );
			rx = read( fd, counter, sizeof( counter ) );
 			if ( rx < sizeof( counter ) ) break;
			for (i = 0; i < 512; i++)
				{
				int	cpu = i / 256;
				int	irq = i % 256;
				int	row = ( irq / 16 ) + 6;
				int	col = ( irq % 16 ) * 2 + 40 * cpu + 7;
				int	what = counter[ i ] % 10;
				printf( "\e[%d;%dH", row, col );
				if ( counter[ i ] == 0 ) printf( "-" );
				else printf( "%01d", what );
				}			
			fflush( stdout );
			}
			
		// process any new data read from the keyboard
		if ( FD_ISSET( STDIN_FILENO, &readset ) )
			{
			int	rx, inch = 0;
			rx = read( STDIN_FILENO, &inch, sizeof( inch ) );
			if (( rx > 0 )&&( inch == KEY_ESCAPE )) break;
			}
		}

	// restore the standard user-interface
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &tty_orig );
	printf( "\e[23;1H\e[?25h\n" );	// restore cursor visibility 	
}
