/* $Id: PCIRawDevImpl.h $ */
/** @file
 * VirtualBox Driver interface to raw PCI device
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

#ifndef ____H_PCIRAWDEV
#define ____H_PCIRAWDEV

#include "VirtualBoxBase.h"
#include <VBox/vmm/pdmdrv.h>

class Console;
struct DRVMAINPCIRAWDEV;

class PCIRawDev
{
  public:
    PCIRawDev(Console *console);
    virtual ~PCIRawDev();

    static const PDMDRVREG DrvReg;
    struct DRVMAINPCIRAWDEV *mpDrv;

    Console *getParent() const
    {
        return mParent;
    }

  private:
    static DECLCALLBACK(void *) drvQueryInterface(PPDMIBASE pInterface, const char *pszIID);
    static DECLCALLBACK(int)    drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void)   drvDestruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(int)    drvDeviceConstructComplete(PPDMIPCIRAWCONNECTOR pInterface, const char *pcszName,
                                                           uint32_t uHostPCIAddress, uint32_t uGuestPCIAddress,
                                                           int rc);


    Console * const mParent;
};

#endif // !____H_PCIRAWDEV
