/* $Id: VBoxMPVModes.h $ */
/** @file
 * VBox WDDM Miniport driver
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxMPVModes_h__
#define ___VBoxMPVModes_h__

//#include "../../common/VBoxVideoTools.h"

#include <cr_sortarray.h>


#define _CR_TYPECAST(_Type, _pVal) ((_Type*)((void*)(_pVal)))

DECLINLINE(uint64_t) vboxRSize2U64(RTRECTSIZE size) { return *_CR_TYPECAST(uint64_t, &(size)); }
DECLINLINE(RTRECTSIZE) vboxU642RSize2(uint64_t size) { return *_CR_TYPECAST(RTRECTSIZE, &(size)); }

#define CR_RSIZE2U64 vboxRSize2U64
#define CR_U642RSIZE vboxU642RSize2

int VBoxWddmVModesInit(PVBOXMP_DEVEXT pExt);
void VBoxWddmVModesCleanup();
const CR_SORTARRAY* VBoxWddmVModesGet(PVBOXMP_DEVEXT pExt, uint32_t u32Target);
int VBoxWddmVModesRemove(PVBOXMP_DEVEXT pExt, uint32_t u32Target, const RTRECTSIZE *pResolution);
int VBoxWddmVModesAdd(PVBOXMP_DEVEXT pExt, uint32_t u32Target, const RTRECTSIZE *pResolution, BOOLEAN fTrancient);

NTSTATUS VBoxWddmChildStatusReportReconnected(PVBOXMP_DEVEXT pDevExt, uint32_t iChild);
NTSTATUS VBoxWddmChildStatusConnect(PVBOXMP_DEVEXT pDevExt, uint32_t iChild, BOOLEAN fConnect);

#endif /* ___VBoxMPVModes_h__ */
