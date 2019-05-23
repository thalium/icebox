/* $Id: DarwinKeyboard.h $ */
/** @file
 * VBox Qt GUI - Common GUI Library - Darwin Keyboard routines.
 *
 * @todo Move this up somewhere so that the two SDL GUIs can use this code too.
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


#ifndef ___DarwinKeyboard_h___
#define ___DarwinKeyboard_h___

#include <iprt/cdefs.h>
#include <CoreFoundation/CFBase.h>

RT_C_DECLS_BEGIN

/** Private hack for missing rightCmdKey enum. */
#define kEventKeyModifierRightCmdKeyMask (1<<27)

/** The scancode mask. */
#define VBOXKEY_SCANCODE_MASK       0x007f
/** Extended key. */
#define VBOXKEY_EXTENDED            0x0080
/** Modifier key. */
#define VBOXKEY_MODIFIER            0x0400
/** Lock key (like num lock and caps lock). */
#define VBOXKEY_LOCK                0x0800

unsigned DarwinKeycodeToSet1Scancode(unsigned uKeyCode);
UInt32   DarwinAdjustModifierMask(UInt32 fModifiers, const void *pvCocoaEvent);
unsigned DarwinModifierMaskToSet1Scancode(UInt32 fModifiers);
unsigned DarwinModifierMaskToDarwinKeycode(UInt32 fModifiers);
UInt32   DarwinKeyCodeToDarwinModifierMask(unsigned uKeyCode);
void     DarwinDisableGlobalHotKeys(bool fDisable);
void     DarwinGrabKeyboard(bool fGlobalHotkeys);
void     DarwinReleaseKeyboard(void);

void   * DarwinHidDevicesKeepLedsState(void);
int      DarwinHidDevicesApplyAndReleaseLedsState(void *pState);
void     DarwinHidDevicesBroadcastLeds(void *pState, bool fNumLockOn, bool fCapsLockOn, bool fScrollLockOn);

RT_C_DECLS_END

#endif

