/** @file
 * IPRT / No-CRT - Our own setjmp header.
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
 */

#ifndef ___iprt_nocrt_setjmp_h
#define ___iprt_nocrt_setjmp_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

#ifdef RT_ARCH_AMD64
# ifdef RT_OS_WINDOWS /* Also saves rsi, rdi and xmm6-xmm15. */
typedef uint64_t RT_NOCRT(jmp_buf)[10 + (2*10)];
# else
typedef uint64_t RT_NOCRT(jmp_buf)[8];
# endif
#else
typedef uint32_t RT_NOCRT(jmp_buf)[6+2];
#endif

extern DECL_RETURNS_TWICE(int) RT_NOCRT(setjmp)(RT_NOCRT(jmp_buf));
extern DECL_NO_RETURN(int)     RT_NOCRT(longjmp)(RT_NOCRT(jmp_buf), int);

#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
# define jmp_buf RT_NOCRT(jmp_buf)
# define setjmp RT_NOCRT(setjmp)
# define longjmp RT_NOCRT(longjmp)
#endif

RT_C_DECLS_END

#endif

