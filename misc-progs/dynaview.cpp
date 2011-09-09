//-------------------------------------------------------------------
//	dynaview.cpp
//
//	This program produces a dynamic view of the volatile text in
//	a Linux pseudo-file whose filename is supplied as a command-
//	line argument (provided this file's contents does not exceed 
//	the bounds of the current terminal window).  It continuously 
//	rereads and redraws this file's text, hiding cursor-movement
//	in order to eliminate the distraction of its screen-flashes,
//	and 'non-canonical' keyboard-input is employed so keypresses 
//	do not get echoed.  The user quits by just hitting <ESCAPE>.
//
//		to compile:  $ g++ dynaview.cpp -o dynaview
//		to execute:  $ ./dynaview <filename>
//
//	NOTE: Examples of standard Linux 'pseudo-files' you can look
//	at are: '/proc/uptime', '/proc/meminfo', '/proc/interrupts'.  
//
//	programmer: ALLAN CRUSE
//	written on: 24 AUG 2009
//	revised on: 16 SEP 2009
//-------------------------------------------------------------------

#include <stdio.h>	// for printf(), perror() 
#include <fcntl.h>	// for open() 
#include <stdlib.h>	// for exit() 
#include <unistd.h>	// for read() 
#include <string.h>	// for strncpy()
#include <termios.h>	// for tcgetattr(), tcsetattr()

int main( int argc, char **argv )
{
	// get the filename entered as a command-line argument
	char filename[ 64 ] = { 0 }; 
	if ( argc > 1 ) strncpy( filename, argv[1], 63 );
	else	{ printf( "missing filename\n" ); exit(1); }

	// open the specified file for reading
	int	fd = open( filename, O_RDONLY );
	if ( fd < 0 ) { perror( filename ); exit(1); }

	// enable 'non-canonical' terminal input
	struct termios	orig_tty;
	tcgetattr( STDIN_FILENO, &orig_tty );
	struct termios	work_tty = orig_tty;
	work_tty.c_lflag &= ~( ECHO | ICANON | ISIG );
	work_tty.c_cc[ VMIN ] = 0;
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &work_tty );

	// erase the screen and show the file's name
	printf( "\e[H\e[J" );		// clear the screen
	printf( "\e[1;1H\e[7m" );	// enable reverse-video
	printf( " FILENAME: \'%s\' ", filename ); 
	printf( "\e[0m\n" );		// disable reverse-video
	printf( "\e[?25l" );		// hide the cursor

	// main loop to continuously exhibit the file's text  
	int	done = 0;
	while ( !done )
		{	
		// read and draw the specifued file's contents 
		char	buf[ 4096 ] = { 0 };
		lseek( fd, 0, SEEK_SET );	// rewind the file
		read( fd, buf, sizeof( buf ) );	// reread the file
		printf( "\e[2;2H%s", buf );	// redraw the file 

		// allow other processes a chance to execute
		usleep( 100000 );		// sleep briefly  

		// check for <ESCAPE>-key pressed by the user
		int	inch = 0;
		read( STDIN_FILENO, &inch, sizeof( inch ) );
		if ( inch == 0x1B ) done = 1;
		}

	// restore the normal terminal behavior that users expect
	printf( "\e[?25h" );		// show the cursor
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &orig_tty );
}
