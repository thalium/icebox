/* $Id: DisplaySourceBitmapImpl.cpp $ */
/** @file
 * Bitmap of a guest screen implementation.
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

#define LOG_GROUP LOG_GROUP_MAIN_DISPLAYSOURCEBITMAP
#include "LoggingNew.h"

#include "DisplayImpl.h"

/*
 * DisplaySourceBitmap implementation.
 */
DEFINE_EMPTY_CTOR_DTOR(DisplaySourceBitmap)

HRESULT DisplaySourceBitmap::FinalConstruct()
{
    return BaseFinalConstruct();
}

void DisplaySourceBitmap::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

HRESULT DisplaySourceBitmap::init(ComObjPtr<Display> pDisplay, unsigned uScreenId, DISPLAYFBINFO *pFBInfo)
{
    LogFlowThisFunc(("[%u]\n", uScreenId));

    ComAssertRet(!pDisplay.isNull(), E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.pDisplay = pDisplay;
    m.uScreenId = uScreenId;
    m.pFBInfo = pFBInfo;

    m.pu8Allocated = NULL;

    m.pu8Address = NULL;
    m.ulWidth = 0;
    m.ulHeight = 0;
    m.ulBitsPerPixel = 0;
    m.ulBytesPerLine = 0;
    m.bitmapFormat = BitmapFormat_Opaque;

    int rc = initSourceBitmap(uScreenId, pFBInfo);

    if (RT_FAILURE(rc))
        return E_FAIL;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

void DisplaySourceBitmap::uninit()
{
    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    LogFlowThisFunc(("[%u]\n", m.uScreenId));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m.pDisplay.setNull();
    RTMemFree(m.pu8Allocated);
}

HRESULT DisplaySourceBitmap::getScreenId(ULONG *aScreenId)
{
    HRESULT hr = S_OK;
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aScreenId = m.uScreenId;
    return hr;
}

HRESULT DisplaySourceBitmap::queryBitmapInfo(BYTE **aAddress,
                                             ULONG *aWidth,
                                             ULONG *aHeight,
                                             ULONG *aBitsPerPixel,
                                             ULONG *aBytesPerLine,
                                             BitmapFormat_T *aBitmapFormat)
{
    HRESULT hr = S_OK;
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAddress      = m.pu8Address;
    *aWidth        = m.ulWidth;
    *aHeight       = m.ulHeight;
    *aBitsPerPixel = m.ulBitsPerPixel;
    *aBytesPerLine = m.ulBytesPerLine;
    *aBitmapFormat  = m.bitmapFormat;

    return hr;
}

int DisplaySourceBitmap::initSourceBitmap(unsigned aScreenId,
                                          DISPLAYFBINFO *pFBInfo)
{
    RT_NOREF(aScreenId);
    int rc = VINF_SUCCESS;

    if (pFBInfo->w == 0 || pFBInfo->h == 0)
    {
        return VERR_NOT_SUPPORTED;
    }

    BYTE *pAddress = NULL;
    ULONG ulWidth = 0;
    ULONG ulHeight = 0;
    ULONG ulBitsPerPixel = 0;
    ULONG ulBytesPerLine = 0;
    BitmapFormat_T bitmapFormat = BitmapFormat_Opaque;

    if (pFBInfo->pu8FramebufferVRAM && pFBInfo->u16BitsPerPixel == 32 && !pFBInfo->fDisabled)
    {
        /* From VRAM. */
        LogFunc(("%d from VRAM\n", aScreenId));
        pAddress       = pFBInfo->pu8FramebufferVRAM;
        ulWidth        = pFBInfo->w;
        ulHeight       = pFBInfo->h;
        ulBitsPerPixel = pFBInfo->u16BitsPerPixel;
        ulBytesPerLine = pFBInfo->u32LineSize;
        bitmapFormat   = BitmapFormat_BGR;
        m.pu8Allocated = NULL;
    }
    else
    {
        /* Allocated byffer */
        LogFunc(("%d allocated\n", aScreenId));
        pAddress       = NULL;
        ulWidth        = pFBInfo->w;
        ulHeight       = pFBInfo->h;
        ulBitsPerPixel = 32;
        ulBytesPerLine = ulWidth * 4;
        bitmapFormat   = BitmapFormat_BGR;

        m.pu8Allocated = (uint8_t *)RTMemAlloc(ulBytesPerLine * ulHeight);
        if (m.pu8Allocated == NULL)
        {
            rc = VERR_NO_MEMORY;
        }
        else
        {
            pAddress = m.pu8Allocated;
        }
    }

    if (RT_SUCCESS(rc))
    {
        m.pu8Address = pAddress;
        m.ulWidth = ulWidth;
        m.ulHeight = ulHeight;
        m.ulBitsPerPixel = ulBitsPerPixel;
        m.ulBytesPerLine = ulBytesPerLine;
        m.bitmapFormat = bitmapFormat;
        if (pFBInfo->fDisabled)
        {
            RT_BZERO(pAddress, ulBytesPerLine * ulHeight);
        }
    }

    return rc;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
