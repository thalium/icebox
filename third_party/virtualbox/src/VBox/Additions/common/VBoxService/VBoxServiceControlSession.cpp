/* $Id: VBoxServiceControlSession.cpp $ */
/** @file
 * VBoxServiceControlSession - Guest session handling. Also handles the spawned session processes.
 */

/*
 * Copyright (C) 2013-2016 Oracle Corporation
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
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/handle.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/pipe.h>
#include <iprt/poll.h>
#include <iprt/process.h>

#include "VBoxServiceInternal.h"
#include "VBoxServiceUtils.h"
#include "VBoxServiceControl.h"

using namespace guestControl;


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Generic option indices for session spawn arguments. */
enum
{
    VBOXSERVICESESSIONOPT_FIRST = 1000, /* For initialization. */
    VBOXSERVICESESSIONOPT_DOMAIN,
#ifdef DEBUG
    VBOXSERVICESESSIONOPT_DUMP_STDOUT,
    VBOXSERVICESESSIONOPT_DUMP_STDERR,
#endif
    VBOXSERVICESESSIONOPT_LOG_FILE,
    VBOXSERVICESESSIONOPT_USERNAME,
    VBOXSERVICESESSIONOPT_SESSION_ID,
    VBOXSERVICESESSIONOPT_SESSION_PROTO,
    VBOXSERVICESESSIONOPT_THREAD_ID
};



static int vgsvcGstCtrlSessionFileDestroy(PVBOXSERVICECTRLFILE pFile)
{
    AssertPtrReturn(pFile, VERR_INVALID_POINTER);

    int rc = RTFileClose(pFile->hFile);
    if (RT_SUCCESS(rc))
    {
        /* Remove file entry in any case. */
        RTListNodeRemove(&pFile->Node);
        /* Destroy this object. */
        RTMemFree(pFile);
    }

    return rc;
}


/** @todo No locking done yet! */
static PVBOXSERVICECTRLFILE vgsvcGstCtrlSessionFileGetLocked(const PVBOXSERVICECTRLSESSION pSession, uint32_t uHandle)
{
    AssertPtrReturn(pSession, NULL);

    /** @todo Use a map later! */
    PVBOXSERVICECTRLFILE pFileCur;
    RTListForEach(&pSession->lstFiles, pFileCur, VBOXSERVICECTRLFILE, Node)
    {
        if (pFileCur->uHandle == uHandle)
            return pFileCur;
    }

    return NULL;
}


static int vgsvcGstCtrlSessionHandleDirRemove(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    char szDir[RTPATH_MAX];
    uint32_t fFlags = 0;

    int rc = VbglR3GuestCtrlDirGetRemove(pHostCtx,
                                         /* Directory to remove. */
                                         szDir, sizeof(szDir),
                                         /* Flags of type DIRREMOVE_FLAG_. */
                                         &fFlags);
    if (RT_SUCCESS(rc))
    {
        AssertReturn(!(fFlags & ~DIRREMOVE_FLAG_VALID_MASK), VERR_INVALID_PARAMETER);
        if (!(fFlags & ~DIRREMOVE_FLAG_VALID_MASK))
        {
            if (fFlags & DIRREMOVE_FLAG_RECURSIVE)
            {
                uint32_t fFlagsRemRec = RTDIRRMREC_F_CONTENT_AND_DIR; /* Set default. */
                if (fFlags & DIRREMOVE_FLAG_CONTENT_ONLY)
                    fFlagsRemRec |= RTDIRRMREC_F_CONTENT_ONLY;

                rc = RTDirRemoveRecursive(szDir, fFlagsRemRec);
            }
            else /* Only delete directory if not empty. */
                rc = RTDirRemove(szDir);
        }
        else
            rc = VERR_NOT_SUPPORTED;

        VGSvcVerbose(4, "[Dir %s]: Removing with fFlags=0x%x, rc=%Rrc\n", szDir, fFlags, rc);

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlMsgReply(pHostCtx, rc);
        if (RT_FAILURE(rc2))
            VGSvcError("[Dir %s]: Failed to report removing status, rc=%Rrc\n", szDir, rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Removing directory '%s' returned rc=%Rrc\n", szDir, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileOpen(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    char szFile[RTPATH_MAX];
    char szAccess[64];
    char szDisposition[64];
    char szSharing[64];
    uint32_t uCreationMode = 0;
    uint64_t offOpen = 0;
    uint32_t uHandle = 0;

    int rc = VbglR3GuestCtrlFileGetOpen(pHostCtx,
                                        /* File to open. */
                                        szFile, sizeof(szFile),
                                        /* Open mode. */
                                        szAccess, sizeof(szAccess),
                                        /* Disposition. */
                                        szDisposition, sizeof(szDisposition),
                                        /* Sharing. */
                                        szSharing, sizeof(szSharing),
                                        /* Creation mode. */
                                        &uCreationMode,
                                        /* Offset. */
                                        &offOpen);
    VGSvcVerbose(4, "[File %s]: szAccess=%s, szDisposition=%s, szSharing=%s, offOpen=%RU64, rc=%Rrc\n",
                 szFile, szAccess, szDisposition, szSharing, offOpen, rc);
    if (RT_SUCCESS(rc))
    {
        PVBOXSERVICECTRLFILE pFile = (PVBOXSERVICECTRLFILE)RTMemAllocZ(sizeof(VBOXSERVICECTRLFILE));
        if (pFile)
        {
            if (!strlen(szFile))
                rc = VERR_INVALID_PARAMETER;

            if (RT_SUCCESS(rc))
            {
                /** @todo r=bird: Plase, use RTStrCopy for stuff like this! */
                RTStrPrintf(pFile->szName, sizeof(pFile->szName), "%s", szFile);

                uint64_t fFlags;
                rc = RTFileModeToFlagsEx(szAccess, szDisposition, NULL /* pszSharing, not used yet */, &fFlags);
                VGSvcVerbose(4, "[File %s]: Opening with fFlags=0x%x, rc=%Rrc\n", pFile->szName, fFlags, rc);

                if (RT_SUCCESS(rc))
                    rc = RTFileOpen(&pFile->hFile, pFile->szName, fFlags);
                if (   RT_SUCCESS(rc)
                    && offOpen)
                {
                    /* Seeking is optional. However, the whole operation
                     * will fail if we don't succeed seeking to the wanted position. */
                    rc = RTFileSeek(pFile->hFile, (int64_t)offOpen, RTFILE_SEEK_BEGIN, NULL /* Current offset */);
                    if (RT_FAILURE(rc))
                        VGSvcError("[File %s]: Seeking to offset %RU64 failed; rc=%Rrc\n", pFile->szName, offOpen, rc);
                }
                else if (RT_FAILURE(rc))
                    VGSvcError("[File %s]: Opening failed with rc=%Rrc\n", pFile->szName, rc);
            }

            if (RT_SUCCESS(rc))
            {
                uHandle = VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(pHostCtx->uContextID);
                pFile->uHandle = uHandle;

                RTListAppend(&pSession->lstFiles, &pFile->Node);

                VGSvcVerbose(3, "[File %s]: Opened (ID=%RU32)\n", pFile->szName, pFile->uHandle);
            }

            if (RT_FAILURE(rc))
            {
                if (pFile->hFile)
                    RTFileClose(pFile->hFile);
                RTMemFree(pFile);
            }
        }
        else
            rc = VERR_NO_MEMORY;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbOpen(pHostCtx, rc, uHandle);
        if (RT_FAILURE(rc2))
            VGSvcError("[File %s]: Failed to report file open status, rc=%Rrc\n", szFile, rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Opening file '%s' (open mode='%s', disposition='%s', creation mode=0x%x returned rc=%Rrc\n",
                 szFile, szAccess, szDisposition, uCreationMode, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileClose(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    int rc = VbglR3GuestCtrlFileGetClose(pHostCtx, &uHandle /* File handle to close */);
    if (RT_SUCCESS(rc))
    {
        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
            rc = vgsvcGstCtrlSessionFileDestroy(pFile);
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbClose(pHostCtx, rc);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file close status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Closing file '%s' (handle=%RU32) returned rc=%Rrc\n", pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileRead(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                                             void *pvScratchBuf, size_t cbScratchBuf)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    uint32_t cbToRead;
    int rc = VbglR3GuestCtrlFileGetRead(pHostCtx, &uHandle, &cbToRead);
    if (RT_SUCCESS(rc))
    {
        void *pvDataRead = pvScratchBuf;
        size_t cbRead = 0;

        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            if (cbToRead)
            {
                if (cbToRead > cbScratchBuf)
                {
                    pvDataRead = RTMemAlloc(cbToRead);
                    if (!pvDataRead)
                        rc = VERR_NO_MEMORY;
                }

                if (RT_LIKELY(RT_SUCCESS(rc)))
                    rc = RTFileRead(pFile->hFile, pvDataRead, cbToRead, &cbRead);
            }
            else
                rc = VERR_BUFFER_UNDERFLOW;
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbRead(pHostCtx, rc, pvDataRead, (uint32_t)cbRead);
        if (   cbToRead > cbScratchBuf
            && pvDataRead)
            RTMemFree(pvDataRead);

        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file read status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Reading file '%s' (handle=%RU32) returned rc=%Rrc\n", pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileReadAt(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                                               void *pvScratchBuf, size_t cbScratchBuf)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    uint32_t cbToRead;
    uint64_t offReadAt;
    int rc = VbglR3GuestCtrlFileGetReadAt(pHostCtx, &uHandle, &cbToRead, &offReadAt);
    if (RT_SUCCESS(rc))
    {
        void *pvDataRead = pvScratchBuf;
        size_t cbRead = 0;

        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            if (cbToRead)
            {
                if (cbToRead > cbScratchBuf)
                {
                    pvDataRead = RTMemAlloc(cbToRead);
                    if (!pvDataRead)
                        rc = VERR_NO_MEMORY;
                }

                if (RT_SUCCESS(rc))
                    rc = RTFileReadAt(pFile->hFile, (RTFOFF)offReadAt, pvDataRead, cbToRead, &cbRead);
            }
            else
                rc = VERR_BUFFER_UNDERFLOW;
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbRead(pHostCtx, rc, pvDataRead, (uint32_t)cbRead);
        if (   cbToRead > cbScratchBuf
            && pvDataRead)
            RTMemFree(pvDataRead);

        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file read status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Reading file '%s' at offset (handle=%RU32) returned rc=%Rrc\n",
                 pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileWrite(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                                              void *pvScratchBuf, size_t cbScratchBuf)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pvScratchBuf, VERR_INVALID_POINTER);
    AssertPtrReturn(cbScratchBuf, VERR_INVALID_PARAMETER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    uint32_t cbToWrite;
    int rc = VbglR3GuestCtrlFileGetWrite(pHostCtx, &uHandle, pvScratchBuf, (uint32_t)cbScratchBuf, &cbToWrite);
    if (RT_SUCCESS(rc))
    {
        size_t cbWritten = 0;
        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            rc = RTFileWrite(pFile->hFile, pvScratchBuf, cbToWrite, &cbWritten);
#ifdef DEBUG
            VGSvcVerbose(4, "[File %s]: Writing pvScratchBuf=%p, cbToWrite=%RU32, cbWritten=%zu, rc=%Rrc\n",
                         pFile->szName, pvScratchBuf, cbToWrite, cbWritten, rc);
#endif
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbWrite(pHostCtx, rc, (uint32_t)cbWritten);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file write status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Writing file '%s' (handle=%RU32) returned rc=%Rrc\n", pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileWriteAt(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                                                void *pvScratchBuf, size_t cbScratchBuf)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pvScratchBuf, VERR_INVALID_POINTER);
    AssertPtrReturn(cbScratchBuf, VERR_INVALID_PARAMETER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    uint32_t cbToWrite;
    uint64_t offWriteAt;

    int rc = VbglR3GuestCtrlFileGetWriteAt(pHostCtx, &uHandle, pvScratchBuf, (uint32_t)cbScratchBuf, &cbToWrite, &offWriteAt);
    if (RT_SUCCESS(rc))
    {
        size_t cbWritten = 0;
        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            rc = RTFileWriteAt(pFile->hFile, (RTFOFF)offWriteAt, pvScratchBuf, cbToWrite, &cbWritten);
#ifdef DEBUG
            VGSvcVerbose(4, "[File %s]: Writing offWriteAt=%RI64, pvScratchBuf=%p, cbToWrite=%RU32, cbWritten=%zu, rc=%Rrc\n",
                         pFile->szName, offWriteAt, pvScratchBuf, cbToWrite, cbWritten, rc);
#endif
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbWrite(pHostCtx, rc, (uint32_t)cbWritten);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file write status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Writing file '%s' at offset (handle=%RU32) returned rc=%Rrc\n",
                 pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileSeek(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    uint32_t uSeekMethod;
    uint64_t offSeek; /* Will be converted to int64_t. */
    int rc = VbglR3GuestCtrlFileGetSeek(pHostCtx, &uHandle, &uSeekMethod, &offSeek);
    if (RT_SUCCESS(rc))
    {
        uint64_t offActual = 0;
        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            unsigned uSeekMethodIprt;
            switch (uSeekMethod)
            {
                case GUEST_FILE_SEEKTYPE_BEGIN:
                    uSeekMethodIprt = RTFILE_SEEK_BEGIN;
                    break;

                case GUEST_FILE_SEEKTYPE_CURRENT:
                    uSeekMethodIprt = RTFILE_SEEK_CURRENT;
                    break;

                case GUEST_FILE_SEEKTYPE_END:
                    uSeekMethodIprt = RTFILE_SEEK_END;
                    break;

                default:
                    rc = VERR_NOT_SUPPORTED;
                    uSeekMethodIprt = RTFILE_SEEK_BEGIN; /* Shut up MSC */
                    break;
            }

            if (RT_SUCCESS(rc))
            {
                rc = RTFileSeek(pFile->hFile, (int64_t)offSeek, uSeekMethodIprt, &offActual);
#ifdef DEBUG
                VGSvcVerbose(4, "[File %s]: Seeking to offSeek=%RI64, uSeekMethodIPRT=%RU16, rc=%Rrc\n",
                             pFile->szName, offSeek, uSeekMethodIprt, rc);
#endif
            }
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbSeek(pHostCtx, rc, offActual);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file seek status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Seeking file '%s' (handle=%RU32) returned rc=%Rrc\n", pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleFileTell(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    PVBOXSERVICECTRLFILE pFile = NULL;

    uint32_t uHandle = 0;
    int rc = VbglR3GuestCtrlFileGetTell(pHostCtx, &uHandle);
    if (RT_SUCCESS(rc))
    {
        uint64_t off = 0;
        pFile = vgsvcGstCtrlSessionFileGetLocked(pSession, uHandle);
        if (pFile)
        {
            off = RTFileTell(pFile->hFile);
#ifdef DEBUG
            VGSvcVerbose(4, "[File %s]: Telling off=%RU64\n", pFile->szName, off);
#endif
        }
        else
            rc = VERR_NOT_FOUND;

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlFileCbTell(pHostCtx, rc, off);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report file tell status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Telling file '%s' (handle=%RU32) returned rc=%Rrc\n", pFile ? pFile->szName : "<Not found>", uHandle, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandlePathRename(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    char szSource[RTPATH_MAX];
    char szDest[RTPATH_MAX];
    uint32_t fFlags = 0;

    int rc = VbglR3GuestCtrlPathGetRename(pHostCtx,
                                          szSource, sizeof(szSource),
                                          szDest, sizeof(szDest),
                                          /* Flags of type PATHRENAME_FLAG_. */
                                          &fFlags);
    if (RT_SUCCESS(rc))
    {
        if (fFlags & ~PATHRENAME_FLAG_VALID_MASK)
            rc = VERR_NOT_SUPPORTED;

        VGSvcVerbose(4, "Renaming '%s' to '%s', fFlags=0x%x, rc=%Rrc\n", szSource, szDest, fFlags, rc);

        if (RT_SUCCESS(rc))
        {
/** @todo r=bird: shouldn't you use a different variable here for the IPRT flags??? */
            if (fFlags & PATHRENAME_FLAG_NO_REPLACE)
                fFlags |= RTPATHRENAME_FLAGS_NO_REPLACE;

            if (fFlags & PATHRENAME_FLAG_REPLACE)
                fFlags |= RTPATHRENAME_FLAGS_REPLACE;

            if (fFlags & PATHRENAME_FLAG_NO_SYMLINKS)
                fFlags |= RTPATHRENAME_FLAGS_NO_SYMLINKS;

            rc = RTPathRename(szSource, szDest, fFlags);
        }

        /* Report back in any case. */
        int rc2 = VbglR3GuestCtrlMsgReply(pHostCtx, rc);
        if (RT_FAILURE(rc2))
            VGSvcError("Failed to report renaming status, rc=%Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Renaming '%s' to '%s' returned rc=%Rrc\n", szSource, szDest, rc);
#endif
    return rc;
}


/**
 * Handles starting a guest processes.
 *
 * @returns VBox status code.
 * @param   pSession        Guest session.
 * @param   pHostCtx        Host context.
 */
static int vgsvcGstCtrlSessionHandleProcExec(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    bool fStartAllowed = false; /* Flag indicating whether starting a process is allowed or not. */

    switch (pHostCtx->uProtocol)
    {
        case 1: /* Guest Additions < 4.3. */
            if (pHostCtx->uNumParms != 11)
                rc = VERR_NOT_SUPPORTED;
            break;

        case 2: /* Guest Additions >= 4.3. */
            if (pHostCtx->uNumParms != 12)
                rc = VERR_NOT_SUPPORTED;
            break;

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        VBOXSERVICECTRLPROCSTARTUPINFO startupInfo;
        RT_ZERO(startupInfo);

        /* Initialize maximum environment block size -- needed as input
         * parameter to retrieve the stuff from the host. On output this then
         * will contain the actual block size. */
        startupInfo.cbEnv = sizeof(startupInfo.szEnv);

        rc = VbglR3GuestCtrlProcGetStart(pHostCtx,
                                         /* Command */
                                         startupInfo.szCmd,      sizeof(startupInfo.szCmd),
                                         /* Flags */
                                         &startupInfo.uFlags,
                                         /* Arguments */
                                         startupInfo.szArgs,     sizeof(startupInfo.szArgs),    &startupInfo.uNumArgs,
                                         /* Environment */
                                         startupInfo.szEnv,      &startupInfo.cbEnv,            &startupInfo.uNumEnvVars,
                                         /* Credentials; for hosts with VBox < 4.3 (protocol version 1).
                                          * For protocl v2 and up the credentials are part of the session
                                          * opening call. */
                                         startupInfo.szUser,     sizeof(startupInfo.szUser),
                                         startupInfo.szPassword, sizeof(startupInfo.szPassword),
                                         /* Timeout (in ms) */
                                         &startupInfo.uTimeLimitMS,
                                         /* Process priority */
                                         &startupInfo.uPriority,
                                         /* Process affinity */
                                         startupInfo.uAffinity,  sizeof(startupInfo.uAffinity), &startupInfo.uNumAffinity);
        if (RT_SUCCESS(rc))
        {
            VGSvcVerbose(3, "Request to start process szCmd=%s, fFlags=0x%x, szArgs=%s, szEnv=%s, uTimeout=%RU32\n",
                         startupInfo.szCmd, startupInfo.uFlags,
                         startupInfo.uNumArgs ? startupInfo.szArgs : "<None>",
                         startupInfo.uNumEnvVars ? startupInfo.szEnv : "<None>",
                         startupInfo.uTimeLimitMS);

            rc = VGSvcGstCtrlSessionProcessStartAllowed(pSession, &fStartAllowed);
            if (RT_SUCCESS(rc))
            {
                if (fStartAllowed)
                    rc = VGSvcGstCtrlProcessStart(pSession, &startupInfo, pHostCtx->uContextID);
                else
                    rc = VERR_MAX_PROCS_REACHED; /* Maximum number of processes reached. */
            }
        }
    }

    /* In case of an error we need to notify the host to not wait forever for our response. */
    if (RT_FAILURE(rc))
    {
        VGSvcError("Starting process failed with rc=%Rrc, protocol=%RU32, parameters=%RU32\n",
                   rc, pHostCtx->uProtocol, pHostCtx->uNumParms);

        /* Don't report back if we didn't supply sufficient buffer for getting
         * the actual command -- we don't have the matching context ID. */
        if (rc != VERR_TOO_MUCH_DATA)
        {
            /*
             * Note: The context ID can be 0 because we mabye weren't able to fetch the command
             *       from the host. The host in case has to deal with that!
             */
            int rc2 = VbglR3GuestCtrlProcCbStatus(pHostCtx, 0 /* PID, invalid */,
                                                  PROC_STS_ERROR, rc,
                                                  NULL /* pvData */, 0 /* cbData */);
            if (RT_FAILURE(rc2))
                VGSvcError("Error sending start process status to host, rc=%Rrc\n", rc2);
        }
    }

    return rc;
}


/**
 * Sends stdin input to a specific guest process.
 *
 * @returns VBox status code.
 * @param   pSession            The session which is in charge.
 * @param   pHostCtx            The host context to use.
 * @param   pvScratchBuf        The scratch buffer.
 * @param   cbScratchBuf        The scratch buffer size for retrieving the input
 *                              data.
 */
static int vgsvcGstCtrlSessionHandleProcInput(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                                              void *pvScratchBuf, size_t cbScratchBuf)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(cbScratchBuf, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvScratchBuf, VERR_INVALID_POINTER);

    uint32_t uPID;
    uint32_t fFlags;
    uint32_t cbSize;

#if 0 /* unused */
    uint32_t uStatus = INPUT_STS_UNDEFINED; /* Status sent back to the host. */
    uint32_t cbWritten = 0; /* Number of bytes written to the guest. */
#endif

    /*
     * Ask the host for the input data.
     */
    int rc = VbglR3GuestCtrlProcGetInput(pHostCtx, &uPID, &fFlags,
                                         pvScratchBuf, (uint32_t)cbScratchBuf, &cbSize);
    if (RT_FAILURE(rc))
        VGSvcError("Failed to retrieve process input command for PID=%RU32, rc=%Rrc\n", uPID, rc);
    else if (cbSize > cbScratchBuf)
    {
        VGSvcError("Too much process input received, rejecting: uPID=%RU32, cbSize=%RU32, cbScratchBuf=%RU32\n",
                   uPID, cbSize, cbScratchBuf);
        rc = VERR_TOO_MUCH_DATA;
    }
    else
    {
        /*
         * Is this the last input block we need to deliver? Then let the pipe know ...
         */
        bool fPendingClose = false;
        if (fFlags & INPUT_FLAG_EOF)
        {
            fPendingClose = true;
#ifdef DEBUG
            VGSvcVerbose(4, "Got last process input block for PID=%RU32 (%RU32 bytes) ...\n", uPID, cbSize);
#endif
        }

        PVBOXSERVICECTRLPROCESS pProcess = VGSvcGstCtrlSessionRetainProcess(pSession, uPID);
        if (pProcess)
        {
            rc = VGSvcGstCtrlProcessHandleInput(pProcess, pHostCtx, fPendingClose, pvScratchBuf, cbSize);
            if (RT_FAILURE(rc))
                VGSvcError("Error handling input command for PID=%RU32, rc=%Rrc\n", uPID, rc);
            VGSvcGstCtrlProcessRelease(pProcess);
        }
        else
            rc = VERR_NOT_FOUND;
    }

#ifdef DEBUG
    VGSvcVerbose(4, "Setting input for PID=%RU32 resulted in rc=%Rrc\n", uPID, rc);
#endif
    return rc;
}


/**
 * Gets stdout/stderr output of a specific guest process.
 *
 * @returns VBox status code.
 * @param   pSession            The session which is in charge.
 * @param   pHostCtx            The host context to use.
 */
static int vgsvcGstCtrlSessionHandleProcOutput(PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    uint32_t uPID;
    uint32_t uHandleID;
    uint32_t fFlags;

    int rc = VbglR3GuestCtrlProcGetOutput(pHostCtx, &uPID, &uHandleID, &fFlags);
#ifdef DEBUG_andy
    VGSvcVerbose(4, "Getting output for PID=%RU32, CID=%RU32, uHandleID=%RU32, fFlags=%RU32\n",
                 uPID, pHostCtx->uContextID, uHandleID, fFlags);
#endif
    if (RT_SUCCESS(rc))
    {
        PVBOXSERVICECTRLPROCESS pProcess = VGSvcGstCtrlSessionRetainProcess(pSession, uPID);
        if (pProcess)
        {
            rc = VGSvcGstCtrlProcessHandleOutput(pProcess, pHostCtx, uHandleID, _64K /* cbToRead */, fFlags);
            if (RT_FAILURE(rc))
                VGSvcError("Error getting output for PID=%RU32, rc=%Rrc\n", uPID, rc);
            VGSvcGstCtrlProcessRelease(pProcess);
        }
        else
            rc = VERR_NOT_FOUND;
    }

#ifdef DEBUG_andy
    VGSvcVerbose(4, "Getting output for PID=%RU32 resulted in rc=%Rrc\n", uPID, rc);
#endif
    return rc;
}


/**
 * Tells a guest process to terminate.
 *
 * @returns VBox status code.
 * @param   pSession            The session which is in charge.
 * @param   pHostCtx            The host context to use.
 */
static int vgsvcGstCtrlSessionHandleProcTerminate(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    uint32_t uPID;
    int rc = VbglR3GuestCtrlProcGetTerminate(pHostCtx, &uPID);
    if (RT_SUCCESS(rc))
    {
        PVBOXSERVICECTRLPROCESS pProcess = VGSvcGstCtrlSessionRetainProcess(pSession, uPID);
        if (pProcess)
        {
            rc = VGSvcGstCtrlProcessHandleTerm(pProcess);

            VGSvcGstCtrlProcessRelease(pProcess);
        }
        else
            rc = VERR_NOT_FOUND;
    }

#ifdef DEBUG_andy
    VGSvcVerbose(4, "Terminating PID=%RU32 resulted in rc=%Rrc\n", uPID, rc);
#endif
    return rc;
}


static int vgsvcGstCtrlSessionHandleProcWaitFor(const PVBOXSERVICECTRLSESSION pSession, PVBGLR3GUESTCTRLCMDCTX pHostCtx)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);

    uint32_t uPID;
    uint32_t uWaitFlags; uint32_t uTimeoutMS;

    int rc = VbglR3GuestCtrlProcGetWaitFor(pHostCtx, &uPID, &uWaitFlags, &uTimeoutMS);
    if (RT_SUCCESS(rc))
    {
        PVBOXSERVICECTRLPROCESS pProcess = VGSvcGstCtrlSessionRetainProcess(pSession, uPID);
        if (pProcess)
        {
            rc = VERR_NOT_IMPLEMENTED; /** @todo */
            VGSvcGstCtrlProcessRelease(pProcess);
        }
        else
            rc = VERR_NOT_FOUND;
    }

    return rc;
}


int VGSvcGstCtrlSessionHandler(PVBOXSERVICECTRLSESSION pSession, uint32_t uMsg, PVBGLR3GUESTCTRLCMDCTX pHostCtx,
                               void *pvScratchBuf, size_t cbScratchBuf, volatile bool *pfShutdown)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pHostCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pvScratchBuf, VERR_INVALID_POINTER);
    AssertPtrReturn(pfShutdown, VERR_INVALID_POINTER);


    /*
     * Only anonymous sessions (that is, sessions which run with local
     * service privileges) or spawned session processes can do certain
     * operations.
     */
    bool const fImpersonated = RT_BOOL(pSession->fFlags & (  VBOXSERVICECTRLSESSION_FLAG_SPAWN
                                                           | VBOXSERVICECTRLSESSION_FLAG_ANONYMOUS));
    int rc;
    switch (uMsg)
    {
        case HOST_SESSION_CLOSE:
            /* Shutdown (this spawn). */
            rc = VGSvcGstCtrlSessionClose(pSession);
            *pfShutdown = true; /* Shutdown in any case. */
            break;

        case HOST_DIR_REMOVE:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleDirRemove(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_EXEC_CMD:
            rc = vgsvcGstCtrlSessionHandleProcExec(pSession, pHostCtx);
            break;

        case HOST_EXEC_SET_INPUT:
            rc = vgsvcGstCtrlSessionHandleProcInput(pSession, pHostCtx, pvScratchBuf, cbScratchBuf);
            break;

        case HOST_EXEC_GET_OUTPUT:
            rc = vgsvcGstCtrlSessionHandleProcOutput(pSession, pHostCtx);
            break;

        case HOST_EXEC_TERMINATE:
            rc = vgsvcGstCtrlSessionHandleProcTerminate(pSession, pHostCtx);
            break;

        case HOST_EXEC_WAIT_FOR:
            rc = vgsvcGstCtrlSessionHandleProcWaitFor(pSession, pHostCtx);
            break;

        case HOST_FILE_OPEN:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileOpen(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_CLOSE:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileClose(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_READ:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileRead(pSession, pHostCtx, pvScratchBuf, cbScratchBuf);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_READ_AT:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileReadAt(pSession, pHostCtx, pvScratchBuf, cbScratchBuf);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_WRITE:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileWrite(pSession, pHostCtx, pvScratchBuf, cbScratchBuf);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_WRITE_AT:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileWriteAt(pSession, pHostCtx, pvScratchBuf, cbScratchBuf);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_SEEK:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileSeek(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_FILE_TELL:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandleFileTell(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        case HOST_PATH_RENAME:
            if (fImpersonated)
                rc = vgsvcGstCtrlSessionHandlePathRename(pSession, pHostCtx);
            else
                rc = VERR_NOT_SUPPORTED;
            break;

        default:
            rc = VbglR3GuestCtrlMsgSkip(pHostCtx->uClientID);
            VGSvcVerbose(3, "Unsupported message (uMsg=%RU32, cParms=%RU32) from host, skipping\n", uMsg, pHostCtx->uNumParms);
            break;
    }

    if (RT_FAILURE(rc))
        VGSvcError("Error while handling message (uMsg=%RU32, cParms=%RU32), rc=%Rrc\n", uMsg, pHostCtx->uNumParms, rc);

    return rc;
}


/**
 * Thread main routine for a spawned guest session process.
 * This thread runs in the main executable to control the spawned session process.
 *
 * @returns VBox status code.
 * @param   hThreadSelf     Thread handle.
 * @param   pvUser          Pointer to a VBOXSERVICECTRLSESSIONTHREAD structure.
 *
 */
static DECLCALLBACK(int) vgsvcGstCtrlSessionThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PVBOXSERVICECTRLSESSIONTHREAD pThread = (PVBOXSERVICECTRLSESSIONTHREAD)pvUser;
    AssertPtrReturn(pThread, VERR_INVALID_POINTER);

    uint32_t uSessionID = pThread->StartupInfo.uSessionID;

    uint32_t uClientID;
    int rc = VbglR3GuestCtrlConnect(&uClientID);
    if (RT_SUCCESS(rc))
    {
        VGSvcVerbose(3, "Session ID=%RU32 thread running, client ID=%RU32\n", uSessionID, uClientID);

        /* The session thread is not interested in receiving any commands;
         * tell the host service. */
        rc = VbglR3GuestCtrlMsgFilterSet(uClientID, 0 /* Skip all */, 0 /* Filter mask to add */, 0 /* Filter mask to remove */);
        if (RT_FAILURE(rc))
        {
            VGSvcError("Unable to set message filter, rc=%Rrc\n", rc);
            /* Non-critical. */
            rc = VINF_SUCCESS;
        }
    }
    else
        VGSvcError("Error connecting to guest control service, rc=%Rrc\n", rc);

    if (RT_FAILURE(rc))
        pThread->fShutdown = true;

    /* Let caller know that we're done initializing, regardless of the result. */
    int rc2 = RTThreadUserSignal(hThreadSelf);
    AssertRC(rc2);

    if (RT_FAILURE(rc))
        return rc;

    bool fProcessAlive = true;
    RTPROCSTATUS ProcessStatus;
    RT_ZERO(ProcessStatus);

    int rcWait;
    uint32_t uTimeoutsMS = 30 * 1000; /** @todo Make this configurable. Later. */
    uint64_t u64TimeoutStart = 0;

    for (;;)
    {
        rcWait = RTProcWaitNoResume(pThread->hProcess, RTPROCWAIT_FLAGS_NOBLOCK, &ProcessStatus);
        if (RT_UNLIKELY(rcWait == VERR_INTERRUPTED))
            continue;

        if (   rcWait == VINF_SUCCESS
            || rcWait == VERR_PROCESS_NOT_FOUND)
        {
            fProcessAlive = false;
            break;
        }
        AssertMsgBreak(rcWait == VERR_PROCESS_RUNNING,
                       ("Got unexpected rc=%Rrc while waiting for session process termination\n", rcWait));

        if (ASMAtomicReadBool(&pThread->fShutdown))
        {
            if (!u64TimeoutStart)
            {
                VGSvcVerbose(3, "Notifying guest session process (PID=%RU32, session ID=%RU32) ...\n",
                             pThread->hProcess, uSessionID);

                VBGLR3GUESTCTRLCMDCTX hostCtx =
                {
                    /* .idClient  = */  uClientID,
                    /* .idContext = */  VBOX_GUESTCTRL_CONTEXTID_MAKE_SESSION(uSessionID),
                    /* .uProtocol = */  pThread->StartupInfo.uProtocol,
                    /* .cParams   = */  2
                };
                rc = VbglR3GuestCtrlSessionClose(&hostCtx, 0 /* fFlags */);
                if (RT_FAILURE(rc))
                {
                    VGSvcError("Unable to notify guest session process (PID=%RU32, session ID=%RU32), rc=%Rrc\n",
                               pThread->hProcess, uSessionID, rc);

                    if (rc == VERR_NOT_SUPPORTED)
                    {
                        /* Terminate guest session process in case it's not supported by a too old host. */
                        rc = RTProcTerminate(pThread->hProcess);
                        VGSvcVerbose(3, "Terminating guest session process (PID=%RU32) ended with rc=%Rrc\n",
                                     pThread->hProcess, rc);
                    }
                    break;
                }

                VGSvcVerbose(3, "Guest session ID=%RU32 thread was asked to terminate, waiting for session process to exit (%RU32ms timeout) ...\n",
                             uSessionID, uTimeoutsMS);
                u64TimeoutStart = RTTimeMilliTS();
                continue; /* Don't waste time on waiting. */
            }
            if (RTTimeMilliTS() - u64TimeoutStart > uTimeoutsMS)
            {
                 VGSvcVerbose(3, "Guest session ID=%RU32 process did not shut down within time\n", uSessionID);
                 break;
            }
        }

        RTThreadSleep(100); /* Wait a bit. */
    }

    if (!fProcessAlive)
    {
        VGSvcVerbose(2, "Guest session process (ID=%RU32) terminated with rc=%Rrc, reason=%d, status=%d\n",
                     uSessionID, rcWait, ProcessStatus.enmReason, ProcessStatus.iStatus);
        if (ProcessStatus.iStatus == RTEXITCODE_INIT)
        {
            VGSvcError("Guest session process (ID=%RU32) failed to initialize. Here some hints:\n", uSessionID);
            VGSvcError("- Is logging enabled and the output directory is read-only by the guest session user?\n");
            /** @todo Add more here. */
        }
    }

    uint32_t uSessionStatus = GUEST_SESSION_NOTIFYTYPE_UNDEFINED;
    uint32_t uSessionRc = VINF_SUCCESS; /** uint32_t vs. int. */

    if (fProcessAlive)
    {
        for (int i = 0; i < 3; i++)
        {
            VGSvcVerbose(2, "Guest session ID=%RU32 process still alive, killing attempt %d/3\n", uSessionID, i + 1);

            rc = RTProcTerminate(pThread->hProcess);
            if (RT_SUCCESS(rc))
                break;
            /** @todo r=bird: What's the point of sleeping 3 second after the last attempt? */
            RTThreadSleep(3000);
        }

        VGSvcVerbose(2, "Guest session ID=%RU32 process termination resulted in rc=%Rrc\n", uSessionID, rc);

        uSessionStatus = RT_SUCCESS(rc) ? GUEST_SESSION_NOTIFYTYPE_TOK : GUEST_SESSION_NOTIFYTYPE_TOA;
    }
    else if (RT_SUCCESS(rcWait))
    {
        switch (ProcessStatus.enmReason)
        {
            case RTPROCEXITREASON_NORMAL:
                uSessionStatus = GUEST_SESSION_NOTIFYTYPE_TEN;
                break;

            case RTPROCEXITREASON_ABEND:
                uSessionStatus = GUEST_SESSION_NOTIFYTYPE_TEA;
                break;

            case RTPROCEXITREASON_SIGNAL:
                uSessionStatus = GUEST_SESSION_NOTIFYTYPE_TES;
                break;

            default:
                AssertMsgFailed(("Unhandled process termination reason (%d)\n", ProcessStatus.enmReason));
                uSessionStatus = GUEST_SESSION_NOTIFYTYPE_TEA;
                break;
        }
    }
    else
    {
        /* If we didn't find the guest process anymore, just assume it
         * terminated normally. */
        uSessionStatus = GUEST_SESSION_NOTIFYTYPE_TEN;
    }

    VGSvcVerbose(3, "Guest session ID=%RU32 thread ended with sessionStatus=%RU32, sessionRc=%Rrc\n",
                 uSessionID, uSessionStatus, uSessionRc);

    /* Report final status. */
    Assert(uSessionStatus != GUEST_SESSION_NOTIFYTYPE_UNDEFINED);
    VBGLR3GUESTCTRLCMDCTX ctx = { uClientID, VBOX_GUESTCTRL_CONTEXTID_MAKE_SESSION(uSessionID) };
    rc2 = VbglR3GuestCtrlSessionNotify(&ctx, uSessionStatus, uSessionRc);
    if (RT_FAILURE(rc2))
        VGSvcError("Reporting session ID=%RU32 final status failed with rc=%Rrc\n", uSessionID, rc2);

    VbglR3GuestCtrlDisconnect(uClientID);

    VGSvcVerbose(3, "Session ID=%RU32 thread ended with rc=%Rrc\n", uSessionID, rc);
    return rc;
}


static RTEXITCODE vgsvcGstCtrlSessionSpawnWorker(PVBOXSERVICECTRLSESSION pSession)
{
    AssertPtrReturn(pSession, RTEXITCODE_FAILURE);

    bool fSessionFilter = true;

    VGSvcVerbose(0, "Hi, this is guest session ID=%RU32\n", pSession->StartupInfo.uSessionID);

    uint32_t uClientID;
    int rc = VbglR3GuestCtrlConnect(&uClientID);
    if (RT_SUCCESS(rc))
    {
        /* Set session filter. This prevents the guest control
         * host service to send messages which belong to another
         * session we don't want to handle. */
        uint32_t uFilterAdd = VBOX_GUESTCTRL_FILTER_BY_SESSION(pSession->StartupInfo.uSessionID);
        rc = VbglR3GuestCtrlMsgFilterSet(uClientID,
                                         VBOX_GUESTCTRL_CONTEXTID_MAKE_SESSION(pSession->StartupInfo.uSessionID),
                                         uFilterAdd, 0 /* Filter remove */);
        VGSvcVerbose(3, "Setting message filterAdd=0x%x returned %Rrc\n", uFilterAdd, rc);

        if (   RT_FAILURE(rc)
            && rc == VERR_NOT_SUPPORTED)
        {
            /* No session filter available. Skip. */
            fSessionFilter = false;

            rc = VINF_SUCCESS;
        }

        VGSvcVerbose(1, "Using client ID=%RU32\n", uClientID);
    }
    else
        VGSvcError("Error connecting to guest control service, rc=%Rrc\n", rc);

    /* Report started status. */
    VBGLR3GUESTCTRLCMDCTX ctx = { uClientID, VBOX_GUESTCTRL_CONTEXTID_MAKE_SESSION(pSession->StartupInfo.uSessionID) };
    int rc2 = VbglR3GuestCtrlSessionNotify(&ctx, GUEST_SESSION_NOTIFYTYPE_STARTED, VINF_SUCCESS);
    if (RT_FAILURE(rc2))
    {
        VGSvcError("Reporting session ID=%RU32 started status failed with rc=%Rrc\n", pSession->StartupInfo.uSessionID, rc2);

        /*
         * If session status cannot be posted to the host for
         * some reason, bail out.
         */
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    /* Allocate a scratch buffer for commands which also send
     * payload data with them. */
    uint32_t cbScratchBuf = _64K; /** @todo Make buffer size configurable via guest properties/argv! */
    AssertReturn(RT_IS_POWER_OF_TWO(cbScratchBuf), RTEXITCODE_FAILURE);
    uint8_t *pvScratchBuf = NULL;

    if (RT_SUCCESS(rc))
    {
        pvScratchBuf = (uint8_t*)RTMemAlloc(cbScratchBuf);
        if (!pvScratchBuf)
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        bool fShutdown = false;

        VBGLR3GUESTCTRLCMDCTX ctxHost = { uClientID, 0 /* Context ID */, pSession->StartupInfo.uProtocol };
        for (;;)
        {
            VGSvcVerbose(3, "Waiting for host msg ...\n");
            uint32_t uMsg = 0;
            uint32_t cParms = 0;
            rc = VbglR3GuestCtrlMsgWaitFor(uClientID, &uMsg, &cParms);
            if (rc == VERR_TOO_MUCH_DATA)
            {
#ifdef DEBUG
                VGSvcVerbose(4, "Message requires %RU32 parameters, but only 2 supplied -- retrying request (no error!)...\n",
                             cParms);
#endif
                rc = VINF_SUCCESS; /* Try to get "real" message in next block below. */
            }
            else if (RT_FAILURE(rc))
                VGSvcVerbose(3, "Getting host message failed with %Rrc\n", rc); /* VERR_GEN_IO_FAILURE seems to be normal if ran into timeout. */
            if (RT_SUCCESS(rc))
            {
                VGSvcVerbose(4, "Msg=%RU32 (%RU32 parms) retrieved\n", uMsg, cParms);

                /* Set number of parameters for current host context. */
                ctxHost.uNumParms = cParms;

                /* ... and pass it on to the session handler. */
                rc = VGSvcGstCtrlSessionHandler(pSession, uMsg, &ctxHost, pvScratchBuf, cbScratchBuf, &fShutdown);
            }

            if (fShutdown)
                break;

            /* Let others run ... */
            RTThreadYield();
        }
    }

    VGSvcVerbose(0, "Session %RU32 ended\n", pSession->StartupInfo.uSessionID);

    if (pvScratchBuf)
        RTMemFree(pvScratchBuf);

    if (uClientID)
    {
        VGSvcVerbose(3, "Disconnecting client ID=%RU32 ...\n", uClientID);
        VbglR3GuestCtrlDisconnect(uClientID);
    }

    VGSvcVerbose(3, "Session worker returned with rc=%Rrc\n", rc);
    return RT_SUCCESS(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}


/**
 * Finds a (formerly) started guest process given by its PID and increases its
 * reference count.
 *
 * Must be decreased by the caller with VGSvcGstCtrlProcessRelease().
 *
 * @returns Guest process if found, otherwise NULL.
 * @param   pSession    Pointer to guest session where to search process in.
 * @param   uPID        PID to search for.
 *
 * @note    This does *not lock the process!
 */
PVBOXSERVICECTRLPROCESS VGSvcGstCtrlSessionRetainProcess(PVBOXSERVICECTRLSESSION pSession, uint32_t uPID)
{
    AssertPtrReturn(pSession, NULL);

    PVBOXSERVICECTRLPROCESS pProcess = NULL;
    int rc = RTCritSectEnter(&pSession->CritSect);
    if (RT_SUCCESS(rc))
    {
        PVBOXSERVICECTRLPROCESS pCurProcess;
        RTListForEach(&pSession->lstProcesses, pCurProcess, VBOXSERVICECTRLPROCESS, Node)
        {
            if (pCurProcess->uPID == uPID)
            {
                rc = RTCritSectEnter(&pCurProcess->CritSect);
                if (RT_SUCCESS(rc))
                {
                    pCurProcess->cRefs++;
                    rc = RTCritSectLeave(&pCurProcess->CritSect);
                    AssertRC(rc);
                }

                if (RT_SUCCESS(rc))
                    pProcess = pCurProcess;
                break;
            }
        }

        rc = RTCritSectLeave(&pSession->CritSect);
        AssertRC(rc);
    }

    return pProcess;
}


int VGSvcGstCtrlSessionClose(PVBOXSERVICECTRLSESSION pSession)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    VGSvcVerbose(0, "Session %RU32 is about to close ...\n", pSession->StartupInfo.uSessionID);

    int rc = RTCritSectEnter(&pSession->CritSect);
    if (RT_SUCCESS(rc))
    {
        /*
         * Close all guest processes.
         */
        VGSvcVerbose(0, "Stopping all guest processes ...\n");

        /* Signal all guest processes in the active list that we want to shutdown. */
        size_t cProcesses = 0;
        PVBOXSERVICECTRLPROCESS pProcess;
        RTListForEach(&pSession->lstProcesses, pProcess, VBOXSERVICECTRLPROCESS, Node)
        {
            VGSvcGstCtrlProcessStop(pProcess);
            cProcesses++;
        }

        VGSvcVerbose(1, "%zu guest processes were signalled to stop\n", cProcesses);

        /* Wait for all active threads to shutdown and destroy the active thread list. */
        pProcess = RTListGetFirst(&pSession->lstProcesses, VBOXSERVICECTRLPROCESS, Node);
        while (pProcess)
        {
            PVBOXSERVICECTRLPROCESS pNext = RTListNodeGetNext(&pProcess->Node, VBOXSERVICECTRLPROCESS, Node);
            bool fLast = RTListNodeIsLast(&pSession->lstProcesses, &pProcess->Node);

            int rc2 = RTCritSectLeave(&pSession->CritSect);
            AssertRC(rc2);

            rc2 = VGSvcGstCtrlProcessWait(pProcess, 30 * 1000 /* Wait 30 seconds max. */, NULL /* rc */);

            int rc3 = RTCritSectEnter(&pSession->CritSect);
            AssertRC(rc3);

            if (RT_SUCCESS(rc2))
                VGSvcGstCtrlProcessFree(pProcess);

            if (fLast)
                break;

            pProcess = pNext;
        }

#ifdef DEBUG
        pProcess = RTListGetFirst(&pSession->lstProcesses, VBOXSERVICECTRLPROCESS, Node);
        while (pProcess)
        {
            PVBOXSERVICECTRLPROCESS pNext = RTListNodeGetNext(&pProcess->Node, VBOXSERVICECTRLPROCESS, Node);
            bool fLast = RTListNodeIsLast(&pSession->lstProcesses, &pProcess->Node);

            VGSvcVerbose(1, "Process %p (PID %RU32) still in list\n", pProcess, pProcess->uPID);
            if (fLast)
                break;

            pProcess = pNext;
        }
#endif
        AssertMsg(RTListIsEmpty(&pSession->lstProcesses),
                  ("Guest process list still contains entries when it should not\n"));

        /*
         * Close all left guest files.
         */
        VGSvcVerbose(0, "Closing all guest files ...\n");

        PVBOXSERVICECTRLFILE pFile;
        pFile = RTListGetFirst(&pSession->lstFiles, VBOXSERVICECTRLFILE, Node);
        while (pFile)
        {
            PVBOXSERVICECTRLFILE pNext = RTListNodeGetNext(&pFile->Node, VBOXSERVICECTRLFILE, Node);
            bool fLast = RTListNodeIsLast(&pSession->lstFiles, &pFile->Node);

            int rc2 = vgsvcGstCtrlSessionFileDestroy(pFile);
            if (RT_FAILURE(rc2))
            {
                VGSvcError("Unable to close file '%s'; rc=%Rrc\n", pFile->szName, rc2);
                if (RT_SUCCESS(rc))
                    rc = rc2;
                /* Keep going. */
            }

            if (fLast)
                break;

            pFile = pNext;
        }

        AssertMsg(RTListIsEmpty(&pSession->lstFiles), ("Guest file list still contains entries when it should not\n"));

        int rc2 = RTCritSectLeave(&pSession->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}


int VGSvcGstCtrlSessionDestroy(PVBOXSERVICECTRLSESSION pSession)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    int rc = VGSvcGstCtrlSessionClose(pSession);

    /* Destroy critical section. */
    RTCritSectDelete(&pSession->CritSect);

    return rc;
}


int VGSvcGstCtrlSessionInit(PVBOXSERVICECTRLSESSION pSession, uint32_t fFlags)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    RTListInit(&pSession->lstProcesses);
    RTListInit(&pSession->lstFiles);

    pSession->fFlags = fFlags;

    /* Init critical section for protecting the thread lists. */
    int rc = RTCritSectInit(&pSession->CritSect);
    AssertRC(rc);

    return rc;
}


/**
 * Adds a guest process to a session's process list.
 *
 * @return  VBox status code.
 * @param   pSession                Guest session to add process to.
 * @param   pProcess                Guest process to add.
 */
int VGSvcGstCtrlSessionProcessAdd(PVBOXSERVICECTRLSESSION pSession, PVBOXSERVICECTRLPROCESS pProcess)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pProcess, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSession->CritSect);
    if (RT_SUCCESS(rc))
    {
        VGSvcVerbose( 3, "Adding process (PID %RU32) to session ID=%RU32\n", pProcess->uPID, pSession->StartupInfo.uSessionID);

        /* Add process to session list. */
        RTListAppend(&pSession->lstProcesses, &pProcess->Node);

        int rc2 = RTCritSectLeave(&pSession->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return VINF_SUCCESS;
}


/**
 * Removes a guest process from a session's process list.
 *
 * @return  VBox status code.
 * @param   pSession                Guest session to remove process from.
 * @param   pProcess                Guest process to remove.
 */
int VGSvcGstCtrlSessionProcessRemove(PVBOXSERVICECTRLSESSION pSession, PVBOXSERVICECTRLPROCESS pProcess)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pProcess, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSession->CritSect);
    if (RT_SUCCESS(rc))
    {
        VGSvcVerbose(3, "Removing process (PID %RU32) from session ID=%RU32\n", pProcess->uPID, pSession->StartupInfo.uSessionID);
        Assert(pProcess->cRefs == 0);

        RTListNodeRemove(&pProcess->Node);

        int rc2 = RTCritSectLeave(&pSession->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return VINF_SUCCESS;
}


/**
 * Determines whether starting a new guest process according to the
 * maximum number of concurrent guest processes defined is allowed or not.
 *
 * @return  VBox status code.
 * @param   pSession            The guest session.
 * @param   pbAllowed           True if starting (another) guest process
 *                              is allowed, false if not.
 */
int VGSvcGstCtrlSessionProcessStartAllowed(const PVBOXSERVICECTRLSESSION pSession, bool *pbAllowed)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pbAllowed, VERR_INVALID_POINTER);

    int rc = RTCritSectEnter(&pSession->CritSect);
    if (RT_SUCCESS(rc))
    {
        /*
         * Check if we're respecting our memory policy by checking
         * how many guest processes are started and served already.
         */
        bool fLimitReached = false;
        if (pSession->uProcsMaxKept) /* If we allow unlimited processes (=0), take a shortcut. */
        {
            uint32_t uProcsRunning = 0;
            PVBOXSERVICECTRLPROCESS pProcess;
            RTListForEach(&pSession->lstProcesses, pProcess, VBOXSERVICECTRLPROCESS, Node)
                uProcsRunning++;

            VGSvcVerbose(3, "Maximum served guest processes set to %u, running=%u\n", pSession->uProcsMaxKept, uProcsRunning);

            int32_t iProcsLeft = (pSession->uProcsMaxKept - uProcsRunning - 1);
            if (iProcsLeft < 0)
            {
                VGSvcVerbose(3, "Maximum running guest processes reached (%u)\n", pSession->uProcsMaxKept);
                fLimitReached = true;
            }
        }

        *pbAllowed = !fLimitReached;

        int rc2 = RTCritSectLeave(&pSession->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}


/**
 * Creates the process for a guest session.
 *
 *
 * @return  VBox status code.
 * @param   pSessionStartupInfo     Session startup info.
 * @param   pSessionThread          The session thread under construction.
 * @param   uCtrlSessionThread      The session thread debug ordinal.
 */
static int vgsvcVGSvcGstCtrlSessionThreadCreateProcess(const PVBOXSERVICECTRLSESSIONSTARTUPINFO pSessionStartupInfo,
                                                       PVBOXSERVICECTRLSESSIONTHREAD pSessionThread, uint32_t uCtrlSessionThread)
{
    RT_NOREF1(uCtrlSessionThread);

    /*
     * Is this an anonymous session?  Anonymous sessions run with the same
     * privileges as the main VBoxService executable.
     */
    bool const fAnonymous = pSessionThread->StartupInfo.szUser[0] == '\0';
    if (fAnonymous)
    {
        Assert(!strlen(pSessionThread->StartupInfo.szPassword));
        Assert(!strlen(pSessionThread->StartupInfo.szDomain));

        VGSvcVerbose(3, "New anonymous guest session ID=%RU32 created, fFlags=%x, using protocol %RU32\n",
                     pSessionStartupInfo->uSessionID,
                     pSessionStartupInfo->fFlags,
                     pSessionStartupInfo->uProtocol);
    }
    else
    {
        VGSvcVerbose(3, "Spawning new guest session ID=%RU32, szUser=%s, szPassword=%s, szDomain=%s, fFlags=%x, using protocol %RU32\n",
                     pSessionStartupInfo->uSessionID,
                     pSessionStartupInfo->szUser,
#ifdef DEBUG
                     pSessionStartupInfo->szPassword,
#else
                     "XXX", /* Never show passwords in release mode. */
#endif
                     pSessionStartupInfo->szDomain,
                     pSessionStartupInfo->fFlags,
                     pSessionStartupInfo->uProtocol);
    }

    /*
     * Spawn a child process for doing the actual session handling.
     * Start by assembling the argument list.
     */
    int  rc = VINF_SUCCESS;
    char szExeName[RTPATH_MAX];
    char *pszExeName = RTProcGetExecutablePath(szExeName, sizeof(szExeName));
    if (pszExeName)
    {
        char szParmSessionID[32];
        RTStrPrintf(szParmSessionID, sizeof(szParmSessionID), "--session-id=%RU32", pSessionThread->StartupInfo.uSessionID);

        char szParmSessionProto[32];
        RTStrPrintf(szParmSessionProto, sizeof(szParmSessionProto), "--session-proto=%RU32",
                    pSessionThread->StartupInfo.uProtocol);
#ifdef DEBUG
        char szParmThreadId[32];
        RTStrPrintf(szParmThreadId, sizeof(szParmThreadId), "--thread-id=%RU32", uCtrlSessionThread);
#endif
        unsigned    idxArg = 0; /* Next index in argument vector. */
        char const *apszArgs[24];

        apszArgs[idxArg++] = pszExeName;
        apszArgs[idxArg++] = "guestsession";
        apszArgs[idxArg++] = szParmSessionID;
        apszArgs[idxArg++] = szParmSessionProto;
#ifdef DEBUG
        apszArgs[idxArg++] = szParmThreadId;
#endif
        if (!fAnonymous) /* Do we need to pass a user name? */
        {
            apszArgs[idxArg++] = "--user";
            apszArgs[idxArg++] = pSessionThread->StartupInfo.szUser;

            if (strlen(pSessionThread->StartupInfo.szDomain))
            {
                apszArgs[idxArg++] = "--domain";
                apszArgs[idxArg++] = pSessionThread->StartupInfo.szDomain;
            }
        }

        /* Add same verbose flags as parent process. */
        char szParmVerbose[32];
        if (g_cVerbosity > 0)
        {
            unsigned cVs = RT_MIN(g_cVerbosity, RT_ELEMENTS(szParmVerbose) - 2);
            szParmVerbose[0] = '-';
            memset(&szParmVerbose[1], 'v', cVs);
            szParmVerbose[1 + cVs] = '\0';
            apszArgs[idxArg++] = szParmVerbose;
        }

        /* Add log file handling. Each session will have an own
         * log file, naming based on the parent log file. */
        char szParmLogFile[sizeof(g_szLogFile) + 128];
        if (g_szLogFile[0])
        {
            const char *pszSuffix = RTPathSuffix(g_szLogFile);
            if (!pszSuffix)
                pszSuffix = strchr(g_szLogFile, '\0');
            size_t cchBase = pszSuffix - g_szLogFile;
#ifndef DEBUG
            RTStrPrintf(szParmLogFile, sizeof(szParmLogFile), "%.*s-%RU32-%s%s",
                        cchBase, g_szLogFile, pSessionStartupInfo->uSessionID, pSessionStartupInfo->szUser, pszSuffix);
#else
            RTStrPrintf(szParmLogFile, sizeof(szParmLogFile), "%.*s-%RU32-%RU32-%s%s",
                        cchBase, g_szLogFile, pSessionStartupInfo->uSessionID, uCtrlSessionThread,
                        pSessionStartupInfo->szUser, pszSuffix);
#endif
            apszArgs[idxArg++] = "--logfile";
            apszArgs[idxArg++] = szParmLogFile;
        }

#ifdef DEBUG
        VGSvcVerbose(4, "Argv building rc=%Rrc, session flags=%x\n", rc, g_Session.fFlags);
        if (RT_SUCCESS(rc))
        {
            if (g_Session.fFlags & VBOXSERVICECTRLSESSION_FLAG_DUMPSTDOUT)
                apszArgs[idxArg++] = "--dump-stdout";
            if (g_Session.fFlags & VBOXSERVICECTRLSESSION_FLAG_DUMPSTDERR)
                apszArgs[idxArg++] = "--dump-stderr";
        }
#endif
        apszArgs[idxArg] = NULL;
        Assert(idxArg < RT_ELEMENTS(apszArgs));

        if (g_cVerbosity > 3)
        {
            VGSvcVerbose(4, "Spawning parameters:\n");
            for (idxArg = 0; apszArgs[idxArg]; idxArg++)
                VGSvcVerbose(4, "\t%s\n", apszArgs[idxArg]);
        }

        /*
         * Configure standard handles and finally create the process.
         */
        uint32_t fProcCreate = RTPROC_FLAGS_PROFILE;
#ifdef RT_OS_WINDOWS /* Windows only flags: */
        fProcCreate         |= RTPROC_FLAGS_SERVICE
                            |  RTPROC_FLAGS_HIDDEN;       /** @todo More flags from startup info? */
#endif

#if 0 /* Pipe handling not needed (yet). */
        /* Setup pipes. */
        rc = GstcntlProcessSetupPipe("|", 0 /*STDIN_FILENO*/,
                                     &pSession->StdIn.hChild, &pSession->StdIn.phChild, &pSession->hStdInW);
        if (RT_SUCCESS(rc))
        {
            rc = GstcntlProcessSetupPipe("|", 1 /*STDOUT_FILENO*/,
                                         &pSession->StdOut.hChild, &pSession->StdOut.phChild, &pSession->hStdOutR);
            if (RT_SUCCESS(rc))
            {
                rc = GstcntlProcessSetupPipe("|", 2 /*STDERR_FILENO*/,
                                             &pSession->StdErr.hChild, &pSession->StdErr.phChild, &pSession->hStdErrR);
                if (RT_SUCCESS(rc))
                {
                    rc = RTPollSetCreate(&pSession->hPollSet);
                    if (RT_SUCCESS(rc))
                        rc = RTPollSetAddPipe(pSession->hPollSet, pSession->hStdInW, RTPOLL_EVT_ERROR,
                                              VBOXSERVICECTRLPIPEID_STDIN);
                    if (RT_SUCCESS(rc))
                        rc = RTPollSetAddPipe(pSession->hPollSet, pSession->hStdOutR, RTPOLL_EVT_READ | RTPOLL_EVT_ERROR,
                                              VBOXSERVICECTRLPIPEID_STDOUT);
                    if (RT_SUCCESS(rc))
                        rc = RTPollSetAddPipe(pSession->hPollSet, pSession->hStdErrR, RTPOLL_EVT_READ | RTPOLL_EVT_ERROR,
                                              VBOXSERVICECTRLPIPEID_STDERR);
                }

                if (RT_SUCCESS(rc))
                    rc = RTProcCreateEx(pszExeName, apszArgs, hEnv, fProcCreate,
                                        pSession->StdIn.phChild, pSession->StdOut.phChild, pSession->StdErr.phChild,
                                        !fAnonymous ? pSession->StartupInfo.szUser : NULL,
                                        !fAnonymous ? pSession->StartupInfo.szPassword : NULL,
                                        &pSession->hProcess);

                if (RT_SUCCESS(rc))
                {
                    /*
                     * Close the child ends of any pipes and redirected files.
                     */
                    int rc2 = RTHandleClose(pSession->StdIn.phChild); AssertRC(rc2);
                    pSession->StdIn.phChild     = NULL;
                    rc2 = RTHandleClose(pSession->StdOut.phChild);    AssertRC(rc2);
                    pSession->StdOut.phChild    = NULL;
                    rc2 = RTHandleClose(pSession->StdErr.phChild);    AssertRC(rc2);
                    pSession->StdErr.phChild    = NULL;
                }
            }
        }
#else
        if (RT_SUCCESS(rc))
        {
            RTHANDLE hStdIn;
            rc = RTFileOpenBitBucket(&hStdIn.u.hFile, RTFILE_O_READ);
            if (RT_SUCCESS(rc))
            {
                hStdIn.enmType = RTHANDLETYPE_FILE;

                RTHANDLE hStdOutAndErr;
                rc = RTFileOpenBitBucket(&hStdOutAndErr.u.hFile, RTFILE_O_WRITE);
                if (RT_SUCCESS(rc))
                {
                    hStdOutAndErr.enmType = RTHANDLETYPE_FILE;

                    const char *pszUser = pSessionThread->StartupInfo.szUser;
# ifdef RT_OS_WINDOWS
                    /* If a domain name is given, construct an UPN (User Principle Name) with
                     * the domain name built-in, e.g. "joedoe@example.com". */
                    char *pszUserUPN = NULL;
                    if (strlen(pSessionThread->StartupInfo.szDomain))
                    {
                        int cbUserUPN = RTStrAPrintf(&pszUserUPN, "%s@%s",
                                                     pSessionThread->StartupInfo.szUser,
                                                     pSessionThread->StartupInfo.szDomain);
                        if (cbUserUPN > 0)
                        {
                            pszUser = pszUserUPN;
                            VGSvcVerbose(3, "Using UPN: %s\n", pszUserUPN);
                        }
                    }
# endif

                    rc = RTProcCreateEx(pszExeName, apszArgs, RTENV_DEFAULT, fProcCreate,
                                        &hStdIn, &hStdOutAndErr, &hStdOutAndErr,
                                        !fAnonymous ? pszUser : NULL,
                                        !fAnonymous ? pSessionThread->StartupInfo.szPassword : NULL,
                                        &pSessionThread->hProcess);
# ifdef RT_OS_WINDOWS
                    if (pszUserUPN)
                        RTStrFree(pszUserUPN);
# endif
                    RTFileClose(hStdOutAndErr.u.hFile);
                }

                RTFileClose(hStdIn.u.hFile);
            }
        }
#endif
    }
    else
        rc = VERR_FILE_NOT_FOUND;
    return rc;
}


/**
 * Creates a guest session.
 *
 * This will spawn a new VBoxService.exe instance under behalf of the given user
 * which then will act as a session host. On successful open, the session will
 * be added to the given session thread list.
 *
 * @return  VBox status code.
 * @param   pList                   Which list to use to store the session thread in.
 * @param   pSessionStartupInfo     Session startup info.
 * @param   ppSessionThread         Returns newly created session thread on success.
 *                                  Optional.
 */
int VGSvcGstCtrlSessionThreadCreate(PRTLISTANCHOR pList, const PVBOXSERVICECTRLSESSIONSTARTUPINFO pSessionStartupInfo,
                                    PVBOXSERVICECTRLSESSIONTHREAD *ppSessionThread)
{
    AssertPtrReturn(pList, VERR_INVALID_POINTER);
    AssertPtrReturn(pSessionStartupInfo, VERR_INVALID_POINTER);
    /* ppSessionThread is optional. */

#ifdef VBOX_STRICT
    /* Check for existing session in debug mode. Should never happen because of
     * Main consistency. */
    PVBOXSERVICECTRLSESSIONTHREAD pSessionCur;
    RTListForEach(pList, pSessionCur, VBOXSERVICECTRLSESSIONTHREAD, Node)
    {
        AssertMsgReturn(pSessionCur->StartupInfo.uSessionID != pSessionStartupInfo->uSessionID,
                        ("Guest session thread ID=%RU32 (%p) already exists when it should not\n",
                         pSessionCur->StartupInfo.uSessionID, pSessionCur), VERR_ALREADY_EXISTS);
    }
#endif
    int rc = VINF_SUCCESS;

    /* Static counter to help tracking session thread <-> process relations. */
    static uint32_t s_uCtrlSessionThread = 0;
    if (s_uCtrlSessionThread++ == UINT32_MAX)
        s_uCtrlSessionThread = 0; /* Wrap around to not let IPRT freak out. */

    /*
     * Allocate and initialize the session thread structure.
     */
    PVBOXSERVICECTRLSESSIONTHREAD pSessionThread = (PVBOXSERVICECTRLSESSIONTHREAD)RTMemAllocZ(sizeof(*pSessionThread));
    if (pSessionThread)
    {
        /* Copy over session startup info. */
        memcpy(&pSessionThread->StartupInfo, pSessionStartupInfo, sizeof(VBOXSERVICECTRLSESSIONSTARTUPINFO));

        pSessionThread->fShutdown = false;
        pSessionThread->fStarted  = false;
        pSessionThread->fStopped  = false;

        rc = RTCritSectInit(&pSessionThread->CritSect);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            /*
             * Start the session thread.
             */
            rc = vgsvcVGSvcGstCtrlSessionThreadCreateProcess(pSessionStartupInfo, pSessionThread, s_uCtrlSessionThread);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Start the session thread.
                 */
                rc = RTThreadCreateF(&pSessionThread->Thread, vgsvcGstCtrlSessionThread,
                                     pSessionThread /*pvUser*/, 0 /*cbStack*/,
                                     RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "sess%u", s_uCtrlSessionThread);
                if (RT_SUCCESS(rc))
                {
                    /* Wait for the thread to initialize. */
                    rc = RTThreadUserWait(pSessionThread->Thread, RT_MS_1MIN);
                    if (   RT_SUCCESS(rc)
                        && !ASMAtomicReadBool(&pSessionThread->fShutdown))
                    {
                        VGSvcVerbose(2, "Thread for session ID=%RU32 started\n", pSessionThread->StartupInfo.uSessionID);

                        ASMAtomicXchgBool(&pSessionThread->fStarted, true);

                        /* Add session to list. */
                        RTListAppend(pList, &pSessionThread->Node);
                        if (ppSessionThread) /* Return session if wanted. */
                            *ppSessionThread = pSessionThread;
                        return VINF_SUCCESS;
                    }

                    /*
                     * Bail out.
                     */
                    VGSvcError("Thread for session ID=%RU32 failed to start, rc=%Rrc\n",
                               pSessionThread->StartupInfo.uSessionID, rc);
                    if (RT_SUCCESS_NP(rc))
                        rc = VERR_CANT_CREATE; /** @todo Find a better rc. */
                }
                else
                    VGSvcError("Creating session thread failed, rc=%Rrc\n", rc);

                RTProcTerminate(pSessionThread->hProcess);
                uint32_t cMsWait = 1;
                while (   RTProcWait(pSessionThread->hProcess, RTPROCWAIT_FLAGS_NOBLOCK, NULL) == VERR_PROCESS_RUNNING
                       && cMsWait <= 9) /* 1023 ms */
                {
                    RTThreadSleep(cMsWait);
                    cMsWait <<= 1;
                }
            }
            RTCritSectDelete(&pSessionThread->CritSect);
        }
        RTMemFree(pSessionThread);
    }
    else
        rc = VERR_NO_MEMORY;

    VGSvcVerbose(3, "Spawning session thread returned returned rc=%Rrc\n", rc);
    return rc;
}


/**
 * Waits for a formerly opened guest session process to close.
 *
 * @return  VBox status code.
 * @param   pThread                 Guest session thread to wait for.
 * @param   uTimeoutMS              Waiting timeout (in ms).
 * @param   fFlags                  Closing flags.
 */
int VGSvcGstCtrlSessionThreadWait(PVBOXSERVICECTRLSESSIONTHREAD pThread, uint32_t uTimeoutMS, uint32_t fFlags)
{
    RT_NOREF1(fFlags);
    AssertPtrReturn(pThread, VERR_INVALID_POINTER);
    /** @todo Validate closing flags. */

    AssertMsgReturn(pThread->Thread != NIL_RTTHREAD,
                    ("Guest session thread of session %p does not exist when it should\n", pThread),
                    VERR_NOT_FOUND);

    int rc = VINF_SUCCESS;

    /*
     * The spawned session process should have received the same closing request,
     * so just wait for the process to close.
     */
    if (ASMAtomicReadBool(&pThread->fStarted))
    {
        /* Ask the thread to shutdown. */
        ASMAtomicXchgBool(&pThread->fShutdown, true);

        VGSvcVerbose(3, "Waiting for session thread ID=%RU32 to close (%RU32ms) ...\n",
                     pThread->StartupInfo.uSessionID, uTimeoutMS);

        int rcThread;
        rc = RTThreadWait(pThread->Thread, uTimeoutMS, &rcThread);
        if (RT_SUCCESS(rc))
            VGSvcVerbose(3, "Session thread ID=%RU32 ended with rc=%Rrc\n", pThread->StartupInfo.uSessionID, rcThread);
        else
            VGSvcError("Waiting for session thread ID=%RU32 to close failed with rc=%Rrc\n", pThread->StartupInfo.uSessionID, rc);
    }

    return rc;
}

/**
 * Waits for the specified session thread to end and remove
 * it from the session thread list.
 *
 * @return  VBox status code.
 * @param   pThread                 Session thread to destroy.
 * @param   fFlags                  Closing flags.
 */
int VGSvcGstCtrlSessionThreadDestroy(PVBOXSERVICECTRLSESSIONTHREAD pThread, uint32_t fFlags)
{
    AssertPtrReturn(pThread, VERR_INVALID_POINTER);

    int rc = VGSvcGstCtrlSessionThreadWait(pThread, 5 * 60 * 1000 /* 5 minutes timeout */, fFlags);

    /* Remove session from list and destroy object. */
    RTListNodeRemove(&pThread->Node);

    RTMemFree(pThread);
    pThread = NULL;

    return rc;
}

/**
 * Close all open guest session threads.
 *
 * @note    Caller is responsible for locking!
 *
 * @return  VBox status code.
 * @param   pList                   Which list to close the session threads for.
 * @param   fFlags                  Closing flags.
 */
int VGSvcGstCtrlSessionThreadDestroyAll(PRTLISTANCHOR pList, uint32_t fFlags)
{
    AssertPtrReturn(pList, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    /*int rc = VbglR3GuestCtrlClose
        if (RT_FAILURE(rc))
            VGSvcError("Cancelling pending waits failed; rc=%Rrc\n", rc);*/

    PVBOXSERVICECTRLSESSIONTHREAD pSessIt;
    PVBOXSERVICECTRLSESSIONTHREAD pSessItNext;
    RTListForEachSafe(pList, pSessIt, pSessItNext, VBOXSERVICECTRLSESSIONTHREAD, Node)
    {
        int rc2 = VGSvcGstCtrlSessionThreadDestroy(pSessIt, fFlags);
        if (RT_FAILURE(rc2))
        {
            VGSvcError("Closing session thread '%s' failed with rc=%Rrc\n", RTThreadGetName(pSessIt->Thread), rc2);
            if (RT_SUCCESS(rc))
                rc = rc2;
            /* Keep going. */
        }
    }

    VGSvcVerbose(4, "Destroying guest session threads ended with %Rrc\n", rc);
    return rc;
}


/**
 * Main function for the session process.
 *
 * @returns exit code.
 * @param   argc        Argument count.
 * @param   argv        Argument vector (UTF-8).
 */
RTEXITCODE VGSvcGstCtrlSessionSpawnInit(int argc, char **argv)
{
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--domain",          VBOXSERVICESESSIONOPT_DOMAIN,          RTGETOPT_REQ_STRING },
#ifdef DEBUG
        { "--dump-stdout",     VBOXSERVICESESSIONOPT_DUMP_STDOUT,     RTGETOPT_REQ_NOTHING },
        { "--dump-stderr",     VBOXSERVICESESSIONOPT_DUMP_STDERR,     RTGETOPT_REQ_NOTHING },
#endif
        { "--logfile",         VBOXSERVICESESSIONOPT_LOG_FILE,        RTGETOPT_REQ_STRING },
        { "--user",            VBOXSERVICESESSIONOPT_USERNAME,        RTGETOPT_REQ_STRING },
        { "--session-id",      VBOXSERVICESESSIONOPT_SESSION_ID,      RTGETOPT_REQ_UINT32 },
        { "--session-proto",   VBOXSERVICESESSIONOPT_SESSION_PROTO,   RTGETOPT_REQ_UINT32 },
#ifdef DEBUG
        { "--thread-id",       VBOXSERVICESESSIONOPT_THREAD_ID,       RTGETOPT_REQ_UINT32 },
#endif /* DEBUG */
        { "--verbose",         'v',                                   RTGETOPT_REQ_NOTHING }
    };

    int ch;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv,
                 s_aOptions, RT_ELEMENTS(s_aOptions),
                 1 /*iFirst*/, RTGETOPTINIT_FLAGS_OPTS_FIRST);

    uint32_t fSession = VBOXSERVICECTRLSESSION_FLAG_SPAWN;

    /* Protocol and session ID must be specified explicitly. */
    g_Session.StartupInfo.uProtocol  = UINT32_MAX;
    g_Session.StartupInfo.uSessionID = UINT32_MAX;

    while ((ch = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        /* For options that require an argument, ValueUnion has received the value. */
        switch (ch)
        {
            case VBOXSERVICESESSIONOPT_DOMAIN:
                /* Information not needed right now, skip. */
                break;
#ifdef DEBUG
            case VBOXSERVICESESSIONOPT_DUMP_STDOUT:
                fSession |= VBOXSERVICECTRLSESSION_FLAG_DUMPSTDOUT;
                break;

            case VBOXSERVICESESSIONOPT_DUMP_STDERR:
                fSession |= VBOXSERVICECTRLSESSION_FLAG_DUMPSTDERR;
                break;
#endif
            case VBOXSERVICESESSIONOPT_SESSION_ID:
                g_Session.StartupInfo.uSessionID = ValueUnion.u32;
                break;

            case VBOXSERVICESESSIONOPT_SESSION_PROTO:
                g_Session.StartupInfo.uProtocol = ValueUnion.u32;
                break;
#ifdef DEBUG
            case VBOXSERVICESESSIONOPT_THREAD_ID:
                /* Not handled. Mainly for processs listing. */
                break;
#endif
            case VBOXSERVICESESSIONOPT_LOG_FILE:
            {
                int rc = RTStrCopy(g_szLogFile, sizeof(g_szLogFile), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExit(RTEXITCODE_FAILURE, "Error copying log file name: %Rrc", rc);
                break;
            }

            case VBOXSERVICESESSIONOPT_USERNAME:
                /* Information not needed right now, skip. */
                break;

            /** @todo Implement help? */

            case 'v':
                g_cVerbosity++;
                break;

            case VINF_GETOPT_NOT_OPTION:
                /* Ignore; might be "guestsession" main command. */
                /** @todo r=bird: We DO NOT ignore stuff on the command line! */
                break;

            default:
                return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Unknown command '%s'", ValueUnion.psz);
        }
    }

    /* Check that we've got all the required options. */
    if (g_Session.StartupInfo.uProtocol == UINT32_MAX)
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "No protocol version specified");

    if (g_Session.StartupInfo.uSessionID == UINT32_MAX)
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "No session ID specified");

    /* Init the session object. */
    int rc = VGSvcGstCtrlSessionInit(&g_Session, fSession);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_INIT, "Failed to initialize session object, rc=%Rrc\n", rc);

    rc = VGSvcLogCreate(g_szLogFile[0] ? g_szLogFile : NULL);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_INIT, "Failed to create log file '%s', rc=%Rrc\n",
                              g_szLogFile[0] ? g_szLogFile : "<None>", rc);

    RTEXITCODE rcExit = vgsvcGstCtrlSessionSpawnWorker(&g_Session);

    VGSvcLogDestroy();
    return rcExit;
}

