/** @file
 * Safe way to include ntddk.h.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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


#ifndef ___iprt_nt_ntddk_h___
#define ___iprt_nt_ntddk_h___

/* Make sure we get the right prototypes. */
#pragma warning(push)
#pragma warning(disable:4668) /* Several incorrect __cplusplus uses. */
#pragma warning(disable:4255) /* Incorrect __slwpcb prototype. */
#include <intrin.h>
#pragma warning(pop)

#define _InterlockedExchange           _InterlockedExchange_StupidDDKVsCompilerCrap
#define _InterlockedExchangeAdd        _InterlockedExchangeAdd_StupidDDKVsCompilerCrap
#define _InterlockedCompareExchange    _InterlockedCompareExchange_StupidDDKVsCompilerCrap
#define _InterlockedAddLargeStatistic  _InterlockedAddLargeStatistic_StupidDDKVsCompilerCrap
#define _interlockedbittestandset      _interlockedbittestandset_StupidDDKVsCompilerCrap
#define _interlockedbittestandreset    _interlockedbittestandreset_StupidDDKVsCompilerCrap
#define _interlockedbittestandset64    _interlockedbittestandset64_StupidDDKVsCompilerCrap
#define _interlockedbittestandreset64  _interlockedbittestandreset64_StupidDDKVsCompilerCrap

#pragma warning(push)
#pragma warning(disable:4163)
#pragma warning(disable:4668) /* warning C4668: 'WHEA_DOWNLEVEL_TYPE_NAMES' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
#pragma warning(disable:4255) /* warning C4255: 'ObGetFilterVersion' : no function prototype given: converting '()' to '(void)' */
#if _MSC_VER >= 1800 /*RT_MSC_VER_VC120*/
# pragma warning(disable:4005) /* sdk/v7.1/include/sal_supp.h(57) : warning C4005: '__useHeader' : macro redefinition */
# pragma warning(disable:4471) /* wdm.h(11057) : warning C4471: '_POOL_TYPE' : a forward declaration of an unscoped enumeration must have an underlying type (int assumed) */
#endif
/*RT_C_DECLS_BEGIN - no longer necessary it seems */
#include <ntddk.h>
/*RT_C_DECLS_END - no longer necessary it seems */
#pragma warning(pop)

#undef  _InterlockedExchange
#undef  _InterlockedExchangeAdd
#undef  _InterlockedCompareExchange
#undef  _InterlockedAddLargeStatistic
#undef  _interlockedbittestandset
#undef  _interlockedbittestandreset
#undef  _interlockedbittestandset64
#undef  _interlockedbittestandreset64

#endif

