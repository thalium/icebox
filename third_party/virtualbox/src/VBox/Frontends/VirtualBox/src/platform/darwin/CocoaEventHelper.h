/* $Id: CocoaEventHelper.h $ */
/** @file
 * VBox Qt GUI - Declarations of utility functions for handling Darwin Cocoa
 * specific event handling tasks.
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

#ifndef ___CocoaEventHelper_h
#define ___CocoaEventHelper_h

#include <VBox/VBoxCocoa.h>
#include <iprt/cdefs.h> /* for RT_C_DECLS_BEGIN/RT_C_DECLS_END & stuff */

ADD_COCOA_NATIVE_REF(NSEvent);

RT_C_DECLS_BEGIN

/********************************************************************************
 *
 * Event/Keyboard helpers (OS System native)
 *
 ********************************************************************************/
unsigned long darwinEventModifierFlags(ConstNativeNSEventRef pEvent);
uint32_t darwinEventModifierFlagsXlated(ConstNativeNSEventRef pEvent);
const char *darwinEventTypeName(unsigned long eEvtType);
void darwinPrintEvent(const char *pszPrefix, ConstNativeNSEventRef pEvent);
void darwinPostStrippedMouseEvent(ConstNativeNSEventRef pEvent);

RT_C_DECLS_END

#endif /* !___CocoaEventHelper_h */

