/* $Id: tstDeviceVMMStubs.cpp $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers, shim library exporting methods
 *             originally for VBoxVMM for intercepting (we don't want to use the PDM module
 *             to use the originals from VBoxVMM).
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
/* Methods imported from VBoxVMM by VBoxDD are extracted with:
 *     objdump -T VBoxDD.so | grep "UND" | awk -F ' ' '{print $5}' | grep -E "^TM|^PGM|^PDM|^CFGM|^IOM|^MM|^VM|^PDM|^SUP" | sort
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT /** @todo */
#include <iprt/assert.h>
#include <iprt/mem.h>

#include <VBox/err.h>
#include <VBox/cdefs.h>

#include "tstDeviceVMMInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * @copydoc CFGMR3AreValuesValid
 */
VMMR3DECL(bool) CFGMR3AreValuesValid(PCFGMNODE pNode, const char *pszzValid)
{
    return pNode->pVmmCallbacks->pfnCFGMR3AreValuesValid(pNode, pszzValid);
}


/**
 * @copydoc CFGMR3Dump
 */
VMMR3DECL(void) CFGMR3Dump(PCFGMNODE pRoot)
{
    pRoot->pVmmCallbacks->pfnCFGMR3Dump(pRoot);
}


/**
 * @copydoc CFGMR3GetChild
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetChild(PCFGMNODE pNode, const char *pszPath)
{
    return pNode->pVmmCallbacks->pfnCFGMR3GetChild(pNode, pszPath);
}


/**
 * @copydoc CFGMR3GetChildF
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetChildF(PCFGMNODE pNode, const char *pszPathFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3)
{
    va_list Args;
    va_start(Args, pszPathFormat);
    PCFGMNODE pChild = pNode->pVmmCallbacks->pfnCFGMR3GetChildFV(pNode, pszPathFormat, Args);
    va_end(Args);

    return pChild;
}


/**
 * @copydoc CFGMR3GetFirstChild
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetFirstChild(PCFGMNODE pNode)
{
    return pNode->pVmmCallbacks->pfnCFGMR3GetFirstChild(pNode);
}


/**
 * @copydoc CFGMR3GetName
 */
VMMR3DECL(int) CFGMR3GetName(PCFGMNODE pCur, char *pszName, size_t cchName)
{
    return pCur->pVmmCallbacks->pfnCFGMR3GetName(pCur, pszName, cchName);
}


/**
 * @copydoc CFGMR3GetFirstChild
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetNextChild(PCFGMNODE pNode)
{
    return pNode->pVmmCallbacks->pfnCFGMR3GetNextChild(pNode);
}


/**
 * @copydoc CFGMR3GetParent
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetParent(PCFGMNODE pNode)
{
    return pNode->pVmmCallbacks->pfnCFGMR3GetParent(pNode);
}


/**
 * @copydoc CFGMR3GetRoot
 */
VMMR3DECL(PCFGMNODE) CFGMR3GetRoot(PVM pVM)
{
    return pVM->vm.s.pVmmCallbacks->pfnCFGMR3GetRoot(pVM);
}


/**
 * @copydoc CFGMR3InsertNode
 */
VMMR3DECL(int) CFGMR3InsertNode(PCFGMNODE pNode, const char *pszName, PCFGMNODE *ppChild)
{
    return pNode->pVmmCallbacks->pfnCFGMR3InsertNode(pNode, pszName, ppChild);
}


/**
 * @copydoc CFGMR3InsertNodeF
 */
VMMR3DECL(int) CFGMR3InsertNodeF(PCFGMNODE pNode, PCFGMNODE *ppChild,
                                 const char *pszNameFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4)
{
    va_list Args;
    va_start(Args, pszNameFormat);
    int rc = pNode->pVmmCallbacks->pfnCFGMR3InsertNodeFV(pNode, ppChild, pszNameFormat, Args);
    va_end(Args);

    return rc;
}


/**
 * @copydoc CFGMR3InsertString
 */
VMMR3DECL(int) CFGMR3InsertString(PCFGMNODE pNode, const char *pszName, const char *pszString)
{
    return pNode->pVmmCallbacks->pfnCFGMR3InsertString(pNode, pszName, pszString);
}


/**
 * @copydoc CFGMR3QueryBool
 */
VMMR3DECL(int) CFGMR3QueryBool(PCFGMNODE pNode, const char *pszName, bool *pf)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
        *pf = u64 ? true : false;
    return rc;
}


/**
 * @copydoc CFGMR3QueryBoolDef
 */
VMMR3DECL(int) CFGMR3QueryBoolDef(PCFGMNODE pNode, const char *pszName, bool *pf, bool fDef)
{
    uint64_t u64;
    int rc = CFGMR3QueryIntegerDef(pNode, pszName, &u64, fDef);
    *pf = u64 ? true : false;
    return rc;
}


/**
 * @copydoc CFGMR3QueryBytes
 */
VMMR3DECL(int) CFGMR3QueryBytes(PCFGMNODE pNode, const char *pszName, void *pvData, size_t cbData)
{
    return pNode->pVmmCallbacks->pfnCFGMR3QueryBytes(pNode, pszName, pvData, cbData);
}


/**
 * @copydoc CFGMR3QueryInteger
 */
VMMR3DECL(int) CFGMR3QueryInteger(PCFGMNODE pNode, const char *pszName, uint64_t *pu64)
{
    return pNode->pVmmCallbacks->pfnCFGMR3QueryInteger(pNode, pszName, pu64);
}


/**
 * @copydoc CFGMR3QueryIntegerDef
 */
VMMR3DECL(int) CFGMR3QueryIntegerDef(PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def)
{
    int rc = CFGMR3QueryInteger(pNode, pszName, pu64);
    if (RT_FAILURE(rc))
    {
        *pu64 = u64Def;
        if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
            rc = VINF_SUCCESS;
    }

    return rc;
}


/**
 * @copydoc CFGMR3QueryPortDef
 */
VMMR3DECL(int) CFGMR3QueryPortDef(PCFGMNODE pNode, const char *pszName, PRTIOPORT pPort, RTIOPORT PortDef)
{
    AssertCompileSize(RTIOPORT, 2);
    return CFGMR3QueryU16Def(pNode, pszName, pPort, PortDef);
}


/**
 * @copydoc CFGMR3QueryPtr
 */
VMMR3DECL(int) CFGMR3QueryPtr(PCFGMNODE pNode, const char *pszName, void **ppv)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
    {
        uintptr_t u = (uintptr_t)u64;
        if (u64 == u)
            *ppv = (void *)u;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}


/**
 * @copydoc CFGMR3QueryS32
 */
VMMR3DECL(int) CFGMR3QueryS32(PCFGMNODE pNode, const char *pszName, int32_t *pi32)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
    {
        if (   !(u64 & UINT64_C(0xffffffff80000000))
            ||  (u64 & UINT64_C(0xffffffff80000000)) == UINT64_C(0xffffffff80000000))
            *pi32 = (int32_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}


/**
 * @copydoc CFGMR3QueryS32Def
 */
VMMR3DECL(int) CFGMR3QueryS32Def(PCFGMNODE pNode, const char *pszName, int32_t *pi32, int32_t i32Def)
{
    uint64_t u64;
    int rc = CFGMR3QueryIntegerDef(pNode, pszName, &u64, i32Def);
    if (RT_SUCCESS(rc))
    {
        if (   !(u64 & UINT64_C(0xffffffff80000000))
            ||  (u64 & UINT64_C(0xffffffff80000000)) == UINT64_C(0xffffffff80000000))
            *pi32 = (int32_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    if (RT_FAILURE(rc))
        *pi32 = i32Def;
    return rc;
}


/**
 * @copydoc CFGMR3QuerySIntDef
 */
VMMR3DECL(int) CFGMR3QuerySIntDef(PCFGMNODE pNode, const char *pszName, signed int *pi, signed int iDef)
{
    AssertCompileSize(signed int, 4);
    return CFGMR3QueryS32Def(pNode, pszName, (int32_t *)pi, iDef);
}


/**
 * @copydoc CFGMR3QuerySize
 */
VMMDECL(int) CFGMR3QuerySize(PCFGMNODE pNode, const char *pszName, size_t *pcb)
{
    return pNode->pVmmCallbacks->pfnCFGMR3QuerySize(pNode, pszName, pcb);
}


/**
 * @copydoc CFGMR3QueryString
 */
VMMR3DECL(int) CFGMR3QueryString(PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString)
{
    return pNode->pVmmCallbacks->pfnCFGMR3QueryString(pNode, pszName, pszString, cchString);
}


/**
 * @copydoc CFGMR3QueryStringDef
 */
VMMR3DECL(int) CFGMR3QueryStringDef(PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString, const char *pszDef)
{
    int rc = CFGMR3QueryString(pNode, pszName, pszString, cchString);
    if (RT_FAILURE(rc) && rc != VERR_CFGM_NOT_ENOUGH_SPACE)
    {
        size_t cchDef = strlen(pszDef);
        if (cchString > cchDef)
        {
            memcpy(pszString, pszDef, cchDef);
            memset(pszString + cchDef, 0, cchString - cchDef);
            if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
                rc = VINF_SUCCESS;
        }
        else if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
            rc = VERR_CFGM_NOT_ENOUGH_SPACE;
    }

    return rc;
}


/**
 * @copydoc CFGMR3QueryStringAlloc
 */
VMMR3DECL(int) CFGMR3QueryStringAlloc(PCFGMNODE pNode, const char *pszName, char **ppszString)
{
#if 0
    size_t cbString;
    int rc = CFGMR3QuerySize(pNode, pszName, &cbString);
    if (RT_SUCCESS(rc))
    {
        char *pszString = cfgmR3StrAlloc(pNode->pVM, MM_TAG_CFGM_USER, cbString);
        if (pszString)
        {
            rc = CFGMR3QueryString(pNode, pszName, pszString, cbString);
            if (RT_SUCCESS(rc))
                *ppszString = pszString;
            else
                cfgmR3StrFree(pNode->pVM, pszString);
        }
        else
            rc = VERR_NO_MEMORY;
    }
    return rc;
#endif
    RT_NOREF(pNode, pszName, ppszString);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @copydoc CFGMR3QueryStringAllocDef
 */
VMMR3DECL(int) CFGMR3QueryStringAllocDef(PCFGMNODE pNode, const char *pszName, char **ppszString, const char *pszDef)
{
    RT_NOREF(pNode, pszName, ppszString, pszDef);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @copydoc CFGMR3QueryU16
 */
VMMR3DECL(int) CFGMR3QueryU16(PCFGMNODE pNode, const char *pszName, uint16_t *pu16)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffffffff0000)))
            *pu16 = (int16_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}


/**
 * @copydoc CFGMR3QueryU16Def
 */
VMMR3DECL(int) CFGMR3QueryU16Def(PCFGMNODE pNode, const char *pszName, uint16_t *pu16, uint16_t u16Def)
{
    uint64_t u64;
    int rc = CFGMR3QueryIntegerDef(pNode, pszName, &u64, u16Def);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffffffff0000)))
            *pu16 = (int16_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    if (RT_FAILURE(rc))
        *pu16 = u16Def;
    return rc;
}


/**
 * @copydoc CFGMR3QueryU32
 */
VMMR3DECL(int) CFGMR3QueryU32(PCFGMNODE pNode, const char *pszName, uint32_t *pu32)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffff00000000)))
            *pu32 = (uint32_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}


/**
 * @copydoc CFGMR3QueryU32Def
 */
VMMR3DECL(int) CFGMR3QueryU32Def(PCFGMNODE pNode, const char *pszName, uint32_t *pu32, uint32_t u32Def)
{
    uint64_t u64;
    int rc = CFGMR3QueryIntegerDef(pNode, pszName, &u64, u32Def);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffff00000000)))
            *pu32 = (uint32_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    if (RT_FAILURE(rc))
        *pu32 = u32Def;
    return rc;
}


/**
 * @copydoc CFGMR3QueryU64
 */
VMMR3DECL(int) CFGMR3QueryU64(PCFGMNODE pNode, const char *pszName, uint64_t *pu64)
{
    return CFGMR3QueryInteger(pNode, pszName, pu64);
}


/**
 * @copydoc CFGMR3QueryU64Def
 */
VMMR3DECL(int) CFGMR3QueryU64Def(PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def)
{
    return CFGMR3QueryIntegerDef(pNode, pszName, pu64, u64Def);
}


/**
 * @copydoc CFGMR3QueryU8
 */
VMMR3DECL(int) CFGMR3QueryU8(PCFGMNODE pNode, const char *pszName, uint8_t *pu8)
{
    uint64_t u64;
    int rc = CFGMR3QueryInteger(pNode, pszName, &u64);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffffffffff00)))
            *pu8 = (uint8_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    return rc;
}


/**
 * @copydoc CFGMR3QueryU8Def
 */
VMMR3DECL(int) CFGMR3QueryU8Def(PCFGMNODE pNode, const char *pszName, uint8_t *pu8, uint8_t u8Def)
{
    uint64_t u64;
    int rc = CFGMR3QueryIntegerDef(pNode, pszName, &u64, u8Def);
    if (RT_SUCCESS(rc))
    {
        if (!(u64 & UINT64_C(0xffffffffffffff00)))
            *pu8 = (uint8_t)u64;
        else
            rc = VERR_CFGM_INTEGER_TOO_BIG;
    }
    if (RT_FAILURE(rc))
        *pu8 = u8Def;
    return rc;
}


/**
 * @copydoc CFGMR3RemoveNode
 */
VMMR3DECL(void) CFGMR3RemoveNode(PCFGMNODE pNode)
{
    pNode->pVmmCallbacks->pfnCFGMR3RemoveNode(pNode);
}


/**
 * @copydoc CFGMR3ValidateConfig
 */
VMMR3DECL(int) CFGMR3ValidateConfig(PCFGMNODE pNode, const char *pszNode,
                                    const char *pszValidValues, const char *pszValidNodes,
                                    const char *pszWho, uint32_t uInstance)
{
    return pNode->pVmmCallbacks->pfnCFGMR3ValidateConfig(pNode, pszNode, pszValidValues, pszValidNodes, pszWho, uInstance);
}


/**
 * @copydoc IOMIOPortWrite
 */
VMMDECL(VBOXSTRICTRC) IOMIOPortWrite(PVM pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue)
{
    return pVM->vm.s.pVmmCallbacks->pfnIOMIOPortWrite(pVM, pVCpu, Port, u32Value, cbValue);
}


/**
 * @copydoc IOMMMIOMapMMIO2Page
 */
VMMDECL(int) IOMMMIOMapMMIO2Page(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysRemapped, uint64_t fPageFlags)
{
    return pVM->vm.s.pVmmCallbacks->pfnIOMMMIOMapMMIO2Page(pVM, GCPhys, GCPhysRemapped, fPageFlags);
}


/**
 * @copydoc IOMMMIOResetRegion
 */
VMMDECL(int) IOMMMIOResetRegion(PVM pVM, RTGCPHYS GCPhys)
{
    return pVM->vm.s.pVmmCallbacks->pfnIOMMMIOResetRegion(pVM, GCPhys);
}


/**
 * @copydoc MMHyperAlloc
 */
VMMDECL(int) MMHyperAlloc(PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMHyperAlloc(pVM, cb, uAlignment, enmTag, ppv);
}


/**
 * @copydoc MMHyperFree
 */
VMMDECL(int) MMHyperFree(PVM pVM, void *pv)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMHyperFree(pVM, pv);
}


/**
 * @copydoc MMHyperR3ToR0
 */
VMMDECL(RTR0PTR) MMHyperR3ToR0(PVM pVM, RTR3PTR R3Ptr)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMHyperR3ToR0(pVM, R3Ptr);
}


/**
 * @copydoc MMHyperR3ToRC
 */
VMMDECL(RTRCPTR) MMHyperR3ToRC(PVM pVM, RTR3PTR R3Ptr)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMHyperR3ToRC(pVM, R3Ptr);
}


/**
 * @copydoc MMR3HeapFree
 */
VMMR3DECL(void) MMR3HeapFree(void *pv)
{
    PTSTDEVMMHEAPALLOC pHeapAlloc = (PTSTDEVMMHEAPALLOC)((uint8_t *)pv - RT_OFFSETOF(TSTDEVMMHEAPALLOC, abAlloc[0]));
    pHeapAlloc->pVmmCallbacks->pfnMMR3HeapFree(pv);
}


/**
 * @copydoc MMR3HyperAllocOnceNoRel
 */
VMMR3DECL(int) MMR3HyperAllocOnceNoRel(PVM pVM, size_t cb, uint32_t uAlignment, MMTAG enmTag, void **ppv)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMR3HyperAllocOnceNoRel(pVM, cb, uAlignment, enmTag, ppv);
}


/**
 * @copydoc MMR3PhysGetRamSize
 */
VMMR3DECL(uint64_t) MMR3PhysGetRamSize(PVM pVM)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMR3PhysGetRamSize(pVM);
}


/**
 * @copydoc MMR3PhysGetRamSizeBelow4GB
 */
VMMR3DECL(uint32_t) MMR3PhysGetRamSizeBelow4GB(PVM pVM)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMR3PhysGetRamSizeBelow4GB(pVM);
}


/**
 * @copydoc MMR3PhysGetRamSizeAbove4GB
 */
VMMR3DECL(uint64_t) MMR3PhysGetRamSizeAbove4GB(PVM pVM)
{
    return pVM->vm.s.pVmmCallbacks->pfnMMR3PhysGetRamSizeAbove4GB(pVM);
}


/**
 * @copydoc PDMCritSectEnterDebug
 */
VMMDECL(int) PDMCritSectEnterDebug(PPDMCRITSECT pCritSect, int rcBusy, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMCritSectEnterDebug(pCritSect, rcBusy, uId, RT_SRC_POS_ARGS);
}


/**
 * @copydoc PDMCritSectIsInitialized
 */
VMMDECL(bool) PDMCritSectIsInitialized(PCPDMCRITSECT pCritSect)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMCritSectIsInitialized(pCritSect);
}


/**
 * @copydoc PDMCritSectIsOwner
 */
VMMDECL(bool) PDMCritSectIsOwner(PCPDMCRITSECT pCritSect)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMCritSectIsOwner(pCritSect);
}


/**
 * @copydoc PDMCritSectLeave
 */
VMMDECL(int) PDMCritSectLeave(PPDMCRITSECT pCritSect)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMCritSectLeave(pCritSect);
}


/**
 * @copydoc PDMCritSectTryEnterDebug
 */
VMMDECL(int) PDMCritSectTryEnterDebug(PPDMCRITSECT pCritSect, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMCritSectTryEnterDebug(pCritSect, uId, RT_SRC_POS_ARGS);
}


/**
 * @copydoc PDMHCCritSectScheduleExitEvent
 */
VMMDECL(int) PDMHCCritSectScheduleExitEvent(PPDMCRITSECT pCritSect, SUPSEMEVENT hEventToSignal)
{
    return pCritSect->s.pVmmCallbacks->pfnPDMHCCritSectScheduleExitEvent(pCritSect, hEventToSignal);
}


/**
 * @copydoc PDMNsAllocateBandwidth
 */
VMMDECL(bool) PDMNsAllocateBandwidth(PPDMNSFILTER pFilter, size_t cbTransfer)
{
#if 0
    return pFilter->pVmmCallbacks->pfnPDMNsAllocateBandwidth(pFilter, cbTransfer);
#endif
    RT_NOREF(pFilter, cbTransfer);
    AssertFailed();
    return false;
}


/**
 * @copydoc PDMQueueAlloc
 */
VMMDECL(PPDMQUEUEITEMCORE) PDMQueueAlloc(PPDMQUEUE pQueue)
{
    return pQueue->pVmmCallbacks->pfnPDMQueueAlloc(pQueue);
}


/**
 * @copydoc PDMQueueFlushIfNecessary
 */
VMMDECL(bool) PDMQueueFlushIfNecessary(PPDMQUEUE pQueue)
{
    return pQueue->pVmmCallbacks->pfnPDMQueueFlushIfNecessary(pQueue);
}


/**
 * @copydoc PDMQueueInsert
 */
VMMDECL(void) PDMQueueInsert(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem)
{
    pQueue->pVmmCallbacks->pfnPDMQueueInsert(pQueue, pItem);
}


/**
 * @copydoc PDMQueueR0Ptr
 */
VMMDECL(R0PTRTYPE(PPDMQUEUE)) PDMQueueR0Ptr(PPDMQUEUE pQueue)
{
    return pQueue->pVmmCallbacks->pfnPDMQueueR0Ptr(pQueue);
}


/**
 * @copydoc PDMQueueRCPtr
 */
VMMDECL(RCPTRTYPE(PPDMQUEUE)) PDMQueueRCPtr(PPDMQUEUE pQueue)
{
    return pQueue->pVmmCallbacks->pfnPDMQueueRCPtr(pQueue);
}


/**
 * @copydoc PGMHandlerPhysicalPageTempOff
 */
VMMDECL(int) PGMHandlerPhysicalPageTempOff(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS GCPhysPage)
{
    RT_NOREF(pVM, GCPhys, GCPhysPage);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @copydoc PGMShwMakePageWritable
 */
VMMDECL(int) PGMShwMakePageWritable(PVMCPU pVCpu, RTGCPTR GCPtr, uint32_t fFlags)
{
    RT_NOREF(pVCpu, GCPtr, fFlags);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

/** @todo PDMR3AsyncCompletion + BlkCache + CritSect + QueryLun + Thread. */

/**
 * @copydoc TMCpuTicksPerSecond
 */
VMMDECL(uint64_t) TMCpuTicksPerSecond(PVM pVM)
{
    return pVM->vm.s.pVmmCallbacks->pfnTMCpuTicksPerSecond(pVM);
}


/**
 * @copydoc TMR3TimerDestroy
 */
VMMR3DECL(int) TMR3TimerDestroy(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMR3TimerDestroy(pTimer);
}


/**
 * @copydoc TMR3TimerSave
 */
VMMR3DECL(int) TMR3TimerSave(PTMTIMERR3 pTimer, PSSMHANDLE pSSM)
{
    return pTimer->pVmmCallbacks->pfnTMR3TimerSave(pTimer, pSSM);
}


/**
 * @copydoc TMR3TimerLoad
 */
VMMR3DECL(int) TMR3TimerLoad(PTMTIMERR3 pTimer, PSSMHANDLE pSSM)
{
    return pTimer->pVmmCallbacks->pfnTMR3TimerLoad(pTimer, pSSM);
}


/**
 * @copydoc TMR3TimerSetCritSect
 */
VMMR3DECL(int) TMR3TimerSetCritSect(PTMTIMERR3 pTimer, PPDMCRITSECT pCritSect)
{
    return pTimer->pVmmCallbacks->pfnTMR3TimerSetCritSect(pTimer, pCritSect);
}


/**
 * @copydoc TMTimerFromMilli
 */
VMMDECL(uint64_t) TMTimerFromMilli(PTMTIMER pTimer, uint64_t cMilliSecs)
{
    return pTimer->pVmmCallbacks->pfnTMTimerFromMilli(pTimer, cMilliSecs);
}


/**
 * @copydoc TMTimerFromNano
 */
VMMDECL(uint64_t) TMTimerFromNano(PTMTIMER pTimer, uint64_t cNanoSecs)
{
    return pTimer->pVmmCallbacks->pfnTMTimerFromNano(pTimer, cNanoSecs);
}


/**
 * @copydoc TMTimerGet
 */
VMMDECL(uint64_t) TMTimerGet(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerGet(pTimer);
}


/**
 * @copydoc TMTimerGetFreq
 */
VMMDECL(uint64_t) TMTimerGetFreq(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerGetFreq(pTimer);
}


/**
 * @copydoc TMTimerGetNano
 */
VMMDECL(uint64_t) TMTimerGetNano(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerGetNano(pTimer);
}


/**
 * @copydoc TMTimerIsActive
 */
VMMDECL(bool) TMTimerIsActive(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerIsActive(pTimer);
}


/**
 * @copydoc TMTimerIsLockOwner
 */
VMMDECL(bool) TMTimerIsLockOwner(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerIsLockOwner(pTimer);
}


/**
 * @copydoc TMTimerLock
 */
VMMDECL(int) TMTimerLock(PTMTIMER pTimer, int rcBusy)
{
    return pTimer->pVmmCallbacks->pfnTMTimerLock(pTimer, rcBusy);
}


/**
 * @copydoc TMTimerR0Ptr
 */
VMMDECL(PTMTIMERR0) TMTimerR0Ptr(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerR0Ptr(pTimer);
}


/**
 * @copydoc TMTimerRCPtr
 */
VMMDECL(PTMTIMERRC) TMTimerRCPtr(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerRCPtr(pTimer);
}


/**
 * @copydoc TMTimerSet
 */
VMMDECL(int) TMTimerSet(PTMTIMER pTimer, uint64_t u64Expire)
{
    return pTimer->pVmmCallbacks->pfnTMTimerSet(pTimer, u64Expire);
}


/**
 * @copydoc TMTimerSetFrequencyHint
 */
VMMDECL(int) TMTimerSetFrequencyHint(PTMTIMER pTimer, uint32_t uHz)
{
    return pTimer->pVmmCallbacks->pfnTMTimerSetFrequencyHint(pTimer, uHz);
}


/**
 * @copydoc TMTimerSetMicro
 */
VMMDECL(int) TMTimerSetMicro(PTMTIMER pTimer, uint64_t cMicrosToNext)
{
    return pTimer->pVmmCallbacks->pfnTMTimerSetMicro(pTimer, cMicrosToNext);
}


/**
 * @copydoc TMTimerSetMillies
 */
VMMDECL(int) TMTimerSetMillies(PTMTIMER pTimer, uint32_t cMilliesToNext)
{
    return pTimer->pVmmCallbacks->pfnTMTimerSetMillies(pTimer, cMilliesToNext);
}


/**
 * @copydoc TMTimerSetNano
 */
VMMDECL(int) TMTimerSetNano(PTMTIMER pTimer, uint64_t cNanosToNext)
{
    return pTimer->pVmmCallbacks->pfnTMTimerSetNano(pTimer, cNanosToNext);
}


/**
 * @copydoc TMTimerStop
 */
VMMDECL(int) TMTimerStop(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerStop(pTimer);
}


/**
 * @copydoc TMTimerUnlock
 */
VMMDECL(void) TMTimerUnlock(PTMTIMER pTimer)
{
    return pTimer->pVmmCallbacks->pfnTMTimerUnlock(pTimer);
}

