/** @file
 * VirtualBox - VMCPUSET Operation.
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

#ifndef ___VBox_vmm_vmcpuset_h
#define ___VBox_vmm_vmcpuset_h

#include <VBox/types.h>
#include <iprt/asm.h>
#include <iprt/string.h>

/** @defgroup grp_vmcpuset VMCPUSET Operations
 * @ingroup grp_types_both
 * @sa VMCPUSET
 * @{
 */

/** Tests if a valid CPU ID is present in the set. */
#define VMCPUSET_IS_PRESENT(pSet, idCpu)    ASMBitTest( &(pSet)->au32Bitmap[0], (idCpu))
/** Adds a CPU to the set. */
#define VMCPUSET_ADD(pSet, idCpu)           ASMBitSet(  &(pSet)->au32Bitmap[0], (idCpu))
/** Deletes a CPU from the set. */
#define VMCPUSET_DEL(pSet, idCpu)           ASMBitClear(&(pSet)->au32Bitmap[0], (idCpu))
/** Adds a CPU to the set, atomically. */
#define VMCPUSET_ATOMIC_ADD(pSet, idCpu)    ASMAtomicBitSet(  &(pSet)->au32Bitmap[0], (idCpu))
/** Deletes a CPU from the set, atomically. */
#define VMCPUSET_ATOMIC_DEL(pSet, idCpu)    ASMAtomicBitClear(&(pSet)->au32Bitmap[0], (idCpu))
/** Empties the set. */
#define VMCPUSET_EMPTY(pSet)                memset(&(pSet)->au32Bitmap[0], '\0', sizeof((pSet)->au32Bitmap))
/** Fills the set. */
#define VMCPUSET_FILL(pSet)                 memset(&(pSet)->au32Bitmap[0], 0xff, sizeof((pSet)->au32Bitmap))
/** Checks if two sets are equal to one another. */
#define VMCPUSET_IS_EQUAL(pSet1, pSet2)     (memcmp(&(pSet1)->au32Bitmap[0], &(pSet2)->au32Bitmap[0], sizeof((pSet1)->au32Bitmap)) == 0)
/** Checks if the set is empty. */
#define VMCPUSET_IS_EMPTY(a_pSet)           (   (a_pSet)->au32Bitmap[0] == 0 \
                                             && (a_pSet)->au32Bitmap[1] == 0 \
                                             && (a_pSet)->au32Bitmap[2] == 0 \
                                             && (a_pSet)->au32Bitmap[3] == 0 \
                                             && (a_pSet)->au32Bitmap[4] == 0 \
                                             && (a_pSet)->au32Bitmap[5] == 0 \
                                             && (a_pSet)->au32Bitmap[6] == 0 \
                                             && (a_pSet)->au32Bitmap[7] == 0 \
                                            )
/** Finds the first CPU present in the SET.
 * @returns CPU index if found, NIL_VMCPUID if not. */
#define VMCPUSET_FIND_FIRST_PRESENT(a_pSet) VMCpuSetFindFirstPresentInternal(a_pSet)

/** Implements VMCPUSET_FIND_FIRST_PRESENT.
 *
 * @returns CPU index of the first CPU present in the set, NIL_VMCPUID if none
 *          are present.
 * @param   pSet        The set to scan.
 */
DECLINLINE(int32_t) VMCpuSetFindFirstPresentInternal(PCVMCPUSET pSet)
{
    int i = ASMBitFirstSet(&pSet->au32Bitmap[0], RT_ELEMENTS(pSet->au32Bitmap) * 32);
    return i >= 0 ? (VMCPUID)i : NIL_VMCPUID;
}

/** Finds the first CPU present in the SET.
 * @returns CPU index if found, NIL_VMCPUID if not. */
#define VMCPUSET_FIND_LAST_PRESENT(a_pSet)  VMCpuSetFindLastPresentInternal(a_pSet)

/** Implements VMCPUSET_FIND_LAST_PRESENT.
 *
 * @returns CPU index of the last CPU present in the set, NIL_VMCPUID if none
 *          are present.
 * @param   pSet        The set to scan.
 */
DECLINLINE(int32_t) VMCpuSetFindLastPresentInternal(PCVMCPUSET pSet)
{
    uint32_t i = RT_ELEMENTS(pSet->au32Bitmap);
    while (i-- > 0)
    {
        uint32_t u = pSet->au32Bitmap[i];
        if (u)
        {
            u = ASMBitLastSetU32(u);
            u--;
            u |= i << 5;
            return u;
        }
    }
    return NIL_VMCPUID;
}

/** @} */

#endif

