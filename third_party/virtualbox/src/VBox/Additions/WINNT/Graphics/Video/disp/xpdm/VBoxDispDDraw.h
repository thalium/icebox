/* $Id: VBoxDispDDraw.h $ */
/** @file
 * VBox XPDM Display driver, direct draw callbacks
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
 */

#ifndef VBOXDISPDDRAW_H
#define VBOXDISPDDRAW_H

#include <winddi.h>

DWORD APIENTRY VBoxDispDDCanCreateSurface(PDD_CANCREATESURFACEDATA lpCanCreateSurface);
DWORD APIENTRY VBoxDispDDCreateSurface(PDD_CREATESURFACEDATA lpCreateSurface);
DWORD APIENTRY VBoxDispDDDestroySurface(PDD_DESTROYSURFACEDATA lpDestroySurface);
DWORD APIENTRY VBoxDispDDLock(PDD_LOCKDATA lpLock);
DWORD APIENTRY VBoxDispDDUnlock(PDD_UNLOCKDATA lpUnlock);
DWORD APIENTRY VBoxDispDDMapMemory(PDD_MAPMEMORYDATA lpMapMemory);

#ifdef VBOX_WITH_VIDEOHWACCEL
int VBoxDispVHWAUpdateDDHalInfo(PVBOXDISPDEV pDev, DD_HALINFO *pHalInfo);

DWORD APIENTRY VBoxDispDDGetDriverInfo(DD_GETDRIVERINFODATA *lpData);
DWORD APIENTRY VBoxDispDDSetColorKey(PDD_SETCOLORKEYDATA lpSetColorKey);
DWORD APIENTRY VBoxDispDDAddAttachedSurface(PDD_ADDATTACHEDSURFACEDATA lpAddAttachedSurface);
DWORD APIENTRY VBoxDispDDBlt(PDD_BLTDATA lpBlt);
DWORD APIENTRY VBoxDispDDFlip(PDD_FLIPDATA lpFlip);
DWORD APIENTRY VBoxDispDDGetBltStatus(PDD_GETBLTSTATUSDATA lpGetBltStatus);
DWORD APIENTRY VBoxDispDDGetFlipStatus(PDD_GETFLIPSTATUSDATA lpGetFlipStatus);
DWORD APIENTRY VBoxDispDDSetOverlayPosition(PDD_SETOVERLAYPOSITIONDATA lpSetOverlayPosition);
DWORD APIENTRY VBoxDispDDUpdateOverlay(PDD_UPDATEOVERLAYDATA lpUpdateOverlay);
#endif

#endif /*!VBOXDISPDDRAW_H*/
