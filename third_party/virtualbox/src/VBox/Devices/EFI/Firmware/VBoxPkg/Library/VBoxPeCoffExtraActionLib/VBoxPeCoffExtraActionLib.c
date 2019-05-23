/* $Id: VBoxPeCoffExtraActionLib.c $ */
/** @file
 * VBox implementation of DebugAgentLib that reports EFI state transitions
 * to DevEFI for debugging purposes.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <Base.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/DebugLib.h>

#include "VBoxPkg.h"
#include "../../../../DevEFI.h"



static void vboxImageEvtU64(uint32_t uCmd, uint64_t uValue)
{
    RTUINT64U u; /* 64-bit shift builtins. */
    u.u = uValue;
    ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_MAKE(uCmd, u.au16[3]));
    ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_MAKE(uCmd, u.au16[2]));
    ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_MAKE(uCmd, u.au16[1]));
    ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_MAKE(uCmd, u.au16[0]));
}

static void vboxImageEvtString(uint32_t uCmd, const char *pszName)
{
    unsigned char uch;
    while ((uch = *pszName++) != '\0')
        ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_MAKE(uCmd, uch));
}

static void vboxImageEvtEmitOne(PE_COFF_LOADER_IMAGE_CONTEXT const *pImageCtx, uint32_t uEvt)
{
    ASMOutU32(EFI_PORT_IMAGE_EVENT, uEvt);
    if (pImageCtx->DestinationAddress)
        vboxImageEvtU64(EFI_IMAGE_EVT_CMD_ADDR0, pImageCtx->DestinationAddress);
    else
        vboxImageEvtU64(EFI_IMAGE_EVT_CMD_ADDR0, pImageCtx->ImageAddress);
    vboxImageEvtU64(EFI_IMAGE_EVT_CMD_SIZE0, pImageCtx->ImageSize);
    if (pImageCtx->PdbPointer)
        vboxImageEvtString(EFI_IMAGE_EVT_CMD_NAME, pImageCtx->PdbPointer);
    ASMOutU32(EFI_PORT_IMAGE_EVENT, EFI_IMAGE_EVT_CMD_COMPLETE);
}


/* Note! Despite this saying relocate, it's always called immediately after
         loading an image.  So, treating it as a pure load event until we find
         evidence of other usage.  */
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction(
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  )
{
    ASSERT(ImageContext != NULL);
#if ARCH_BITS == 32
    vboxImageEvtEmitOne(ImageContext, EFI_IMAGE_EVT_CMD_START_LOAD32);
#else
    vboxImageEvtEmitOne(ImageContext, EFI_IMAGE_EVT_CMD_START_LOAD64);
#endif
}

VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction(
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT *ImageContext
  )
{
    ASSERT(ImageContext != NULL);
#if ARCH_BITS == 32
    vboxImageEvtEmitOne(ImageContext, EFI_IMAGE_EVT_CMD_START_UNLOAD32);
#else
    vboxImageEvtEmitOne(ImageContext, EFI_IMAGE_EVT_CMD_START_UNLOAD64);
#endif
}

