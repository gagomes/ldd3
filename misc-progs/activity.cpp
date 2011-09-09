//-------------------------------------------------------------------
//	activity.cpp
//
//	This application will continually reread the volatile kernel 
//	data from the pseudo-file (named '/proc/activity') which our
//	'activity,c' kernel module has created, and will display its
//	formatted contents onscreen (until the <ESCAPE>-key is hit).
//	The 'data' consists of an array of 256 counters representing
//	invocations of the kernel's 256 interrupt service routines.
//
//	       compile using:  $ g++ activiry.cpp -o activity
//	       execute using:  $ ./activity
//
//	NOTE: Our 'activity.c' kernel module has to be preinstalled.
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

char filename[] = "/proc/activity";

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
	int	i, ndev = 1+fd, row = 2, col = 27;
	printf( "\e[%d;%dH INTERRUPT ACTIVITY MONITOR ", row, col );
	for (i = 0; i < 16; i++)
		{
		// draw the sideline 
		row = i+6;
		col = 6;
		printf( "\e[%d;%dH %X0:", row, col, i );
		// draw the headline 
		row = 5;
		col = i*4 + 12;
		printf( "\e[%d;%dH %X", row, col );
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
			unsigned int	rx, counter[ 256 ] = { 0 };
			lseek( fd, 0, SEEK_SET );
			rx = read( fd, counter, sizeof( counter ) );
 			if ( rx < sizeof( counter ) ) break;
			for (i = 0; i < 256; i++)
				{
				int	row = ( i / 16 ) + 6;
				int	col = ( i % 16 ) * 4 + 11;
				int	what = counter[ i ] % 1000;
				printf( "\e[%d;%dH", row, col );
				if ( counter[ i ] == 0 ) printf( "---" );
				else printf( "%03d", what );
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
