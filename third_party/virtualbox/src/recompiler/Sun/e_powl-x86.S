/* ix87 specific implementation of pow function.
   Copyright (C) 1996, 1997, 1998, 1999, 2001, 2004, 2005
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*#include <machine/asm.h>*/
#include <iprt/cdefs.h>

#ifdef __MINGW32__
# define ASM_TYPE_DIRECTIVE(name,typearg) 
# define ASM_SIZE_DIRECTIVE(name) 
# define cfi_adjust_cfa_offset(a)
# define C_LABEL(name)       _ ## name:
# define C_SYMBOL_NAME(name) _ ## name
# define ASM_GLOBAL_DIRECTIVE .global
# define ALIGNARG(log2) 1<<log2
#elif __APPLE__
# define ASM_TYPE_DIRECTIVE(name,typearg) 
# define ASM_SIZE_DIRECTIVE(name) 
# define cfi_adjust_cfa_offset(a)
# define C_LABEL(name)       _ ## name:
# define C_SYMBOL_NAME(name) _ ## name
# define ASM_GLOBAL_DIRECTIVE .globl
# define ALIGNARG(log2) log2
#else
# define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
# define ASM_SIZE_DIRECTIVE(name) .size name,.-name;
# define C_LABEL(name)		name:
# define C_SYMBOL_NAME(name) name
# /* figure this one out. */
# define cfi_adjust_cfa_offset(a)
# define ASM_GLOBAL_DIRECTIVE .global
# define ALIGNARG(log2) 1<<log2
#endif

#define	ENTRY(name)							      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(4);							      \
  C_LABEL(name)

#undef	END
#define END(name)							      \
  ASM_SIZE_DIRECTIVE(name)

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif

	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(infinity,@object)
inf_zero:
infinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0x7f
	ASM_SIZE_DIRECTIVE(infinity)
	ASM_TYPE_DIRECTIVE(zero,@object)
zero:	.double 0.0
	ASM_SIZE_DIRECTIVE(zero)
	ASM_TYPE_DIRECTIVE(minf_mzero,@object)
minf_mzero:
minfinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0xff
mzero:
	.byte 0, 0, 0, 0, 0, 0, 0, 0x80
	ASM_SIZE_DIRECTIVE(minf_mzero)
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(limit,@object)
limit:	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)
	ASM_TYPE_DIRECTIVE(p63,@object)
p63:	.byte 0, 0, 0, 0, 0, 0, 0xe0, 0x43
	ASM_SIZE_DIRECTIVE(p63)

#ifdef PIC
#define MO(op) op##@GOTOFF(%ecx)
#define MOX(op,x,f) op##@GOTOFF(%ecx,x,f)
#else
#define MO(op) op
#define MOX(op,x,f) op(,x,f)
#endif

	.text
//ENTRY(__ieee754_powl)
ENTRY(RT_NOCRT(powl))
#ifdef RT_OS_DARWIN /* 16-byte long double with 8 byte alignment requirements */
	fldt	20(%esp)	// y
#else
	fldt	16(%esp)	// y
#endif
	fxam

#ifdef	PIC
	LOAD_PIC_REG (cx)
#endif

	fnstsw
	movb	%ah, %dl
	andb	$0x45, %ah
	cmpb	$0x40, %ah	// is y == 0 ?
	je	.L11

	cmpb	$0x05, %ah	// is y == ±inf ?
	je	.L12

	cmpb	$0x01, %ah	// is y == NaN ?
	je	.L30

	fldt	4(%esp)		// x : y

	subl	$8,%esp
	cfi_adjust_cfa_offset (8)

	fxam
	fnstsw
	movb	%ah, %dh
	andb	$0x45, %ah
	cmpb	$0x40, %ah
	je	.L20		// x is ±0

	cmpb	$0x05, %ah
	je	.L15		// x is ±inf

	fxch			// y : x

	/* fistpll raises invalid exception for |y| >= 1L<<63.  */
	fld	%st		// y : y : x
	fabs			// |y| : y : x
	fcompl	MO(p63)		// y : x
	fnstsw
	sahf
	jnc	.L2

	/* First see whether `y' is a natural number.  In this case we
	   can use a more precise algorithm.  */
	fld	%st		// y : y : x
	fistpll	(%esp)		// y : x
	fildll	(%esp)		// int(y) : y : x
	fucomp	%st(1)		// y : x
	fnstsw
	sahf
	jne	.L2

	/* OK, we have an integer value for y.  */
	popl	%eax
	cfi_adjust_cfa_offset (-4)
	popl	%edx
	cfi_adjust_cfa_offset (-4)
	orl	$0, %edx
	fstp	%st(0)		// x
	jns	.L4		// y >= 0, jump
	fdivrl	MO(one)		// 1/x		(now referred to as x)
	negl	%eax
	adcl	$0, %edx
	negl	%edx
.L4:	fldl	MO(one)		// 1 : x
	fxch

.L6:	shrdl	$1, %edx, %eax
	jnc	.L5
	fxch
	fmul	%st(1)		// x : ST*x
	fxch
.L5:	fmul	%st(0), %st	// x*x : ST*x
	shrl	$1, %edx
	movl	%eax, %ecx
	orl	%edx, %ecx
	jnz	.L6
	fstp	%st(0)		// ST*x
	ret

	/* y is ±NAN */
.L30:	fldt	4(%esp)		// x : y
	fldl	MO(one)		// 1.0 : x : y
	fucomp	%st(1)		// x : y
	fnstsw
	sahf
	je	.L31
	fxch			// y : x
.L31:	fstp	%st(1)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
.L2:	/* y is a real number.  */
	fxch			// x : y
	fldl	MO(one)		// 1.0 : x : y
	fld	%st(1)		// x : 1.0 : x : y
	fsub	%st(1)		// x-1 : 1.0 : x : y
	fabs			// |x-1| : 1.0 : x : y
	fcompl	MO(limit)	// 1.0 : x : y
	fnstsw
	fxch			// x : 1.0 : y
	sahf
	ja	.L7
	fsub	%st(1)		// x-1 : 1.0 : y
	fyl2xp1			// log2(x) : y
	jmp	.L8

.L7:	fyl2x			// log2(x) : y
.L8:	fmul	%st(1)		// y*log2(x) : y
	fxam
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x05, %ah	// is y*log2(x) == ±inf ?
	je	.L28
	fst	%st(1)		// y*log2(x) : y*log2(x)
	frndint			// int(y*log2(x)) : y*log2(x)
	fsubr	%st, %st(1)	// int(y*log2(x)) : fract(y*log2(x))
	fxch			// fract(y*log2(x)) : int(y*log2(x))
	f2xm1			// 2^fract(y*log2(x))-1 : int(y*log2(x))
	faddl	MO(one)		// 2^fract(y*log2(x)) : int(y*log2(x))
	fscale			// 2^fract(y*log2(x))*2^int(y*log2(x)) : int(y*log2(x))
	addl	$8, %esp
	cfi_adjust_cfa_offset (-8)
	fstp	%st(1)		// 2^fract(y*log2(x))*2^int(y*log2(x))
	ret

	cfi_adjust_cfa_offset (8)
.L28:	fstp	%st(1)		// y*log2(x)
	fldl	MO(one)		// 1 : y*log2(x)
	fscale			// 2^(y*log2(x)) : y*log2(x)
	addl	$8, %esp
	cfi_adjust_cfa_offset (-8)
	fstp	%st(1)		// 2^(y*log2(x))
	ret

	// pow(x,±0) = 1
	.align ALIGNARG(4)
.L11:	fstp	%st(0)		// pop y
	fldl	MO(one)
	ret

	// y == ±inf
	.align ALIGNARG(4)
.L12:	fstp	%st(0)		// pop y
	fldt	4(%esp)		// x
	fabs
	fcompl	MO(one)		// < 1, == 1, or > 1
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x45, %ah
	je	.L13		// jump if x is NaN

	cmpb	$0x40, %ah
	je	.L14		// jump if |x| == 1

	shlb	$1, %ah
	xorb	%ah, %dl
	andl	$2, %edx
	fldl	MOX(inf_zero, %edx, 4)
	ret

	.align ALIGNARG(4)
.L14:	fldl	MO(one)
	ret

	.align ALIGNARG(4)
.L13:	fldt	4(%esp)		// load x == NaN
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±inf
.L15:	fstp	%st(0)		// y
	testb	$2, %dh
	jz	.L16		// jump if x == +inf

	// We must find out whether y is an odd integer.
	fld	%st		// y : y
	fistpll	(%esp)		// y
	fildll	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	.L17

	// OK, the value is an integer, but is it odd?
	popl	%eax
	cfi_adjust_cfa_offset (-4)
	popl	%edx
	cfi_adjust_cfa_offset (-4)
	andb	$1, %al
	jz	.L18		// jump if not odd
	// It's an odd integer.
	shrl	$31, %edx
	fldl	MOX(minf_mzero, %edx, 8)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
.L16:	fcompl	MO(zero)
	addl	$8, %esp
	cfi_adjust_cfa_offset (-8)
	fnstsw
	shrl	$5, %eax
	andl	$8, %eax
	fldl	MOX(inf_zero, %eax, 1)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
.L17:	shll	$30, %edx	// sign bit for y in right position
	addl	$8, %esp
	cfi_adjust_cfa_offset (-8)
.L18:	shrl	$31, %edx
	fldl	MOX(inf_zero, %edx, 8)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±0
.L20:	fstp	%st(0)		// y
	testb	$2, %dl
	jz	.L21		// y > 0

	// x is ±0 and y is < 0.  We must find out whether y is an odd integer.
	testb	$2, %dh
	jz	.L25

	fld	%st		// y : y
	fistpll	(%esp)		// y
	fildll	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	.L26

	// OK, the value is an integer, but is it odd?
	popl	%eax
	cfi_adjust_cfa_offset (-4)
	popl	%edx
	cfi_adjust_cfa_offset (-4)
	andb	$1, %al
	jz	.L27		// jump if not odd
	// It's an odd integer.
	// Raise divide-by-zero exception and get minus infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	fchs
	ret

	cfi_adjust_cfa_offset (8)
.L25:	fstp	%st(0)
.L26:	addl	$8, %esp
	cfi_adjust_cfa_offset (-8)
.L27:	// Raise divide-by-zero exception and get infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	ret

	cfi_adjust_cfa_offset (8)
	.align ALIGNARG(4)
	// x is ±0 and y is > 0.  We must find out whether y is an odd integer.
.L21:	testb	$2, %dh
	jz	.L22

	fld	%st		// y : y
	fistpll	(%esp)		// y
	fildll	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	.L23

	// OK, the value is an integer, but is it odd?
	popl	%eax
	cfi_adjust_cfa_offset (-4)
	popl	%edx
	cfi_adjust_cfa_offset (-4)
	andb	$1, %al
	jz	.L24		// jump if not odd
	// It's an odd integer.
	fldl	MO(mzero)
	ret

	cfi_adjust_cfa_offset (8)
.L22:	fstp	%st(0)
.L23:	addl	$8, %esp	// Don't use 2 x pop
	cfi_adjust_cfa_offset (-8)
.L24:	fldl	MO(zero)
	ret

END(RT_NOCRT(powl))
//END(__ieee754_powl)
