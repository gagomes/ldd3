//-----------------------------------------------------------------
//	genuine.s
//
//	This program illustrates use of the 'cpuid' instruction on
//	an x86_64 Linux platform, where we know the instruction is
//	implemented, and where we can assemble for the processor's
//	32-bit 'compatibility mode' in which 'pushal' can execute.
//
//	    to assemble:  $ as --32 genuine.s -o genuine.o
//	    and to link:  $ ld -melf_i386 genuine.o -o genuine 
//	    and execute:  $ ./genuine
//
//	HINT: Try executing this program on the 'stargate' server.
//
//	programmer: ALLAN CRUSE
//	written on: 13 NOV 2008
//-----------------------------------------------------------------


	# manifest constants
	.equ	sys_EXIT, 1
	.equ	sys_WRITE, 4
	.equ	dev_STDOUT, 1


	.section	.text
_start:	.code32		# must assemble for 'compatibility-mode'

	# setup register EAX for getting vendor-identification

	xor	%eax, %eax		# zero-value into EAX
	cpuid				# and execute 'cpuid'

	# setup 'newline' ASCII-code in EAX for string-output 

	mov	$0x0A, %eax		# newline ascii-code
	pushal				# push vendor-string

	# write vendor-string (from EBX,ECX,EDX) to the console

	mov	$sys_WRITE, %eax	# system-call ID-number
	mov	$dev_STDOUT, %ebx	# device-file ID-number
	lea	16(%esp), %ecx		# address of the string
	mov	$13, %edx		# length of the string
	int	$0x80			# invoke kernel service

	# terminate this program, and return to command-shell

	mov	$sys_EXIT, %eax		# system-call ID-number
	mov	$0, %ebx		# use zero as exit-code
	int	$0x80			# invoke ketnel service

	.global	_start			# entry-point is public
	.end				# assembly is concluded 

