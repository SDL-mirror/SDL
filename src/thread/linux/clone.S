
/* Taken with thanks from LinuxThreads 0.6 */

/* This is no longer necessary with glibc-2.1, which has it's own clone() */
#ifdef linux
/* Look to see if glibc is available, and if so, what version */
#include <features.h>

#if (__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1))
#define HAVE_CLONE
#endif /* glibc 2.1 or newer */
#endif /* linux */

#if defined(linux) && !defined(SDL_USE_PTHREADS) && !defined(HAVE_CLONE)

#if defined(__i386__)
/************************************************************************/
/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   Contributed by Richard Henderson (rth@tamu.edu)

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif
Cambridge, MA 02139, USA.  */

/* clone() is even more special than fork() as it mucks with stacks
   and invokes a function in the right context after its all over.  */

#include <asm/errno.h>
#include <asm/unistd.h>

/* int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg); */

        .text
	.align	4
	.globl	__clone
        .type   __clone,@function
	.weak	clone
	clone   = __clone
__clone:
	/* Sanity check arguments.  */
	movl	$-EINVAL,%eax
	movl	4(%esp),%ecx		/* no NULL function pointers */
	testl	%ecx,%ecx
	jz	syscall_error
	movl	8(%esp),%ecx		/* no NULL stack pointers */
	testl	%ecx,%ecx
	jz	syscall_error

	/* Insert the argument onto the new stack.  */
	subl	$8,%ecx
	movl	16(%esp),%eax
	movl	%eax,4(%ecx)

	/* Save the function pointer as the zeroth argument. */
	/* It will be popped off in the child in the ebx frobbing below.  */
	movl	4(%esp),%eax
	movl	%eax,0(%ecx)

	/* Do the system call */
	pushl	%ebx
	movl	16(%esp),%ebx
	movl	$__NR_clone,%eax
	int	$0x80
	popl	%ebx

	test	%eax,%eax
	jl	syscall_error
	jz	thread_start

	ret

syscall_error:
	negl    %eax
        pushl   %eax
#ifdef __PIC__
        call    __errno_location@PLT
#else
        call    __errno_location
#endif
        popl    0(%eax)
	movl	$-1, %eax
	ret

thread_start:
	subl	%ebp,%ebp	/* terminate the stack frame */
	call	*%ebx
        pushl   %eax
#ifdef __PIC__
	call	_exit@PLT
#else
	call	_exit
#endif
/************************************************************************/
#elif defined(sparc)
/************************************************************************/
/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   Contributed by Miguel de Icaza (miguel@nuclecu.unam.mx)
   Based on code written for the Intel by Richard 
   Henderson (rth@tamu.edu)
	
The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* clone() is even more special than fork() as it mucks with stacks
   and invokes a function in the right context after its all over.  */
	
#include <asm/errno.h>
#include <asm/unistd.h>

/* int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg); */

	.text
	.align	4
	.globl	__clone
	.type	__clone,@function
	.weak	clone
	clone   = __clone
__clone:	
	save	%sp,-96,%sp
	/* sanity check arguments */
	tst	%i0
	be	__clone_syscall_error
	tst	%i1
	be	__clone_syscall_error
        nop

	/* Do the system call */
	mov	%i1,%o1
	mov	%i2,%o0
	set	__NR_clone,%g1
	ta	0x10
	bcs	__clone_syscall_error
	tst	%o1
	bne	__thread_start
	nop
	mov	%o0,%i0
	ret
	restore
	
__clone_syscall_error:
	call	__errno_location
	set	EINVAL,%i0
	st	%i0,[%o0]
	mov	-1,%i0
	ret
	restore

__thread_start:
	call	%i0
	mov	%i3,%o0
	call	_exit,0
	nop
/************************************************************************/
#else 
#error "Unknown Linux architecture"
#endif

#endif /* Linux && ! SDL_USE_PTHREADS */
