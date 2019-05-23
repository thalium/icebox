/* $Id: IOMAllMMIO.cpp $ */
/** @file
 * IOM - Input / Output Monitor - Any Context, MMIO & String I/O.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_IOM
#include <VBox/vmm/iom.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/iem.h>
#include "IOMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/hm.h>
#include "IOMInline.h"

#include <VBox/dis.h>
#include <VBox/disopcode.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/string.h>



#ifndef IN_RING3
/**
 * Defers a pending MMIO write to ring-3.
 *
 * @returns VINF_IOM_R3_MMIO_COMMIT_WRITE
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   GCPhys      The write address.
 * @param   pvBuf       The bytes being written.
 * @param   cbBuf       How many bytes.
 * @param   pRange      The range, if resolved.
 */
static VBOXSTRICTRC iomMmioRing3WritePending(PVMCPU pVCpu, RTGCPHYS GCPhys, void const *pvBuf, size_t cbBuf, PIOMMMIORANGE pRange)
{
    Log5(("iomMmioRing3WritePending: %RGp LB %#x\n", GCPhys, cbBuf));
    AssertReturn(pVCpu->iom.s.PendingMmioWrite.cbValue == 0, VERR_IOM_MMIO_IPE_1);
    pVCpu->iom.s.PendingMmioWrite.GCPhys  = GCPhys;
    AssertReturn(cbBuf <= sizeof(pVCpu->iom.s.PendingMmioWrite.abValue), VERR_IOM_MMIO_IPE_2);
    pVCpu->iom.s.PendingMmioWrite.cbValue = (uint32_t)cbBuf;
    memcpy(pVCpu->iom.s.PendingMmioWrite.abValue, pvBuf, cbBuf);
    VMCPU_FF_SET(pVCpu, VMCPU_FF_IOM);
    RT_NOREF_PV(pRange);
    return VINF_IOM_R3_MMIO_COMMIT_WRITE;
}
#endif


/**
 * Deals with complicated MMIO writes.
 *
 * Complicated means unaligned or non-dword/qword sized accesses depending on
 * the MMIO region's access mode flags.
 *
 * @returns Strict VBox status code. Any EM scheduling status code,
 *          VINF_IOM_R3_MMIO_WRITE, VINF_IOM_R3_MMIO_READ_WRITE or
 *          VINF_IOM_R3_MMIO_READ may be returned.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   pRange      The range to write to.
 * @param   GCPhys      The physical address to start writing.
 * @param   pvValue     Where to store the value.
 * @param   cbValue     The size of the value to write.
 */
static VBOXSTRICTRC iomMMIODoComplicatedWrite(PVM pVM, PVMCPU pVCpu, PIOMMMIORANGE pRange, RTGCPHYS GCPhys,
                                              void const *pvValue, unsigned cbValue)
{
    RT_NOREF_PV(pVCpu);
    AssertReturn(   (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) != IOMMMIO_FLAGS_WRITE_PASSTHRU
                 && (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) <= IOMMMIO_FLAGS_WRITE_DWORD_QWORD_READ_MISSING,
                 VERR_IOM_MMIO_IPE_1);
    AssertReturn(cbValue != 0 && cbValue <= 16, VERR_IOM_MMIO_IPE_2);
    RTGCPHYS const GCPhysStart  = GCPhys; NOREF(GCPhysStart);
    bool const     fReadMissing = (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_DWORD_READ_MISSING
                               || (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_DWORD_QWORD_READ_MISSING;

    /*
     * Do debug stop if requested.
     */
    int rc = VINF_SUCCESS; NOREF(pVM);
#ifdef VBOX_STRICT
    if (pRange->fFlags & IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_WRITE)
    {
# ifdef IN_RING3
        LogRel(("IOM: Complicated write %#x byte at %RGp to %s, initiating debugger intervention\n", cbValue, GCPhys,
                R3STRING(pRange->pszDesc)));
        rc = DBGFR3EventSrc(pVM, DBGFEVENT_DEV_STOP, RT_SRC_POS,
                            "Complicated write %#x byte at %RGp to %s\n", cbValue, GCPhys, R3STRING(pRange->pszDesc));
        if (rc == VERR_DBGF_NOT_ATTACHED)
            rc = VINF_SUCCESS;
# else
        return VINF_IOM_R3_MMIO_WRITE;
# endif
    }
#endif

    /*
     * Check if we should ignore the write.
     */
    if ((pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_ONLY_DWORD)
    {
        Assert(cbValue != 4 || (GCPhys & 3));
        return VINF_SUCCESS;
    }
    if ((pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_ONLY_DWORD_QWORD)
    {
        Assert((cbValue != 4 && cbValue != 8) || (GCPhys & (cbValue - 1)));
        return VINF_SUCCESS;
    }

    /*
     * Split and conquer.
     */
    for (;;)
    {
        unsigned const  offAccess  = GCPhys & 3;
        unsigned        cbThisPart = 4 - offAccess;
        if (cbThisPart > cbValue)
            cbThisPart = cbValue;

        /*
         * Get the missing bits (if any).
         */
        uint32_t u32MissingValue = 0;
        if (fReadMissing && cbThisPart != 4)
        {
            int rc2 = pRange->CTX_SUFF(pfnReadCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser),
                                                        GCPhys & ~(RTGCPHYS)3, &u32MissingValue, sizeof(u32MissingValue));
            switch (rc2)
            {
                case VINF_SUCCESS:
                    break;
                case VINF_IOM_MMIO_UNUSED_FF:
                    u32MissingValue = UINT32_C(0xffffffff);
                    break;
                case VINF_IOM_MMIO_UNUSED_00:
                    u32MissingValue = 0;
                    break;
#ifndef IN_RING3
                case VINF_IOM_R3_MMIO_READ:
                case VINF_IOM_R3_MMIO_READ_WRITE:
                case VINF_IOM_R3_MMIO_WRITE:
                    LogFlow(("iomMMIODoComplicatedWrite: GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc [read]\n", GCPhys, GCPhysStart, cbValue, rc2));
                    rc2 = VBOXSTRICTRC_TODO(iomMmioRing3WritePending(pVCpu, GCPhys, pvValue, cbValue, pRange));
                    if (rc == VINF_SUCCESS || rc2 < rc)
                        rc = rc2;
                    return rc;
#endif
                default:
                    if (RT_FAILURE(rc2))
                    {
                        Log(("iomMMIODoComplicatedWrite: GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc [read]\n", GCPhys, GCPhysStart, cbValue, rc2));
                        return rc2;
                    }
                    AssertMsgReturn(rc2 >= VINF_EM_FIRST && rc2 <= VINF_EM_LAST, ("%Rrc\n", rc2), VERR_IPE_UNEXPECTED_INFO_STATUS);
                    if (rc == VINF_SUCCESS || rc2 < rc)
                        rc = rc2;
                    break;
            }
        }

        /*
         * Merge missing and given bits.
         */
        uint32_t u32GivenMask;
        uint32_t u32GivenValue;
        switch (cbThisPart)
        {
            case 1:
                u32GivenValue = *(uint8_t  const *)pvValue;
                u32GivenMask  = UINT32_C(0x000000ff);
                break;
            case 2:
                u32GivenValue = *(uint16_t const *)pvValue;
                u32GivenMask  = UINT32_C(0x0000ffff);
                break;
            case 3:
                u32GivenValue = RT_MAKE_U32_FROM_U8(((uint8_t const *)pvValue)[0], ((uint8_t const *)pvValue)[1],
                                                    ((uint8_t const *)pvValue)[2], 0);
                u32GivenMask  = UINT32_C(0x00ffffff);
                break;
            case 4:
                u32GivenValue = *(uint32_t const *)pvValue;
                u32GivenMask  = UINT32_C(0xffffffff);
                break;
            default:
                AssertFailedReturn(VERR_IOM_MMIO_IPE_3);
        }
        if (offAccess)
        {
            u32GivenValue <<= offAccess * 8;
            u32GivenMask  <<= offAccess * 8;
        }

        uint32_t u32Value = (u32MissingValue & ~u32GivenMask)
                          | (u32GivenValue & u32GivenMask);

        /*
         * Do DWORD write to the device.
         */
        int rc2 = pRange->CTX_SUFF(pfnWriteCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser),
                                                     GCPhys & ~(RTGCPHYS)3, &u32Value, sizeof(u32Value));
        switch (rc2)
        {
            case VINF_SUCCESS:
                break;
#ifndef IN_RING3
            case VINF_IOM_R3_MMIO_READ:
            case VINF_IOM_R3_MMIO_READ_WRITE:
            case VINF_IOM_R3_MMIO_WRITE:
                Log3(("iomMMIODoComplicatedWrite: deferring GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc [write]\n", GCPhys, GCPhysStart, cbValue, rc2));
                AssertReturn(pVCpu->iom.s.PendingMmioWrite.cbValue == 0, VERR_IOM_MMIO_IPE_1);
                AssertReturn(cbValue + (GCPhys & 3) <= sizeof(pVCpu->iom.s.PendingMmioWrite.abValue), VERR_IOM_MMIO_IPE_2);
                pVCpu->iom.s.PendingMmioWrite.GCPhys  = GCPhys & ~(RTGCPHYS)3;
                pVCpu->iom.s.PendingMmioWrite.cbValue = cbValue + (GCPhys & 3);
                *(uint32_t *)pVCpu->iom.s.PendingMmioWrite.abValue = u32Value;
                if (cbValue > cbThisPart)
                    memcpy(&pVCpu->iom.s.PendingMmioWrite.abValue[4],
                           (uint8_t const *)pvValue + cbThisPart, cbValue - cbThisPart);
                VMCPU_FF_SET(pVCpu, VMCPU_FF_IOM);
                if (rc == VINF_SUCCESS)
                    rc = VINF_IOM_R3_MMIO_COMMIT_WRITE;
                return rc2;
#endif
            default:
                if (RT_FAILURE(rc2))
                {
                    Log(("iomMMIODoComplicatedWrite: GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc [write]\n", GCPhys, GCPhysStart, cbValue, rc2));
                    return rc2;
                }
                AssertMsgReturn(rc2 >= VINF_EM_FIRST && rc2 <= VINF_EM_LAST, ("%Rrc\n", rc2), VERR_IPE_UNEXPECTED_INFO_STATUS);
                if (rc == VINF_SUCCESS || rc2 < rc)
                    rc = rc2;
                break;
        }

        /*
         * Advance.
         */
        cbValue -= cbThisPart;
        if (!cbValue)
            break;
        GCPhys += cbThisPart;
        pvValue = (uint8_t const *)pvValue + cbThisPart;
    }

    return rc;
}




/**
 * Wrapper which does the write and updates range statistics when such are enabled.
 * @warning RT_SUCCESS(rc=VINF_IOM_R3_MMIO_WRITE) is TRUE!
 */
static VBOXSTRICTRC iomMMIODoWrite(PVM pVM, PVMCPU pVCpu, PIOMMMIORANGE pRange, RTGCPHYS GCPhysFault,
                                   const void *pvData, unsigned cb)
{
#ifdef VBOX_WITH_STATISTICS
    int rcSem = IOM_LOCK_SHARED(pVM);
    if (rcSem == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_WRITE;
    PIOMMMIOSTATS pStats = iomMmioGetStats(pVM, pVCpu, GCPhysFault, pRange);
    if (!pStats)
# ifdef IN_RING3
        return VERR_NO_MEMORY;
# else
        return VINF_IOM_R3_MMIO_WRITE;
# endif
    STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfWrite), a);
#else
    NOREF(pVCpu);
#endif

    VBOXSTRICTRC rcStrict;
    if (RT_LIKELY(pRange->CTX_SUFF(pfnWriteCallback)))
    {
        if (   (cb == 4 && !(GCPhysFault & 3))
            || (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_PASSTHRU
            || (cb == 8 && !(GCPhysFault & 7) && IOMMMIO_DOES_WRITE_MODE_ALLOW_QWORD(pRange->fFlags)) )
            rcStrict = pRange->CTX_SUFF(pfnWriteCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser),
                                                          GCPhysFault, (void *)pvData, cb); /** @todo fix const!! */
        else
            rcStrict = iomMMIODoComplicatedWrite(pVM, pVCpu, pRange, GCPhysFault, pvData, cb);
    }
    else
        rcStrict = VINF_SUCCESS;

    STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfWrite), a);
    STAM_COUNTER_INC(&pStats->Accesses);
    return rcStrict;
}


/**
 * Deals with complicated MMIO reads.
 *
 * Complicated means unaligned or non-dword/qword sized accesses depending on
 * the MMIO region's access mode flags.
 *
 * @returns Strict VBox status code. Any EM scheduling status code,
 *          VINF_IOM_R3_MMIO_READ, VINF_IOM_R3_MMIO_READ_WRITE or
 *          VINF_IOM_R3_MMIO_WRITE may be returned.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pRange              The range to read from.
 * @param   GCPhys              The physical address to start reading.
 * @param   pvValue             Where to store the value.
 * @param   cbValue             The size of the value to read.
 */
static VBOXSTRICTRC iomMMIODoComplicatedRead(PVM pVM, PIOMMMIORANGE pRange, RTGCPHYS GCPhys, void *pvValue, unsigned cbValue)
{
    AssertReturn(   (pRange->fFlags & IOMMMIO_FLAGS_READ_MODE) == IOMMMIO_FLAGS_READ_DWORD
                 || (pRange->fFlags & IOMMMIO_FLAGS_READ_MODE) == IOMMMIO_FLAGS_READ_DWORD_QWORD,
                 VERR_IOM_MMIO_IPE_1);
    AssertReturn(cbValue != 0 && cbValue <= 16, VERR_IOM_MMIO_IPE_2);
    RTGCPHYS const GCPhysStart = GCPhys; NOREF(GCPhysStart);

    /*
     * Do debug stop if requested.
     */
    int rc = VINF_SUCCESS; NOREF(pVM);
#ifdef VBOX_STRICT
    if (pRange->fFlags & IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_READ)
    {
# ifdef IN_RING3
        rc = DBGFR3EventSrc(pVM, DBGFEVENT_DEV_STOP, RT_SRC_POS,
                            "Complicated read %#x byte at %RGp to %s\n", cbValue, GCPhys, R3STRING(pRange->pszDesc));
        if (rc == VERR_DBGF_NOT_ATTACHED)
            rc = VINF_SUCCESS;
# else
        return VINF_IOM_R3_MMIO_READ;
# endif
    }
#endif

    /*
     * Split and conquer.
     */
    for (;;)
    {
        /*
         * Do DWORD read from the device.
         */
        uint32_t u32Value;
        int rc2 = pRange->CTX_SUFF(pfnReadCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser),
                                                    GCPhys & ~(RTGCPHYS)3, &u32Value, sizeof(u32Value));
        switch (rc2)
        {
            case VINF_SUCCESS:
                break;
            case VINF_IOM_MMIO_UNUSED_FF:
                u32Value = UINT32_C(0xffffffff);
                break;
            case VINF_IOM_MMIO_UNUSED_00:
                u32Value = 0;
                break;
            case VINF_IOM_R3_MMIO_READ:
            case VINF_IOM_R3_MMIO_READ_WRITE:
            case VINF_IOM_R3_MMIO_WRITE:
                /** @todo What if we've split a transfer and already read
                 * something?  Since reads can have sideeffects we could be
                 * kind of screwed here... */
                LogFlow(("iomMMIODoComplicatedRead: GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc\n", GCPhys, GCPhysStart, cbValue, rc2));
                return rc2;
            default:
                if (RT_FAILURE(rc2))
                {
                    Log(("iomMMIODoComplicatedRead: GCPhys=%RGp GCPhysStart=%RGp cbValue=%u rc=%Rrc\n", GCPhys, GCPhysStart, cbValue, rc2));
                    return rc2;
                }
                AssertMsgReturn(rc2 >= VINF_EM_FIRST && rc2 <= VINF_EM_LAST, ("%Rrc\n", rc2), VERR_IPE_UNEXPECTED_INFO_STATUS);
                if (rc == VINF_SUCCESS || rc2 < rc)
                    rc = rc2;
                break;
        }
        u32Value >>= (GCPhys & 3) * 8;

        /*
         * Write what we've read.
         */
        unsigned cbThisPart = 4 - (GCPhys & 3);
        if (cbThisPart > cbValue)
            cbThisPart = cbValue;

        switch (cbThisPart)
        {
            case 1:
                *(uint8_t *)pvValue = (uint8_t)u32Value;
                break;
            case 2:
                *(uint16_t *)pvValue = (uint16_t)u32Value;
                break;
            case 3:
                ((uint8_t *)pvValue)[0] = RT_BYTE1(u32Value);
                ((uint8_t *)pvValue)[1] = RT_BYTE2(u32Value);
                ((uint8_t *)pvValue)[2] = RT_BYTE3(u32Value);
                break;
            case 4:
                *(uint32_t *)pvValue = u32Value;
                break;
        }

        /*
         * Advance.
         */
        cbValue -= cbThisPart;
        if (!cbValue)
            break;
        GCPhys += cbThisPart;
        pvValue = (uint8_t *)pvValue + cbThisPart;
    }

    return rc;
}


/**
 * Implements VINF_IOM_MMIO_UNUSED_FF.
 *
 * @returns VINF_SUCCESS.
 * @param   pvValue             Where to store the zeros.
 * @param   cbValue             How many bytes to read.
 */
static int iomMMIODoReadFFs(void *pvValue, size_t cbValue)
{
    switch (cbValue)
    {
        case 1: *(uint8_t  *)pvValue = UINT8_C(0xff); break;
        case 2: *(uint16_t *)pvValue = UINT16_C(0xffff); break;
        case 4: *(uint32_t *)pvValue = UINT32_C(0xffffffff); break;
        case 8: *(uint64_t *)pvValue = UINT64_C(0xffffffffffffffff); break;
        default:
        {
            uint8_t *pb = (uint8_t *)pvValue;
            while (cbValue--)
                *pb++ = UINT8_C(0xff);
            break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Implements VINF_IOM_MMIO_UNUSED_00.
 *
 * @returns VINF_SUCCESS.
 * @param   pvValue             Where to store the zeros.
 * @param   cbValue             How many bytes to read.
 */
static int iomMMIODoRead00s(void *pvValue, size_t cbValue)
{
    switch (cbValue)
    {
        case 1: *(uint8_t  *)pvValue = UINT8_C(0x00); break;
        case 2: *(uint16_t *)pvValue = UINT16_C(0x0000); break;
        case 4: *(uint32_t *)pvValue = UINT32_C(0x00000000); break;
        case 8: *(uint64_t *)pvValue = UINT64_C(0x0000000000000000); break;
        default:
        {
            uint8_t *pb = (uint8_t *)pvValue;
            while (cbValue--)
                *pb++ = UINT8_C(0x00);
            break;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Wrapper which does the read and updates range statistics when such are enabled.
 */
DECLINLINE(VBOXSTRICTRC) iomMMIODoRead(PVM pVM, PVMCPU pVCpu, PIOMMMIORANGE pRange, RTGCPHYS GCPhys,
                                       void *pvValue, unsigned cbValue)
{
#ifdef VBOX_WITH_STATISTICS
    int rcSem = IOM_LOCK_SHARED(pVM);
    if (rcSem == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_READ;
    PIOMMMIOSTATS pStats = iomMmioGetStats(pVM, pVCpu, GCPhys, pRange);
    if (!pStats)
# ifdef IN_RING3
        return VERR_NO_MEMORY;
# else
        return VINF_IOM_R3_MMIO_READ;
# endif
    STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfRead), a);
#else
    NOREF(pVCpu);
#endif

    VBOXSTRICTRC rcStrict;
    if (RT_LIKELY(pRange->CTX_SUFF(pfnReadCallback)))
    {
        if (   (   cbValue == 4
                && !(GCPhys & 3))
            || (pRange->fFlags & IOMMMIO_FLAGS_READ_MODE) == IOMMMIO_FLAGS_READ_PASSTHRU
            || (    cbValue == 8
                && !(GCPhys & 7)
                && (pRange->fFlags & IOMMMIO_FLAGS_READ_MODE) == IOMMMIO_FLAGS_READ_DWORD_QWORD ) )
            rcStrict = pRange->CTX_SUFF(pfnReadCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser), GCPhys,
                                                         pvValue, cbValue);
        else
            rcStrict = iomMMIODoComplicatedRead(pVM, pRange, GCPhys, pvValue, cbValue);
    }
    else
        rcStrict = VINF_IOM_MMIO_UNUSED_FF;
    if (rcStrict != VINF_SUCCESS)
    {
        switch (VBOXSTRICTRC_VAL(rcStrict))
        {
            case VINF_IOM_MMIO_UNUSED_FF: rcStrict = iomMMIODoReadFFs(pvValue, cbValue); break;
            case VINF_IOM_MMIO_UNUSED_00: rcStrict = iomMMIODoRead00s(pvValue, cbValue); break;
        }
    }

    STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfRead), a);
    STAM_COUNTER_INC(&pStats->Accesses);
    return rcStrict;
}

/**
 * Common worker for the \#PF handler and IOMMMIOPhysHandler (APIC+VT-x).
 *
 * @returns VBox status code (appropriate for GC return).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   uErrorCode  CPU Error code.  This is UINT32_MAX when we don't have
 *                      any error code (the EPT misconfig hack).
 * @param   pCtxCore    Trap register frame.
 * @param   GCPhysFault The GC physical address corresponding to pvFault.
 * @param   pvUser      Pointer to the MMIO ring-3 range entry.
 */
static VBOXSTRICTRC iomMmioCommonPfHandler(PVM pVM, PVMCPU pVCpu, uint32_t uErrorCode, PCPUMCTXCORE pCtxCore,
                                           RTGCPHYS GCPhysFault, void *pvUser)
{
    RT_NOREF_PV(uErrorCode);
    int rc = IOM_LOCK_SHARED(pVM);
#ifndef IN_RING3
    if (rc == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_READ_WRITE;
#endif
    AssertRC(rc);

    STAM_PROFILE_START(&pVM->iom.s.StatRZMMIOHandler, a);
    Log(("iomMmioCommonPfHandler: GCPhys=%RGp uErr=%#x rip=%RGv\n", GCPhysFault, uErrorCode, (RTGCPTR)pCtxCore->rip));

    PIOMMMIORANGE pRange = (PIOMMMIORANGE)pvUser;
    Assert(pRange);
    Assert(pRange == iomMmioGetRange(pVM, pVCpu, GCPhysFault));
    iomMmioRetainRange(pRange);
#ifndef VBOX_WITH_STATISTICS
    IOM_UNLOCK_SHARED(pVM);

#else
    /*
     * Locate the statistics.
     */
    PIOMMMIOSTATS pStats = iomMmioGetStats(pVM, pVCpu, GCPhysFault, pRange);
    if (!pStats)
    {
        iomMmioReleaseRange(pVM, pRange);
# ifdef IN_RING3
        return VERR_NO_MEMORY;
# else
        STAM_PROFILE_STOP(&pVM->iom.s.StatRZMMIOHandler, a);
        STAM_COUNTER_INC(&pVM->iom.s.StatRZMMIOFailures);
        return VINF_IOM_R3_MMIO_READ_WRITE;
# endif
    }
#endif

#ifndef IN_RING3
    /*
     * Should we defer the request right away?  This isn't usually the case, so
     * do the simple test first and the try deal with uErrorCode being N/A.
     */
    if (RT_UNLIKELY(   (   !pRange->CTX_SUFF(pfnWriteCallback)
                        || !pRange->CTX_SUFF(pfnReadCallback))
                    && (  uErrorCode == UINT32_MAX
                        ? pRange->pfnWriteCallbackR3 || pRange->pfnReadCallbackR3
                        : uErrorCode & X86_TRAP_PF_RW
                          ? !pRange->CTX_SUFF(pfnWriteCallback) && pRange->pfnWriteCallbackR3
                          : !pRange->CTX_SUFF(pfnReadCallback)  && pRange->pfnReadCallbackR3
                        )
                   )
       )
    {
        if (uErrorCode & X86_TRAP_PF_RW)
            STAM_COUNTER_INC(&pStats->CTX_MID_Z(Write,ToR3));
        else
            STAM_COUNTER_INC(&pStats->CTX_MID_Z(Read,ToR3));

        STAM_PROFILE_STOP(&pVM->iom.s.StatRZMMIOHandler, a);
        STAM_COUNTER_INC(&pVM->iom.s.StatRZMMIOFailures);
        iomMmioReleaseRange(pVM, pRange);
        return VINF_IOM_R3_MMIO_READ_WRITE;
    }
#endif /* !IN_RING3 */

    /*
     * Retain the range and do locking.
     */
    PPDMDEVINS pDevIns = pRange->CTX_SUFF(pDevIns);
    rc = PDMCritSectEnter(pDevIns->CTX_SUFF(pCritSectRo), VINF_IOM_R3_MMIO_READ_WRITE);
    if (rc != VINF_SUCCESS)
    {
        iomMmioReleaseRange(pVM, pRange);
        return rc;
    }

    /*
     * Let IEM call us back via iomMmioHandler.
     */
    VBOXSTRICTRC rcStrict = IEMExecOne(pVCpu);

    NOREF(pCtxCore); NOREF(GCPhysFault);
    STAM_PROFILE_STOP(&pVM->iom.s.StatRZMMIOHandler, a);
    PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
    iomMmioReleaseRange(pVM, pRange);
    if (RT_SUCCESS(rcStrict))
        return rcStrict;
    if (   rcStrict == VERR_IEM_ASPECT_NOT_IMPLEMENTED
        || rcStrict == VERR_IEM_INSTR_NOT_IMPLEMENTED)
    {
        Log(("IOM: Hit unsupported IEM feature!\n"));
        rcStrict = VINF_EM_RAW_EMULATE_INSTR;
    }
    return rcStrict;
}


/**
 * @callback_method_impl{FNPGMRZPHYSPFHANDLER,
 *      \#PF access handler callback for MMIO pages.}
 *
 * @remarks The @a pvUser argument points to the IOMMMIORANGE.
 */
DECLEXPORT(VBOXSTRICTRC) iomMmioPfHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pCtxCore, RTGCPTR pvFault,
                                          RTGCPHYS GCPhysFault, void *pvUser)
{
    LogFlow(("iomMmioPfHandler: GCPhys=%RGp uErr=%#x pvFault=%RGv rip=%RGv\n",
             GCPhysFault, (uint32_t)uErrorCode, pvFault, (RTGCPTR)pCtxCore->rip)); NOREF(pvFault);
    return iomMmioCommonPfHandler(pVM, pVCpu, (uint32_t)uErrorCode, pCtxCore, GCPhysFault, pvUser);
}


/**
 * Physical access handler for MMIO ranges.
 *
 * @returns VBox status code (appropriate for GC return).
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   uErrorCode  CPU Error code.
 * @param   pCtxCore    Trap register frame.
 * @param   GCPhysFault The GC physical address.
 */
VMMDECL(VBOXSTRICTRC) IOMMMIOPhysHandler(PVM pVM, PVMCPU pVCpu, RTGCUINT uErrorCode, PCPUMCTXCORE pCtxCore, RTGCPHYS GCPhysFault)
{
    /*
     * We don't have a range here, so look it up before calling the common function.
     */
    int rc2 = IOM_LOCK_SHARED(pVM); NOREF(rc2);
#ifndef IN_RING3
    if (rc2 == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_READ_WRITE;
#endif
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhysFault);
    if (RT_UNLIKELY(!pRange))
    {
        IOM_UNLOCK_SHARED(pVM);
        return VERR_IOM_MMIO_RANGE_NOT_FOUND;
    }
    iomMmioRetainRange(pRange);
    IOM_UNLOCK_SHARED(pVM);

    VBOXSTRICTRC rcStrict = iomMmioCommonPfHandler(pVM, pVCpu, (uint32_t)uErrorCode, pCtxCore, GCPhysFault, pRange);

    iomMmioReleaseRange(pVM, pRange);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * @callback_method_impl{FNPGMPHYSHANDLER, MMIO page accesses}
 *
 * @remarks The @a pvUser argument points to the MMIO range entry.
 */
PGM_ALL_CB2_DECL(VBOXSTRICTRC) iomMmioHandler(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhysFault, void *pvPhys, void *pvBuf,
                                              size_t cbBuf, PGMACCESSTYPE enmAccessType, PGMACCESSORIGIN enmOrigin, void *pvUser)
{
    PIOMMMIORANGE pRange = (PIOMMMIORANGE)pvUser;
    STAM_COUNTER_INC(&pVM->iom.s.StatR3MMIOHandler);

    NOREF(pvPhys); NOREF(enmOrigin);
    AssertPtr(pRange);
    AssertMsg(cbBuf >= 1, ("%zu\n", cbBuf));


#ifndef IN_RING3
    /*
     * If someone is doing FXSAVE, FXRSTOR, XSAVE, XRSTOR or other stuff dealing with
     * large amounts of data, just go to ring-3 where we don't need to deal with partial
     * successes.  No chance any of these will be problematic read-modify-write stuff.
     */
    if (cbBuf > sizeof(pVCpu->iom.s.PendingMmioWrite.abValue))
        return enmAccessType == PGMACCESSTYPE_WRITE ? VINF_IOM_R3_MMIO_WRITE : VINF_IOM_R3_MMIO_READ;
#endif

    /*
     * Validate the range.
     */
    int rc = IOM_LOCK_SHARED(pVM);
#ifndef IN_RING3
    if (rc == VERR_SEM_BUSY)
    {
        if (enmAccessType == PGMACCESSTYPE_READ)
            return VINF_IOM_R3_MMIO_READ;
        Assert(enmAccessType == PGMACCESSTYPE_WRITE);
        return iomMmioRing3WritePending(pVCpu, GCPhysFault, pvBuf, cbBuf, NULL /*pRange*/);
    }
#endif
    AssertRC(rc);
    Assert(pRange == iomMmioGetRange(pVM, pVCpu, GCPhysFault));

    /*
     * Perform locking.
     */
    iomMmioRetainRange(pRange);
    PPDMDEVINS pDevIns = pRange->CTX_SUFF(pDevIns);
    IOM_UNLOCK_SHARED(pVM);
    VBOXSTRICTRC rcStrict = PDMCritSectEnter(pDevIns->CTX_SUFF(pCritSectRo), VINF_IOM_R3_MMIO_READ_WRITE);
    if (rcStrict == VINF_SUCCESS)
    {
        /*
         * Perform the access.
         */
        if (enmAccessType == PGMACCESSTYPE_READ)
            rcStrict = iomMMIODoRead(pVM, pVCpu, pRange, GCPhysFault, pvBuf, (unsigned)cbBuf);
        else
        {
            rcStrict = iomMMIODoWrite(pVM, pVCpu, pRange, GCPhysFault, pvBuf, (unsigned)cbBuf);
#ifndef IN_RING3
            if (rcStrict == VINF_IOM_R3_MMIO_WRITE)
                rcStrict = iomMmioRing3WritePending(pVCpu, GCPhysFault, pvBuf, cbBuf, pRange);
#endif
        }

        /* Check the return code. */
#ifdef IN_RING3
        AssertMsg(rcStrict == VINF_SUCCESS, ("%Rrc - %RGp - %s\n", VBOXSTRICTRC_VAL(rcStrict), GCPhysFault, pRange->pszDesc));
#else
        AssertMsg(   rcStrict == VINF_SUCCESS
                  || rcStrict == (enmAccessType == PGMACCESSTYPE_READ ? VINF_IOM_R3_MMIO_READ :  VINF_IOM_R3_MMIO_WRITE)
                  || (rcStrict == VINF_IOM_R3_MMIO_COMMIT_WRITE && enmAccessType == PGMACCESSTYPE_WRITE)
                  || rcStrict == VINF_IOM_R3_MMIO_READ_WRITE
                  || rcStrict == VINF_EM_DBG_STOP
                  || rcStrict == VINF_EM_DBG_EVENT
                  || rcStrict == VINF_EM_DBG_BREAKPOINT
                  || rcStrict == VINF_EM_OFF
                  || rcStrict == VINF_EM_SUSPEND
                  || rcStrict == VINF_EM_RESET
                  || rcStrict == VINF_EM_RAW_EMULATE_IO_BLOCK
                  //|| rcStrict == VINF_EM_HALT       /* ?? */
                  //|| rcStrict == VINF_EM_NO_MEMORY  /* ?? */
                  , ("%Rrc - %RGp - %p\n", VBOXSTRICTRC_VAL(rcStrict), GCPhysFault, pDevIns));
#endif

        iomMmioReleaseRange(pVM, pRange);
        PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
    }
#ifdef IN_RING3
    else
        iomMmioReleaseRange(pVM, pRange);
#else
    else
    {
        if (rcStrict == VINF_IOM_R3_MMIO_READ_WRITE)
        {
            if (enmAccessType == PGMACCESSTYPE_READ)
                rcStrict = VINF_IOM_R3_MMIO_READ;
            else
            {
                Assert(enmAccessType == PGMACCESSTYPE_WRITE);
                rcStrict = iomMmioRing3WritePending(pVCpu, GCPhysFault, pvBuf, cbBuf, pRange);
            }
        }
        iomMmioReleaseRange(pVM, pRange);
    }
#endif
    return rcStrict;
}


#ifdef IN_RING3 /* Only used by REM. */

/**
 * Reads a MMIO register.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   GCPhys      The physical address to read.
 * @param   pu32Value   Where to store the value read.
 * @param   cbValue     The size of the register to read in bytes. 1, 2 or 4 bytes.
 */
VMMDECL(VBOXSTRICTRC) IOMMMIORead(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, uint32_t *pu32Value, size_t cbValue)
{
    Assert(pVCpu->iom.s.PendingMmioWrite.cbValue == 0);
    /* Take the IOM lock before performing any MMIO. */
    VBOXSTRICTRC rc = IOM_LOCK_SHARED(pVM);
#ifndef IN_RING3
    if (rc == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_WRITE;
#endif
    AssertRC(VBOXSTRICTRC_VAL(rc));
#if defined(IEM_VERIFICATION_MODE) && defined(IN_RING3)
    IEMNotifyMMIORead(pVM, GCPhys, cbValue);
#endif

    /*
     * Lookup the current context range node and statistics.
     */
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhys);
    if (!pRange)
    {
        AssertMsgFailed(("Handlers and page tables are out of sync or something! GCPhys=%RGp cbValue=%d\n", GCPhys, cbValue));
        IOM_UNLOCK_SHARED(pVM);
        return VERR_IOM_MMIO_RANGE_NOT_FOUND;
    }
    iomMmioRetainRange(pRange);
#ifndef VBOX_WITH_STATISTICS
    IOM_UNLOCK_SHARED(pVM);

#else  /* VBOX_WITH_STATISTICS */
    PIOMMMIOSTATS pStats = iomMmioGetStats(pVM, pVCpu, GCPhys, pRange);
    if (!pStats)
    {
        iomMmioReleaseRange(pVM, pRange);
# ifdef IN_RING3
        return VERR_NO_MEMORY;
# else
        return VINF_IOM_R3_MMIO_READ;
# endif
    }
    STAM_COUNTER_INC(&pStats->Accesses);
#endif /* VBOX_WITH_STATISTICS */

    if (pRange->CTX_SUFF(pfnReadCallback))
    {
        /*
         * Perform locking.
         */
        PPDMDEVINS pDevIns = pRange->CTX_SUFF(pDevIns);
        rc = PDMCritSectEnter(pDevIns->CTX_SUFF(pCritSectRo), VINF_IOM_R3_MMIO_WRITE);
        if (rc != VINF_SUCCESS)
        {
            iomMmioReleaseRange(pVM, pRange);
            return rc;
        }

        /*
         * Perform the read and deal with the result.
         */
        STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfRead), a);
        if (   (cbValue == 4 && !(GCPhys & 3))
            || (pRange->fFlags & IOMMMIO_FLAGS_READ_MODE) == IOMMMIO_FLAGS_READ_PASSTHRU
            || (cbValue == 8 && !(GCPhys & 7)) )
            rc = pRange->CTX_SUFF(pfnReadCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser), GCPhys,
                                                   pu32Value, (unsigned)cbValue);
        else
            rc = iomMMIODoComplicatedRead(pVM, pRange, GCPhys, pu32Value, (unsigned)cbValue);
        STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfRead), a);
        switch (VBOXSTRICTRC_VAL(rc))
        {
            case VINF_SUCCESS:
                Log4(("IOMMMIORead: GCPhys=%RGp *pu32=%08RX32 cb=%d rc=VINF_SUCCESS\n", GCPhys, *pu32Value, cbValue));
                PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
                iomMmioReleaseRange(pVM, pRange);
                return rc;
#ifndef IN_RING3
            case VINF_IOM_R3_MMIO_READ:
            case VINF_IOM_R3_MMIO_READ_WRITE:
                STAM_COUNTER_INC(&pStats->CTX_MID_Z(Read,ToR3));
#endif
            default:
                Log4(("IOMMMIORead: GCPhys=%RGp *pu32=%08RX32 cb=%d rc=%Rrc\n", GCPhys, *pu32Value, cbValue, VBOXSTRICTRC_VAL(rc)));
                PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
                iomMmioReleaseRange(pVM, pRange);
                return rc;

            case VINF_IOM_MMIO_UNUSED_00:
                iomMMIODoRead00s(pu32Value, cbValue);
                Log4(("IOMMMIORead: GCPhys=%RGp *pu32=%08RX32 cb=%d rc=%Rrc\n", GCPhys, *pu32Value, cbValue, VBOXSTRICTRC_VAL(rc)));
                PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
                iomMmioReleaseRange(pVM, pRange);
                return VINF_SUCCESS;

            case VINF_IOM_MMIO_UNUSED_FF:
                iomMMIODoReadFFs(pu32Value, cbValue);
                Log4(("IOMMMIORead: GCPhys=%RGp *pu32=%08RX32 cb=%d rc=%Rrc\n", GCPhys, *pu32Value, cbValue, VBOXSTRICTRC_VAL(rc)));
                PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
                iomMmioReleaseRange(pVM, pRange);
                return VINF_SUCCESS;
        }
        /* not reached */
    }
#ifndef IN_RING3
    if (pRange->pfnReadCallbackR3)
    {
        STAM_COUNTER_INC(&pStats->CTX_MID_Z(Read,ToR3));
        iomMmioReleaseRange(pVM, pRange);
        return VINF_IOM_R3_MMIO_READ;
    }
#endif

    /*
     * Unassigned memory - this is actually not supposed t happen...
     */
    STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfRead), a); /** @todo STAM_PROFILE_ADD_ZERO_PERIOD */
    STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfRead), a);
    iomMMIODoReadFFs(pu32Value, cbValue);
    Log4(("IOMMMIORead: GCPhys=%RGp *pu32=%08RX32 cb=%d rc=VINF_SUCCESS\n", GCPhys, *pu32Value, cbValue));
    iomMmioReleaseRange(pVM, pRange);
    return VINF_SUCCESS;
}


/**
 * Writes to a MMIO register.
 *
 * @returns VBox status code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   GCPhys      The physical address to write to.
 * @param   u32Value    The value to write.
 * @param   cbValue     The size of the register to read in bytes. 1, 2 or 4 bytes.
 */
VMMDECL(VBOXSTRICTRC) IOMMMIOWrite(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, uint32_t u32Value, size_t cbValue)
{
    Assert(pVCpu->iom.s.PendingMmioWrite.cbValue == 0);
    /* Take the IOM lock before performing any MMIO. */
    VBOXSTRICTRC rc = IOM_LOCK_SHARED(pVM);
#ifndef IN_RING3
    if (rc == VERR_SEM_BUSY)
        return VINF_IOM_R3_MMIO_WRITE;
#endif
    AssertRC(VBOXSTRICTRC_VAL(rc));
#if defined(IEM_VERIFICATION_MODE) && defined(IN_RING3)
    IEMNotifyMMIOWrite(pVM, GCPhys, u32Value, cbValue);
#endif

    /*
     * Lookup the current context range node.
     */
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhys);
    if (!pRange)
    {
        AssertMsgFailed(("Handlers and page tables are out of sync or something! GCPhys=%RGp cbValue=%d\n", GCPhys, cbValue));
        IOM_UNLOCK_SHARED(pVM);
        return VERR_IOM_MMIO_RANGE_NOT_FOUND;
    }
    iomMmioRetainRange(pRange);
#ifndef VBOX_WITH_STATISTICS
    IOM_UNLOCK_SHARED(pVM);

#else  /* VBOX_WITH_STATISTICS */
    PIOMMMIOSTATS pStats = iomMmioGetStats(pVM, pVCpu, GCPhys, pRange);
    if (!pStats)
    {
        iomMmioReleaseRange(pVM, pRange);
# ifdef IN_RING3
        return VERR_NO_MEMORY;
# else
        return VINF_IOM_R3_MMIO_WRITE;
# endif
    }
    STAM_COUNTER_INC(&pStats->Accesses);
#endif /* VBOX_WITH_STATISTICS */

    if (pRange->CTX_SUFF(pfnWriteCallback))
    {
        /*
         * Perform locking.
         */
        PPDMDEVINS pDevIns = pRange->CTX_SUFF(pDevIns);
        rc = PDMCritSectEnter(pDevIns->CTX_SUFF(pCritSectRo), VINF_IOM_R3_MMIO_READ);
        if (rc != VINF_SUCCESS)
        {
            iomMmioReleaseRange(pVM, pRange);
            return rc;
        }

        /*
         * Perform the write.
         */
        STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfWrite), a);
        if (   (cbValue == 4 && !(GCPhys & 3))
            || (pRange->fFlags & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_PASSTHRU
            || (cbValue == 8 && !(GCPhys & 7)) )
            rc = pRange->CTX_SUFF(pfnWriteCallback)(pRange->CTX_SUFF(pDevIns), pRange->CTX_SUFF(pvUser),
                                                    GCPhys, &u32Value, (unsigned)cbValue);
        else
            rc = iomMMIODoComplicatedWrite(pVM, pVCpu, pRange, GCPhys, &u32Value, (unsigned)cbValue);
        STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfWrite), a);
#ifndef IN_RING3
        if (    rc == VINF_IOM_R3_MMIO_WRITE
            ||  rc == VINF_IOM_R3_MMIO_READ_WRITE)
            STAM_COUNTER_INC(&pStats->CTX_MID_Z(Write,ToR3));
#endif
        Log4(("IOMMMIOWrite: GCPhys=%RGp u32=%08RX32 cb=%d rc=%Rrc\n", GCPhys, u32Value, cbValue, VBOXSTRICTRC_VAL(rc)));
        iomMmioReleaseRange(pVM, pRange);
        PDMCritSectLeave(pDevIns->CTX_SUFF(pCritSectRo));
        return rc;
    }
#ifndef IN_RING3
    if (pRange->pfnWriteCallbackR3)
    {
        STAM_COUNTER_INC(&pStats->CTX_MID_Z(Write,ToR3));
        iomMmioReleaseRange(pVM, pRange);
        return VINF_IOM_R3_MMIO_WRITE;
    }
#endif

    /*
     * No write handler, nothing to do.
     */
    STAM_PROFILE_START(&pStats->CTX_SUFF_Z(ProfWrite), a);
    STAM_PROFILE_STOP(&pStats->CTX_SUFF_Z(ProfWrite), a);
    Log4(("IOMMMIOWrite: GCPhys=%RGp u32=%08RX32 cb=%d rc=%Rrc\n", GCPhys, u32Value, cbValue, VINF_SUCCESS));
    iomMmioReleaseRange(pVM, pRange);
    return VINF_SUCCESS;
}

#endif /* IN_RING3 - only used by REM. */
#ifndef IN_RC

/**
 * Mapping an MMIO2 page in place of an MMIO page for direct access.
 *
 * (This is a special optimization used by the VGA device.)
 *
 * @returns VBox status code.  This API may return VINF_SUCCESS even if no
 *          remapping is made,.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          The address of the MMIO page to be changed.
 * @param   GCPhysRemapped  The address of the MMIO2 page.
 * @param   fPageFlags      Page flags to set. Must be (X86_PTE_RW | X86_PTE_P)
 *                          for the time being.
 */
VMMDECL(int) IOMMMIOMapMMIO2Page(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysRemapped, uint64_t fPageFlags)
{
# ifndef IEM_VERIFICATION_MODE_FULL
    /* Currently only called from the VGA device during MMIO. */
    Log(("IOMMMIOMapMMIO2Page %RGp -> %RGp flags=%RX64\n", GCPhys, GCPhysRemapped, fPageFlags));
    AssertReturn(fPageFlags == (X86_PTE_RW | X86_PTE_P), VERR_INVALID_PARAMETER);
    PVMCPU pVCpu = VMMGetCpu(pVM);

    /* This currently only works in real mode, protected mode without paging or with nested paging. */
    if (    !HMIsEnabled(pVM)       /* useless without VT-x/AMD-V */
        ||  (   CPUMIsGuestInPagedProtectedMode(pVCpu)
             && !HMIsNestedPagingActive(pVM)))
        return VINF_SUCCESS;    /* ignore */

    int rc = IOM_LOCK_SHARED(pVM);
    if (RT_FAILURE(rc))
        return VINF_SUCCESS; /* better luck the next time around */

    /*
     * Lookup the context range node the page belongs to.
     */
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhys);
    AssertMsgReturn(pRange,
                    ("Handlers and page tables are out of sync or something! GCPhys=%RGp\n", GCPhys), VERR_IOM_MMIO_RANGE_NOT_FOUND);

    Assert((pRange->GCPhys       & PAGE_OFFSET_MASK) == 0);
    Assert((pRange->Core.KeyLast & PAGE_OFFSET_MASK) == PAGE_OFFSET_MASK);

    /*
     * Do the aliasing; page align the addresses since PGM is picky.
     */
    GCPhys         &= ~(RTGCPHYS)PAGE_OFFSET_MASK;
    GCPhysRemapped &= ~(RTGCPHYS)PAGE_OFFSET_MASK;

    rc = PGMHandlerPhysicalPageAlias(pVM, pRange->GCPhys, GCPhys, GCPhysRemapped);

    IOM_UNLOCK_SHARED(pVM);
    AssertRCReturn(rc, rc);

    /*
     * Modify the shadow page table. Since it's an MMIO page it won't be present and we
     * can simply prefetch it.
     *
     * Note: This is a NOP in the EPT case; we'll just let it fault again to resync the page.
     */
#  if 0 /* The assertion is wrong for the PGM_SYNC_CLEAR_PGM_POOL and VINF_PGM_HANDLER_ALREADY_ALIASED cases. */
#   ifdef VBOX_STRICT
    uint64_t fFlags;
    RTHCPHYS HCPhys;
    rc = PGMShwGetPage(pVCpu, (RTGCPTR)GCPhys, &fFlags, &HCPhys);
    Assert(rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);
#   endif
#  endif
    rc = PGMPrefetchPage(pVCpu, (RTGCPTR)GCPhys);
    Assert(rc == VINF_SUCCESS || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);
# else
    RT_NOREF_PV(pVM); RT_NOREF(GCPhys); RT_NOREF(GCPhysRemapped); RT_NOREF(fPageFlags);
# endif /* !IEM_VERIFICATION_MODE_FULL */
    return VINF_SUCCESS;
}


# ifndef IEM_VERIFICATION_MODE_FULL
/**
 * Mapping a HC page in place of an MMIO page for direct access.
 *
 * (This is a special optimization used by the APIC in the VT-x case.)
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   GCPhys          The address of the MMIO page to be changed.
 * @param   HCPhys          The address of the host physical page.
 * @param   fPageFlags      Page flags to set. Must be (X86_PTE_RW | X86_PTE_P)
 *                          for the time being.
 */
VMMDECL(int) IOMMMIOMapMMIOHCPage(PVM pVM, PVMCPU pVCpu, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint64_t fPageFlags)
{
    /* Currently only called from VT-x code during a page fault. */
    Log(("IOMMMIOMapMMIOHCPage %RGp -> %RGp flags=%RX64\n", GCPhys, HCPhys, fPageFlags));

    AssertReturn(fPageFlags == (X86_PTE_RW | X86_PTE_P), VERR_INVALID_PARAMETER);
    Assert(HMIsEnabled(pVM));

    /*
     * Lookup the context range node the page belongs to.
     */
#  ifdef VBOX_STRICT
    /* Can't lock IOM here due to potential deadlocks in the VGA device; not safe to access. */
    PIOMMMIORANGE pRange = iomMMIOGetRangeUnsafe(pVM, pVCpu, GCPhys);
    AssertMsgReturn(pRange,
            ("Handlers and page tables are out of sync or something! GCPhys=%RGp\n", GCPhys), VERR_IOM_MMIO_RANGE_NOT_FOUND);
    Assert((pRange->GCPhys       & PAGE_OFFSET_MASK) == 0);
    Assert((pRange->Core.KeyLast & PAGE_OFFSET_MASK) == PAGE_OFFSET_MASK);
#  endif

    /*
     * Do the aliasing; page align the addresses since PGM is picky.
     */
    GCPhys &= ~(RTGCPHYS)PAGE_OFFSET_MASK;
    HCPhys &= ~(RTHCPHYS)PAGE_OFFSET_MASK;

    int rc = PGMHandlerPhysicalPageAliasHC(pVM, GCPhys, GCPhys, HCPhys);
    AssertRCReturn(rc, rc);

    /*
     * Modify the shadow page table. Since it's an MMIO page it won't be present and we
     * can simply prefetch it.
     *
     * Note: This is a NOP in the EPT case; we'll just let it fault again to resync the page.
     */
    rc = PGMPrefetchPage(pVCpu, (RTGCPTR)GCPhys);
    Assert(rc == VINF_SUCCESS || rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);
    return VINF_SUCCESS;
}
# endif /* !IEM_VERIFICATION_MODE_FULL */


/**
 * Reset a previously modified MMIO region; restore the access flags.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   GCPhys          Physical address that's part of the MMIO region to be reset.
 */
VMMDECL(int) IOMMMIOResetRegion(PVM pVM, RTGCPHYS GCPhys)
{
    Log(("IOMMMIOResetRegion %RGp\n", GCPhys));

    PVMCPU pVCpu = VMMGetCpu(pVM);

    /* This currently only works in real mode, protected mode without paging or with nested paging. */
    if (    !HMIsEnabled(pVM)       /* useless without VT-x/AMD-V */
        ||  (   CPUMIsGuestInPagedProtectedMode(pVCpu)
             && !HMIsNestedPagingActive(pVM)))
        return VINF_SUCCESS;    /* ignore */

    /*
     * Lookup the context range node the page belongs to.
     */
# ifdef VBOX_STRICT
    /* Can't lock IOM here due to potential deadlocks in the VGA device; not safe to access. */
    PIOMMMIORANGE pRange = iomMMIOGetRangeUnsafe(pVM, pVCpu, GCPhys);
    AssertMsgReturn(pRange,
            ("Handlers and page tables are out of sync or something! GCPhys=%RGp\n", GCPhys), VERR_IOM_MMIO_RANGE_NOT_FOUND);
    Assert((pRange->GCPhys       & PAGE_OFFSET_MASK) == 0);
    Assert((pRange->Core.KeyLast & PAGE_OFFSET_MASK) == PAGE_OFFSET_MASK);
# endif

    /*
     * Call PGM to do the job work.
     *
     * After the call, all the pages should be non-present... unless there is
     * a page pool flush pending (unlikely).
     */
    int rc = PGMHandlerPhysicalReset(pVM, GCPhys);
    AssertRC(rc);

# ifdef VBOX_STRICT
    if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3))
    {
        uint32_t cb = pRange->cb;
        GCPhys = pRange->GCPhys;
        while (cb)
        {
            uint64_t fFlags;
            RTHCPHYS HCPhys;
            rc = PGMShwGetPage(pVCpu, (RTGCPTR)GCPhys, &fFlags, &HCPhys);
            Assert(rc == VERR_PAGE_NOT_PRESENT || rc == VERR_PAGE_TABLE_NOT_PRESENT);
            cb     -= PAGE_SIZE;
            GCPhys += PAGE_SIZE;
        }
    }
# endif
    return rc;
}

#endif /* !IN_RC */

