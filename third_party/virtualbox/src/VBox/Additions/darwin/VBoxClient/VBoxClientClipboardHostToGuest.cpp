/** $Id: VBoxClientClipboardHostToGuest.cpp $ */
/** @file
 * VBoxClient - Shared Clipboard Host -> Guest copying, Darwin.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <Carbon/Carbon.h>
#include <signal.h>
#include <stdlib.h>

#include <iprt/thread.h>
#include <iprt/mem.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include "VBoxClientInternal.h"

/**
 * Allocate memory for host buffer and receive it.
 *
 * @param   u32ClientId    Host connection.
 * @param   fFormat        Buffer data format.
 * @param   pData          Where to store received data.
 * @param   cbDataSize     The size of the received data.
 * @param   cbMemSize      The actual size of memory occupied by *pData.
 *
 * @returns IPRT status code.
 */
static int vbclClipboardReadHostData(uint32_t u32ClientId, uint32_t fFormat, void **pData, uint32_t *cbDataSize, uint32_t *cbMemSize)
{
    int rc;

    AssertReturn(pData && cbDataSize && cbMemSize, VERR_INVALID_PARAMETER);

    uint32_t  cbDataSizeInternal = _4K;
    uint32_t  cbMemSizeInternal  = cbDataSizeInternal;
    void     *pDataInternal      = RTMemPageAllocZ(cbDataSizeInternal);

    if (!pDataInternal)
        return VERR_NO_MEMORY;

    rc = VbglR3ClipboardReadData(u32ClientId, fFormat, pDataInternal, cbMemSizeInternal, &cbDataSizeInternal);
    if (rc == VINF_BUFFER_OVERFLOW)
    {
        /* Reallocate bigger buffer and receive all the data */
        RTMemPageFree(pDataInternal, cbMemSizeInternal);
        cbDataSizeInternal = cbMemSizeInternal = RT_ALIGN_32(cbDataSizeInternal, PAGE_SIZE);
        pDataInternal = RTMemPageAllocZ(cbMemSizeInternal);
        if (!pDataInternal)
            return VERR_NO_MEMORY;

        rc = VbglR3ClipboardReadData(u32ClientId, fFormat, pDataInternal, cbMemSizeInternal, &cbDataSizeInternal);
    }

    /* Error occurred of zero-sized buffer */
    if (RT_FAILURE(rc))
    {
        RTMemPageFree(pDataInternal, cbMemSizeInternal);
        return VERR_NO_MEMORY;
    }

    *pData      = pDataInternal;
    *cbDataSize = cbDataSizeInternal;
    *cbMemSize  = cbMemSizeInternal;

    return rc;
}

/**
 * Release memory occupied by host buffer.
 *
 * @param   pData          Pointer to memory occupied by host buffer.
 * @param   cbMemSize      The actual size of memory occupied by *pData.
 */
static void vbclClipboardReleaseHostData(void **pData, uint32_t cbMemSize)
{
    AssertReturnVoid(pData && cbMemSize > 0);
    RTMemPageFree(*pData, cbMemSize);
}

/**
 * Paste buffer into guest clipboard.
 *
 * @param   pPasteboard    Guest PasteBoard reference.
 * @param   pData          Data to be pasted.
 * @param   cbDataSize     The size of *pData.
 * @param   fFormat        Buffer data format.
 * @param   fClear         Whether or not clear guest clipboard before insert data.
 *
 * @returns IPRT status code.
 */
static int vbclClipboardGuestPasteData(PasteboardRef pPasteboard, UInt8 *pData, CFIndex cbDataSize, CFStringRef sFormat, bool fClear)
{
    PasteboardItemID    itemId   = (PasteboardItemID)1;
    CFDataRef           textData = NULL;
    OSStatus            rc;

    /* Ignoring sunchronization flags here */
    PasteboardSynchronize(pPasteboard);

    if (fClear)
    {
        rc = PasteboardClear(pPasteboard);
        AssertReturn(rc == noErr, VERR_NOT_SUPPORTED);
    }

    /* Create a CData object which we could pass to the pasteboard */
    if ((textData = CFDataCreate(kCFAllocatorDefault, pData, cbDataSize)))
    {
        /* Put the Utf-8 version to the pasteboard */
        rc = PasteboardPutItemFlavor(pPasteboard, itemId, sFormat, textData, 0);
        CFRelease(textData);
        if (rc != noErr)
        {
            VBoxClientVerbose(3, "unable to put data into guest's clipboard: %d\n", rc);
            return VERR_GENERAL_FAILURE;
        }
    }
    else
        return VERR_NO_MEMORY;

    /* Synchronize updated content */
    PasteboardSynchronize(pPasteboard);

    return VINF_SUCCESS;
}

/**
 * Paste text data into guest clipboard.
 *
 * @param   pPasteboard    Guest PasteBoard reference.
 * @param   pData          Data to be pasted.
 * @param   cbDataSize     Size of *pData.
 */
static int vbclClipboardGuestPasteText(PasteboardRef pPasteboard, void *pData, uint32_t cbDataSize)
{
    size_t   cbActualLen;
    int      rc;
    char    *pszUtf8Buf;
    RTUTF16 *pDataInternal;

    AssertReturn(pData, VERR_INVALID_PARAMETER);

    /* Skip zero-sized buffer */
    AssertReturn(cbDataSize > 0, VINF_SUCCESS);

    /* If buffer content is Unicode text, then deliver
       it in both formats UTF16 (original) and UTF8. */

    /* Convert END-OF-LINE */
    rc = vboxClipboardUtf16GetLinSize((RTUTF16 *)pData, cbDataSize / 2, &cbActualLen);
    AssertReturn(RT_SUCCESS(rc), rc);
    pDataInternal = (RTUTF16 *)RTMemAlloc(cbActualLen * 2);
    AssertReturn(pDataInternal, VERR_NO_MEMORY);
    rc = vboxClipboardUtf16WinToLin((RTUTF16 *)pData, cbDataSize / 2, pDataInternal, cbActualLen);

    /* Do actual paste */
    if (RT_SUCCESS(rc))
    {
        /* Paste UTF16 */
        rc = vbclClipboardGuestPasteData(pPasteboard, (UInt8 *)pDataInternal, cbActualLen * 2, kUTTypeUTF16PlainText, true);
        if (RT_SUCCESS(rc))
        {
            /* Paste UTF8 */
            rc = RTUtf16ToUtf8((RTUTF16 *)pDataInternal, &pszUtf8Buf);
            if (RT_SUCCESS(rc))
            {
                rc = vbclClipboardGuestPasteData(pPasteboard, (UInt8 *)pszUtf8Buf, strlen(pszUtf8Buf), kUTTypeUTF8PlainText, false);
                RTStrFree(pszUtf8Buf);
            }
        }

    }

    RTMemFree(pDataInternal);

    return rc;
}

/**
 * Paste picture data into guest clipboard.
 *
 * @param   pPasteboard    Guest PasteBoard reference.
 * @param   pData          Data to be pasted.
 * @param   cbDataSize     The size of *pData.
 *
 * @returns IPRT status code.
 */
static int vbclClipboardGuestPastePicture(PasteboardRef pPasteboard, void *pData, uint32_t cbDataSize)
{
    int     rc;
    void   *pBmp;
    size_t  cbBmpSize;

    AssertReturn(pData, VERR_INVALID_PARAMETER);
    /* Skip zero-sized buffer */
    AssertReturn(cbDataSize > 0, VINF_SUCCESS);

    rc = vboxClipboardDibToBmp(pData, cbDataSize, &pBmp, &cbBmpSize);
    AssertReturn(RT_SUCCESS(rc), rc);

    rc = vbclClipboardGuestPasteData(pPasteboard, (UInt8 *)pBmp, cbBmpSize, kUTTypeBMP, true);
    RTMemFree(pBmp);

    return rc;
}

/**
 * Read host's clipboard buffer and put its content to guest clipboard.
 *
 * @param   u32ClientId    Host connection.
 * @param   pPasteboard    Guest PasteBoard reference.
 * @param   fFormats       List of data formats (bit field) received from host.
 *
 * @returns IPRT status code.
 */
int vbclClipboardForwardToGuest(uint32_t u32ClientId, PasteboardRef pPasteboard, uint32_t fFormats)
{
    int       rc = VERR_INVALID_PARAMETER;
    void     *pData;
    uint32_t  cbDataSize, cbMemSize;
    uint32_t  fFormatsInternal = fFormats;

    /* Walk across all item(s) formats */
    while (fFormatsInternal)
    {
        if (fFormatsInternal & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
        {
            VBoxClientVerbose(3, "found VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT: %d\n", fFormatsInternal);

            rc = vbclClipboardReadHostData(u32ClientId, VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT, &pData, &cbDataSize, &cbMemSize);
            if (RT_SUCCESS(rc))
            {
                /* Store data in guest buffer */
                rc = vbclClipboardGuestPasteText(pPasteboard, pData, cbDataSize);

                /* Release occupied resources */
                vbclClipboardReleaseHostData(&pData, cbMemSize);
            }

            fFormatsInternal &= ~((uint32_t)VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT);
        }

        else if (fFormatsInternal & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
        {
            VBoxClientVerbose(3, "found VBOX_SHARED_CLIPBOARD_FMT_BITMAP: %d\n", fFormatsInternal);

            rc = vbclClipboardReadHostData(u32ClientId, VBOX_SHARED_CLIPBOARD_FMT_BITMAP, &pData, &cbDataSize, &cbMemSize);
            if (RT_SUCCESS(rc))
            {
                /* Store data in guest buffer */
                rc = vbclClipboardGuestPastePicture(pPasteboard, pData, cbDataSize);

                /* Release occupied resources */
                vbclClipboardReleaseHostData(&pData, cbMemSize);
            }

            fFormatsInternal &= ~((uint32_t)VBOX_SHARED_CLIPBOARD_FMT_BITMAP);
        }

        else if (fFormatsInternal & VBOX_SHARED_CLIPBOARD_FMT_HTML)
        {
            VBoxClientVerbose(3, "found VBOX_SHARED_CLIPBOARD_FMT_HTML: %d\n", fFormatsInternal);

            rc = vbclClipboardReadHostData(u32ClientId, VBOX_SHARED_CLIPBOARD_FMT_HTML, &pData, &cbDataSize, &cbMemSize);
            if (RT_SUCCESS(rc))
            {
                /* Store data in guest buffer */
                rc = vbclClipboardGuestPasteData(pPasteboard, (UInt8 *)pData, cbDataSize, kUTTypeHTML, true);

                /* Release occupied resources */
                vbclClipboardReleaseHostData(&pData, cbMemSize);
            }

            fFormatsInternal &= ~((uint32_t)VBOX_SHARED_CLIPBOARD_FMT_HTML);
        }

        else
        {
            VBoxClientVerbose(3, "received data in unsupported format: %d\n", fFormats);
            break;
        }
    }

    return rc;
}
