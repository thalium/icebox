/** @file
 * Shared Clipboard - Common header for the service extension.
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

#ifndef ___VBox_HostService_VBoxClipboardExt_h
#define ___VBox_HostService_VBoxClipboardExt_h

#include <VBox/types.h>

#define VBOX_CLIPBOARD_EXT_FN_SET_CALLBACK    (0)
#define VBOX_CLIPBOARD_EXT_FN_FORMAT_ANNOUNCE (1)
#define VBOX_CLIPBOARD_EXT_FN_DATA_READ       (2)
#define VBOX_CLIPBOARD_EXT_FN_DATA_WRITE      (3)

typedef DECLCALLBACK(int) VRDPCLIPBOARDEXTCALLBACK (uint32_t u32Function, uint32_t u32Format, void *pvData, uint32_t cbData);
typedef VRDPCLIPBOARDEXTCALLBACK *PFNVRDPCLIPBOARDEXTCALLBACK;

typedef struct _VBOXCLIPBOARDEXTPARMS
{
    uint32_t   u32Format;
    union
    {
        void                        *pvData;
        PFNVRDPCLIPBOARDEXTCALLBACK pfnCallback;
    } u;
    uint32_t   cbData;
} VBOXCLIPBOARDEXTPARMS;

#endif
