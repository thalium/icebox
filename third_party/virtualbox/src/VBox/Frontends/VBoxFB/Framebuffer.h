/** @file
 * VBoxFB - Declaration of VBoxDirectFB class.
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
 */

#ifndef __H_FRAMEBUFFER
#define __H_FRAMEBUFFER

#include "VBoxFB.h"

class VBoxDirectFB : public IFramebuffer
{
public:
    VBoxDirectFB(IDirectFB *aDFB, IDirectFBSurface *aSurface);
    virtual ~VBoxDirectFB();

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetWidth(PRUint32 *width);
    NS_IMETHOD GetHeight(PRUint32 *height);
    NS_IMETHOD Lock();
    NS_IMETHOD Unlock();
    NS_IMETHOD GetAddress(PRUint8 **address);
    NS_IMETHOD GetBitsPerPixel(PRUint32 *bitsPerPixel);
    NS_IMETHOD GetBytesPerLine(PRUint32 *bytesPerLine);
    NS_IMETHOD GetPixelFormat(PRUint32 *pixelFormat);
    NS_IMETHOD GetUsesGuestVRAM(PRBool *usesGuestVRAM);
    NS_IMETHOD GetHeightReduction(PRUint32 *heightReduction);
    NS_IMETHOD GetOverlay(IFramebufferOverlay **aOverlay);
    NS_IMETHOD GetWinId(PRUint64 *winId);
    NS_IMETHOD NotifyUpdate(PRUint32 x, PRUint32 y, PRUint32 w, PRUint32 h);
    NS_IMETHOD RequestResize(PRUint32 aScreenId, PRUint32 pixelFormat, PRUint8 *vram,
                             PRUint32 bitsPerPixel, PRUint32 bytesPerLine,
                             PRUint32 w, PRUint32 h,
                             PRBool *finished);
    NS_IMETHOD VideoModeSupported(PRUint32 width, PRUint32 height, PRUint32 bpp, PRBool *supported);
    NS_IMETHOD GetVisibleRegion(PRUint8 *aRectangles, PRUint32 aCount, PRUint32 *aCountCopied);
    NS_IMETHOD SetVisibleRegion(PRUint8 *aRectangles, PRUint32 aCount);

    NS_IMETHOD ProcessVHWACommand(PRUint8 *pCommand, LONG enmCmd, BOOL fGuestCmd);

    NS_IMETHOD Notify3DEvent(PRUint32 type, PRUint8 *reserved);
private:
    int createSurface(uint32_t w, uint32_t h);

    IDirectFB *dfb;
    IDirectFBSurface *surface;
    uint32_t screenWidth;
    uint32_t screenHeight;
    IDirectFBSurface *fbInternalSurface;
    void *fbBufferAddress;
    uint32_t fbWidth;
    uint32_t fbHeight;
    uint32_t fbPitch;
    int fbSurfaceLocked;
};


#endif // !__H_FRAMEBUFFER

