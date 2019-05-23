/* $Id: errorprint.cpp $ */

/** @file
 * MS COM / XPCOM Abstraction Layer:
 * Error info print helpers. This implements the shared code from the macros from errorprint.h.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/log.h>

#include <ProgressImpl.h>

#include <iprt/stream.h>
#include <iprt/message.h>
#include <iprt/path.h>

namespace com
{

void GluePrintErrorInfo(const com::ErrorInfo &info)
{
    bool haveResultCode = false;
#if defined (RT_OS_WIN)
    haveResultCode = info.isFullAvailable();
    bool haveComponent = true;
    bool haveInterfaceID = true;
#else /* defined (RT_OS_WIN) */
    haveResultCode = true;
    bool haveComponent = info.isFullAvailable();
    bool haveInterfaceID = info.isFullAvailable();
#endif

    Utf8Str str;
    RTCList<Utf8Str> comp;

    Bstr bstrDetailsText = info.getText();
    if (!bstrDetailsText.isEmpty())
        str = Utf8StrFmt("%ls\n",
                         bstrDetailsText.raw());
    if (haveResultCode)
        comp.append(Utf8StrFmt("code %Rhrc (0x%RX32)",
                               info.getResultCode(),
                               info.getResultCode()));
    if (haveComponent)
        comp.append(Utf8StrFmt("component %ls",
                               info.getComponent().raw()));
    if (haveInterfaceID)
        comp.append(Utf8StrFmt("interface %ls",
                               info.getInterfaceName().raw()));
    if (!info.getCalleeName().isEmpty())
        comp.append(Utf8StrFmt("callee %ls",
                               info.getCalleeName().raw()));

    if (comp.size() > 0)
    {
        str += "Details: ";
        for (size_t i = 0; i < comp.size() - 1; ++i)
            str += comp.at(i) + ", ";
        str += comp.last();
        str += "\n";
    }

    // print and log
    RTMsgError("%s", str.c_str());
    Log(("ERROR: %s", str.c_str()));
}

void GluePrintErrorContext(const char *pcszContext, const char *pcszSourceFile, uint32_t ulLine)
{
    // pcszSourceFile comes from __FILE__ macro, which always contains the full path,
    // which we don't want to see printed:
    Utf8Str strFilename(RTPathFilename(pcszSourceFile));
    Utf8Str str = Utf8StrFmt("Context: \"%s\" at line %d of file %s\n",
                             pcszContext,
                             ulLine,
                             strFilename.c_str());
    // print and log
    RTMsgError("%s", str.c_str());
    Log(("%s", str.c_str()));
}

void GluePrintRCMessage(HRESULT rc)
{
    Utf8Str str = Utf8StrFmt("Code %Rhra (extended info not available)\n", rc);
    // print and log
    RTMsgError("%s", str.c_str());
    Log(("ERROR: %s", str.c_str()));
}

static void glueHandleComErrorInternal(com::ErrorInfo &info,
                                       const char *pcszContext,
                                       HRESULT rc,
                                       const char *pcszSourceFile,
                                       uint32_t ulLine)
{
    if (info.isFullAvailable() || info.isBasicAvailable())
    {
        const com::ErrorInfo *pInfo = &info;
        do
        {
            GluePrintErrorInfo(*pInfo);

            pInfo = pInfo->getNext();
            /* If there is more than one error, separate them visually. */
            if (pInfo)
            {
                /* If there are several errors then at least basic error
                 * information must be available, otherwise something went
                 * horribly wrong. */
                Assert(pInfo->isFullAvailable() || pInfo->isBasicAvailable());

                RTMsgError("--------\n");
            }
        }
        while (pInfo);
    }
    else
        GluePrintRCMessage(rc);

    GluePrintErrorContext(pcszContext, pcszSourceFile, ulLine);
}

void GlueHandleComError(ComPtr<IUnknown> iface,
                        const char *pcszContext,
                        HRESULT rc,
                        const char *pcszSourceFile,
                        uint32_t ulLine)
{
    /* If we have full error info, print something nice, and start with the
     * actual error message. */
    com::ErrorInfo info(iface, COM_IIDOF(IUnknown));

    glueHandleComErrorInternal(info,
                               pcszContext,
                               rc,
                               pcszSourceFile,
                               ulLine);

}

void GlueHandleComErrorProgress(ComPtr<IProgress> progress,
                                const char *pcszContext,
                                HRESULT rc,
                                const char *pcszSourceFile,
                                uint32_t ulLine)
{
    /* Get the error info out of the progress object. */
    ProgressErrorInfo ei(progress);

    glueHandleComErrorInternal(ei,
                               pcszContext,
                               rc,
                               pcszSourceFile,
                               ulLine);
}

} /* namespace com */

