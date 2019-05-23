/** @file
 * Safe way to include rx.h (DDK/IFS).
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


#ifndef ___iprt_nt_rx_h___
#define ___iprt_nt_rx_h___

#include <iprt/cdefs.h>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4668) /* rxovride.h(38) : warning C4668: 'DBG' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
# pragma warning(disable: 4100) /* Fcb.h(1558) : warning C4100: 'SrvOpen' : unreferenced formal parameter */
# pragma warning(disable: 4115) /* rxce.h(106) : warning C4115: '_ADAPTER_STATUS' : named type definition in parentheses */
# ifndef __cplusplus
#  pragma warning(disable: 4255) /* rxworkq.h(235) : warning C4255: 'RxInitializeDispatcher' : no function prototype given: converting '()' to '(void)' */
# endif
#endif

#include <rx.h>

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif

