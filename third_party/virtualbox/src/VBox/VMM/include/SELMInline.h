/* $Id: SELMInline.h $ */
/** @file
 * SELM - Internal header file.
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

#ifndef ___SELMInline_h
#define ___SELMInline_h

#ifdef VBOX_WITH_RAW_MODE_NOT_R0

/**
 * Checks if a shadow descriptor table entry is good for the given segment
 * register.
 *
 * @returns @c true if good, @c false if not.
 * @param   pSReg               The segment register.
 * @param   pShwDesc            The shadow descriptor table entry.
 * @param   iSReg               The segment register index (X86_SREG_XXX).
 * @param   uCpl                The CPL.
 */
DECLINLINE(bool) selmIsShwDescGoodForSReg(PCCPUMSELREG pSReg, PCX86DESC pShwDesc, uint32_t iSReg, uint32_t uCpl)
{
    /*
     * See iemMiscValidateNewSS, iemCImpl_LoadSReg and intel+amd manuals.
     */

    if (!pShwDesc->Gen.u1Present)
    {
        Log(("selmIsShwDescGoodForSReg: Not present\n"));
        return false;
    }

    if (!pShwDesc->Gen.u1DescType)
    {
        Log(("selmIsShwDescGoodForSReg: System descriptor\n"));
        return false;
    }

    if (iSReg == X86_SREG_SS)
    {
        if ((pShwDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE)
        {
            Log(("selmIsShwDescGoodForSReg: Stack must be writable\n"));
            return false;
        }
        if (uCpl > (unsigned)pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available)
        {
            Log(("selmIsShwDescGoodForSReg: CPL(%d) > DPL(%d)\n", uCpl, pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available));
            return false;
        }
    }
    else
    {
        if (iSReg == X86_SREG_CS)
        {
            if (!(pShwDesc->Gen.u4Type & X86_SEL_TYPE_CODE))
            {
                Log(("selmIsShwDescGoodForSReg: CS needs code segment\n"));
                return false;
            }
        }
        else if ((pShwDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
        {
            Log(("selmIsShwDescGoodForSReg: iSReg=%u execute only\n", iSReg));
            return false;
        }

        if (       (pShwDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
                != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF)
            &&  (   (   (pSReg->Sel & X86_SEL_RPL) > (unsigned)pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available
                     && (pSReg->Sel & X86_SEL_RPL) != pShwDesc->Gen.u1Available )
                 || uCpl > (unsigned)pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available ) )
        {
            Log(("selmIsShwDescGoodForSReg: iSReg=%u DPL=%u CPL=%u RPL=%u\n", iSReg,
                 pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available, uCpl, pSReg->Sel & X86_SEL_RPL));
            return false;
        }
    }

    return true;
}


/**
 * Checks if a guest descriptor table entry is good for the given segment
 * register.
 *
 * @returns @c true if good, @c false if not.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   pSReg               The segment register.
 * @param   pGstDesc            The guest descriptor table entry.
 * @param   iSReg               The segment register index (X86_SREG_XXX).
 * @param   uCpl                The CPL.
 */
DECLINLINE(bool) selmIsGstDescGoodForSReg(PVMCPU pVCpu, PCCPUMSELREG pSReg, PCX86DESC pGstDesc, uint32_t iSReg, uint32_t uCpl)
{
    /*
     * See iemMiscValidateNewSS, iemCImpl_LoadSReg and intel+amd manuals.
     */

    if (!pGstDesc->Gen.u1Present)
    {
        Log(("selmIsGstDescGoodForSReg: Not present\n"));
        return false;
    }

    if (!pGstDesc->Gen.u1DescType)
    {
        Log(("selmIsGstDescGoodForSReg: System descriptor\n"));
        return false;
    }

    if (iSReg == X86_SREG_SS)
    {
        if ((pGstDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_WRITE)) != X86_SEL_TYPE_WRITE)
        {
            Log(("selmIsGstDescGoodForSReg: Stack must be writable\n"));
            return false;
        }
        if (uCpl > pGstDesc->Gen.u2Dpl)
        {
            Log(("selmIsGstDescGoodForSReg: CPL(%d) > DPL(%d)\n", uCpl, pGstDesc->Gen.u2Dpl));
            return false;
        }
    }
    else
    {
        if (iSReg == X86_SREG_CS)
        {
            if (!(pGstDesc->Gen.u4Type & X86_SEL_TYPE_CODE))
            {
                Log(("selmIsGstDescGoodForSReg: CS needs code segment\n"));
                return false;
            }
        }
        else if ((pGstDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_READ)) == X86_SEL_TYPE_CODE)
        {
            Log(("selmIsGstDescGoodForSReg: iSReg=%u execute only\n", iSReg));
            return false;
        }

        if (       (pGstDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
                != (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF)
            &&  (   (   (pSReg->Sel & X86_SEL_RPL) > pGstDesc->Gen.u2Dpl
                     && (   (pSReg->Sel & X86_SEL_RPL) != 1
                         || !CPUMIsGuestInRawMode(pVCpu) ) )
                 || uCpl > (unsigned)pGstDesc->Gen.u2Dpl
                )
           )
        {
            Log(("selmIsGstDescGoodForSReg: iSReg=%u DPL=%u CPL=%u RPL=%u InRawMode=%u\n", iSReg,
                 pGstDesc->Gen.u2Dpl, uCpl, pSReg->Sel & X86_SEL_RPL, CPUMIsGuestInRawMode(pVCpu)));
            return false;
        }
    }

    return true;
}


/**
 * Converts a guest GDT or LDT entry to a shadow table entry.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pDesc       Guest entry on input, shadow entry on return.
 */
DECL_FORCE_INLINE(void) selmGuestToShadowDesc(PVM pVM, PX86DESC pDesc)
{
    /*
     * Code and data selectors are generally 1:1, with the
     * 'little' adjustment we do for DPL 0 selectors.
     */
    if (pDesc->Gen.u1DescType)
    {
        /*
         * Hack for A-bit against Trap E on read-only GDT.
         */
        /** @todo Fix this by loading ds and cs before turning off WP. */
        pDesc->Gen.u4Type |= X86_SEL_TYPE_ACCESSED;

        /*
         * All DPL 0 code and data segments are squeezed into DPL 1.
         *
         * We're skipping conforming segments here because those
         * cannot give us any trouble.
         */
        if (    pDesc->Gen.u2Dpl == 0
            &&      (pDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
                !=  (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF) )
        {
            pDesc->Gen.u2Dpl       = 1;
            pDesc->Gen.u1Available = 1;
        }
# ifdef VBOX_WITH_RAW_RING1
        else if (    pDesc->Gen.u2Dpl == 1
                 &&  EMIsRawRing1Enabled(pVM)
                 &&      (pDesc->Gen.u4Type & (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF))
                     !=  (X86_SEL_TYPE_CODE | X86_SEL_TYPE_CONF) )
        {
            pDesc->Gen.u2Dpl       = 2;
            pDesc->Gen.u1Available = 1;
        }
# endif
        else
            pDesc->Gen.u1Available = 0;
    }
    else
    {
        /*
         * System type selectors are marked not present.
         * Recompiler or special handling is required for these.
         */
        /** @todo what about interrupt gates and rawr0? */
        pDesc->Gen.u1Present = 0;
    }
}


/**
 * Checks if a segment register is stale given the shadow descriptor table
 * entry.
 *
 * @returns @c true if stale, @c false if not.
 * @param   pSReg               The segment register.
 * @param   pShwDesc            The shadow descriptor entry.
 * @param   iSReg               The segment register number (X86_SREG_XXX).
 */
DECLINLINE(bool) selmIsSRegStale32(PCCPUMSELREG pSReg, PCX86DESC pShwDesc, uint32_t iSReg)
{
    if (   pSReg->Attr.n.u1Present     != pShwDesc->Gen.u1Present
        || pSReg->Attr.n.u4Type        != pShwDesc->Gen.u4Type
        || pSReg->Attr.n.u1DescType    != pShwDesc->Gen.u1DescType
        || pSReg->Attr.n.u1DefBig      != pShwDesc->Gen.u1DefBig
        || pSReg->Attr.n.u1Granularity != pShwDesc->Gen.u1Granularity
        || pSReg->Attr.n.u2Dpl         != pShwDesc->Gen.u2Dpl - pShwDesc->Gen.u1Available)
    {
        Log(("selmIsSRegStale32: Attributes changed (%#x -> %#x) for %u\n", pSReg->Attr.u, X86DESC_GET_HID_ATTR(pShwDesc), iSReg));
        return true;
    }

    if (pSReg->u64Base != X86DESC_BASE(pShwDesc))
    {
        Log(("selmIsSRegStale32: base changed (%#llx -> %#x) for %u\n", pSReg->u64Base, X86DESC_BASE(pShwDesc), iSReg));
        return true;
    }

    if (pSReg->u32Limit != X86DESC_LIMIT_G(pShwDesc))
    {
        Log(("selmIsSRegStale32: limit changed (%#x -> %#x) for %u\n", pSReg->u32Limit, X86DESC_LIMIT_G(pShwDesc), iSReg));
        return true;
    }

    RT_NOREF_PV(iSReg);
    return false;
}


/**
 * Loads the hidden bits of a selector register from a shadow descriptor table
 * entry.
 *
 * @param   pSReg               The segment register in question.
 * @param   pShwDesc            The shadow descriptor table entry.
 */
DECLINLINE(void) selmLoadHiddenSRegFromShadowDesc(PCPUMSELREG pSReg, PCX86DESC pShwDesc)
{
    pSReg->Attr.u         = X86DESC_GET_HID_ATTR(pShwDesc);
    pSReg->Attr.n.u2Dpl  -= pSReg->Attr.n.u1Available;
    Assert(pSReg->Attr.n.u4Type & X86_SEL_TYPE_ACCESSED);
    pSReg->u32Limit       = X86DESC_LIMIT_G(pShwDesc);
    pSReg->u64Base        = X86DESC_BASE(pShwDesc);
    pSReg->ValidSel       = pSReg->Sel;
/** @todo VBOX_WITH_RAW_RING1 */
    if (pSReg->Attr.n.u1Available)
        pSReg->ValidSel  &= ~(RTSEL)1;
    pSReg->fFlags         = CPUMSELREG_FLAGS_VALID;
}


/**
 * Loads the hidden bits of a selector register from a guest descriptor table
 * entry.
 *
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   pSReg               The segment register in question.
 * @param   pGstDesc            The guest descriptor table entry.
 */
DECLINLINE(void) selmLoadHiddenSRegFromGuestDesc(PVMCPU pVCpu, PCPUMSELREG pSReg, PCX86DESC pGstDesc)
{
    pSReg->Attr.u         = X86DESC_GET_HID_ATTR(pGstDesc);
    pSReg->Attr.n.u4Type |= X86_SEL_TYPE_ACCESSED;
    pSReg->u32Limit       = X86DESC_LIMIT_G(pGstDesc);
    pSReg->u64Base        = X86DESC_BASE(pGstDesc);
    pSReg->ValidSel       = pSReg->Sel;
/** @todo VBOX_WITH_RAW_RING1 */
    if ((pSReg->ValidSel & 1) && CPUMIsGuestInRawMode(pVCpu))
        pSReg->ValidSel  &= ~(RTSEL)1;
    pSReg->fFlags         = CPUMSELREG_FLAGS_VALID;
}

#endif /* VBOX_WITH_RAW_MODE_NOT_R0 */

/** @} */

#endif
