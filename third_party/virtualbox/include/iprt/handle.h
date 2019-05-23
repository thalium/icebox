/** @file
 * IPRT - Generic Handle Operations.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

#ifndef ___iprt_handle_h
#define ___iprt_handle_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_handle     RTHandle - Generic Handle Operations
 * @ingroup grp_rt
 * @{
 */

/**
 * Closes or destroy a generic handle.
 *
 * @returns IPRT status code.
 * @param   ph              Pointer to the generic handle.  The structure handle
 *                          will be set to NIL.  A NULL pointer or a NIL handle
 *                          will be quietly ignore (VINF_SUCCESS).
 */
RTDECL(int) RTHandleClose(PRTHANDLE ph);

/**
 * Gets one of the standard handles.
 *
 * @returns IPRT status code.
 * @param   enmStdHandle    The standard handle.
 * @param   ph              Pointer to the generic handle.  This will contain
 *                          the most appropriate IPRT handle on success.
 */
RTDECL(int) RTHandleGetStandard(RTHANDLESTD enmStdHandle, PRTHANDLE ph);

/** @} */

RT_C_DECLS_END

#endif

