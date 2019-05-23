/* $Id: VBoxUtils-win.cpp $ */
/** @file
 * VBox Qt GUI - Utility classes and functions for handling Win specific tasks.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Includes: */
#include "VBoxUtils-win.h"

/* Namespace for native window sub-system functions: */
namespace NativeWindowSubsystem
{
    /* Enumerates visible always-on-top (top-most) windows: */
    BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
    /* Contain visible top-most-window rectangles: */
    QList<QRect> topMostRects;
}

/* Enumerates visible always-on-top (top-most) windows: */
BOOL CALLBACK NativeWindowSubsystem::EnumWindowsProc(HWND hWnd, LPARAM /* lParam */)
{
    /* Ignore NULL HWNDs: */
    if (!hWnd)
        return TRUE;

    /* Ignore hidden windows: */
    if (!IsWindowVisible(hWnd))
        return TRUE;

    /* Get window style: */
    LONG uStyle = GetWindowLong(hWnd, GWL_STYLE);
    /* Ignore minimized windows: */
    if (uStyle & WS_MINIMIZE)
        return TRUE;

    /* Get extended window style: */
    LONG uExtendedStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    /* Ignore non-top-most windows: */
    if (!(uExtendedStyle & WS_EX_TOPMOST))
        return TRUE;

    /* Get that window rectangle: */
    RECT rect;
    GetWindowRect(hWnd, &rect);
    topMostRects << QRect(QPoint(rect.left, rect.top), QPoint(rect.right - 1, rect.bottom - 1));

    /* Proceed to the next window: */
    return TRUE;
}

/* Returns area covered by visible always-on-top (top-most) windows: */
const QRegion NativeWindowSubsystem::areaCoveredByTopMostWindows()
{
    /* Prepare the top-most region: */
    QRegion topMostRegion;
    /* Initialize the list of the top-most rectangles: */
    topMostRects.clear();
    /* Populate the list of top-most rectangles: */
    EnumWindows((WNDENUMPROC)EnumWindowsProc, 0);
    /* Update the top-most region with top-most rectangles: */
    for (int iRectIndex = 0; iRectIndex < topMostRects.size(); ++iRectIndex)
        topMostRegion += topMostRects[iRectIndex];
    /* Return top-most region: */
    return topMostRegion;
}

