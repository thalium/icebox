/* $Id: VBoxDD2.cpp $ */
/** @file
 * VBoxDD2 - Built-in drivers & devices part 2.
 *
 * These drivers and devices are in separate modules because of LGPL.
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
#define LOG_GROUP LOG_GROUP_DEV
#include <VBox/vmm/pdm.h>
#include <VBox/version.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/assert.h>

#include "VBoxDD2.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
const void *g_apvVBoxDDDependencies2[] =
{
    (void *)&g_abPcBiosBinary386,
    (void *)&g_abPcBiosBinary286,
    (void *)&g_abPcBiosBinary8086,
    (void *)&g_abVgaBiosBinary386,
    (void *)&g_abVgaBiosBinary286,
    (void *)&g_abVgaBiosBinary8086,
#ifdef VBOX_WITH_PXE_ROM
    (void *)&g_abNetBiosBinary,
#endif
};


/**
 * Register builtin devices.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, uint32_t u32Version)
{
    LogFlow(("VBoxDevicesRegister: u32Version=%#x\n", u32Version));
    AssertReleaseMsg(u32Version == VBOX_VERSION, ("u32Version=%#x VBOX_VERSION=%#x\n", u32Version, VBOX_VERSION));

    int rc = pCallbacks->pfnRegister(pCallbacks, &g_DeviceLPC);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}

