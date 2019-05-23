/** @file
 * DBGF - Debugger Facility, selector interface partly shared with SELM.
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


#ifndef ___VBox_vmm_dbgfsel_h
#define ___VBox_vmm_dbgfsel_h

#include <VBox/types.h>
#include <iprt/x86.h>


/** @addtogroup grp_dbgf
 * @{ */

/**
 * Selector information structure.
 */
typedef struct DBGFSELINFO
{
    /** The base address.
     * For gate descriptors, this is the target address.  */
    RTGCPTR         GCPtrBase;
    /** The limit (-1).
     * For gate descriptors, this is set to zero. */
    RTGCUINTPTR     cbLimit;
    /** The raw descriptor. */
    union
    {
        X86DESC     Raw;
        X86DESC64   Raw64;
    } u;
    /** The selector. */
    RTSEL           Sel;
    /** The target selector for a gate.
     * This is 0 if non-gate descriptor. */
    RTSEL           SelGate;
    /** Flags. */
    uint32_t        fFlags;
} DBGFSELINFO;
/** Pointer to a SELM selector information struct. */
typedef DBGFSELINFO *PDBGFSELINFO;
/** Pointer to a const SELM selector information struct. */
typedef const DBGFSELINFO *PCDBGFSELINFO;

/** @name DBGFSELINFO::fFlags
 * @{ */
/** The CPU is in real mode. */
#define DBGFSELINFO_FLAGS_REAL_MODE     RT_BIT_32(0)
/** The CPU is in protected mode. */
#define DBGFSELINFO_FLAGS_PROT_MODE     RT_BIT_32(1)
/** The CPU is in long mode. */
#define DBGFSELINFO_FLAGS_LONG_MODE     RT_BIT_32(2)
/** The selector is a hyper selector. */
#define DBGFSELINFO_FLAGS_HYPER         RT_BIT_32(3)
/** The selector is a gate selector. */
#define DBGFSELINFO_FLAGS_GATE          RT_BIT_32(4)
/** The selector is invalid. */
#define DBGFSELINFO_FLAGS_INVALID       RT_BIT_32(5)
/** The selector not present. */
#define DBGFSELINFO_FLAGS_NOT_PRESENT   RT_BIT_32(6)
/** @}  */


/**
 * Tests whether the selector info describes an expand-down selector or now.
 *
 * @returns true / false.
 * @param   pSelInfo        The selector info.
 */
DECLINLINE(bool) DBGFSelInfoIsExpandDown(PCDBGFSELINFO pSelInfo)
{
    return (pSelInfo)->u.Raw.Gen.u1DescType
        && ((pSelInfo)->u.Raw.Gen.u4Type & (X86_SEL_TYPE_DOWN | X86_SEL_TYPE_CODE)) == X86_SEL_TYPE_DOWN;
}


VMMR3DECL(int) DBGFR3SelInfoValidateCS(PCDBGFSELINFO pSelInfo, RTSEL SelCPL);

/** @}  */

#endif

