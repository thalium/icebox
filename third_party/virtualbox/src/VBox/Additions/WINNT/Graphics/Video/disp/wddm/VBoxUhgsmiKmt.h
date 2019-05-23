/* $Id: VBoxUhgsmiKmt.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#ifndef ___VBoxUhgsmiKmt_h__
#define ___VBoxUhgsmiKmt_h__

#include "VBoxDispD3DCmn.h"


typedef struct VBOXUHGSMI_PRIVATE_KMT
{
    VBOXUHGSMI_PRIVATE_BASE BasePrivate;
    VBOXDISPKMT_CALLBACKS Callbacks;
    VBOXDISPKMT_ADAPTER Adapter;
    VBOXDISPKMT_DEVICE Device;
    VBOXDISPKMT_CONTEXT Context;
} VBOXUHGSMI_PRIVATE_KMT, *PVBOXUHGSMI_PRIVATE_KMT;

#define VBOXUHGSMIKMT_GET_PRIVATE(_p, _t) ((_t*)(((uint8_t*)_p) - RT_OFFSETOF(_t, BasePrivate.Base)))
#define VBOXUHGSMIKMT_GET(_p) VBOXUHGSMIKMT_GET_PRIVATE(_p, VBOXUHGSMI_PRIVATE_KMT)

HRESULT vboxUhgsmiKmtDestroy(PVBOXUHGSMI_PRIVATE_KMT pHgsmi);

HRESULT vboxUhgsmiKmtCreate(PVBOXUHGSMI_PRIVATE_KMT pHgsmi, BOOL bD3D);

#endif /* #ifndef ___VBoxUhgsmiKmt_h__ */
