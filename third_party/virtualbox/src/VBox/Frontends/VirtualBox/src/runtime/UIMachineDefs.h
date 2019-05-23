/* $Id: UIMachineDefs.h $ */
/** @file
 * VBox Qt GUI - Defines for Virtual Machine classes.
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

#ifndef __UIMachineDefs_h__
#define __UIMachineDefs_h__

/* Global includes */
#include <iprt/cdefs.h>

/* Machine elements enum: */
enum UIVisualElement
{
    UIVisualElement_WindowTitle           = RT_BIT(0),
    UIVisualElement_MouseIntegrationStuff = RT_BIT(1),
    UIVisualElement_IndicatorPoolStuff    = RT_BIT(2),
    UIVisualElement_HDStuff               = RT_BIT(3),
    UIVisualElement_CDStuff               = RT_BIT(4),
    UIVisualElement_FDStuff               = RT_BIT(5),
    UIVisualElement_AudioStuff            = RT_BIT(6),
    UIVisualElement_NetworkStuff          = RT_BIT(7),
    UIVisualElement_USBStuff              = RT_BIT(8),
    UIVisualElement_SharedFolderStuff     = RT_BIT(9),
    UIVisualElement_Display               = RT_BIT(10),
    UIVisualElement_VideoCapture          = RT_BIT(11),
    UIVisualElement_FeaturesStuff         = RT_BIT(12),
#ifndef VBOX_WS_MAC
    UIVisualElement_MiniToolBar           = RT_BIT(13),
#endif /* !VBOX_WS_MAC */
    UIVisualElement_AllStuff              = 0xFFFF
};

/* Mouse states enum: */
enum UIMouseStateType
{
    UIMouseStateType_MouseCaptured         = RT_BIT(0),
    UIMouseStateType_MouseAbsolute         = RT_BIT(1),
    UIMouseStateType_MouseAbsoluteDisabled = RT_BIT(2),
    UIMouseStateType_MouseNeedsHostCursor  = RT_BIT(3)
};

/* Machine View states enum: */
enum UIViewStateType
{
    UIViewStateType_KeyboardCaptured = RT_BIT(0),
    UIViewStateType_HostKeyPressed   = RT_BIT(1)
};

#endif // __UIMachineDefs_h__

