/* $Id: VBoxDDUDeps.cpp $ */
/** @file
 * VBoxDDU - For dragging in library objects.
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/types.h>
#include <VBox/vd.h>
#ifdef VBOX_WITH_USB
# include <VBox/usblib.h>
# include <VBox/usbfilter.h>
# ifdef RT_OS_OS2
#  include <os2.h>
#  include <usbcalls.h>
# endif
#endif

/** Just a dummy global structure containing a bunch of
 * function pointers to code which is wanted in the link.
 */
PFNRT g_apfnVBoxDDUDeps[] =
{
    (PFNRT)VDInit,
    (PFNRT)VDIfCreateVfsStream,
    (PFNRT)VDIfCreateFromVfsStream,
    (PFNRT)VDCreateVfsFileFromDisk,
#ifdef VBOX_WITH_USB
    (PFNRT)USBFilterInit,
    (PFNRT)USBLibHashSerial,
# ifdef RT_OS_OS2
    (PFNRT)UsbOpen,
# endif
# if defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_WINDOWS) /* PORTME */
    (PFNRT)USBLibInit,
# endif
#endif /* VBOX_WITH_USB */
    NULL
};

