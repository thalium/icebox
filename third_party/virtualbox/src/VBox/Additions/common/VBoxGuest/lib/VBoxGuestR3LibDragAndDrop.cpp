/* $Id: VBoxGuestR3LibDragAndDrop.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Drag & Drop.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/path.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/uri.h>
#include <iprt/thread.h>

#include <iprt/cpp/list.h>
#include <iprt/cpp/ministring.h>

#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

#include <VBox/VBoxGuestLib.h>
#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/DragAndDropSvc.h>

using namespace DragAndDropSvc;

#include "VBoxGuestR3LibInternal.h"


/*********************************************************************************************************************************
*   Private internal functions                                                                                                   *
*********************************************************************************************************************************/

static int vbglR3DnDGetNextMsgType(PVBGLR3GUESTDNDCMDCTX pCtx, uint32_t *puMsg, uint32_t *pcParms, bool fWait)
{
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(puMsg,   VERR_INVALID_POINTER);
    AssertPtrReturn(pcParms, VERR_INVALID_POINTER);

    VBOXDNDNEXTMSGMSG Msg;
    RT_ZERO(Msg);
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GET_NEXT_HOST_MSG, 3);
    Msg.uMsg.SetUInt32(0);
    Msg.cParms.SetUInt32(0);
    Msg.fBlock.SetUInt32(fWait ? 1 : 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        rc = Msg.uMsg.GetUInt32(puMsg);     AssertRC(rc);
        rc = Msg.cParms.GetUInt32(pcParms); AssertRC(rc);
    }

    return rc;
}

/** @todo r=andy Clean up the parameter list. */
static int vbglR3DnDHGRecvAction(PVBGLR3GUESTDNDCMDCTX pCtx,
                                 uint32_t  uMsg,
                                 uint32_t *puScreenId,
                                 uint32_t *puX,
                                 uint32_t *puY,
                                 uint32_t *puDefAction,
                                 uint32_t *puAllActions,
                                 char     *pszFormats,
                                 uint32_t  cbFormats,
                                 uint32_t *pcbFormatsRecv)
{
    AssertPtrReturn(pCtx,           VERR_INVALID_POINTER);
    AssertPtrReturn(puScreenId,     VERR_INVALID_POINTER);
    AssertPtrReturn(puX,            VERR_INVALID_POINTER);
    AssertPtrReturn(puY,            VERR_INVALID_POINTER);
    AssertPtrReturn(puDefAction,    VERR_INVALID_POINTER);
    AssertPtrReturn(puAllActions,   VERR_INVALID_POINTER);
    AssertPtrReturn(pszFormats,     VERR_INVALID_POINTER);
    AssertReturn(cbFormats,         VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbFormatsRecv, VERR_INVALID_POINTER);

    VBOXDNDHGACTIONMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, uMsg, 7);
        Msg.u.v1.uScreenId.SetUInt32(0);
        Msg.u.v1.uX.SetUInt32(0);
        Msg.u.v1.uY.SetUInt32(0);
        Msg.u.v1.uDefAction.SetUInt32(0);
        Msg.u.v1.uAllActions.SetUInt32(0);
        Msg.u.v1.pvFormats.SetPtr(pszFormats, cbFormats);
        Msg.u.v1.cbFormats.SetUInt32(0);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, uMsg, 8);
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.uScreenId.SetUInt32(0);
        Msg.u.v3.uX.SetUInt32(0);
        Msg.u.v3.uY.SetUInt32(0);
        Msg.u.v3.uDefAction.SetUInt32(0);
        Msg.u.v3.uAllActions.SetUInt32(0);
        Msg.u.v3.pvFormats.SetPtr(pszFormats, cbFormats);
        Msg.u.v3.cbFormats.SetUInt32(0);
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        if (pCtx->uProtocol < 3)
        {
            rc = Msg.u.v1.uScreenId.GetUInt32(puScreenId);     AssertRC(rc);
            rc = Msg.u.v1.uX.GetUInt32(puX);                   AssertRC(rc);
            rc = Msg.u.v1.uY.GetUInt32(puY);                   AssertRC(rc);
            rc = Msg.u.v1.uDefAction.GetUInt32(puDefAction);   AssertRC(rc);
            rc = Msg.u.v1.uAllActions.GetUInt32(puAllActions); AssertRC(rc);
            rc = Msg.u.v1.cbFormats.GetUInt32(pcbFormatsRecv); AssertRC(rc);
        }
        else
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v3.uScreenId.GetUInt32(puScreenId);     AssertRC(rc);
            rc = Msg.u.v3.uX.GetUInt32(puX);                   AssertRC(rc);
            rc = Msg.u.v3.uY.GetUInt32(puY);                   AssertRC(rc);
            rc = Msg.u.v3.uDefAction.GetUInt32(puDefAction);   AssertRC(rc);
            rc = Msg.u.v3.uAllActions.GetUInt32(puAllActions); AssertRC(rc);
            rc = Msg.u.v3.cbFormats.GetUInt32(pcbFormatsRecv); AssertRC(rc);
        }

        AssertReturn(cbFormats >= *pcbFormatsRecv, VERR_TOO_MUCH_DATA);
    }

    return rc;
}

static int vbglR3DnDHGRecvLeave(PVBGLR3GUESTDNDCMDCTX pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    VBOXDNDHGLEAVEMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_EVT_LEAVE, 0);
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_EVT_LEAVE, 1);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

static int vbglR3DnDHGRecvCancel(PVBGLR3GUESTDNDCMDCTX pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    VBOXDNDHGCANCELMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_EVT_CANCEL, 0);
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_EVT_CANCEL, 1);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

static int vbglR3DnDHGRecvDir(PVBGLR3GUESTDNDCMDCTX pCtx,
                              char     *pszDirname,
                              uint32_t  cbDirname,
                              uint32_t *pcbDirnameRecv,
                              uint32_t *pfMode)
{
    AssertPtrReturn(pCtx,           VERR_INVALID_POINTER);
    AssertPtrReturn(pszDirname,     VERR_INVALID_POINTER);
    AssertReturn(cbDirname,         VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbDirnameRecv, VERR_INVALID_POINTER);
    AssertPtrReturn(pfMode,         VERR_INVALID_POINTER);

    VBOXDNDHGSENDDIRMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_DIR, 3);
        Msg.u.v1.pvName.SetPtr(pszDirname, cbDirname);
        Msg.u.v1.cbName.SetUInt32(cbDirname);
        Msg.u.v1.fMode.SetUInt32(0);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_DIR, 4);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvName.SetPtr(pszDirname, cbDirname);
        Msg.u.v3.cbName.SetUInt32(cbDirname);
        Msg.u.v3.fMode.SetUInt32(0);
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        if (pCtx->uProtocol < 3)
        {
            rc = Msg.u.v1.cbName.GetUInt32(pcbDirnameRecv); AssertRC(rc);
            rc = Msg.u.v1.fMode.GetUInt32(pfMode);          AssertRC(rc);
        }
        else
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v3.cbName.GetUInt32(pcbDirnameRecv); AssertRC(rc);
            rc = Msg.u.v3.fMode.GetUInt32(pfMode);          AssertRC(rc);
        }

        AssertReturn(cbDirname >= *pcbDirnameRecv, VERR_TOO_MUCH_DATA);
    }

    return rc;
}

static int vbglR3DnDHGRecvFileData(PVBGLR3GUESTDNDCMDCTX pCtx,
                                   char                 *pszFilename,
                                   uint32_t              cbFilename,
                                   uint32_t             *pcbFilenameRecv,
                                   void                 *pvData,
                                   uint32_t              cbData,
                                   uint32_t             *pcbDataRecv,
                                   uint32_t             *pfMode)
{
    AssertPtrReturn(pCtx,            VERR_INVALID_POINTER);
    AssertPtrReturn(pszFilename,     VERR_INVALID_POINTER);
    AssertReturn(cbFilename,         VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbFilenameRecv, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,          VERR_INVALID_POINTER);
    AssertReturn(cbData,             VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbDataRecv,     VERR_INVALID_POINTER);
    AssertPtrReturn(pfMode,          VERR_INVALID_POINTER);

    VBOXDNDHGSENDFILEDATAMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol <= 1)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_FILE_DATA, 5);
        Msg.u.v1.pvName.SetPtr(pszFilename, cbFilename);
        Msg.u.v1.cbName.SetUInt32(0);
        Msg.u.v1.pvData.SetPtr(pvData, cbData);
        Msg.u.v1.cbData.SetUInt32(0);
        Msg.u.v1.fMode.SetUInt32(0);
    }
    else if (pCtx->uProtocol == 2)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_FILE_DATA, 3);
        Msg.u.v2.uContext.SetUInt32(0);
        Msg.u.v2.pvData.SetPtr(pvData, cbData);
        Msg.u.v2.cbData.SetUInt32(cbData);
    }
    else if (pCtx->uProtocol >= 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_FILE_DATA, 5);
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvData.SetPtr(pvData, cbData);
        Msg.u.v3.cbData.SetUInt32(0);
        Msg.u.v3.pvChecksum.SetPtr(NULL, 0);
        Msg.u.v3.cbChecksum.SetUInt32(0);
    }
    else
        AssertMsgFailed(("Protocol %RU32 not implemented\n", pCtx->uProtocol));

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        if (pCtx->uProtocol <= 1)
        {
            rc = Msg.u.v1.cbName.GetUInt32(pcbFilenameRecv); AssertRC(rc);
            rc = Msg.u.v1.cbData.GetUInt32(pcbDataRecv);     AssertRC(rc);
            rc = Msg.u.v1.fMode.GetUInt32(pfMode);           AssertRC(rc);

            AssertReturn(cbFilename >= *pcbFilenameRecv, VERR_TOO_MUCH_DATA);
            AssertReturn(cbData     >= *pcbDataRecv,     VERR_TOO_MUCH_DATA);
        }
        else if (pCtx->uProtocol == 2)
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v2.cbData.GetUInt32(pcbDataRecv);     AssertRC(rc);
            AssertReturn(cbData     >= *pcbDataRecv,     VERR_TOO_MUCH_DATA);
        }
        else if (pCtx->uProtocol >= 3)
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v3.cbData.GetUInt32(pcbDataRecv);     AssertRC(rc);
            AssertReturn(cbData     >= *pcbDataRecv,     VERR_TOO_MUCH_DATA);
            /** @todo Add checksum support. */
        }
        else
            AssertMsgFailed(("Protocol %RU32 not implemented\n", pCtx->uProtocol));
    }

    return rc;
}

static int vbglR3DnDHGRecvFileHdr(PVBGLR3GUESTDNDCMDCTX  pCtx,
                                  char                  *pszFilename,
                                  uint32_t               cbFilename,
                                  uint32_t              *puFlags,
                                  uint32_t              *pfMode,
                                  uint64_t              *pcbTotal)
{
    AssertPtrReturn(pCtx,        VERR_INVALID_POINTER);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertReturn(cbFilename,     VERR_INVALID_PARAMETER);
    AssertPtrReturn(puFlags,     VERR_INVALID_POINTER);
    AssertPtrReturn(pfMode,      VERR_INVALID_POINTER);
    AssertReturn(pcbTotal,       VERR_INVALID_POINTER);

    VBOXDNDHGSENDFILEHDRMSG Msg;
    RT_ZERO(Msg);
    int rc;
    if (pCtx->uProtocol <= 1)
        rc = VERR_NOT_SUPPORTED;
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_FILE_HDR, 6);
        Msg.uContext.SetUInt32(0); /** @todo Not used yet. */
        Msg.pvName.SetPtr(pszFilename, cbFilename);
        Msg.cbName.SetUInt32(cbFilename);
        Msg.uFlags.SetUInt32(0);
        Msg.fMode.SetUInt32(0);
        Msg.cbTotal.SetUInt64(0);

        rc = VINF_SUCCESS;
    }

    if (RT_SUCCESS(rc))
        rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        /** @todo Get context ID. */
        rc = Msg.uFlags.GetUInt32(puFlags);   AssertRC(rc);
        rc = Msg.fMode.GetUInt32(pfMode);     AssertRC(rc);
        rc = Msg.cbTotal.GetUInt64(pcbTotal); AssertRC(rc);
    }

    return rc;
}

static int vbglR3DnDHGRecvURIData(PVBGLR3GUESTDNDCMDCTX pCtx, PVBOXDNDSNDDATAHDR pDataHdr, DnDDroppedFiles *pDroppedFiles)
{
    AssertPtrReturn(pCtx,          VERR_INVALID_POINTER);
    AssertPtrReturn(pDataHdr,      VERR_INVALID_POINTER);
    AssertPtrReturn(pDroppedFiles, VERR_INVALID_POINTER);

    /* Only count the raw data minus the already received meta data. */
    Assert(pDataHdr->cbTotal >= pDataHdr->cbMeta);
    uint64_t cbToRecvBytes = pDataHdr->cbTotal - pDataHdr->cbMeta;
    uint64_t cToRecvObjs   = pDataHdr->cObjects;

    LogFlowFunc(("cbToRecvBytes=%RU64, cToRecvObjs=%RU64, (cbTotal=%RU64, cbMeta=%RU32)\n",
                 cbToRecvBytes, cToRecvObjs, pDataHdr->cbTotal, pDataHdr->cbMeta));

    /*
     * Only do accounting for protocol v3 and up.
     * The older protocols did not have any data accounting available, so
     * we simply tried to receive as much data as available and bail out.
     */
    const bool fDoAccounting = pCtx->uProtocol >= 3;

    /* Anything to do at all? */
    if (fDoAccounting)
    {
        if (   !cbToRecvBytes
            && !cToRecvObjs)
        {
            return VINF_SUCCESS;
        }
    }

    /*
     * Allocate temporary chunk buffer.
     */
    uint32_t cbChunkMax = pCtx->cbMaxChunkSize;
    void *pvChunk = RTMemAlloc(cbChunkMax);
    if (!pvChunk)
        return VERR_NO_MEMORY;
    uint32_t cbChunkRead   = 0;

    uint64_t cbFileSize    = 0; /* Total file size (in bytes). */
    uint64_t cbFileWritten = 0; /* Written bytes. */

    /*
     * Create and query the (unique) drop target directory in the user's temporary directory.
     */
    int rc = pDroppedFiles->OpenTemp(0 /* fFlags */);
    if (RT_FAILURE(rc))
    {
        RTMemFree(pvChunk);
        return VERR_NO_MEMORY;
    }

    const char *pszDropDir = pDroppedFiles->GetDirAbs();
    AssertPtr(pszDropDir);

    /*
     * Enter the main loop of retieving files + directories.
     */
    DnDURIObject objFile(DnDURIObject::File);

    char szPathName[RTPATH_MAX] = { 0 };
    uint32_t cbPathName = 0;
    uint32_t fFlags     = 0;
    uint32_t fMode      = 0;

    /*
     * Only wait for new incoming commands for protocol v3 and up.
     * The older protocols did not have any data accounting available, so
     * we simply tried to receive as much data as available and bail out.
     */
    const bool fWait = pCtx->uProtocol >= 3;

    do
    {
        LogFlowFunc(("Wating for new message ...\n"));

        uint32_t uNextMsg;
        uint32_t cNextParms;
        rc = vbglR3DnDGetNextMsgType(pCtx, &uNextMsg, &cNextParms, fWait);
        if (RT_SUCCESS(rc))
        {
            LogFlowFunc(("uNextMsg=%RU32, cNextParms=%RU32\n", uNextMsg, cNextParms));

            switch (uNextMsg)
            {
                case HOST_DND_HG_SND_DIR:
                {
                    rc = vbglR3DnDHGRecvDir(pCtx,
                                            szPathName,
                                            sizeof(szPathName),
                                            &cbPathName,
                                            &fMode);
                    LogFlowFunc(("HOST_DND_HG_SND_DIR pszPathName=%s, cbPathName=%RU32, fMode=0x%x, rc=%Rrc\n",
                                 szPathName, cbPathName, fMode, rc));

                    char *pszPathAbs = RTPathJoinA(pszDropDir, szPathName);
                    if (pszPathAbs)
                    {
#ifdef RT_OS_WINDOWS
                        uint32_t fCreationMode = (fMode & RTFS_DOS_MASK) | RTFS_DOS_NT_NORMAL;
#else
                        uint32_t fCreationMode = (fMode & RTFS_UNIX_MASK) | RTFS_UNIX_IRWXU;
#endif
                        rc = RTDirCreate(pszPathAbs, fCreationMode, 0);
                        if (RT_SUCCESS(rc))
                            rc = pDroppedFiles->AddDir(pszPathAbs);

                        if (   RT_SUCCESS(rc)
                            && fDoAccounting)
                        {
                            Assert(cToRecvObjs);
                            cToRecvObjs--;
                        }

                        RTStrFree(pszPathAbs);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                    break;
                }
                case HOST_DND_HG_SND_FILE_HDR:
                case HOST_DND_HG_SND_FILE_DATA:
                {
                    if (uNextMsg == HOST_DND_HG_SND_FILE_HDR)
                    {
                        rc = vbglR3DnDHGRecvFileHdr(pCtx,
                                                    szPathName,
                                                    sizeof(szPathName),
                                                    &fFlags,
                                                    &fMode,
                                                    &cbFileSize);
                        LogFlowFunc(("HOST_DND_HG_SND_FILE_HDR: "
                                     "szPathName=%s, fFlags=0x%x, fMode=0x%x, cbFileSize=%RU64, rc=%Rrc\n",
                                     szPathName, fFlags, fMode, cbFileSize, rc));
                    }
                    else
                    {
                        rc = vbglR3DnDHGRecvFileData(pCtx,
                                                     szPathName,
                                                     sizeof(szPathName),
                                                     &cbPathName,
                                                     pvChunk,
                                                     cbChunkMax,
                                                     &cbChunkRead,
                                                     &fMode);

                        LogFlowFunc(("HOST_DND_HG_SND_FILE_DATA: "
                                     "szPathName=%s, cbPathName=%RU32, cbChunkRead=%RU32, fMode=0x%x, rc=%Rrc\n",
                                     szPathName, cbPathName, cbChunkRead, fMode, rc));
                    }

                    if (   RT_SUCCESS(rc)
                        && (   uNextMsg == HOST_DND_HG_SND_FILE_HDR
                            /* Protocol v1 always sends the file name, so opening the file every time. */
                            || pCtx->uProtocol <= 1)
                       )
                    {
                        char *pszPathAbs = RTPathJoinA(pszDropDir, szPathName);
                        if (pszPathAbs)
                        {
                            LogFlowFunc(("Opening pszPathName=%s, cbPathName=%RU32, fMode=0x%x, cbFileSize=%zu\n",
                                         szPathName, cbPathName, fMode, cbFileSize));

                            uint64_t fOpen = RTFILE_O_WRITE | RTFILE_O_DENY_WRITE;
                            if (pCtx->uProtocol <= 1)
                                fOpen |= RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND;
                            else
                                fOpen |= RTFILE_O_CREATE_REPLACE;

                            /* Is there already a file open, e.g. in transfer? */
                            if (!objFile.IsOpen())
                            {
                                RTCString strPathAbs(pszPathAbs);
#ifdef RT_OS_WINDOWS
                                uint32_t fCreationMode = (fMode & RTFS_DOS_MASK) | RTFS_DOS_NT_NORMAL;
#else
                                uint32_t fCreationMode = (fMode & RTFS_UNIX_MASK) | RTFS_UNIX_IRUSR | RTFS_UNIX_IWUSR;
#endif
                                rc = objFile.OpenEx(strPathAbs, DnDURIObject::File, DnDURIObject::Target, fOpen, fCreationMode);
                                if (RT_SUCCESS(rc))
                                {
                                    rc = pDroppedFiles->AddFile(strPathAbs.c_str());
                                    if (RT_SUCCESS(rc))
                                    {
                                        cbFileWritten = 0;

                                        if (pCtx->uProtocol >= 2) /* Set the expected file size. */
                                            objFile.SetSize(cbFileSize);
                                    }
                                }
                            }
                            else
                            {
                                AssertMsgFailed(("ObjType=%RU32, Proto=%RU32\n", objFile.GetType(), pCtx->uProtocol));
                                rc = VERR_WRONG_ORDER;
                            }

                            RTStrFree(pszPathAbs);
                        }
                        else
                            rc = VERR_NO_MEMORY;
                    }

                    if (   RT_SUCCESS(rc)
                        && uNextMsg == HOST_DND_HG_SND_FILE_DATA)
                    {
                        uint32_t cbChunkWritten;
                        rc = objFile.Write(pvChunk, cbChunkRead, &cbChunkWritten);
                        if (RT_SUCCESS(rc))
                        {
                            LogFlowFunc(("HOST_DND_HG_SND_FILE_DATA "
                                         "cbChunkRead=%RU32, cbChunkWritten=%RU32, cbFileWritten=%RU64 cbFileSize=%RU64\n",
                                         cbChunkRead, cbChunkWritten, cbFileWritten + cbChunkWritten, cbFileSize));

                            cbFileWritten += cbChunkWritten;

                            if (fDoAccounting)
                            {
                                Assert(cbChunkRead <= cbToRecvBytes);
                                cbToRecvBytes -= cbChunkRead;
                            }
                        }
                    }

                    bool fClose = false;
                    if (pCtx->uProtocol >= 2)
                    {
                        /* Data transfer complete? Close the file. */
                        fClose = objFile.IsComplete();
                        if (   fClose
                            && fDoAccounting)
                        {
                            Assert(cToRecvObjs);
                            cToRecvObjs--;
                        }

                        /* Only since protocol v2 we know the file size upfront. */
                        Assert(cbFileWritten <= cbFileSize);
                    }
                    else
                        fClose = true; /* Always close the file after each chunk. */

                    if (fClose)
                    {
                        LogFlowFunc(("Closing file\n"));
                        objFile.Close();
                    }

                    break;
                }
                case HOST_DND_HG_EVT_CANCEL:
                {
                    rc = vbglR3DnDHGRecvCancel(pCtx);
                    if (RT_SUCCESS(rc))
                        rc = VERR_CANCELLED;
                    break;
                }
                default:
                {
                    LogFlowFunc(("Message %RU32 not supported\n", uNextMsg));
                    rc = VERR_NOT_SUPPORTED;
                    break;
                }
            }
        }

        if (RT_FAILURE(rc))
            break;

        if (fDoAccounting)
        {
            LogFlowFunc(("cbToRecvBytes=%RU64, cToRecvObjs=%RU64\n", cbToRecvBytes, cToRecvObjs));
            if (   !cbToRecvBytes
                && !cToRecvObjs)
            {
                break;
            }
        }

    } while (RT_SUCCESS(rc));

    LogFlowFunc(("Loop ended with %Rrc\n", rc));

    /* All URI data processed? */
    if (rc == VERR_NO_DATA)
        rc = VINF_SUCCESS;

    /* Delete temp buffer again. */
    if (pvChunk)
        RTMemFree(pvChunk);

    /* Cleanup on failure or if the user has canceled the operation or
     * something else went wrong. */
    if (RT_FAILURE(rc))
    {
        objFile.Close();
        pDroppedFiles->Rollback();
    }
    else
    {
        /** @todo Compare the URI list with the dirs/files we really transferred. */
        /** @todo Implement checksum verification, if any. */
    }

    /*
     * Close the dropped files directory.
     * Don't try to remove it here, however, as the files are being needed
     * by the client's drag'n drop operation lateron.
     */
    int rc2 = pDroppedFiles->Reset(false /* fRemoveDropDir */);
    if (RT_FAILURE(rc2)) /* Not fatal, don't report back to host. */
        LogFlowFunc(("Closing dropped files directory failed with %Rrc\n", rc2));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDHGRecvDataRaw(PVBGLR3GUESTDNDCMDCTX pCtx, PVBOXDNDSNDDATAHDR pDataHdr,
                                  void *pvData, uint32_t cbData, uint32_t *pcbDataRecv)
{
    AssertPtrReturn(pCtx,            VERR_INVALID_POINTER);
    AssertPtrReturn(pDataHdr,        VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,          VERR_INVALID_POINTER);
    AssertReturn(cbData,             VERR_INVALID_PARAMETER);
    AssertPtrNullReturn(pcbDataRecv, VERR_INVALID_POINTER);

    LogFlowFunc(("pvDate=%p, cbData=%RU32\n", pvData, cbData));

    VBOXDNDHGSENDDATAMSG Msg;
    RT_ZERO(Msg);
    int rc;
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_DATA, 5);
        Msg.u.v1.uScreenId.SetUInt32(0);
        Msg.u.v1.pvFormat.SetPtr(pDataHdr->pvMetaFmt, pDataHdr->cbMetaFmt);
        Msg.u.v1.cbFormat.SetUInt32(0);
        Msg.u.v1.pvData.SetPtr(pvData, cbData);
        Msg.u.v1.cbData.SetUInt32(0);

        rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
        if (   RT_SUCCESS(rc)
            || rc == VERR_BUFFER_OVERFLOW)
        {
            /** @todo r=bird: The VERR_BUFFER_OVERFLOW case is probably broken as the
             *        status isn't returned to the caller (vbglR3DnDHGRecvDataLoop).
             *        This was the case before fixing the uninitalized variable.  As
             *        other V0-2 protocol functions have been marked deprecated, it's
             *        probably a good idea to just remove this code and tell the 1-2 users
             *        to upgrade the host instead.  Unused and untested weird code like this
             *        is just hard+costly to maintain and liability.
             *        (VERR_BUFFER_OVERFLOW == weird, no disrespect intended) */

            /* Unmarshal the whole message first. */
            rc = Msg.u.v1.uScreenId.GetUInt32(&pDataHdr->uScreenId);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                uint32_t cbFormatRecv;
                rc = Msg.u.v1.cbFormat.GetUInt32(&cbFormatRecv);
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    uint32_t cbDataRecv;
                    rc = Msg.u.v1.cbData.GetUInt32(&cbDataRecv);
                    AssertRC(rc);
                    if (RT_SUCCESS(rc))
                    {
                        /*
                         * In case of VERR_BUFFER_OVERFLOW get the data sizes required
                         * for the format + data blocks.
                         */
                        if (   cbFormatRecv >= pDataHdr->cbMetaFmt
                            || cbDataRecv   >= pDataHdr->cbMeta)
                            rc = VERR_TOO_MUCH_DATA;
                        else
                        {
                            pDataHdr->cbMetaFmt = cbFormatRecv;
                            if (pcbDataRecv)
                                *pcbDataRecv = cbDataRecv;
                            LogFlowFuncLeaveRC(rc);
                            return rc;
                        }
                    }
                }
            }
        }
    }
    else /* Protocol v3 and up. */
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_DATA, 5);
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvData.SetPtr(pvData, cbData);
        Msg.u.v3.cbData.SetUInt32(0);
        Msg.u.v3.pvChecksum.SetPtr(NULL, 0);
        Msg.u.v3.cbChecksum.SetUInt32(0);

        rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
        if (RT_SUCCESS(rc))
        {
            uint32_t cbDataRecv;
            rc = Msg.u.v3.cbData.GetUInt32(&cbDataRecv);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                /** @todo Use checksum for validating the received data. */
                if (pcbDataRecv)
                    *pcbDataRecv = cbDataRecv;
                LogFlowFuncLeaveRC(rc);
                return rc;
            }
        }
    }

    /* failure */
    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDHGRecvDataHdr(PVBGLR3GUESTDNDCMDCTX pCtx, PVBOXDNDSNDDATAHDR pDataHdr)
{
    AssertPtrReturn(pCtx,     VERR_INVALID_POINTER);
    AssertPtrReturn(pDataHdr, VERR_INVALID_POINTER);

    Assert(pCtx->uProtocol >= 3); /* Only for protocol v3 and up. */

    VBOXDNDHGSENDDATAHDRMSG Msg;
    RT_ZERO(Msg);
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_DATA_HDR, 12);
    Msg.uContext.SetUInt32(0);
    Msg.uFlags.SetUInt32(0);
    Msg.uScreenId.SetUInt32(0);
    Msg.cbTotal.SetUInt64(0);
    Msg.cbMeta.SetUInt32(0);
    Msg.pvMetaFmt.SetPtr(pDataHdr->pvMetaFmt, pDataHdr->cbMetaFmt);
    Msg.cbMetaFmt.SetUInt32(0);
    Msg.cObjects.SetUInt64(0);
    Msg.enmCompression.SetUInt32(0);
    Msg.enmChecksumType.SetUInt32(0);
    Msg.pvChecksum.SetPtr(pDataHdr->pvChecksum, pDataHdr->cbChecksum);
    Msg.cbChecksum.SetUInt32(0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        /* Msg.uContext not needed here. */
        Msg.uFlags.GetUInt32(&pDataHdr->uFlags);
        Msg.uScreenId.GetUInt32(&pDataHdr->uScreenId);
        Msg.cbTotal.GetUInt64(&pDataHdr->cbTotal);
        Msg.cbMeta.GetUInt32(&pDataHdr->cbMeta);
        Msg.cbMetaFmt.GetUInt32(&pDataHdr->cbMetaFmt);
        Msg.cObjects.GetUInt64(&pDataHdr->cObjects);
        Msg.enmCompression.GetUInt32(&pDataHdr->enmCompression);
        Msg.enmChecksumType.GetUInt32((uint32_t *)&pDataHdr->enmChecksumType);
        Msg.cbChecksum.GetUInt32(&pDataHdr->cbChecksum);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @todo Deprecated function; will be removed. */
static int vbglR3DnDHGRecvMoreData(PVBGLR3GUESTDNDCMDCTX pCtx, void *pvData, uint32_t cbData, uint32_t *pcbDataRecv)
{
    AssertPtrReturn(pCtx,        VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,      VERR_INVALID_POINTER);
    AssertReturn(cbData,         VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbDataRecv, VERR_INVALID_POINTER);

    VBOXDNDHGSENDMOREDATAMSG Msg;
    RT_ZERO(Msg);
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_HG_SND_MORE_DATA, 2);
    Msg.pvData.SetPtr(pvData, cbData);
    Msg.cbData.SetUInt32(0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (   RT_SUCCESS(rc)
        || rc == VERR_BUFFER_OVERFLOW)
    {
        rc = Msg.cbData.GetUInt32(pcbDataRecv); AssertRC(rc);
        AssertReturn(cbData >= *pcbDataRecv, VERR_TOO_MUCH_DATA);
    }
    return rc;
}

static int vbglR3DnDHGRecvDataLoop(PVBGLR3GUESTDNDCMDCTX pCtx, PVBOXDNDSNDDATAHDR pDataHdr,
                                   void **ppvData, uint64_t *pcbData)
{
    AssertPtrReturn(pCtx,     VERR_INVALID_POINTER);
    AssertPtrReturn(pDataHdr, VERR_INVALID_POINTER);
    AssertPtrReturn(ppvData,  VERR_INVALID_POINTER);
    AssertPtrReturn(pcbData,  VERR_INVALID_POINTER);

    int rc;
    uint32_t cbDataRecv;

    LogFlowFuncEnter();

    if (pCtx->uProtocol < 3)
    {
        uint64_t cbDataTmp = pCtx->cbMaxChunkSize;
        void    *pvDataTmp = RTMemAlloc(cbDataTmp);

        if (!cbDataTmp)
            return VERR_NO_MEMORY;

        /*
         * Protocols < v3 contain the header information in every HOST_DND_HG_SND_DATA
         * message, so do the actual retrieving immediately.
         *
         * Also, the initial implementation used VERR_BUFFER_OVERFLOW as a return code to
         * indicate that there will be more data coming in after the initial data chunk. There
         * was no way of telling the total data size upfront (in form of a header or some such),
         * so also handle this case to not break backwards compatibility.
         */
        rc = vbglR3DnDHGRecvDataRaw(pCtx, pDataHdr, pvDataTmp, pCtx->cbMaxChunkSize, &cbDataRecv);

        /* See comment above. */
        while (rc == VERR_BUFFER_OVERFLOW)
        {
            uint32_t uNextMsg;
            uint32_t cNextParms;
            rc = vbglR3DnDGetNextMsgType(pCtx, &uNextMsg, &cNextParms, false /* fBlock */);
            if (RT_SUCCESS(rc))
            {
                switch(uNextMsg)
                {
                    case HOST_DND_HG_SND_MORE_DATA:
                    {
                        /** @todo r=andy Don't use reallocate here; can go wrong with *really* big URI lists.
                         *               Instead send as many URI entries as possible per chunk and add those entries
                         *               to our to-process list for immediata processing. Repeat the step after processing then. */
                        LogFlowFunc(("HOST_DND_HG_SND_MORE_DATA cbDataTotal: %RU64 -> %RU64\n",
                                     cbDataTmp, cbDataTmp + pCtx->cbMaxChunkSize));
                        void *pvDataNew = RTMemRealloc(pvDataTmp, cbDataTmp + pCtx->cbMaxChunkSize);
                        if (!pvDataNew)
                        {
                            rc = VERR_NO_MEMORY;
                            break;
                        }

                        pvDataTmp = pvDataNew;

                        uint8_t *pvDataOff = (uint8_t *)pvDataTmp + cbDataTmp;
                        rc = vbglR3DnDHGRecvMoreData(pCtx, pvDataOff, pCtx->cbMaxChunkSize, &cbDataRecv);
                        if (   RT_SUCCESS(rc)
                            || rc == VERR_BUFFER_OVERFLOW) /* Still can return VERR_BUFFER_OVERFLOW. */
                        {
                            cbDataTmp += cbDataRecv;
                        }
                        break;
                    }
                    case HOST_DND_HG_EVT_CANCEL:
                    default:
                    {
                        rc = vbglR3DnDHGRecvCancel(pCtx);
                        if (RT_SUCCESS(rc))
                            rc = VERR_CANCELLED;
                        break;
                    }
                }
            }
        }

        if (RT_SUCCESS(rc))
        {
            /* There was no way of telling the total data size upfront
             * (in form of a header or some such), so set the total data size here. */
            pDataHdr->cbTotal = cbDataTmp;

            *ppvData = pvDataTmp;
            *pcbData = cbDataTmp;
        }
        else
            RTMemFree(pvDataTmp);
    }
    else /* Protocol v3 and up. */
    {
        rc = vbglR3DnDHGRecvDataHdr(pCtx, pDataHdr);
        if (RT_SUCCESS(rc))
        {
            LogFlowFunc(("cbTotal=%RU64, cbMeta=%RU32\n", pDataHdr->cbTotal, pDataHdr->cbMeta));
            if (pDataHdr->cbMeta)
            {
                uint64_t cbDataTmp = 0;
                void    *pvDataTmp = RTMemAlloc(pDataHdr->cbMeta);
                if (!pvDataTmp)
                    rc = VERR_NO_MEMORY;

                if (RT_SUCCESS(rc))
                {
                    uint8_t *pvDataOff = (uint8_t *)pvDataTmp;
                    while (cbDataTmp < pDataHdr->cbMeta)
                    {
                        rc = vbglR3DnDHGRecvDataRaw(pCtx, pDataHdr,
                                                    pvDataOff, RT_MIN(pDataHdr->cbMeta - cbDataTmp, pCtx->cbMaxChunkSize),
                                                    &cbDataRecv);
                        if (RT_SUCCESS(rc))
                        {
                            LogFlowFunc(("cbDataRecv=%RU32, cbDataTmp=%RU64\n", cbDataRecv, cbDataTmp));
                            Assert(cbDataTmp + cbDataRecv <= pDataHdr->cbMeta);
                            cbDataTmp += cbDataRecv;
                            pvDataOff += cbDataRecv;
                        }
                        else
                            break;
                    }

                    if (RT_SUCCESS(rc))
                    {
                        Assert(cbDataTmp == pDataHdr->cbMeta);

                        LogFlowFunc(("Received %RU64 bytes of data\n", cbDataTmp));

                        *ppvData = pvDataTmp;
                        *pcbData = cbDataTmp;
                    }
                    else
                        RTMemFree(pvDataTmp);
                }
            }
            else
            {
                *ppvData = NULL;
                *pcbData = 0;
            }
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** @todo Replace the parameters (except pCtx) with PVBOXDNDSNDDATAHDR. Later. */
/** @todo Hand in the DnDURIList + DnDDroppedFiles objects so that this function
 *        can fill it directly instead of passing huge blobs of data around. */
static int vbglR3DnDHGRecvDataMain(PVBGLR3GUESTDNDCMDCTX  pCtx,
                                   uint32_t              *puScreenId,
                                   char                 **ppszFormat,
                                   uint32_t              *pcbFormat,
                                   void                 **ppvData,
                                   uint32_t              *pcbData)
{
    AssertPtrReturn(pCtx,       VERR_INVALID_POINTER);
    AssertPtrReturn(puScreenId, VERR_INVALID_POINTER);
    AssertPtrReturn(ppszFormat, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbFormat,  VERR_INVALID_POINTER);
    AssertPtrReturn(ppvData,    VERR_INVALID_POINTER);
    AssertPtrReturn(pcbData,    VERR_INVALID_POINTER);

    VBOXDNDDATAHDR dataHdr; /** @todo See todo above. */
    RT_ZERO(dataHdr);

    dataHdr.cbMetaFmt = _64K;  /** @todo Make this configurable? */
    dataHdr.pvMetaFmt = RTMemAlloc(dataHdr.cbMetaFmt);
    if (!dataHdr.pvMetaFmt)
        return VERR_NO_MEMORY;

    DnDURIList lstURI;
    DnDDroppedFiles droppedFiles;

    void *pvData;    /** @todo See todo above. */
    uint64_t cbData; /** @todo See todo above. */

    int rc = vbglR3DnDHGRecvDataLoop(pCtx, &dataHdr, &pvData, &cbData);
    if (RT_SUCCESS(rc))
    {
        /**
         * Check if this is an URI event. If so, let VbglR3 do all the actual
         * data transfer + file/directory creation internally without letting
         * the caller know.
         *
         * This keeps the actual (guest OS-)dependent client (like VBoxClient /
         * VBoxTray) small by not having too much redundant code.
         */
        Assert(dataHdr.cbMetaFmt);
        AssertPtr(dataHdr.pvMetaFmt);
        if (DnDMIMEHasFileURLs((char *)dataHdr.pvMetaFmt, dataHdr.cbMetaFmt))
        {
            AssertPtr(pvData);
            Assert(cbData);
            rc = lstURI.RootFromURIData(pvData, cbData, 0 /* fFlags */);
            if (RT_SUCCESS(rc))
                rc = vbglR3DnDHGRecvURIData(pCtx, &dataHdr, &droppedFiles);

            if (RT_SUCCESS(rc)) /** @todo Remove this block as soon as we hand in DnDURIList. */
            {
                if (pvData)
                {
                    /* Reuse data buffer to fill in the transformed URI file list. */
                    RTMemFree(pvData);
                    pvData = NULL;
                }

                RTCString strData = lstURI.RootToString(droppedFiles.GetDirAbs());
                Assert(!strData.isEmpty());

                cbData = strData.length() + 1;
                LogFlowFunc(("URI list has %zu bytes\n", cbData));

                pvData = RTMemAlloc(cbData);
                if (pvData)
                {
                    memcpy(pvData, strData.c_str(), cbData);
                }
                else
                    rc =  VERR_NO_MEMORY;
            }
        }
        else /* Raw data. */
        {
            const uint32_t cbDataRaw = dataHdr.cbMetaFmt;
            if (cbData >= cbDataRaw)
            {
                if (cbDataRaw)
                    memcpy(pvData, dataHdr.pvMetaFmt, cbDataRaw);
                cbData = cbDataRaw;
            }
            else
                rc = VERR_BUFFER_OVERFLOW;
        }
    }

    if (   RT_FAILURE(rc)
        && rc != VERR_CANCELLED)
    {
        if (dataHdr.pvMetaFmt)
            RTMemFree(dataHdr.pvMetaFmt);
        if (pvData)
            RTMemFree(pvData);

        int rc2 = VbglR3DnDHGSendProgress(pCtx, DND_PROGRESS_ERROR, 100 /* Percent */, rc);
        if (RT_FAILURE(rc2))
            LogFlowFunc(("Unable to send progress error %Rrc to host: %Rrc\n", rc, rc2));
    }
    else if (RT_SUCCESS(rc))
    {
        *ppszFormat = (char *)dataHdr.pvMetaFmt;
        *pcbFormat  =         dataHdr.cbMetaFmt;
        *ppvData    = pvData;
        *pcbData    = cbData;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDGHRecvPending(PVBGLR3GUESTDNDCMDCTX pCtx, uint32_t *puScreenId)
{
    AssertPtrReturn(pCtx,       VERR_INVALID_POINTER);
    AssertPtrReturn(puScreenId, VERR_INVALID_POINTER);

    VBOXDNDGHREQPENDINGMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_GH_REQ_PENDING, 1);
        Msg.u.v1.uScreenId.SetUInt32(0);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_GH_REQ_PENDING, 2);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.uScreenId.SetUInt32(0);
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        if (pCtx->uProtocol < 3)
        {
            rc = Msg.u.v1.uScreenId.GetUInt32(puScreenId); AssertRC(rc);
        }
        else
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v3.uContext.GetUInt32(puScreenId); AssertRC(rc);
        }
    }

    return rc;
}

static int vbglR3DnDGHRecvDropped(PVBGLR3GUESTDNDCMDCTX pCtx,
                                  char     *pszFormat,
                                  uint32_t  cbFormat,
                                  uint32_t *pcbFormatRecv,
                                  uint32_t *puAction)
{
    AssertPtrReturn(pCtx,          VERR_INVALID_POINTER);
    AssertPtrReturn(pszFormat,     VERR_INVALID_POINTER);
    AssertReturn(cbFormat,         VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbFormatRecv, VERR_INVALID_POINTER);
    AssertPtrReturn(puAction,      VERR_INVALID_POINTER);

    VBOXDNDGHDROPPEDMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_GH_EVT_DROPPED, 3);
        Msg.u.v1.pvFormat.SetPtr(pszFormat, cbFormat);
        Msg.u.v1.cbFormat.SetUInt32(0);
        Msg.u.v1.uAction.SetUInt32(0);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, HOST_DND_GH_EVT_DROPPED, 4);
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvFormat.SetPtr(pszFormat, cbFormat);
        Msg.u.v3.cbFormat.SetUInt32(0);
        Msg.u.v3.uAction.SetUInt32(0);
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        if (pCtx->uProtocol < 3)
        {
            rc = Msg.u.v1.cbFormat.GetUInt32(pcbFormatRecv); AssertRC(rc);
            rc = Msg.u.v1.uAction.GetUInt32(puAction);       AssertRC(rc);
        }
        else
        {
            /** @todo Context ID not used yet. */
            rc = Msg.u.v3.cbFormat.GetUInt32(pcbFormatRecv); AssertRC(rc);
            rc = Msg.u.v3.uAction.GetUInt32(puAction);       AssertRC(rc);
        }

        AssertReturn(cbFormat >= *pcbFormatRecv, VERR_TOO_MUCH_DATA);
    }

    return rc;
}


/*********************************************************************************************************************************
*   Public functions                                                                                                             *
*********************************************************************************************************************************/

VBGLR3DECL(int) VbglR3DnDConnect(PVBGLR3GUESTDNDCMDCTX pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /* Initialize header */
    int rc = VbglR3HGCMConnect("VBoxDragAndDropSvc", &pCtx->uClientID);
    if (RT_FAILURE(rc))
        return rc;

    /* Set the default protocol version to use. */
    pCtx->uProtocol = 3;
    Assert(pCtx->uClientID);

    /*
     * Get the VM's session ID.
     * This is not fatal in case we're running with an ancient VBox version.
     */
    pCtx->uSessionID = 0;
    int rc2 = VbglR3GetSessionId(&pCtx->uSessionID);
    LogFlowFunc(("uSessionID=%RU64, rc=%Rrc\n", pCtx->uSessionID, rc2));

    /*
     * Check if the host is >= VBox 5.0 which in case supports GUEST_DND_CONNECT.
     */
    bool fSupportsConnectReq = false;
    if (RT_SUCCESS(rc))
    {
        /* The guest property service might not be available. Not fatal. */
        uint32_t uGuestPropSvcClientID;
        rc2 = VbglR3GuestPropConnect(&uGuestPropSvcClientID);
        if (RT_SUCCESS(rc2))
        {
            char *pszHostVersion;
            rc2 = VbglR3GuestPropReadValueAlloc(uGuestPropSvcClientID, "/VirtualBox/HostInfo/VBoxVer", &pszHostVersion);
            if (RT_SUCCESS(rc2))
            {
                fSupportsConnectReq = RTStrVersionCompare(pszHostVersion, "5.0") >= 0;
                LogFlowFunc(("pszHostVersion=%s, fSupportsConnectReq=%RTbool\n", pszHostVersion, fSupportsConnectReq));
                VbglR3GuestPropReadValueFree(pszHostVersion);
            }

            VbglR3GuestPropDisconnect(uGuestPropSvcClientID);
        }

        if (RT_FAILURE(rc2))
            LogFlowFunc(("Retrieving host version failed with rc=%Rrc\n", rc2));
    }

    if (fSupportsConnectReq)
    {
        /*
         * Try sending the connect message to tell the protocol version to use.
         * Note: This might fail when the Guest Additions run on an older VBox host (< VBox 5.0) which
         *       does not implement this command.
         */
        VBOXDNDCONNECTMSG Msg;
        RT_ZERO(Msg);
        if (pCtx->uProtocol < 3)
        {
            VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_CONNECT, 2);
            Msg.u.v2.uProtocol.SetUInt32(pCtx->uProtocol);
            Msg.u.v2.uFlags.SetUInt32(0); /* Unused at the moment. */
        }
        else
        {
            VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_CONNECT, 3);
            /** @todo Context ID not used yet. */
            Msg.u.v3.uContext.SetUInt32(0);
            Msg.u.v3.uProtocol.SetUInt32(pCtx->uProtocol);
            Msg.u.v3.uFlags.SetUInt32(0); /* Unused at the moment. */
        }

        rc2 = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
        if (RT_FAILURE(rc2))
            fSupportsConnectReq = false;

        LogFlowFunc(("Connection request ended with rc=%Rrc\n", rc2));
    }

    /* GUEST_DND_CONNECT not supported; play safe here and just use protocol v1. */
    if (!fSupportsConnectReq)
        pCtx->uProtocol = 1; /* Fall back to protocol version 1 (< VBox 5.0). */

    pCtx->cbMaxChunkSize = _64K; /** @todo Use a scratch buffer on the heap? */

    LogFlowFunc(("uClient=%RU32, uProtocol=%RU32, rc=%Rrc\n", pCtx->uClientID, pCtx->uProtocol, rc));
    return rc;
}

VBGLR3DECL(int) VbglR3DnDDisconnect(PVBGLR3GUESTDNDCMDCTX pCtx)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    int rc = VbglR3HGCMDisconnect(pCtx->uClientID);
    if (RT_SUCCESS(rc))
        pCtx->uClientID = 0;
    return rc;
}

VBGLR3DECL(int) VbglR3DnDRecvNextMsg(PVBGLR3GUESTDNDCMDCTX pCtx, CPVBGLR3DNDHGCMEVENT pEvent)
{
    AssertPtrReturn(pCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    const uint32_t cbFormatMax = pCtx->cbMaxChunkSize;

    uint32_t       uMsg   = 0;
    uint32_t       cParms = 0;
    int rc = vbglR3DnDGetNextMsgType(pCtx, &uMsg, &cParms, true /* fWait */);
    if (RT_SUCCESS(rc))
    {
        /* Check for VM session change. */
        uint64_t uSessionID;
        int rc2 = VbglR3GetSessionId(&uSessionID);
        if (   RT_SUCCESS(rc2)
            && (uSessionID != pCtx->uSessionID))
        {
            LogFlowFunc(("VM session ID changed to %RU64, doing reconnect\n", uSessionID));

            /* Try a reconnect to the DnD service. */
            rc2 = VbglR3DnDDisconnect(pCtx);
            AssertRC(rc2);
            rc2 = VbglR3DnDConnect(pCtx);
            AssertRC(rc2);

            /* At this point we continue processing the messsages with the new client ID. */
        }
    }

    if (RT_SUCCESS(rc))
    {
        pEvent->uType = uMsg;

        switch(uMsg)
        {
            case HOST_DND_HG_EVT_ENTER:
            case HOST_DND_HG_EVT_MOVE:
            case HOST_DND_HG_EVT_DROPPED:
            {
                pEvent->pszFormats = static_cast<char*>(RTMemAlloc(cbFormatMax));
                if (!pEvent->pszFormats)
                    rc = VERR_NO_MEMORY;

                if (RT_SUCCESS(rc))
                    rc = vbglR3DnDHGRecvAction(pCtx,
                                               uMsg,
                                               &pEvent->uScreenId,
                                               &pEvent->u.a.uXpos,
                                               &pEvent->u.a.uYpos,
                                               &pEvent->u.a.uDefAction,
                                               &pEvent->u.a.uAllActions,
                                               pEvent->pszFormats,
                                               cbFormatMax,
                                               &pEvent->cbFormats);
                break;
            }
            case HOST_DND_HG_EVT_LEAVE:
            {
                rc = vbglR3DnDHGRecvLeave(pCtx);
                break;
            }
            case HOST_DND_HG_SND_DATA:
                /* Protocol v1 + v2: Also contains the header data.
                 * Note: Fall through is intentional. */
            case HOST_DND_HG_SND_DATA_HDR:
            {
                rc = vbglR3DnDHGRecvDataMain(pCtx,
                                             /* Screen ID */
                                             &pEvent->uScreenId,
                                             /* Format */
                                             &pEvent->pszFormats,
                                             &pEvent->cbFormats,
                                             /* Data */
                                             &pEvent->u.b.pvData,
                                             &pEvent->u.b.cbData);
                break;
            }
            case HOST_DND_HG_SND_MORE_DATA:
            case HOST_DND_HG_SND_DIR:
            case HOST_DND_HG_SND_FILE_DATA:
            {
                /*
                 * All messages in this case are handled internally
                 * by vbglR3DnDHGRecvDataMain() and must be specified
                 * by a preceding HOST_DND_HG_SND_DATA or HOST_DND_HG_SND_DATA_HDR
                 * calls.
                 */
                rc = VERR_WRONG_ORDER;
                break;
            }
            case HOST_DND_HG_EVT_CANCEL:
            {
                rc = vbglR3DnDHGRecvCancel(pCtx);
                break;
            }
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
            case HOST_DND_GH_REQ_PENDING:
            {
                rc = vbglR3DnDGHRecvPending(pCtx, &pEvent->uScreenId);
                break;
            }
            case HOST_DND_GH_EVT_DROPPED:
            {
                pEvent->pszFormats = static_cast<char*>(RTMemAlloc(cbFormatMax));
                if (!pEvent->pszFormats)
                    rc = VERR_NO_MEMORY;

                if (RT_SUCCESS(rc))
                    rc = vbglR3DnDGHRecvDropped(pCtx,
                                                pEvent->pszFormats,
                                                cbFormatMax,
                                                &pEvent->cbFormats,
                                                &pEvent->u.a.uDefAction);
                break;
            }
#endif
            default:
            {
                rc = VERR_NOT_SUPPORTED;
                break;
            }
        }
    }

    if (RT_FAILURE(rc))
        LogFlowFunc(("Returning error %Rrc\n", rc));
    return rc;
}

VBGLR3DECL(int) VbglR3DnDHGSendAckOp(PVBGLR3GUESTDNDCMDCTX pCtx, uint32_t uAction)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    VBOXDNDHGACKOPMSG Msg;
    RT_ZERO(Msg);
    LogFlowFunc(("uProto=%RU32\n", pCtx->uProtocol));
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_ACK_OP, 1);
        Msg.u.v1.uAction.SetUInt32(uAction);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_ACK_OP, 2);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.uAction.SetUInt32(uAction);
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

VBGLR3DECL(int) VbglR3DnDHGSendReqData(PVBGLR3GUESTDNDCMDCTX pCtx, const char* pcszFormat)
{
    AssertPtrReturn(pCtx,       VERR_INVALID_POINTER);
    AssertPtrReturn(pcszFormat, VERR_INVALID_POINTER);
    if (!RTStrIsValidEncoding(pcszFormat))
        return VERR_INVALID_PARAMETER;

    const uint32_t cbFormat = (uint32_t)strlen(pcszFormat) + 1; /* Include termination */

    VBOXDNDHGREQDATAMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_REQ_DATA, 1);
        Msg.u.v1.pvFormat.SetPtr((void*)pcszFormat, cbFormat);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_REQ_DATA, 3);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvFormat.SetPtr((void*)pcszFormat, cbFormat);
        Msg.u.v3.cbFormat.SetUInt32(cbFormat);
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

VBGLR3DECL(int) VbglR3DnDHGSendProgress(PVBGLR3GUESTDNDCMDCTX pCtx, uint32_t uStatus, uint8_t uPercent, int rcErr)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(uStatus > DND_PROGRESS_UNKNOWN, VERR_INVALID_PARAMETER);

    VBOXDNDHGEVTPROGRESSMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_EVT_PROGRESS, 3);
        Msg.u.v1.uStatus.SetUInt32(uStatus);
        Msg.u.v1.uPercent.SetUInt32(uPercent);
        Msg.u.v1.rc.SetUInt32((uint32_t)rcErr); /* uint32_t vs. int. */
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_HG_EVT_PROGRESS, 4);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.uStatus.SetUInt32(uStatus);
        Msg.u.v3.uPercent.SetUInt32(uPercent);
        Msg.u.v3.rc.SetUInt32((uint32_t)rcErr); /* uint32_t vs. int. */
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH

VBGLR3DECL(int) VbglR3DnDGHSendAckPending(PVBGLR3GUESTDNDCMDCTX pCtx,
                                          uint32_t uDefAction, uint32_t uAllActions, const char* pcszFormats, uint32_t cbFormats)
{
    AssertPtrReturn(pCtx,        VERR_INVALID_POINTER);
    AssertPtrReturn(pcszFormats, VERR_INVALID_POINTER);
    AssertReturn(cbFormats,      VERR_INVALID_PARAMETER);

    if (!RTStrIsValidEncoding(pcszFormats))
        return VERR_INVALID_UTF8_ENCODING;

    VBOXDNDGHACKPENDINGMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_ACK_PENDING, 3);
        Msg.u.v1.uDefAction.SetUInt32(uDefAction);
        Msg.u.v1.uAllActions.SetUInt32(uAllActions);
        Msg.u.v1.pvFormats.SetPtr((void*)pcszFormats, cbFormats);
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_ACK_PENDING, 5);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.uDefAction.SetUInt32(uDefAction);
        Msg.u.v3.uAllActions.SetUInt32(uAllActions);
        Msg.u.v3.pvFormats.SetPtr((void*)pcszFormats, cbFormats);
        Msg.u.v3.cbFormats.SetUInt32(cbFormats);
    }

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

static int vbglR3DnDGHSendDataInternal(PVBGLR3GUESTDNDCMDCTX pCtx,
                                       void *pvData, uint64_t cbData, PVBOXDNDSNDDATAHDR pDataHdr)
{
    AssertPtrReturn(pCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData,    VERR_INVALID_PARAMETER);
    /* cbAdditionalData is optional. */
    /* pDataHdr is optional in protocols < v3. */

    int rc = VINF_SUCCESS;

    /* For protocol v3 and up we need to send the data header first. */
    if (pCtx->uProtocol >= 3)
    {
        AssertPtrReturn(pDataHdr, VERR_INVALID_POINTER);

        VBOXDNDGHSENDDATAHDRMSG Msg;
        RT_ZERO(Msg);
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_DATA_HDR, 12);
        Msg.uContext.SetUInt32(0);                           /** @todo Not used yet. */
        Msg.uFlags.SetUInt32(0);                             /** @todo Not used yet. */
        Msg.uScreenId.SetUInt32(0);                          /** @todo Not used for guest->host (yet). */
        Msg.cbTotal.SetUInt64(pDataHdr->cbTotal);
        Msg.cbMeta.SetUInt32(pDataHdr->cbMeta);
        Msg.pvMetaFmt.SetPtr(pDataHdr->pvMetaFmt, pDataHdr->cbMetaFmt);
        Msg.cbMetaFmt.SetUInt32(pDataHdr->cbMetaFmt);
        Msg.cObjects.SetUInt64(pDataHdr->cObjects);
        Msg.enmCompression.SetUInt32(0);                     /** @todo Not used yet. */
        Msg.enmChecksumType.SetUInt32(RTDIGESTTYPE_INVALID); /** @todo Not used yet. */
        Msg.pvChecksum.SetPtr(NULL, 0);                      /** @todo Not used yet. */
        Msg.cbChecksum.SetUInt32(0);                         /** @todo Not used yet. */

        rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));

        LogFlowFunc(("cbTotal=%RU64, cbMeta=%RU32, cObjects=%RU64, rc=%Rrc\n",
                     pDataHdr->cbTotal, pDataHdr->cbMeta, pDataHdr->cObjects, rc));
    }

    if (RT_SUCCESS(rc))
    {
        VBOXDNDGHSENDDATAMSG Msg;
        RT_ZERO(Msg);
        if (pCtx->uProtocol >= 3)
        {
            VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_DATA, 5);
            Msg.u.v3.uContext.SetUInt32(0);      /** @todo Not used yet. */
            Msg.u.v3.pvChecksum.SetPtr(NULL, 0); /** @todo Not used yet. */
            Msg.u.v3.cbChecksum.SetUInt32(0);    /** @todo Not used yet. */
        }
        else
        {
            VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_DATA, 2);

            /* Total amount of bytes to send (meta data + all directory/file objects). */
            /* Note: Only supports uint32_t, so this is *not* a typo. */
            Msg.u.v1.cbTotalBytes.SetUInt32((uint32_t)pDataHdr->cbTotal);
        }

        uint32_t       cbCurChunk;
        const uint32_t cbMaxChunk = pCtx->cbMaxChunkSize;
        uint32_t       cbSent     = 0;

        HGCMFunctionParameter *pParm = (pCtx->uProtocol >= 3)
                                     ? &Msg.u.v3.pvData
                                     : &Msg.u.v1.pvData;
        while (cbSent < cbData)
        {
            cbCurChunk = RT_MIN(cbData - cbSent, cbMaxChunk);
            pParm->SetPtr(static_cast<uint8_t *>(pvData) + cbSent, cbCurChunk);
            if (pCtx->uProtocol > 2)
                Msg.u.v3.cbData.SetUInt32(cbCurChunk);

            rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
            if (RT_FAILURE(rc))
                break;

            cbSent += cbCurChunk;
        }

        LogFlowFunc(("cbMaxChunk=%RU32, cbData=%RU64, cbSent=%RU32, rc=%Rrc\n",
                     cbMaxChunk, cbData, cbSent, rc));

        if (RT_SUCCESS(rc))
            Assert(cbSent == cbData);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDGHSendDir(PVBGLR3GUESTDNDCMDCTX pCtx, DnDURIObject *pObj)
{
    AssertPtrReturn(pObj,                                    VERR_INVALID_POINTER);
    AssertPtrReturn(pCtx,                                    VERR_INVALID_POINTER);
    AssertReturn(pObj->GetType() == DnDURIObject::Directory, VERR_INVALID_PARAMETER);

    RTCString strPath = pObj->GetDestPath();
    LogFlowFunc(("strDir=%s (%zu), fMode=0x%x\n",
                 strPath.c_str(), strPath.length(), pObj->GetMode()));

    if (strPath.length() > RTPATH_MAX)
        return VERR_INVALID_PARAMETER;

    const uint32_t cbPath = (uint32_t)strPath.length() + 1; /* Include termination. */

    VBOXDNDGHSENDDIRMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_DIR, 3);
        Msg.u.v1.pvName.SetPtr((void *)strPath.c_str(), (uint32_t)cbPath);
        Msg.u.v1.cbName.SetUInt32((uint32_t)cbPath);
        Msg.u.v1.fMode.SetUInt32(pObj->GetMode());
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_DIR, 4);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.pvName.SetPtr((void *)strPath.c_str(), (uint32_t)cbPath);
        Msg.u.v3.cbName.SetUInt32((uint32_t)cbPath);
        Msg.u.v3.fMode.SetUInt32(pObj->GetMode());
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDGHSendFile(PVBGLR3GUESTDNDCMDCTX pCtx, DnDURIObject *pObj)
{
    AssertPtrReturn(pCtx,                               VERR_INVALID_POINTER);
    AssertPtrReturn(pObj,                               VERR_INVALID_POINTER);
    AssertReturn(pObj->GetType() == DnDURIObject::File, VERR_INVALID_PARAMETER);
    AssertReturn(pObj->IsOpen(),                        VERR_INVALID_STATE);

    uint32_t cbBuf = _64K;           /** @todo Make this configurable? */
    void *pvBuf = RTMemAlloc(cbBuf); /** @todo Make this buffer part of PVBGLR3GUESTDNDCMDCTX? */
    if (!pvBuf)
        return VERR_NO_MEMORY;

    int rc;

    RTCString strPath = pObj->GetDestPath();

    LogFlowFunc(("strFile=%s (%zu), cbSize=%RU64, fMode=0x%x\n", strPath.c_str(), strPath.length(),
                 pObj->GetSize(), pObj->GetMode()));
    LogFlowFunc(("uProtocol=%RU32, uClientID=%RU32\n", pCtx->uProtocol, pCtx->uClientID));

    if (pCtx->uProtocol >= 2) /* Protocol version 2 and up sends a file header first. */
    {
        VBOXDNDGHSENDFILEHDRMSG MsgHdr;
        RT_ZERO(MsgHdr);
        VBGL_HGCM_HDR_INIT(&MsgHdr.hdr, pCtx->uClientID, GUEST_DND_GH_SND_FILE_HDR, 6);
        MsgHdr.uContext.SetUInt32(0);                                                    /* Context ID; unused at the moment. */
        MsgHdr.pvName.SetPtr((void *)strPath.c_str(), (uint32_t)(strPath.length() + 1));
        MsgHdr.cbName.SetUInt32((uint32_t)(strPath.length() + 1));
        MsgHdr.uFlags.SetUInt32(0);                                                      /* Flags; unused at the moment. */
        MsgHdr.fMode.SetUInt32(pObj->GetMode());                                         /* File mode */
        MsgHdr.cbTotal.SetUInt64(pObj->GetSize());                                       /* File size (in bytes). */

        rc = VbglR3HGCMCall(&MsgHdr.hdr, sizeof(MsgHdr));

        LogFlowFunc(("Sending file header resulted in %Rrc\n", rc));
    }
    else
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
    {
        /*
         * Send the actual file data, chunk by chunk.
         */
        VBOXDNDGHSENDFILEDATAMSG Msg;
        RT_ZERO(Msg);
        switch (pCtx->uProtocol)
        {
            case 3:
            {
                VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_FILE_DATA, 5);
                Msg.u.v3.uContext.SetUInt32(0);
                Msg.u.v3.pvChecksum.SetPtr(NULL, 0);
                Msg.u.v3.cbChecksum.SetUInt32(0);
                break;
            }

            case 2:
            {
                VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_FILE_DATA, 3);
                Msg.u.v2.uContext.SetUInt32(0);
                break;
            }

            default: /* Protocol v1 */
            {
                VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_SND_FILE_DATA, 5);
                Msg.u.v1.pvName.SetPtr((void *)strPath.c_str(), (uint32_t)(strPath.length() + 1));
                Msg.u.v1.cbName.SetUInt32((uint32_t)(strPath.length() + 1));
                Msg.u.v1.fMode.SetUInt32(pObj->GetMode());
                break;
            }
        }

        uint64_t cbToReadTotal  = pObj->GetSize();
        uint64_t cbWrittenTotal = 0;
        while (cbToReadTotal)
        {
            uint32_t cbToRead = RT_MIN(cbToReadTotal, cbBuf);
            uint32_t cbRead   = 0;
            if (cbToRead)
                rc = pObj->Read(pvBuf, cbToRead, &cbRead);

            LogFlowFunc(("cbToReadTotal=%RU64, cbToRead=%RU32, cbRead=%RU32, rc=%Rrc\n",
                         cbToReadTotal, cbToRead, cbRead, rc));

            if (   RT_SUCCESS(rc)
                && cbRead)
            {
                switch (pCtx->uProtocol)
                {
                    case 3:
                    {
                        Msg.u.v3.pvData.SetPtr(pvBuf, cbRead);
                        Msg.u.v3.cbData.SetUInt32(cbRead);
                        /** @todo Calculate + set checksums. */
                        break;
                    }

                    case 2:
                    {
                        Msg.u.v2.pvData.SetPtr(pvBuf, cbRead);
                        Msg.u.v2.cbData.SetUInt32(cbRead);
                        break;
                    }

                    default:
                    {
                        Msg.u.v1.pvData.SetPtr(pvBuf, cbRead);
                        Msg.u.v1.cbData.SetUInt32(cbRead);
                        break;
                    }
                }

                rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
            }

            if (RT_FAILURE(rc))
            {
                LogFlowFunc(("Reading failed with rc=%Rrc\n", rc));
                break;
            }

            Assert(cbRead  <= cbToReadTotal);
            cbToReadTotal  -= cbRead;
            cbWrittenTotal += cbRead;

            LogFlowFunc(("%RU64/%RU64 -- %RU8%%\n", cbWrittenTotal, pObj->GetSize(), cbWrittenTotal * 100 / pObj->GetSize()));
        };
    }

    RTMemFree(pvBuf);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int vbglR3DnDGHSendURIObject(PVBGLR3GUESTDNDCMDCTX pCtx, DnDURIObject *pObj)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pObj, VERR_INVALID_POINTER);

    int rc;

    switch (pObj->GetType())
    {
        case DnDURIObject::Directory:
            rc = vbglR3DnDGHSendDir(pCtx, pObj);
            break;

        case DnDURIObject::File:
            rc = vbglR3DnDGHSendFile(pCtx, pObj);
            break;

        default:
            AssertMsgFailed(("Object type %ld not implemented\n", pObj->GetType()));
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    return rc;
}

static int vbglR3DnDGHSendRawData(PVBGLR3GUESTDNDCMDCTX pCtx, void *pvData, size_t cbData)
{
    AssertPtrReturn(pCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    /* cbData can be 0. */

    VBOXDNDDATAHDR dataHdr;
    RT_ZERO(dataHdr);

    /* For raw data only the total size is required to be specified. */
    dataHdr.cbTotal = cbData;

    return vbglR3DnDGHSendDataInternal(pCtx, pvData, cbData, &dataHdr);
}

static int vbglR3DnDGHSendURIData(PVBGLR3GUESTDNDCMDCTX pCtx, const void *pvData, size_t cbData)
{
    AssertPtrReturn(pCtx,   VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData,    VERR_INVALID_PARAMETER);

    RTCList<RTCString> lstPaths =
        RTCString((const char *)pvData, cbData).split("\r\n");

    /** @todo Add symlink support (DNDURILIST_FLAGS_RESOLVE_SYMLINKS) here. */
    /** @todo Add lazy loading (DNDURILIST_FLAGS_LAZY) here. */
    uint32_t fFlags = DNDURILIST_FLAGS_KEEP_OPEN;

    DnDURIList lstURI;
    int rc = lstURI.AppendURIPathsFromList(lstPaths, fFlags);
    if (RT_SUCCESS(rc))
    {
        /*
         * Send the (meta) data; in case of URIs it's the (non-recursive) file/directory
         * URI list the host needs to know upfront to set up the drag'n drop operation.
         */
        RTCString strRootDest = lstURI.RootToString();
        if (strRootDest.isNotEmpty())
        {
            void *pvURIList  = (void *)strRootDest.c_str(); /* URI root list. */
            uint32_t cbURLIist = (uint32_t)strRootDest.length() + 1; /* Include string termination. */

            /* The total size also contains the size of the meta data. */
            uint64_t cbTotal  = cbURLIist;
                     cbTotal += lstURI.TotalBytes();

            /* We're going to send an URI list in text format. */
            const char     szMetaFmt[] = "text/uri-list";
            const uint32_t cbMetaFmt   = (uint32_t)strlen(szMetaFmt) + 1; /* Include termination. */

            VBOXDNDDATAHDR dataHdr;
            dataHdr.uFlags    = 0; /* Flags not used yet. */
            dataHdr.cbTotal   = cbTotal;
            dataHdr.cbMeta    = cbURLIist;
            dataHdr.pvMetaFmt = (void *)szMetaFmt;
            dataHdr.cbMetaFmt = cbMetaFmt;
            dataHdr.cObjects  = lstURI.TotalCount();

            rc = vbglR3DnDGHSendDataInternal(pCtx,
                                             pvURIList, cbURLIist, &dataHdr);
        }
        else
            rc = VERR_INVALID_PARAMETER;
    }

    if (RT_SUCCESS(rc))
    {
        while (!lstURI.IsEmpty())
        {
            DnDURIObject *pNextObj = lstURI.First();

            rc = vbglR3DnDGHSendURIObject(pCtx, pNextObj);
            if (RT_FAILURE(rc))
                break;

            lstURI.RemoveFirst();
        }
    }

    return rc;
}

VBGLR3DECL(int) VbglR3DnDGHSendData(PVBGLR3GUESTDNDCMDCTX pCtx, const char *pszFormat, void *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx,      VERR_INVALID_POINTER);
    AssertPtrReturn(pszFormat, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData,    VERR_INVALID_POINTER);
    AssertReturn(cbData,       VERR_INVALID_PARAMETER);

    LogFlowFunc(("pszFormat=%s, pvData=%p, cbData=%RU32\n", pszFormat, pvData, cbData));

    int rc;
    if (DnDMIMEHasFileURLs(pszFormat, strlen(pszFormat)))
    {
        /* Send file data. */
        rc = vbglR3DnDGHSendURIData(pCtx, pvData, cbData);
    }
    else
        rc = vbglR3DnDGHSendRawData(pCtx, pvData, cbData);

    if (RT_FAILURE(rc))
    {
        int rc2 = VbglR3DnDGHSendError(pCtx, rc);
        if (RT_FAILURE(rc2))
            LogFlowFunc(("Unable to send error (%Rrc) to host, rc=%Rrc\n", rc, rc2));
    }

    return rc;
}

VBGLR3DECL(int) VbglR3DnDGHSendError(PVBGLR3GUESTDNDCMDCTX pCtx, int rcErr)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    VBOXDNDGHEVTERRORMSG Msg;
    RT_ZERO(Msg);
    if (pCtx->uProtocol < 3)
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_EVT_ERROR, 1);
        Msg.u.v1.rc.SetUInt32((uint32_t)rcErr); /* uint32_t vs. int. */
    }
    else
    {
        VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_DND_GH_EVT_ERROR, 2);
        /** @todo Context ID not used yet. */
        Msg.u.v3.uContext.SetUInt32(0);
        Msg.u.v3.rc.SetUInt32((uint32_t)rcErr); /* uint32_t vs. int. */
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));

    /*
     * Never return an error if the host did not accept the error at the current
     * time.  This can be due to the host not having any appropriate callbacks
     * set which would handle that error.
     *
     * bird: Looks like VERR_NOT_SUPPORTED is what the host will return if it
     *       doesn't an appropriate callback.  The code used to ignore ALL errors
     *       the host would return, also relevant ones.
     */
    if (RT_FAILURE(rc))
        LogFlowFunc(("Sending error %Rrc failed with rc=%Rrc\n", rcErr, rc));
    if (rc == VERR_NOT_SUPPORTED)
        rc = VINF_SUCCESS;

    return rc;
}

#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

