/* $Id: DrvAudioVRDE.h $ */
/** @file
 * VirtualBox driver interface to VRDE backend.
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

#ifndef ____H_DRVAUDIOVRDE
#define ____H_DRVAUDIOVRDE

#include <VBox/com/ptr.h>
#include <VBox/RemoteDesktop/VRDE.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmifs.h>

class Console;

class AudioVRDE
{

public:

    AudioVRDE(Console *pConsole);
    virtual ~AudioVRDE(void);

public:

    static const PDMDRVREG DrvReg;

    Console *getParent(void) { return mParent; }

public:

    int onVRDEControl(bool fEnable, uint32_t uFlags);
    int onVRDEInputBegin(void *pvContext, PVRDEAUDIOINBEGIN pVRDEAudioBegin);
    int onVRDEInputData(void *pvContext, const void *pvData, uint32_t cbData);
    int onVRDEInputEnd(void *pvContext);
    int onVRDEInputIntercept(bool fIntercept);

public:

    static DECLCALLBACK(int) drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void) drvDestruct(PPDMDRVINS pDrvIns);

private:

    /** Pointer to the associated VRDE audio driver. */
    struct DRVAUDIOVRDE *mpDrv;
    /** Pointer to parent. */
    Console * const mParent;
};

#endif /* !____H_DRVAUDIOVRDE */

