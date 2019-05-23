/* $Id: DrvAudioVideoRec.h $ */
/** @file
 * VirtualBox driver interface video recording audio backend.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_DRVAUDIOVIDEOREC
#define ____H_DRVAUDIOVIDEOREC

#include <VBox/com/ptr.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmifs.h>

class Console;

class AudioVideoRec
{

public:

    AudioVideoRec(Console *pConsole);
    virtual ~AudioVideoRec(void);

public:

    static const PDMDRVREG DrvReg;

    Console *getParent(void) { return mpConsole; }

public:

    static DECLCALLBACK(int)  drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void) drvDestruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(int)  drvAttach(PPDMDRVINS pDrvIns, uint32_t fFlags);
    static DECLCALLBACK(void) drvDetach(PPDMDRVINS pDrvIns, uint32_t fFlags);

private:

    /** Pointer to the associated video recording audio driver. */
    struct DRVAUDIOVIDEOREC *mpDrv;
    /** Pointer to parent. */
    Console * const          mpConsole;
};

#endif /* !____H_DRVAUDIOVIDEOREC */

