/* $Id: HGSMIHost.h $ */
/** @file
 *
 * VBox Host Guest Shared Memory Interface (HGSMI).
 * Host part.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#ifndef ___HGSMIHost_h
#define ___HGSMIHost_h

#include <VBox/vmm/vm.h>

#include <HGSMI.h>
#include <HGSMIChSetup.h>

struct HGSMIINSTANCE;
typedef struct HGSMIINSTANCE *PHGSMIINSTANCE;

/* Callback for the guest notification about a new host buffer. */
typedef DECLCALLBACK(void) FNHGSMINOTIFYGUEST(void *pvCallback);
typedef FNHGSMINOTIFYGUEST *PFNHGSMINOTIFYGUEST;

/*
 * Public Host API for virtual devices.
 */

int HGSMICreate (PHGSMIINSTANCE *ppIns,
                 PVM             pVM,
                 const char     *pszName,
                 HGSMIOFFSET     offBase,
                 uint8_t        *pu8MemBase,
                 HGSMISIZE       cbMem,
                 PFNHGSMINOTIFYGUEST pfnNotifyGuest,
                 void           *pvNotifyGuest,
                 size_t         cbContext);

void HGSMIDestroy (PHGSMIINSTANCE pIns);

void *HGSMIContext (PHGSMIINSTANCE pIns);

void *HGSMIOffsetToPointerHost (PHGSMIINSTANCE pIns,
                                HGSMIOFFSET offBuffer);

HGSMIOFFSET HGSMIPointerToOffsetHost (PHGSMIINSTANCE pIns,
                                      const void *pv);

int HGSMIHostChannelRegister (PHGSMIINSTANCE pIns,
                          uint8_t u8Channel,
                          PFNHGSMICHANNELHANDLER pfnChannelHandler,
                          void *pvChannelHandler);

int HGSMIChannelRegisterName (PHGSMIINSTANCE pIns,
                              const char *pszChannel,
                              PFNHGSMICHANNELHANDLER pfnChannelHandler,
                              void *pvChannelHandler,
                              uint8_t *pu8Channel);

int HGSMIHostHeapSetup(PHGSMIINSTANCE pIns,
                       HGSMIOFFSET    offHeap,
                       HGSMISIZE      cbHeap);

/*
 * Virtual hardware IO handlers.
 */

/* Guests passes a new command buffer to the host. */
void HGSMIGuestWrite (PHGSMIINSTANCE pIns,
                      HGSMIOFFSET offBuffer);

/* Guest reads information about guest buffers. */
HGSMIOFFSET HGSMIGuestRead (PHGSMIINSTANCE pIns);

/* Guest reads the host FIFO to get a command. */
HGSMIOFFSET HGSMIHostRead (PHGSMIINSTANCE pIns);

/* Guest reports that the command at this offset has been processed.  */
void HGSMIHostWrite (PHGSMIINSTANCE pIns,
                     HGSMIOFFSET offBuffer);

void HGSMISetHostGuestFlags(PHGSMIINSTANCE pIns, uint32_t flags);
uint32_t HGSMIGetHostGuestFlags(HGSMIINSTANCE *pIns);

void HGSMIClearHostGuestFlags(PHGSMIINSTANCE pIns, uint32_t flags);

/*
 * Low level interface for submitting buffers to the guest.
 *
 * These functions are not directly available for anyone but the
 * virtual hardware device.
 */

/* Allocate a buffer in the host heap. */
int HGSMIHostCommandAlloc(PHGSMIINSTANCE pIns,
                          void **ppvData,
                          HGSMISIZE cbData,
                          uint8_t u8Channel,
                          uint16_t u16ChannelInfo);

int HGSMIHostCommandSubmitAndFreeAsynch(PHGSMIINSTANCE pIns,
                                        void *pvData,
                                        bool fDoIrq);

int HGSMIHostCommandFree(PHGSMIINSTANCE pIns,
                         void *pvData);

int HGSMIHostLoadStateExec (PHGSMIINSTANCE pIns, PSSMHANDLE pSSM, uint32_t u32Version);

int HGSMIHostSaveStateExec (PHGSMIINSTANCE pIns, PSSMHANDLE pSSM);

#ifdef VBOX_WITH_WDDM
int HGSMICompleteGuestCommand(PHGSMIINSTANCE pIns, void *pvMem, bool bDoIrq);
#endif

#endif /* !___HGSMIHost_h*/

