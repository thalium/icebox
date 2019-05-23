/** $Id: VBoxUSBInterface.h $ */
/** @file
 * VirtualBox USB Driver User<->Kernel Interface.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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

#ifndef ___VBoxUSBInterface_h
#define ___VBoxUSBInterface_h

#include <VBox/usbfilter.h>

/**
 * org_virtualbox_VBoxUSBClient method indexes.
 */
typedef enum VBOXUSBMETHOD
{
    /** org_virtualbox_VBoxUSBClient::addFilter */
    VBOXUSBMETHOD_ADD_FILTER = 0,
    /** org_virtualbox_VBoxUSBClient::removeFilter */
    VBOXUSBMETHOD_REMOVE_FILTER,
    /** End/max. */
    VBOXUSBMETHOD_END
} VBOXUSBMETHOD;

/**
 * Output from a VBOXUSBMETHOD_ADD_FILTER call.
 */
typedef struct VBOXUSBADDFILTEROUT
{
    /** The ID. */
    uintptr_t       uId;
    /** The return code. */
    int             rc;
} VBOXUSBADDFILTEROUT;
/** Pointer to a VBOXUSBADDFILTEROUT. */
typedef VBOXUSBADDFILTEROUT *PVBOXUSBADDFILTEROUT;

/** Cookie used to fend off some unwanted clients to the IOService.  */
#define VBOXUSB_DARWIN_IOSERVICE_COOKIE     UINT32_C(0x62735556) /* 'VUsb' */

#endif

