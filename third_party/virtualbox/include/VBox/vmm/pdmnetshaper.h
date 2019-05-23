/** @file
 * PDM - Pluggable Device Manager, Network Shaper.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

#ifndef ___VBox_vmm_pdmnetshaper_h
#define ___VBox_vmm_pdmnetshaper_h

#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/vmm/pdmnetifs.h>
#include <iprt/assert.h>
#include <iprt/sg.h>


/** @defgroup grp_pdm_net_shaper  The PDM Network Shaper API
 * @ingroup grp_pdm
 * @{
 */


#define PDM_NETSHAPER_MIN_BUCKET_SIZE UINT32_C(65536) /**< bytes */
#define PDM_NETSHAPER_MAX_LATENCY     UINT32_C(100)   /**< milliseconds */

RT_C_DECLS_BEGIN

typedef struct PDMNSFILTER
{
    /** Pointer to the next group in the list (ring-3). */
    R3PTRTYPE(struct PDMNSFILTER *)     pNextR3;
    /** Pointer to the bandwidth group (ring-3). */
    R3PTRTYPE(struct PDMNSBWGROUP *)    pBwGroupR3;
    /** Pointer to the bandwidth group (ring-0). */
    R0PTRTYPE(struct PDMNSBWGROUP *)    pBwGroupR0;
    /** Set when the filter fails to obtain bandwidth. */
    bool                                fChoked;
    /** Aligment padding. */
    bool                                afPadding[HC_ARCH_BITS == 32 ? 3 : 7];
    /** The driver this filter is aggregated into (ring-3). */
    R3PTRTYPE(PPDMINETWORKDOWN)         pIDrvNetR3;
} PDMNSFILTER;

/** Pointer to a PDM filter handle. */
typedef struct PDMNSFILTER *PPDMNSFILTER;
/** Pointer to a network shaper. */
typedef struct PDMNETSHAPER *PPDMNETSHAPER;


VMMDECL(bool)       PDMNsAllocateBandwidth(PPDMNSFILTER pFilter, size_t cbTransfer);
VMMR3_INT_DECL(int) PDMR3NsAttach(PUVM pUVM, PPDMDRVINS pDrvIns, const char *pcszBwGroup, PPDMNSFILTER pFilter);
VMMR3_INT_DECL(int) PDMR3NsDetach(PUVM pUVM, PPDMDRVINS pDrvIns, PPDMNSFILTER pFilter);
VMMR3DECL(int)      PDMR3NsBwGroupSetLimit(PUVM pUVM, const char *pszBwGroup, uint64_t cbPerSecMax);

/** @} */

RT_C_DECLS_END

#endif

