/* $Id: darwin-pasteboard.cpp $ */
/** @file
 * Shared Clipboard: Mac OS X host implementation.
 */

/*
 * Includes contributions from François Revol
 *
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_HGCM
#include <Carbon/Carbon.h>

#include <iprt/mem.h>
#include <iprt/assert.h>
#include "iprt/err.h"

#include "VBox/log.h"
#include "VBox/HostServices/VBoxClipboardSvc.h"
#include "VBox/GuestHost/clipboard-helper.h"

/* For debugging */
//#define SHOW_CLIPBOARD_CONTENT

/**
 * Initialize the global pasteboard and return a reference to it.
 *
 * @param pPasteboardRef Reference to the global pasteboard.
 *
 * @returns IPRT status code.
 */
int initPasteboard(PasteboardRef *pPasteboardRef)
{
    int rc = VINF_SUCCESS;

    if (PasteboardCreate(kPasteboardClipboard, pPasteboardRef))
        rc = VERR_NOT_SUPPORTED;

    return rc;
}

/**
 * Release the reference to the global pasteboard.
 *
 * @param pPasteboardRef Reference to the global pasteboard.
 */
void destroyPasteboard(PasteboardRef *pPasteboardRef)
{
    CFRelease(*pPasteboardRef);
    *pPasteboardRef = NULL;
}

/**
 * Inspect the global pasteboard for new content. Check if there is some type
 * that is supported by vbox and return it.
 *
 * @param   pPasteboard    Reference to the global pasteboard.
 * @param   pfFormats      Pointer for the bit combination of the
 *                         supported types.
 * @param   pfChanged      True if something has changed after the
 *                         last call.
 *
 * @returns IPRT status code. (Always VINF_SUCCESS atm.)
 */
int queryNewPasteboardFormats(PasteboardRef pPasteboard, uint32_t *pfFormats, bool *pfChanged)
{
    Log(("queryNewPasteboardFormats\n"));

    OSStatus err = noErr;
    *pfChanged = true;

    PasteboardSyncFlags syncFlags;
    /* Make sure all is in sync */
    syncFlags = PasteboardSynchronize(pPasteboard);
    /* If nothing changed return */
    if (!(syncFlags & kPasteboardModified))
    {
        *pfChanged = false;
        return VINF_SUCCESS;
    }

    /* Are some items in the pasteboard? */
    ItemCount itemCount;
    err = PasteboardGetItemCount(pPasteboard, &itemCount);
    if (itemCount < 1)
        return VINF_SUCCESS;

    /* The id of the first element in the pasteboard */
    int rc = VINF_SUCCESS;
    PasteboardItemID itemID;
    if (!(err = PasteboardGetItemIdentifier(pPasteboard, 1, &itemID)))
    {
        /* Retrieve all flavors in the pasteboard, maybe there
         * is something we can use. */
        CFArrayRef flavorTypeArray;
        if (!(err = PasteboardCopyItemFlavors(pPasteboard, itemID, &flavorTypeArray)))
        {
            CFIndex flavorCount;
            flavorCount = CFArrayGetCount(flavorTypeArray);
            for (CFIndex flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++)
            {
                CFStringRef flavorType;
                flavorType = static_cast <CFStringRef>(CFArrayGetValueAtIndex(flavorTypeArray,
                                                                               flavorIndex));
                /* Currently only unicode supported */
                if (UTTypeConformsTo(flavorType, kUTTypeUTF8PlainText) ||
                    UTTypeConformsTo(flavorType, kUTTypeUTF16PlainText))
                {
                    Log(("Unicode flavor detected.\n"));
                    *pfFormats |= VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT;
                }
                else if (UTTypeConformsTo(flavorType, kUTTypeBMP))
                {
                    Log(("BMP flavor detected.\n"));
                    *pfFormats |= VBOX_SHARED_CLIPBOARD_FMT_BITMAP;
                }
            }
            CFRelease(flavorTypeArray);
        }
    }

    Log(("queryNewPasteboardFormats: rc = %02X\n", rc));
    return rc;
}

/**
 * Read content from the host clipboard and write it to the internal clipboard
 * structure for further processing.
 *
 * @param   pPasteboard    Reference to the global pasteboard.
 * @param   fFormat        The format type which should be read.
 * @param   pv             The destination buffer.
 * @param   cb             The size of the destination buffer.
 * @param   pcbActual      The size which is needed to transfer the content.
 *
 * @returns IPRT status code.
 */
int readFromPasteboard(PasteboardRef pPasteboard, uint32_t fFormat, void *pv, uint32_t cb, uint32_t *pcbActual)
{
    Log(("readFromPasteboard: fFormat = %02X\n", fFormat));

    OSStatus err = noErr;

    /* Make sure all is in sync */
    PasteboardSynchronize(pPasteboard);

    /* Are some items in the pasteboard? */
    ItemCount itemCount;
    err = PasteboardGetItemCount(pPasteboard, &itemCount);
    if (itemCount < 1)
        return VINF_SUCCESS;

    /* The id of the first element in the pasteboard */
    int rc = VERR_NOT_SUPPORTED;
    PasteboardItemID itemID;
    if (!(err = PasteboardGetItemIdentifier(pPasteboard, 1, &itemID)))
    {
        /* The guest request unicode */
        if (fFormat & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
        {
            CFDataRef outData;
            PRTUTF16 pwszTmp = NULL;
            /* Try utf-16 first */
            if (!(err = PasteboardCopyItemFlavorData(pPasteboard, itemID, kUTTypeUTF16PlainText, &outData)))
            {
                Log(("Clipboard content is utf-16\n"));

                PRTUTF16 pwszString = (PRTUTF16)CFDataGetBytePtr(outData);
                if (pwszString)
                    rc = RTUtf16DupEx(&pwszTmp, pwszString, 0);
                else
                    rc = VERR_INVALID_PARAMETER;
            }
            /* Second try is utf-8 */
            else
                if (!(err = PasteboardCopyItemFlavorData(pPasteboard, itemID, kUTTypeUTF8PlainText, &outData)))
                {
                    Log(("readFromPasteboard: clipboard content is utf-8\n"));
                    const char *pszString = (const char *)CFDataGetBytePtr(outData);
                    if (pszString)
                        rc = RTStrToUtf16(pszString, &pwszTmp);
                    else
                        rc = VERR_INVALID_PARAMETER;
                }
            if (pwszTmp)
            {
                /* Check how much longer will the converted text will be. */
                size_t cwSrc = RTUtf16Len(pwszTmp);
                size_t cwDest;
                rc = vboxClipboardUtf16GetWinSize(pwszTmp, cwSrc, &cwDest);
                if (RT_FAILURE(rc))
                {
                    RTUtf16Free(pwszTmp);
                    Log(("readFromPasteboard: clipboard conversion failed.  vboxClipboardUtf16GetWinSize returned %Rrc.  Abandoning.\n", rc));
                    AssertRCReturn(rc, rc);
                }
                /* Set the actually needed data size */
                *pcbActual = cwDest * 2;
                /* Return success state */
                rc = VINF_SUCCESS;
                /* Do not copy data if the dst buffer is not big enough. */
                if (*pcbActual <= cb)
                {
                    rc = vboxClipboardUtf16LinToWin(pwszTmp, RTUtf16Len(pwszTmp), static_cast <PRTUTF16>(pv), cb / 2);
                    if (RT_FAILURE(rc))
                    {
                        RTUtf16Free(pwszTmp);
                        Log(("readFromPasteboard: clipboard conversion failed.  vboxClipboardUtf16LinToWin() returned %Rrc.  Abandoning.\n", rc));
                        AssertRCReturn(rc, rc);
                    }
#ifdef SHOW_CLIPBOARD_CONTENT
                    Log(("readFromPasteboard: clipboard content: %ls\n", static_cast <PRTUTF16>(pv)));
#endif
                }
                /* Free the temp string */
                RTUtf16Free(pwszTmp);
            }
        }
        /* The guest request BITMAP */
        else if (fFormat & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
        {
            CFDataRef outData;
            const void *pTmp = NULL;
            size_t cbTmpSize;
            /* Get the data from the pasteboard */
            if (!(err = PasteboardCopyItemFlavorData(pPasteboard, itemID, kUTTypeBMP, &outData)))
            {
                Log(("Clipboard content is BMP\n"));
                pTmp = CFDataGetBytePtr(outData);
                cbTmpSize = CFDataGetLength(outData);
            }
            if (pTmp)
            {
                const void *pDib;
                size_t cbDibSize;
                rc = vboxClipboardBmpGetDib(pTmp, cbTmpSize, &pDib, &cbDibSize);
                if (RT_FAILURE(rc))
                {
                    rc = VERR_NOT_SUPPORTED;
                    Log(("readFromPasteboard: unknown bitmap format. vboxClipboardBmpGetDib returned %Rrc.  Abandoning.\n", rc));
                    AssertRCReturn(rc, rc);
                }

                *pcbActual = cbDibSize;
                /* Return success state */
                rc = VINF_SUCCESS;
                /* Do not copy data if the dst buffer is not big enough. */
                if (*pcbActual <= cb)
                {
                    memcpy(pv, pDib, cbDibSize);
#ifdef SHOW_CLIPBOARD_CONTENT
                    Log(("readFromPasteboard: clipboard content bitmap %d bytes\n", cbDibSize));
#endif
                }
            }
        }
    }

    Log(("readFromPasteboard: rc = %02X\n", rc));
    return rc;
}

/**
 * Write clipboard content to the host clipboard from the internal clipboard
 * structure.
 *
 * @param   pPasteboard    Reference to the global pasteboard.
 * @param   pv             The source buffer.
 * @param   cb             The size of the source buffer.
 * @param   fFormat        The format type which should be written.
 *
 * @returns IPRT status code.
 */
int writeToPasteboard(PasteboardRef pPasteboard, void *pv, uint32_t cb, uint32_t fFormat)
{
    Log(("writeToPasteboard: fFormat = %02X\n", fFormat));

    /* Clear the pasteboard */
    if (PasteboardClear(pPasteboard))
        return VERR_NOT_SUPPORTED;

    /* Make sure all is in sync */
    PasteboardSynchronize(pPasteboard);

    int rc = VERR_NOT_SUPPORTED;
    /* Handle the unicode text */
    if (fFormat & VBOX_SHARED_CLIPBOARD_FMT_UNICODETEXT)
    {
        PRTUTF16 pwszSrcText = static_cast <PRTUTF16>(pv);
        size_t cwSrc = cb / 2;
        size_t cwDest = 0;
        /* How long will the converted text be? */
        rc = vboxClipboardUtf16GetLinSize(pwszSrcText, cwSrc, &cwDest);
        if (RT_FAILURE(rc))
        {
            Log(("writeToPasteboard: clipboard conversion failed.  vboxClipboardUtf16GetLinSize returned %Rrc.  Abandoning.\n", rc));
            AssertRCReturn(rc, rc);
        }
        /* Empty clipboard? Not critical */
        if (cwDest == 0)
        {
            Log(("writeToPasteboard: received empty clipboard data from the guest, returning false.\n"));
            return VINF_SUCCESS;
        }
        /* Allocate the necessary memory */
        PRTUTF16 pwszDestText = static_cast <PRTUTF16>(RTMemAlloc(cwDest * 2));
        if (pwszDestText == NULL)
        {
            Log(("writeToPasteboard: failed to allocate %d bytes\n", cwDest * 2));
            return VERR_NO_MEMORY;
        }
        /* Convert the EOL */
        rc = vboxClipboardUtf16WinToLin(pwszSrcText, cwSrc, pwszDestText, cwDest);
        if (RT_FAILURE(rc))
        {
            Log(("writeToPasteboard: clipboard conversion failed.  vboxClipboardUtf16WinToLin() returned %Rrc.  Abandoning.\n", rc));
            RTMemFree(pwszDestText);
            AssertRCReturn(rc, rc);
        }

        CFDataRef textData = NULL;
        /* Item id is 1. Nothing special here. */
        PasteboardItemID itemId = (PasteboardItemID)1;
        /* Create a CData object which we could pass to the pasteboard */
        if ((textData = CFDataCreate(kCFAllocatorDefault,
                                     reinterpret_cast<UInt8*>(pwszDestText), cwDest * 2)))
        {
            /* Put the Utf-16 version to the pasteboard */
            PasteboardPutItemFlavor(pPasteboard, itemId,
                                    kUTTypeUTF16PlainText,
                                    textData, 0);
        }
        /* Create a Utf-8 version */
        char *pszDestText;
        rc = RTUtf16ToUtf8(pwszDestText, &pszDestText);
        if (RT_SUCCESS(rc))
        {
            /* Create a CData object which we could pass to the pasteboard */
            if ((textData = CFDataCreate(kCFAllocatorDefault,
                                         reinterpret_cast<UInt8*>(pszDestText), strlen(pszDestText))))
            {
                /* Put the Utf-8 version to the pasteboard */
                PasteboardPutItemFlavor(pPasteboard, itemId,
                                        kUTTypeUTF8PlainText,
                                        textData, 0);
            }
            RTStrFree(pszDestText);
        }

        RTMemFree(pwszDestText);
        rc = VINF_SUCCESS;
    }
    /* Handle the bitmap */
    else if (fFormat & VBOX_SHARED_CLIPBOARD_FMT_BITMAP)
    {
        /* Create a full BMP from it */
        void *pBmp;
        size_t cbBmpSize;
        CFDataRef bmpData = NULL;
        /* Item id is 1. Nothing special here. */
        PasteboardItemID itemId = (PasteboardItemID)1;

        rc = vboxClipboardDibToBmp(pv, cb, &pBmp, &cbBmpSize);
        if (RT_SUCCESS(rc))
        {
            /* Create a CData object which we could pass to the pasteboard */
            if ((bmpData = CFDataCreate(kCFAllocatorDefault,
                                         reinterpret_cast<UInt8*>(pBmp), cbBmpSize)))
            {
                /* Put the Utf-8 version to the pasteboard */
                PasteboardPutItemFlavor(pPasteboard, itemId,
                                        kUTTypeBMP,
                                        bmpData, 0);
            }
            RTMemFree(pBmp);
        }
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_NOT_IMPLEMENTED;

    Log(("writeToPasteboard: rc = %02X\n", rc));
    return rc;
}

