/* $Id: alignmentchecks.h $ */
/** @file
 * IPRT - Internal header for hacking alignment checks on x86 and AMD64.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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


#ifndef ___internal_alignmentchecks_h
#define ___internal_alignmentchecks_h

/** @def IPRT_WITH_ALIGNMENT_CHECKS
 * Enables or disables the alignment check feature and related hacks. */

#ifndef IPRT_WITH_ALIGNMENT_CHECKS
# if ( defined(DEBUG) && !defined(IN_GUEST) ) || defined(DOXYGEN_RUNNING)
#  define IPRT_WITH_ALIGNMENT_CHECKS 1
# endif
#endif

/** @def IPRT_ALIGNMENT_CHECKS_DISABLE
 * Disables alignment checks.
 * Typically used before calling problematic library functions.
 */

/** @def IPRT_ALIGNMENT_CHECKS_ENABLE
 * (re-)Enables alignment checks if they are supposed to be active.
 * This is used to counter IPRT_ALIGNMENT_CHECKS_DISABLE as well as enabling
 * them for the first time.
 */

#if defined(IPRT_WITH_ALIGNMENT_CHECKS) \
 && (   defined(RT_ARCH_AMD64) \
     || defined(RT_ARCH_X86) )
# include <iprt/asm-amd64-x86.h>

RT_C_DECLS_BEGIN
extern RTDATADECL(bool) g_fRTAlignmentChecks;
RT_C_DECLS_END

# define IPRT_ALIGNMENT_CHECKS_DISABLE() \
    do { if (g_fRTAlignmentChecks) ASMSetFlags(ASMGetFlags() & ~RT_BIT_32(18)); } while (0)

# define IPRT_ALIGNMENT_CHECKS_ENABLE() \
    do { if (g_fRTAlignmentChecks) ASMSetFlags(ASMGetFlags() | RT_BIT_32(18)); } while (0)

#else
# define IPRT_ALIGNMENT_CHECKS_DISABLE() do {} while (0)
# define IPRT_ALIGNMENT_CHECKS_ENABLE()  do {} while (0)
#endif

#endif

