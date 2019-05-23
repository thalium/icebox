/* $Id: VBoxDrvTool.h $ */
/** @file
 * Windows Driver R0 Tooling.
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

#ifndef ___VBoxDrvTool_win_h___
#define ___VBoxDrvTool_win_h___

#include <VBox/cdefs.h>
#include <iprt/stdint.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/nt/wdm.h>


RT_C_DECLS_BEGIN

#if 0
/* enable this in case we include this in a dll*/
# ifdef IN_VBOXDRVTOOL
#  define VBOXDRVTOOL_DECL(a_Type) DECLEXPORT(a_Type)
# else
#  define VBOXDRVTOOL_DECL(a_Type) DECLIMPORT(a_Type)
# endif
#else
/*enable this in case we include this in a static lib*/
# define VBOXDRVTOOL_DECL(a_Type) a_Type VBOXCALL
#endif

VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolRegOpenKeyU(OUT PHANDLE phKey, IN PUNICODE_STRING pName, IN ACCESS_MASK fAccess);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolRegOpenKey(OUT PHANDLE phKey, IN PWCHAR pName, IN ACCESS_MASK fAccess);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolRegCloseKey(IN HANDLE hKey);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolRegQueryValueDword(IN HANDLE hKey, IN PWCHAR pName, OUT PULONG pDword);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolRegSetValueDword(IN HANDLE hKey, IN PWCHAR pName, OUT ULONG val);

VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolIoPostAsync(PDEVICE_OBJECT pDevObj, PIRP pIrp, PKEVENT pEvent);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolIoPostSync(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolIoPostSyncWithTimeout(PDEVICE_OBJECT pDevObj, PIRP pIrp, ULONG dwTimeoutMs);
DECLINLINE(NTSTATUS) VBoxDrvToolIoComplete(PIRP pIrp, NTSTATUS Status, ULONG ulInfo)
{
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = ulInfo;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

typedef struct VBOXDRVTOOL_REF
{
    volatile uint32_t cRefs;
} VBOXDRVTOOL_REF, *PVBOXDRVTOOL_REF;

DECLINLINE(void) VBoxDrvToolRefInit(PVBOXDRVTOOL_REF pRef)
{
    pRef->cRefs = 1;
}

DECLINLINE(uint32_t) VBoxDrvToolRefRetain(PVBOXDRVTOOL_REF pRef)
{
    Assert(pRef->cRefs);
    Assert(pRef->cRefs < UINT32_MAX / 2);
    return ASMAtomicIncU32(&pRef->cRefs);
}

DECLINLINE(uint32_t) VBoxDrvToolRefRelease(PVBOXDRVTOOL_REF pRef)
{
    uint32_t cRefs = ASMAtomicDecU32(&pRef->cRefs);
    Assert(cRefs < UINT32_MAX/2);
    return cRefs;
}

VBOXDRVTOOL_DECL(VOID) VBoxDrvToolRefWaitEqual(PVBOXDRVTOOL_REF pRef, uint32_t u32Val);

VBOXDRVTOOL_DECL(NTSTATUS) VBoxDrvToolStrCopy(PUNICODE_STRING pDst, CONST PUNICODE_STRING pSrc);
VBOXDRVTOOL_DECL(VOID) VBoxDrvToolStrFree(PUNICODE_STRING pStr);

RT_C_DECLS_END

#endif

