/* $Id: VBoxUtils-win.h $ */
/** @file
 * VBox Qt GUI - Declarations of utility classes and functions for handling Win specific tasks.
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

#ifndef ___VBoxUtils_WIN_h___
#define ___VBoxUtils_WIN_h___

/* Qt includes: */
#include <QRegion>

/* Platform includes: */
#include <iprt/win/windows.h>

/* Namespace for native window sub-system functions: */
namespace NativeWindowSubsystem
{
    /* Returns area covered by visible always-on-top (top-most) windows: */
    const QRegion areaCoveredByTopMostWindows();
}

#endif

