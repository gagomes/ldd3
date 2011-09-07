//-------------------------------------------------------------------
//	cpuid.cpp
//
//	This Linux application illustrates the use of the x86 CPU's
//	CPUID instruction, to display "feature-bits" which indicate
//	the specific capabilities which this processor implements.   
//
//	      to compile-and-link: $ g++ cpuid.cpp -o cpuid
//	      and then to execute: $ ./cpuid
//
//	programmer: ALLAN CRUSE
//	written on: 05 MAY 2004
//	revised on: 16 NOV 2006 -- to improve format of the output
//	revised on: 13 NOV 2008 -- to use larger input-value range
//-------------------------------------------------------------------

#include <stdio.h>	// for printf(), perror() 

int main( int argc, char **argv )
{
	unsigned int	reg_eax, reg_ebx, reg_ecx, reg_edx;
	int		i, max = 1;

	for (i = 0; i <= max; i++)
		{
		asm(" movl %0, %%eax " :: "m" (i) );
		asm(" cpuid ");
		asm(" mov %%eax, %0 " : "=m" (reg_eax) );
		asm(" mov %%ebx, %0 " : "=m" (reg_ebx) );
		asm(" mov %%ecx, %0 " : "=m" (reg_ecx) );
		asm(" mov %%edx, %0 " : "=m" (reg_edx) );

		printf( "\n\t" );
		printf( "%2d: ", i );
		printf( "eax=%08X ", reg_eax );
		printf( "ebx=%08X ", reg_ebx );
		printf( "ecx=%08X ", reg_ecx );
		printf( "edx=%08X ", reg_edx );

		if ( i == 0 ) {
                   max = reg_eax;
                   char str[13];
                   int *p = (int *)str;
                   p[0] = reg_ebx;
                   p[1] = reg_edx;
                   p[2] = reg_ecx;
                   str[12] = 0;
                   printf("%s", str);
                }
		}
	printf( "\n\n" );
}
