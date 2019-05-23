/** @file
 * IPRT / No-CRT - fenv.h, AMD64.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * Copyright (c) 2004-2005 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ___iprt_nocrt_amd64_fenv_h
#define ___iprt_nocrt_amd64_fenv_h

#include <iprt/types.h>

typedef struct {
        struct {
                uint32_t      __control;
                uint32_t      __status;
                uint32_t      __tag;
                char          __other[16];
        } __x87;
        uint32_t              __mxcsr;
} fenv_t;

typedef uint16_t      fexcept_t;

/* Exception flags */
#define FE_INVALID      0x01
#define FE_DENORMAL     0x02
#define FE_DIVBYZERO    0x04
#define FE_OVERFLOW     0x08
#define FE_UNDERFLOW    0x10
#define FE_INEXACT      0x20
#define FE_ALL_EXCEPT   (FE_DIVBYZERO | FE_DENORMAL | FE_INEXACT | \
                         FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

/* Rounding modes */
#define FE_TONEAREST    0x0000
#define FE_DOWNWARD     0x0400
#define FE_UPWARD       0x0800
#define FE_TOWARDZERO   0x0c00
#define _ROUND_MASK     (FE_TONEAREST | FE_DOWNWARD | \
                         FE_UPWARD | FE_TOWARDZERO)

/*
 * As compared to the x87 control word, the SSE unit's control word
 * has the rounding control bits offset by 3 and the exception mask
 * bits offset by 7.
 */
#define _SSE_ROUND_SHIFT        3
#define _SSE_EMASK_SHIFT        7

RT_C_DECLS_BEGIN

/* Default floating-point environment */
extern const fenv_t     RT_NOCRT(__fe_dfl_env);
#define FE_DFL_ENV      (&__fe_dfl_env)

#define __fldcw(__cw)           __asm __volatile("fldcw %0" : : "m" (__cw))
#define __fldenv(__env)         __asm __volatile("fldenv %0" : : "m" (__env))
#define __fnclex()              __asm __volatile("fnclex")
#define __fnstenv(__env)        __asm __volatile("fnstenv %0" : "=m" (*(__env)))
#define __fnstcw(__cw)          __asm __volatile("fnstcw %0" : "=m" (*(__cw)))
#define __fnstsw(__sw)          __asm __volatile("fnstsw %0" : "=am" (*(__sw)))
#define __fwait()               __asm __volatile("fwait")
#define __ldmxcsr(__csr)        __asm __volatile("ldmxcsr %0" : : "m" (__csr))
#define __stmxcsr(__csr)        __asm __volatile("stmxcsr %0" : "=m" (*(__csr)))

DECLINLINE(int)
feclearexcept(int __excepts)
{
        fenv_t __env;

        if (__excepts == FE_ALL_EXCEPT) {
                __fnclex();
        } else {
                __fnstenv(&__env.__x87);
                __env.__x87.__status &= ~__excepts;
                __fldenv(__env.__x87);
        }
        __stmxcsr(&__env.__mxcsr);
        __env.__mxcsr &= ~__excepts;
        __ldmxcsr(__env.__mxcsr);
        return (0);
}

DECLINLINE(int)
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
        int __mxcsr, __status;

        __stmxcsr(&__mxcsr);
        __fnstsw(&__status);
        *__flagp = (__mxcsr | __status) & __excepts;
        return (0);
}

int RT_NOCRT(fesetexceptflag)(const fexcept_t *__flagp, int __excepts);
int RT_NOCRT(feraiseexcept)(int __excepts);

DECLINLINE(int)
fetestexcept(int __excepts)
{
        int __mxcsr, __status;

        __stmxcsr(&__mxcsr);
        __fnstsw(&__status);
        return ((__status | __mxcsr) & __excepts);
}

DECLINLINE(int)
fegetround(void)
{
        int __control;

        /*
         * We assume that the x87 and the SSE unit agree on the
         * rounding mode.  Reading the control word on the x87 turns
         * out to be about 5 times faster than reading it on the SSE
         * unit on an Opteron 244.
         */
        __fnstcw(&__control);
        return (__control & _ROUND_MASK);
}

DECLINLINE(int)
fesetround(int __round)
{
        int __mxcsr, __control;

        if (__round & ~_ROUND_MASK)
                return (-1);

        __fnstcw(&__control);
        __control &= ~_ROUND_MASK;
        __control |= __round;
        __fldcw(__control);

        __stmxcsr(&__mxcsr);
        __mxcsr &= ~(_ROUND_MASK << _SSE_ROUND_SHIFT);
        __mxcsr |= __round << _SSE_ROUND_SHIFT;
        __ldmxcsr(__mxcsr);

        return (0);
}

int RT_NOCRT(fegetenv)(fenv_t *__envp);
int RT_NOCRT(feholdexcept)(fenv_t *__envp);

DECLINLINE(int)
fesetenv(const fenv_t *__envp)
{

        __fldenv(__envp->__x87);
        __ldmxcsr(__envp->__mxcsr);
        return (0);
}

int RT_NOCRT(feupdateenv)(const fenv_t *__envp);
int RT_NOCRT(feenableexcept)(int __mask);
int RT_NOCRT(fedisableexcept)(int __mask);

DECLINLINE(int)
fegetexcept(void)
{
        int __control;

        /*
         * We assume that the masks for the x87 and the SSE unit are
         * the same.
         */
        __fnstcw(&__control);
        return (~__control & FE_ALL_EXCEPT);
}

RT_C_DECLS_END

#ifndef RT_WITHOUT_NOCRT_WRAPPERS
# define fesetexceptflag RT_NOCRT(fesetexceptflag)
# define feraiseexcept RT_NOCRT(feraiseexcept)
# define fegetenv RT_NOCRT(fegetenv)
# define feholdexcept RT_NOCRT(feholdexcept)
# define feupdateenv RT_NOCRT(feupdateenv)
# define feenableexcept RT_NOCRT(feenableexcept)
# define fedisableexcept RT_NOCRT(fedisableexcept)
# define __fe_dfl_env RT_NOCRT(__fe_dfl_env)
#endif

#endif  /* !__iprt_nocrt_amd64_fenv_h__ */
