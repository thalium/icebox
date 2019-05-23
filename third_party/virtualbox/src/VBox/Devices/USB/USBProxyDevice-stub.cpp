/* $Id: USBProxyDevice-stub.cpp $ */
/** @file
 * USB device proxy - Stub.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <VBox/vmm/pdm.h>

#include "USBProxyDevice.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Stub USB Proxy Backend.
 */
extern const USBPROXYBACK g_USBProxyDeviceHost =
{
    "host",
    0,          /* cbBackend */
    NULL,       /* Open */
    NULL,       /* Init */
    NULL,       /* Close */
    NULL,       /* Reset */
    NULL,       /* SetConfig */
    NULL,       /* ClaimInterface */
    NULL,       /* ReleaseInterface */
    NULL,       /* SetInterface */
    NULL,       /* ClearHaltedEp */
    NULL,       /* UrbQueue */
    NULL,       /* UrbCancel */
    NULL,       /* UrbReap */
    0
};

