/* $Id: GuestDnDPrivate.cpp $ */
/** @file
 * Private guest drag and drop code, used by GuestDnDTarget + GuestDnDSource.
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

#define LOG_GROUP LOG_GROUP_GUEST_DND
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "AutoCaller.h"

#ifdef VBOX_WITH_DRAG_AND_DROP
# include "ConsoleImpl.h"
# include "ProgressImpl.h"
# include "GuestDnDPrivate.h"

# include <algorithm>

# include <iprt/dir.h>
# include <iprt/path.h>
# include <iprt/stream.h>
# include <iprt/semaphore.h>
# include <iprt/cpp/utils.h>

# include <VMMDev.h>

# include <VBox/GuestHost/DragAndDrop.h>
# include <VBox/HostServices/DragAndDropSvc.h>
# include <VBox/version.h>

/** @page pg_main_dnd  Dungeons & Dragons - Overview
 * Overview:
 *
 * Drag and Drop is handled over the internal HGCM service for the host <->
 * guest communication. Beside that we need to map the Drag and Drop protocols
 * of the various OS's we support to our internal channels, this is also highly
 * communicative in both directions. Unfortunately HGCM isn't really designed
 * for that. Next we have to foul some of the components. This includes to
 * trick X11 on the guest side, but also Qt needs to be tricked on the host
 * side a little bit.
 *
 * The following components are involved:
 *
 * 1. GUI: Uses the Qt classes for Drag and Drop and mainly forward the content
 *    of it to the Main IGuest interface (see UIDnDHandler.cpp).
 * 2. Main: Public interface for doing Drag and Drop. Also manage the IProgress
 *    interfaces for blocking the caller by showing a progress dialog (see
 *    this file).
 * 3. HGCM service: Handle all messages from the host to the guest at once and
 *    encapsulate the internal communication details (see dndmanager.cpp and
 *    friends).
 * 4. Guest additions: Split into the platform neutral part (see
 *    VBoxGuestR3LibDragAndDrop.cpp) and the guest OS specific parts.
 *    Receive/send message from/to the HGCM service and does all guest specific
 *    operations. Currently only X11 is supported (see draganddrop.cpp within
 *    VBoxClient).
 *
 * Host -> Guest:
 * 1. There are DnD Enter, Move, Leave events which are send exactly like this
 *    to the guest. The info includes the pos, mimetypes and allowed actions.
 *    The guest has to respond with an action it would accept, so the GUI could
 *    change the cursor.
 * 2. On drop, first a drop event is sent. If this is accepted a drop data
 *    event follows. This blocks the GUI and shows some progress indicator.
 *
 * Guest -> Host:
 * 1. The GUI is asking the guest if a DnD event is pending when the user moves
 *    the cursor out of the view window. If so, this returns the mimetypes and
 *    allowed actions.
 * (2. On every mouse move this is asked again, to make sure the DnD event is
 *     still valid.)
 * 3. On drop the host request the data from the guest. This blocks the GUI and
 *    shows some progress indicator.
 *
 * Some hints:
 * m_strSupportedFormats here in this file defines the allowed mime-types.
 * This is necessary because we need special handling for some of the
 * mime-types. E.g. for URI lists we need to transfer the actual dirs and
 * files. Text EOL may to be changed. Also unknown mime-types may need special
 * handling as well, which may lead to undefined behavior in the host/guest, if
 * not done.
 *
 * Dropping of a directory, means recursively transferring _all_ the content.
 *
 * Directories and files are placed into the user's temporary directory on the
 * guest (e.g. /tmp/VirtualBox Dropped Files). We can't delete them after the
 * DnD operation, because we didn't know what the DnD target does with it. E.g.
 * it could just be opened in place. This could lead ofc to filling up the disk
 * within the guest. To inform the user about this, a small app could be
 * developed which scans this directory regularly and inform the user with a
 * tray icon hint (and maybe the possibility to clean this up instantly). The
 * same has to be done in the G->H direction when it is implemented.
 *
 * Of course only regularly files are supported. Symlinks are resolved and
 * transfered as regularly files. First we don't know if the other side support
 * symlinks at all and second they could point to somewhere in a directory tree
 * which not exists on the other side.
 *
 * The code tries to preserve the file modes of the transfered dirs/files. This
 * is useful (and maybe necessary) for two things:
 * 1. If a file is executable, it should be also after the transfer, so the
 *    user can just execute it, without manually tweaking the modes first.
 * 2. If a dir/file is not accessible by group/others in the host, it shouldn't
 *    be in the guest.
 * In any case, the user mode is always set to rwx (so that we can access it
 * ourself, in e.g. for a cleanup case after cancel).
 *
 * Cancel is supported in both directions and cleans up all previous steps
 * (thats is: deleting already transfered dirs/files).
 *
 * In general I propose the following changes in the VBox HGCM infrastructure
 * for the future:
 * - Currently it isn't really possible to send messages to the guest from the
 *   host. The host informs the guest just that there is something, the guest
 *   than has to ask which message and depending on that send the appropriate
 *   message to the host, which is filled with the right data.
 * - There is no generic interface for sending bigger memory blocks to/from the
 *   guest. This is now done here, but I guess was also necessary for e.g.
 *   guest execution. So something generic which brake this up into smaller
 *   blocks and send it would be nice (with all the error handling and such
 *   ofc).
 * - I developed a "protocol" for the DnD communication here. So the host and
 *   the guest have always to match in the revision. This is ofc bad, because
 *   the additions could be outdated easily. So some generic protocol number
 *   support in HGCM for asking the host and the guest of the support version,
 *   would be nice. Ofc at least the host should be able to talk to the guest,
 *   even when the version is below the host one.
 * All this stuff would be useful for the current services, but also for future
 * onces.
 *
 ** @todo
 * - ESC doesn't really work (on Windows guests it's already implemented)
 *   ... in any case it seems a little bit difficult to handle from the Qt
 *   side. Maybe also a host specific implementation becomes necessary ...
 *   this would be really worst ofc.
 * - Add support for more mime-types (especially images, csv)
 * - Test unusual behavior:
 *   - DnD service crash in the guest during a DnD op (e.g. crash of VBoxClient or X11)
 *   - Not expected order of the events between HGCM and the guest
 * - Security considerations: We transfer a lot of memory between the guest and
 *   the host and even allow the creation of dirs/files. Maybe there should be
 *   limits introduced to preventing DOS attacks or filling up all the memory
 *   (both in the host and the guest).
 */

GuestDnDCallbackEvent::~GuestDnDCallbackEvent(void)
{
    if (NIL_RTSEMEVENT != mSemEvent)
        RTSemEventDestroy(mSemEvent);
}

int GuestDnDCallbackEvent::Reset(void)
{
    int rc = VINF_SUCCESS;

    if (NIL_RTSEMEVENT == mSemEvent)
        rc = RTSemEventCreate(&mSemEvent);

    mRc = VINF_SUCCESS;
    return rc;
}

int GuestDnDCallbackEvent::Notify(int rc /* = VINF_SUCCESS */)
{
    mRc = rc;
    return RTSemEventSignal(mSemEvent);
}

int GuestDnDCallbackEvent::Wait(RTMSINTERVAL msTimeout)
{
    return RTSemEventWait(mSemEvent, msTimeout);
}

///////////////////////////////////////////////////////////////////////////////

GuestDnDResponse::GuestDnDResponse(const ComObjPtr<Guest>& pGuest)
    : m_EventSem(NIL_RTSEMEVENT)
    , m_defAction(0)
    , m_allActions(0)
    , m_pParent(pGuest)
{
    int rc = RTSemEventCreate(&m_EventSem);
    if (RT_FAILURE(rc))
        throw rc;
}

GuestDnDResponse::~GuestDnDResponse(void)
{
    reset();

    int rc = RTSemEventDestroy(m_EventSem);
    AssertRC(rc);
}

int GuestDnDResponse::notifyAboutGuestResponse(void) const
{
    return RTSemEventSignal(m_EventSem);
}

void GuestDnDResponse::reset(void)
{
    LogFlowThisFuncEnter();

    m_defAction  = 0;
    m_allActions = 0;

    m_lstFormats.clear();
}

HRESULT GuestDnDResponse::resetProgress(const ComObjPtr<Guest>& pParent)
{
    m_pProgress.setNull();

    HRESULT hr = m_pProgress.createObject();
    if (SUCCEEDED(hr))
    {
        hr = m_pProgress->init(static_cast<IGuest *>(pParent),
                               Bstr(pParent->tr("Dropping data")).raw(),
                               TRUE /* aCancelable */);
    }

    return hr;
}

bool GuestDnDResponse::isProgressCanceled(void) const
{
    BOOL fCanceled;
    if (!m_pProgress.isNull())
    {
        HRESULT hr = m_pProgress->COMGETTER(Canceled)(&fCanceled);
        AssertComRC(hr);
    }
    else
        fCanceled = TRUE;

    return RT_BOOL(fCanceled);
}

int GuestDnDResponse::setCallback(uint32_t uMsg, PFNGUESTDNDCALLBACK pfnCallback, void *pvUser /* = NULL */)
{
    GuestDnDCallbackMap::iterator it = m_mapCallbacks.find(uMsg);

    /* Add. */
    if (pfnCallback)
    {
        if (it == m_mapCallbacks.end())
        {
            m_mapCallbacks[uMsg] = GuestDnDCallback(pfnCallback, uMsg, pvUser);
            return VINF_SUCCESS;
        }

        AssertMsgFailed(("Callback for message %RU32 already registered\n", uMsg));
        return VERR_ALREADY_EXISTS;
    }

    /* Remove. */
    if (it != m_mapCallbacks.end())
        m_mapCallbacks.erase(it);

    return VINF_SUCCESS;
}

int GuestDnDResponse::setProgress(unsigned uPercentage,
                                  uint32_t uStatus,
                                  int rcOp /* = VINF_SUCCESS */, const Utf8Str &strMsg /* = "" */)
{
    RT_NOREF(rcOp);
    LogFlowFunc(("uStatus=%RU32, uPercentage=%RU32, rcOp=%Rrc, strMsg=%s\n",
                 uStatus, uPercentage, rcOp, strMsg.c_str()));

    int rc = VINF_SUCCESS;
    if (!m_pProgress.isNull())
    {
        BOOL fCompleted;
        HRESULT hr = m_pProgress->COMGETTER(Completed)(&fCompleted);
        AssertComRC(hr);

        BOOL fCanceled;
        hr = m_pProgress->COMGETTER(Canceled)(&fCanceled);
        AssertComRC(hr);

        LogFlowFunc(("Current: fCompleted=%RTbool, fCanceled=%RTbool\n", fCompleted, fCanceled));

        if (!fCompleted)
        {
            switch (uStatus)
            {
                case DragAndDropSvc::DND_PROGRESS_ERROR:
                {
                    hr = m_pProgress->i_notifyComplete(VBOX_E_IPRT_ERROR,
                                                       COM_IIDOF(IGuest),
                                                       m_pParent->getComponentName(), strMsg.c_str());
                    reset();
                    break;
                }

                case DragAndDropSvc::DND_PROGRESS_CANCELLED:
                {
                    hr = m_pProgress->Cancel();
                    AssertComRC(hr);
                    hr = m_pProgress->i_notifyComplete(S_OK);
                    AssertComRC(hr);

                    reset();
                    break;
                }

                case DragAndDropSvc::DND_PROGRESS_RUNNING:
                case DragAndDropSvc::DND_PROGRESS_COMPLETE:
                {
                    if (!fCanceled)
                    {
                        hr = m_pProgress->SetCurrentOperationProgress(uPercentage);
                        AssertComRC(hr);
                        if (   uStatus     == DragAndDropSvc::DND_PROGRESS_COMPLETE
                            || uPercentage >= 100)
                        {
                            hr = m_pProgress->i_notifyComplete(S_OK);
                            AssertComRC(hr);
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        }

        hr = m_pProgress->COMGETTER(Completed)(&fCompleted);
        AssertComRC(hr);
        hr = m_pProgress->COMGETTER(Canceled)(&fCanceled);
        AssertComRC(hr);

        LogFlowFunc(("New: fCompleted=%RTbool, fCanceled=%RTbool\n", fCompleted, fCanceled));
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDResponse::onDispatch(uint32_t u32Function, void *pvParms, uint32_t cbParms)
{
    LogFlowFunc(("u32Function=%RU32, pvParms=%p, cbParms=%RU32\n", u32Function, pvParms, cbParms));

    int rc = VERR_WRONG_ORDER; /* Play safe. */
    bool fTryCallbacks = false;

    switch (u32Function)
    {
        case DragAndDropSvc::GUEST_DND_CONNECT:
        {
            LogThisFunc(("Client connected\n"));

            /* Nothing to do here (yet). */
            rc = VINF_SUCCESS;
            break;
        }

        case DragAndDropSvc::GUEST_DND_DISCONNECT:
        {
            LogThisFunc(("Client disconnected\n"));
            rc = setProgress(100, DND_PROGRESS_CANCELLED, VINF_SUCCESS);
            break;
        }

        case DragAndDropSvc::GUEST_DND_HG_ACK_OP:
        {
            DragAndDropSvc::PVBOXDNDCBHGACKOPDATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGACKOPDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGACKOPDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_ACK_OP == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            setDefAction(pCBData->uAction);
            rc = notifyAboutGuestResponse();
            break;
        }

        case DragAndDropSvc::GUEST_DND_HG_REQ_DATA:
        {
            DragAndDropSvc::PVBOXDNDCBHGREQDATADATA pCBData = reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGREQDATADATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGREQDATADATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_REQ_DATA == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            if (   pCBData->cbFormat  == 0
                || pCBData->cbFormat  > _64K /** @todo Make this configurable? */
                || pCBData->pszFormat == NULL)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (!RTStrIsValidEncoding(pCBData->pszFormat))
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                setFormats(GuestDnD::toFormatList(pCBData->pszFormat));
                rc = VINF_SUCCESS;
            }

            int rc2 = notifyAboutGuestResponse();
            if (RT_SUCCESS(rc))
                rc = rc2;
            break;
        }

        case DragAndDropSvc::GUEST_DND_HG_EVT_PROGRESS:
        {
            DragAndDropSvc::PVBOXDNDCBHGEVTPROGRESSDATA pCBData =
               reinterpret_cast<DragAndDropSvc::PVBOXDNDCBHGEVTPROGRESSDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBHGEVTPROGRESSDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_HG_EVT_PROGRESS == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = setProgress(pCBData->uPercentage, pCBData->uStatus, pCBData->rc);
            if (RT_SUCCESS(rc))
                rc = notifyAboutGuestResponse();
            break;
        }
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        case DragAndDropSvc::GUEST_DND_GH_ACK_PENDING:
        {
            DragAndDropSvc::PVBOXDNDCBGHACKPENDINGDATA pCBData =
               reinterpret_cast<DragAndDropSvc::PVBOXDNDCBGHACKPENDINGDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(DragAndDropSvc::VBOXDNDCBGHACKPENDINGDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(DragAndDropSvc::CB_MAGIC_DND_GH_ACK_PENDING == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            if (   pCBData->cbFormat  == 0
                || pCBData->cbFormat  > _64K /** @todo Make the maximum size configurable? */
                || pCBData->pszFormat == NULL)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (!RTStrIsValidEncoding(pCBData->pszFormat))
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                setFormats   (GuestDnD::toFormatList(pCBData->pszFormat));
                setDefAction (pCBData->uDefAction);
                setAllActions(pCBData->uAllActions);

                rc = VINF_SUCCESS;
            }

            int rc2 = notifyAboutGuestResponse();
            if (RT_SUCCESS(rc))
                rc = rc2;
            break;
        }
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */
        default:
            /* * Try if the event is covered by a registered callback. */
            fTryCallbacks = true;
            break;
    }

    /*
     * Try the host's installed callbacks (if any).
     */
    if (fTryCallbacks)
    {
        GuestDnDCallbackMap::const_iterator it = m_mapCallbacks.find(u32Function);
        if (it != m_mapCallbacks.end())
        {
            AssertPtr(it->second.pfnCallback);
            rc = it->second.pfnCallback(u32Function, pvParms, cbParms, it->second.pvUser);
        }
        else
        {
            LogFlowFunc(("No callback for function %RU32 defined (%zu callbacks total)\n", u32Function, m_mapCallbacks.size()));
            rc = VERR_NOT_SUPPORTED; /* Tell the guest. */
        }
    }

    LogFlowFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

HRESULT GuestDnDResponse::queryProgressTo(IProgress **ppProgress)
{
    return m_pProgress.queryInterfaceTo(ppProgress);
}

int GuestDnDResponse::waitForGuestResponse(RTMSINTERVAL msTimeout /*= 500 */) const
{
    int rc = RTSemEventWait(m_EventSem, msTimeout);
#ifdef DEBUG_andy
    LogFlowFunc(("msTimeout=%RU32, rc=%Rrc\n", msTimeout, rc));
#endif
    return rc;
}

///////////////////////////////////////////////////////////////////////////////

GuestDnD* GuestDnD::s_pInstance = NULL;

GuestDnD::GuestDnD(const ComObjPtr<Guest> &pGuest)
    : m_pGuest(pGuest)
{
    LogFlowFuncEnter();

    m_pResponse = new GuestDnDResponse(pGuest);

    /* List of supported default MIME types. */
    LogRel2(("DnD: Supported default host formats:\n"));
    const com::Utf8Str arrEntries[] = { VBOX_DND_FORMATS_DEFAULT };
    for (size_t i = 0; i < RT_ELEMENTS(arrEntries); i++)
    {
        m_strDefaultFormats.push_back(arrEntries[i]);
        LogRel2(("DnD: \t%s\n", arrEntries[i].c_str()));
    }
}

GuestDnD::~GuestDnD(void)
{
    LogFlowFuncEnter();

    if (m_pResponse)
        delete m_pResponse;
}

HRESULT GuestDnD::adjustScreenCoordinates(ULONG uScreenId, ULONG *puX, ULONG *puY) const
{
    /** @todo r=andy Save the current screen's shifting coordinates to speed things up.
     *               Only query for new offsets when the screen ID has changed. */

    /* For multi-monitor support we need to add shift values to the coordinates
     * (depending on the screen number). */
    ComObjPtr<Console> pConsole = m_pGuest->i_getConsole();
    ComPtr<IDisplay> pDisplay;
    HRESULT hr = pConsole->COMGETTER(Display)(pDisplay.asOutParam());
    if (FAILED(hr))
        return hr;

    ULONG dummy;
    LONG xShift, yShift;
    GuestMonitorStatus_T monitorStatus;
    hr = pDisplay->GetScreenResolution(uScreenId, &dummy, &dummy, &dummy,
                                       &xShift, &yShift, &monitorStatus);
    if (FAILED(hr))
        return hr;

    if (puX)
        *puX += xShift;
    if (puY)
        *puY += yShift;

    LogFlowFunc(("uScreenId=%RU32, x=%RU32, y=%RU32\n", uScreenId, puX ? *puX : 0, puY ? *puY : 0));
    return S_OK;
}

int GuestDnD::hostCall(uint32_t u32Function, uint32_t cParms, PVBOXHGCMSVCPARM paParms) const
{
    Assert(!m_pGuest.isNull());
    ComObjPtr<Console> pConsole = m_pGuest->i_getConsole();

    /* Forward the information to the VMM device. */
    Assert(!pConsole.isNull());
    VMMDev *pVMMDev = pConsole->i_getVMMDev();
    if (!pVMMDev)
        return VERR_COM_OBJECT_NOT_FOUND;

    return pVMMDev->hgcmHostCall("VBoxDragAndDropSvc", u32Function, cParms, paParms);
}

/* static */
DECLCALLBACK(int) GuestDnD::notifyDnDDispatcher(void *pvExtension, uint32_t u32Function,
                                                void *pvParms, uint32_t cbParms)
{
    LogFlowFunc(("pvExtension=%p, u32Function=%RU32, pvParms=%p, cbParms=%RU32\n",
                 pvExtension, u32Function, pvParms, cbParms));

    GuestDnD *pGuestDnD = reinterpret_cast<GuestDnD*>(pvExtension);
    AssertPtrReturn(pGuestDnD, VERR_INVALID_POINTER);

    /** @todo In case we need to handle multiple guest DnD responses at a time this
     *        would be the place to lookup and dispatch to those. For the moment we
     *        only have one response -- simple. */
    GuestDnDResponse *pResp = pGuestDnD->m_pResponse;
    if (pResp)
        return pResp->onDispatch(u32Function, pvParms, cbParms);

    return VERR_NOT_SUPPORTED;
}

/* static */
bool GuestDnD::isFormatInFormatList(const com::Utf8Str &strFormat, const GuestDnDMIMEList &lstFormats)
{
    return std::find(lstFormats.begin(), lstFormats.end(), strFormat) != lstFormats.end();
}

/* static */
GuestDnDMIMEList GuestDnD::toFormatList(const com::Utf8Str &strFormats)
{
    GuestDnDMIMEList lstFormats;
    RTCList<RTCString> lstFormatsTmp = strFormats.split("\r\n");

    for (size_t i = 0; i < lstFormatsTmp.size(); i++)
        lstFormats.push_back(com::Utf8Str(lstFormatsTmp.at(i)));

    return lstFormats;
}

/* static */
com::Utf8Str GuestDnD::toFormatString(const GuestDnDMIMEList &lstFormats)
{
    com::Utf8Str strFormat;
    for (size_t i = 0; i < lstFormats.size(); i++)
    {
        const com::Utf8Str &f = lstFormats.at(i);
        strFormat += f + "\r\n";
    }

    return strFormat;
}

/* static */
GuestDnDMIMEList GuestDnD::toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const GuestDnDMIMEList &lstFormatsWanted)
{
    GuestDnDMIMEList lstFormats;

    for (size_t i = 0; i < lstFormatsWanted.size(); i++)
    {
        /* Only keep supported format types. */
        if (std::find(lstFormatsSupported.begin(),
                      lstFormatsSupported.end(), lstFormatsWanted.at(i)) != lstFormatsSupported.end())
        {
            lstFormats.push_back(lstFormatsWanted[i]);
        }
    }

    return lstFormats;
}

/* static */
GuestDnDMIMEList GuestDnD::toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const com::Utf8Str &strFormatsWanted)
{
    GuestDnDMIMEList lstFmt;

    RTCList<RTCString> lstFormats = strFormatsWanted.split("\r\n");
    size_t i = 0;
    while (i < lstFormats.size())
    {
        /* Only keep allowed format types. */
        if (std::find(lstFormatsSupported.begin(),
                      lstFormatsSupported.end(), lstFormats.at(i)) != lstFormatsSupported.end())
        {
            lstFmt.push_back(lstFormats[i]);
        }
        i++;
    }

    return lstFmt;
}

/* static */
uint32_t GuestDnD::toHGCMAction(DnDAction_T enmAction)
{
    uint32_t uAction = DND_IGNORE_ACTION;
    switch (enmAction)
    {
        case DnDAction_Copy:
            uAction = DND_COPY_ACTION;
            break;
        case DnDAction_Move:
            uAction = DND_MOVE_ACTION;
            break;
        case DnDAction_Link:
            /* For now it doesn't seems useful to allow a link
               action between host & guest. Later? */
        case DnDAction_Ignore:
            /* Ignored. */
            break;
        default:
            AssertMsgFailed(("Action %RU32 not recognized!\n", enmAction));
            break;
    }

    return uAction;
}

/* static */
void GuestDnD::toHGCMActions(DnDAction_T                    enmDefAction,
                             uint32_t                      *puDefAction,
                             const std::vector<DnDAction_T> vecAllowedActions,
                             uint32_t                      *puAllowedActions)
{
    uint32_t uAllowedActions = DND_IGNORE_ACTION;
    uint32_t uDefAction      = toHGCMAction(enmDefAction);

    if (!vecAllowedActions.empty())
    {
        /* First convert the allowed actions to a bit array. */
        for (size_t i = 0; i < vecAllowedActions.size(); i++)
            uAllowedActions |= toHGCMAction(vecAllowedActions[i]);

        /*
         * If no default action is set (ignoring), try one of the
         * set allowed actions, preferring copy, move (in that order).
         */
        if (isDnDIgnoreAction(uDefAction))
        {
            if (hasDnDCopyAction(uAllowedActions))
                uDefAction = DND_COPY_ACTION;
            else if (hasDnDMoveAction(uAllowedActions))
                uDefAction = DND_MOVE_ACTION;
        }
    }

    if (puDefAction)
        *puDefAction      = uDefAction;
    if (puAllowedActions)
        *puAllowedActions = uAllowedActions;
}

/* static */
DnDAction_T GuestDnD::toMainAction(uint32_t uAction)
{
    /* For now it doesn't seems useful to allow a
     * link action between host & guest. Maybe later! */
    return (isDnDCopyAction(uAction) ? (DnDAction_T)DnDAction_Copy :
            isDnDMoveAction(uAction) ? (DnDAction_T)DnDAction_Move :
            (DnDAction_T)DnDAction_Ignore);
}

/* static */
std::vector<DnDAction_T> GuestDnD::toMainActions(uint32_t uActions)
{
    std::vector<DnDAction_T> vecActions;

    /* For now it doesn't seems useful to allow a
     * link action between host & guest. Maybe later! */
    RTCList<DnDAction_T> lstActions;
    if (hasDnDCopyAction(uActions))
        lstActions.append(DnDAction_Copy);
    if (hasDnDMoveAction(uActions))
        lstActions.append(DnDAction_Move);

    for (size_t i = 0; i < lstActions.size(); ++i)
        vecActions.push_back(lstActions.at(i));

    return vecActions;
}

///////////////////////////////////////////////////////////////////////////////

GuestDnDBase::GuestDnDBase(void)
{
    /* Initialize public attributes. */
    m_lstFmtSupported = GuestDnDInst()->defaultFormats();

    /* Initialzie private stuff. */
    mDataBase.m_cTransfersPending = 0;
    mDataBase.m_uProtocolVersion  = 0;
}

HRESULT GuestDnDBase::i_isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported)
{
    *aSupported = std::find(m_lstFmtSupported.begin(),
                            m_lstFmtSupported.end(), aFormat) != m_lstFmtSupported.end()
                ? TRUE : FALSE;
    return S_OK;
}

HRESULT GuestDnDBase::i_getFormats(GuestDnDMIMEList &aFormats)
{
    aFormats = m_lstFmtSupported;

    return S_OK;
}

HRESULT GuestDnDBase::i_addFormats(const GuestDnDMIMEList &aFormats)
{
    for (size_t i = 0; i < aFormats.size(); ++i)
    {
        Utf8Str strFormat = aFormats.at(i);
        if (std::find(m_lstFmtSupported.begin(),
                      m_lstFmtSupported.end(), strFormat) == m_lstFmtSupported.end())
        {
            m_lstFmtSupported.push_back(strFormat);
        }
    }

    return S_OK;
}

HRESULT GuestDnDBase::i_removeFormats(const GuestDnDMIMEList &aFormats)
{
    for (size_t i = 0; i < aFormats.size(); ++i)
    {
        Utf8Str strFormat = aFormats.at(i);
        GuestDnDMIMEList::iterator itFormat = std::find(m_lstFmtSupported.begin(),
                                                        m_lstFmtSupported.end(), strFormat);
        if (itFormat != m_lstFmtSupported.end())
            m_lstFmtSupported.erase(itFormat);
    }

    return S_OK;
}

HRESULT GuestDnDBase::i_getProtocolVersion(ULONG *puVersion)
{
    int rc = getProtocolVersion((uint32_t *)puVersion);
    return RT_SUCCESS(rc) ? S_OK : E_FAIL;
}

/**
 * Tries to guess the DnD protocol version to use on the guest, based on the
 * installed Guest Additions version + revision.
 *
 * If unable to retrieve the protocol version, VERR_NOT_FOUND is returned along
 * with protocol version 1.
 *
 * @return  IPRT status code.
 * @param   puProto                 Where to store the protocol version.
 */
int GuestDnDBase::getProtocolVersion(uint32_t *puProto)
{
    AssertPtrReturn(puProto, VERR_INVALID_POINTER);

    int rc;

    uint32_t uProto = 0;
    uint32_t uVerAdditions;
    uint32_t uRevAdditions;
    if (   m_pGuest
        && (uVerAdditions = m_pGuest->i_getAdditionsVersion())  > 0
        && (uRevAdditions = m_pGuest->i_getAdditionsRevision()) > 0)
    {
#if 0 && defined(DEBUG)
        /* Hardcode the to-used protocol version; nice for testing side effects. */
        if (true)
            uProto = 3;
        else
#endif
        if (uVerAdditions >= VBOX_FULL_VERSION_MAKE(5, 0, 0))
        {
/** @todo
 *  r=bird: This is just too bad for anyone using an OSE additions build...
 */
            if (uRevAdditions >= 103344) /* Since r103344: Protocol v3. */
                uProto = 3;
            else
                uProto = 2; /* VBox 5.0.0 - 5.0.8: Protocol v2. */
        }
        /* else: uProto: 0 */

        LogFlowFunc(("uVerAdditions=%RU32 (%RU32.%RU32.%RU32), r%RU32\n",
                     uVerAdditions, VBOX_FULL_VERSION_GET_MAJOR(uVerAdditions), VBOX_FULL_VERSION_GET_MINOR(uVerAdditions),
                                    VBOX_FULL_VERSION_GET_BUILD(uVerAdditions), uRevAdditions));
        rc = VINF_SUCCESS;
    }
    else
    {
        uProto = 1; /* Fallback. */
        rc = VERR_NOT_FOUND;
    }

    LogRel2(("DnD: Guest is using protocol v%RU32, rc=%Rrc\n", uProto, rc));

    *puProto = uProto;
    return rc;
}

int GuestDnDBase::msgQueueAdd(GuestDnDMsg *pMsg)
{
    mDataBase.m_lstMsgOut.push_back(pMsg);
    return VINF_SUCCESS;
}

GuestDnDMsg *GuestDnDBase::msgQueueGetNext(void)
{
    if (mDataBase.m_lstMsgOut.empty())
        return NULL;
    return mDataBase.m_lstMsgOut.front();
}

void GuestDnDBase::msgQueueRemoveNext(void)
{
    if (!mDataBase.m_lstMsgOut.empty())
    {
        GuestDnDMsg *pMsg = mDataBase.m_lstMsgOut.front();
        if (pMsg)
            delete pMsg;
        mDataBase.m_lstMsgOut.pop_front();
    }
}

void GuestDnDBase::msgQueueClear(void)
{
    LogFlowFunc(("cMsg=%zu\n", mDataBase.m_lstMsgOut.size()));

    GuestDnDMsgList::iterator itMsg = mDataBase.m_lstMsgOut.begin();
    while (itMsg != mDataBase.m_lstMsgOut.end())
    {
        GuestDnDMsg *pMsg = *itMsg;
        if (pMsg)
            delete pMsg;

        itMsg++;
    }

    mDataBase.m_lstMsgOut.clear();
}

int GuestDnDBase::sendCancel(void)
{
    int rc = GuestDnDInst()->hostCall(HOST_DND_HG_EVT_CANCEL,
                                      0 /* cParms */, NULL /* paParms */);

    LogFlowFunc(("Generated cancelling request, rc=%Rrc\n", rc));
    return rc;
}

int GuestDnDBase::updateProgress(GuestDnDData *pData, GuestDnDResponse *pResp,
                                 uint32_t cbDataAdd /* = 0 */)
{
    AssertPtrReturn(pData, VERR_INVALID_POINTER);
    AssertPtrReturn(pResp, VERR_INVALID_POINTER);
    /* cbDataAdd is optional. */

    LogFlowFunc(("cbTotal=%RU64, cbProcessed=%RU64, cbRemaining=%RU64, cbDataAdd=%RU32\n",
                 pData->getTotal(), pData->getProcessed(), pData->getRemaining(), cbDataAdd));

    if (!pResp)
        return VINF_SUCCESS;

    if (cbDataAdd)
        pData->addProcessed(cbDataAdd);

    int rc = pResp->setProgress(pData->getPercentComplete(),
                                  pData->isComplete()
                                ? DND_PROGRESS_COMPLETE
                                : DND_PROGRESS_RUNNING);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @todo GuestDnDResponse *pResp needs to go. */
int GuestDnDBase::waitForEvent(GuestDnDCallbackEvent *pEvent, GuestDnDResponse *pResp, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    AssertPtrReturn(pResp, VERR_INVALID_POINTER);

    int rc;

    LogFlowFunc(("msTimeout=%RU32\n", msTimeout));

    uint64_t tsStart = RTTimeMilliTS();
    do
    {
        /*
         * Wait until our desired callback triggered the
         * wait event. As we don't want to block if the guest does not
         * respond, do busy waiting here.
         */
        rc = pEvent->Wait(500 /* ms */);
        if (RT_SUCCESS(rc))
        {
            rc = pEvent->Result();
            LogFlowFunc(("Callback done, result is %Rrc\n", rc));
            break;
        }
        else if (rc == VERR_TIMEOUT) /* Continue waiting. */
            rc = VINF_SUCCESS;

        if (   msTimeout != RT_INDEFINITE_WAIT
            && RTTimeMilliTS() - tsStart > msTimeout)
        {
            rc = VERR_TIMEOUT;
            LogRel2(("DnD: Error: Guest did not respond within time\n"));
        }
        else if (pResp->isProgressCanceled()) /** @todo GuestDnDResponse *pResp needs to go. */
        {
            LogRel2(("DnD: Operation was canceled by user\n"));
            rc = VERR_CANCELLED;
        }

    } while (RT_SUCCESS(rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP */

