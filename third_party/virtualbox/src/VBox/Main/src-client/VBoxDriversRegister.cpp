/* $Id: VBoxDriversRegister.cpp $ */
/** @file
 *
 * Main driver registration.
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
#define LOG_GROUP LOG_GROUP_MAIN
#include "LoggingNew.h"

#include "MouseImpl.h"
#include "KeyboardImpl.h"
#include "DisplayImpl.h"
#include "VMMDev.h"
#ifdef VBOX_WITH_VRDE_AUDIO
# include "DrvAudioVRDE.h"
#endif
#ifdef VBOX_WITH_AUDIO_VIDEOREC
# include "DrvAudioVideoRec.h"
#endif
#include "Nvram.h"
#include "UsbWebcamInterface.h"
#ifdef VBOX_WITH_USB_CARDREADER
# include "UsbCardReader.h"
#endif
#include "ConsoleImpl.h"
#ifdef VBOX_WITH_PCI_PASSTHROUGH
# include "PCIRawDevImpl.h"
#endif

#include <VBox/vmm/pdmdrv.h>
#include <VBox/version.h>


/**
 * Register the main drivers.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
extern "C" DECLEXPORT(int) VBoxDriversRegister(PCPDMDRVREGCB pCallbacks, uint32_t u32Version)
{
    LogFlow(("VBoxDriversRegister: u32Version=%#x\n", u32Version));
    AssertReleaseMsg(u32Version == VBOX_VERSION, ("u32Version=%#x VBOX_VERSION=%#x\n", u32Version, VBOX_VERSION));

    int rc = pCallbacks->pfnRegister(pCallbacks, &Mouse::DrvReg);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCallbacks->pfnRegister(pCallbacks, &Keyboard::DrvReg);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCallbacks->pfnRegister(pCallbacks, &Display::DrvReg);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCallbacks->pfnRegister(pCallbacks, &VMMDev::DrvReg);
    if (RT_FAILURE(rc))
        return rc;
#ifdef VBOX_WITH_VRDE_AUDIO
    rc = pCallbacks->pfnRegister(pCallbacks, &AudioVRDE::DrvReg);
    if (RT_FAILURE(rc))
        return rc;
#endif
#ifdef VBOX_WITH_AUDIO_VIDEOREC
    rc = pCallbacks->pfnRegister(pCallbacks, &AudioVideoRec::DrvReg);
    if (RT_FAILURE(rc))
        return rc;
#endif
    rc = pCallbacks->pfnRegister(pCallbacks, &Nvram::DrvReg);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCallbacks->pfnRegister(pCallbacks, &EmWebcam::DrvReg);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_USB_CARDREADER
    rc = pCallbacks->pfnRegister(pCallbacks, &UsbCardReader::DrvReg);
    if (RT_FAILURE(rc))
        return rc;
#endif

    rc = pCallbacks->pfnRegister(pCallbacks, &Console::DrvStatusReg);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_PCI_PASSTHROUGH
    rc = pCallbacks->pfnRegister(pCallbacks, &PCIRawDev::DrvReg);
    if (RT_FAILURE(rc))
        return rc;
#endif

    return VINF_SUCCESS;
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
