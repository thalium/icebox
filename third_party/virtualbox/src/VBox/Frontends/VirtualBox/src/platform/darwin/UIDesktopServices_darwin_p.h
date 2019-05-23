/* $Id: UIDesktopServices_darwin_p.h $ */
/** @file
 * VBox Qt GUI - Qt GUI - Utility Classes and Functions specific to darwin..
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

#ifndef ___UIDesktopServices_p_h___
#define ___UIDesktopServices_p_h___

#include <VBox/VBoxCocoa.h>
#include <iprt/cdefs.h> /* for RT_C_DECLS_BEGIN/RT_C_DECLS_END & stuff */

ADD_COCOA_NATIVE_REF(NSString);

RT_C_DECLS_BEGIN

bool darwinCreateMachineShortcut(NativeNSStringRef pstrSrcFile, NativeNSStringRef pstrDstPath, NativeNSStringRef pstrName, NativeNSStringRef pstrUuid);
bool darwinOpenInFileManager(NativeNSStringRef pstrFile);

RT_C_DECLS_END

#endif /* !___UIDesktopServices_p_h___ */

