/* $Id: UsbWebcamInterface.h $ */
/** @file
 * VirtualBox PDM Driver for Emulated USB Webcam
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

#ifndef ____H_USBWEBCAMINTERFACE
#define ____H_USBWEBCAMINTERFACE

#include <VBox/vmm/pdmdrv.h>
#define VRDE_VIDEOIN_WITH_VRDEINTERFACE /* Get the VRDE interface definitions. */
#include <VBox/RemoteDesktop/VRDEVideoIn.h>

class ConsoleVRDPServer;
typedef struct EMWEBCAMDRV EMWEBCAMDRV;
typedef struct EMWEBCAMREMOTE EMWEBCAMREMOTE;

class EmWebcam
{
    public:
        EmWebcam(ConsoleVRDPServer *pServer);
        virtual ~EmWebcam();

        static const PDMDRVREG DrvReg;

        void EmWebcamConstruct(EMWEBCAMDRV *pDrv);
        void EmWebcamDestruct(EMWEBCAMDRV *pDrv);

        /* Callbacks. */
        void EmWebcamCbNotify(uint32_t u32Id, const void *pvData, uint32_t cbData);
        void EmWebcamCbDeviceDesc(int rcRequest, void *pDeviceCtx, void *pvUser,
                                  const VRDEVIDEOINDEVICEDESC *pDeviceDesc, uint32_t cbDeviceDesc);
        void EmWebcamCbControl(int rcRequest, void *pDeviceCtx, void *pvUser,
                               const VRDEVIDEOINCTRLHDR *pControl, uint32_t cbControl);
        void EmWebcamCbFrame(int rcRequest, void *pDeviceCtx,
                             const VRDEVIDEOINPAYLOADHDR *pFrame, uint32_t cbFrame);

        /* Methods for the PDM driver. */
        int SendControl(EMWEBCAMDRV *pDrv, void *pvUser, uint64_t u64DeviceId,
                        const VRDEVIDEOINCTRLHDR *pControl, uint32_t cbControl);

    private:
        static DECLCALLBACK(void *) drvQueryInterface(PPDMIBASE pInterface, const char *pszIID);
        static DECLCALLBACK(int)    drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
        static DECLCALLBACK(void)   drvDestruct(PPDMDRVINS pDrvIns);

        ConsoleVRDPServer * const mParent;

        EMWEBCAMDRV *mpDrv;
        EMWEBCAMREMOTE *mpRemote;
        uint64_t volatile mu64DeviceIdSrc;
};

#endif /* !____H_USBWEBCAMINTERFACE */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
