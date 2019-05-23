/* $Id: VBoxMPShgsmi.h $ */

/** @file
 * VBox WDDM Miniport driver
 */

/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxMPShgsmi_h___
#define ___VBoxMPShgsmi_h___

#include <iprt/cdefs.h>
#include <VBoxVideo.h>

#include "common/VBoxMPUtils.h"

typedef struct VBOXSHGSMI
{
    KSPIN_LOCK HeapLock;
    HGSMIHEAP Heap;
} VBOXSHGSMI, *PVBOXSHGSMI;

typedef DECLCALLBACK(void) FNVBOXSHGSMICMDCOMPLETION(PVBOXSHGSMI pHeap, void *pvCmd, void *pvContext);
typedef FNVBOXSHGSMICMDCOMPLETION *PFNVBOXSHGSMICMDCOMPLETION;

typedef DECLCALLBACK(PFNVBOXSHGSMICMDCOMPLETION) FNVBOXSHGSMICMDCOMPLETION_IRQ(PVBOXSHGSMI pHeap, void *pvCmd, void *pvContext, void **ppvCompletion);
typedef FNVBOXSHGSMICMDCOMPLETION_IRQ *PFNVBOXSHGSMICMDCOMPLETION_IRQ;


const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynchEvent(PVBOXSHGSMI pHeap, PVOID pvBuff, RTSEMEVENT hEventSem);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepSynch(PVBOXSHGSMI pHeap, PVOID pCmd);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynch(PVBOXSHGSMI pHeap, PVOID pvBuff, PFNVBOXSHGSMICMDCOMPLETION pfnCompletion, PVOID pvCompletion, uint32_t fFlags);
const VBOXSHGSMIHEADER* VBoxSHGSMICommandPrepAsynchIrq(PVBOXSHGSMI pHeap, PVOID pvBuff, PFNVBOXSHGSMICMDCOMPLETION_IRQ pfnCompletion, PVOID pvCompletion, uint32_t fFlags);

void VBoxSHGSMICommandDoneAsynch(PVBOXSHGSMI pHeap, const VBOXSHGSMIHEADER* pHeader);
int VBoxSHGSMICommandDoneSynch(PVBOXSHGSMI pHeap, const VBOXSHGSMIHEADER* pHeader);
void VBoxSHGSMICommandCancelAsynch(PVBOXSHGSMI pHeap, const VBOXSHGSMIHEADER* pHeader);
void VBoxSHGSMICommandCancelSynch(PVBOXSHGSMI pHeap, const VBOXSHGSMIHEADER* pHeader);

DECLINLINE(HGSMIOFFSET) VBoxSHGSMICommandOffset(const PVBOXSHGSMI pHeap, const VBOXSHGSMIHEADER* pHeader)
{
    return HGSMIHeapBufferOffset(&pHeap->Heap, (void*)pHeader);
}

/* allows getting VRAM offset of arbitrary pointer within the SHGSMI command
 * if invalid pointer is passed in, behavior is undefined */
DECLINLINE(HGSMIOFFSET) VBoxSHGSMICommandPtrOffset(const PVBOXSHGSMI pHeap, const void * pvPtr)
{
    return HGSMIPointerToOffset (&pHeap->Heap.area, (const HGSMIBUFFERHEADER *)pvPtr);
}

int VBoxSHGSMIInit(PVBOXSHGSMI pHeap, void *pvBase, HGSMISIZE cbArea, HGSMIOFFSET offBase, const HGSMIENV *pEnv);
void VBoxSHGSMITerm(PVBOXSHGSMI pHeap);
void* VBoxSHGSMIHeapAlloc(PVBOXSHGSMI pHeap, HGSMISIZE cbData, uint8_t u8Channel, uint16_t u16ChannelInfo);
void VBoxSHGSMIHeapFree(PVBOXSHGSMI pHeap, void *pvBuffer);
void* VBoxSHGSMIHeapBufferAlloc(PVBOXSHGSMI pHeap, HGSMISIZE cbData);
void VBoxSHGSMIHeapBufferFree(PVBOXSHGSMI pHeap, void *pvBuffer);
void* VBoxSHGSMICommandAlloc(PVBOXSHGSMI pHeap, HGSMISIZE cbData, uint8_t u8Channel, uint16_t u16ChannelInfo);
void VBoxSHGSMICommandFree(PVBOXSHGSMI pHeap, void *pvBuffer);
int VBoxSHGSMICommandProcessCompletion(PVBOXSHGSMI pHeap, VBOXSHGSMIHEADER* pCmd, bool bIrq, struct VBOXVTLIST * pPostProcessList);
int VBoxSHGSMICommandPostprocessCompletion(PVBOXSHGSMI pHeap, struct VBOXVTLIST * pPostProcessList);

#endif /* #ifndef ___VBoxMPShgsmi_h___ */
