/* $Id: bs3-cmn-paging.h $ */
/** @file
 * BS3Kit - Internal Paging Structures, Variables and Functions.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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

#ifndef ___bs3_cmn_paging_h
#define ___bs3_cmn_paging_h

#include "bs3kit.h"
#include <iprt/asm.h>

RT_C_DECLS_BEGIN

/** Root directory for page protected mode.
 * UINT32_MAX if not initialized. */
extern uint32_t g_PhysPagingRootPP;
/** Root directory pointer table for PAE mode.
 * UINT32_MAX if not initialized. */
extern uint32_t g_PhysPagingRootPAE;
/** Root table (level 4) for long mode.
 * UINT32_MAX if not initialized. */
extern uint32_t g_PhysPagingRootLM;

#undef bs3PagingGetLegacyPte
BS3_CMN_PROTO_STUB(X86PTE BS3_FAR *, bs3PagingGetLegacyPte,(RTCCUINTXREG cr3, uint32_t uFlat, bool fUseInvlPg, int *prc));
#undef bs3PagingGetPaePte
BS3_CMN_PROTO_STUB(X86PTEPAE BS3_FAR *, bs3PagingGetPaePte,(RTCCUINTXREG cr3, uint8_t bMode, uint64_t uFlat,
                                                            bool fUseInvlPg, int *prc));

RT_C_DECLS_END

#include "bs3kit-mangling-code.h"

#endif

