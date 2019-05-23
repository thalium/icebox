/* $Id: DnDURIObject.cpp $ */
/** @file
 * DnD: URI object class. For handling creation/reading/writing to files and directories
 *      on host or guest side.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/fs.h>
#include <iprt/path.h>
#include <iprt/uri.h>

#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

#include <VBox/GuestHost/DragAndDrop.h>

DnDURIObject::DnDURIObject(void)
    : m_Type(Unknown)
    , m_fOpen(false)
    , m_fMode(0)
    , m_cbSize(0)
    , m_cbProcessed(0)
{
    RT_ZERO(u);
}

DnDURIObject::DnDURIObject(Type type,
                           const RTCString &strSrcPath /* = 0 */,
                           const RTCString &strDstPath /* = 0 */,
                           uint32_t fMode  /* = 0 */, uint64_t cbSize  /* = 0 */)
    : m_Type(type)
    , m_strSrcPath(strSrcPath)
    , m_strTgtPath(strDstPath)
    , m_fOpen(false)
    , m_fMode(fMode)
    , m_cbSize(cbSize)
    , m_cbProcessed(0)
{
}

DnDURIObject::~DnDURIObject(void)
{
    closeInternal();
}

void DnDURIObject::closeInternal(void)
{
    LogFlowThisFuncEnter();

    if (!m_fOpen)
        return;

    switch (m_Type)
    {
        case File:
        {
            RTFileClose(u.m_hFile);
            u.m_hFile = NIL_RTFILE;
            break;
        }

        case Directory:
            break;

        default:
            break;
    }

    m_fOpen = false;
}

void DnDURIObject::Close(void)
{
    closeInternal();
}

bool DnDURIObject::IsComplete(void) const
{
    bool fComplete;

    switch (m_Type)
    {
        case File:
            Assert(m_cbProcessed <= m_cbSize);
            fComplete = m_cbProcessed == m_cbSize;
            break;

        case Directory:
            fComplete = true;
            break;

        default:
            fComplete = true;
            break;
    }

    return fComplete;
}

bool DnDURIObject::IsOpen(void) const
{
    return m_fOpen;
}

int DnDURIObject::Open(Dest enmDest, uint64_t fOpen /* = 0 */, uint32_t fMode /* = 0 */)
{
    return OpenEx(  enmDest == Source
                  ? m_strSrcPath : m_strTgtPath
                  , m_Type, enmDest, fOpen, fMode, 0 /* fFlags */);
}

int DnDURIObject::OpenEx(const RTCString &strPath, Type enmType, Dest enmDest,
                         uint64_t fOpen /* = 0 */, uint32_t fMode /* = 0 */, uint32_t fFlags /* = 0 */)
{
    Assert(fFlags == 0); RT_NOREF1(fFlags);
    int rc = VINF_SUCCESS;

    switch (enmDest)
    {
        case Source:
            m_strSrcPath = strPath;
            break;

        case Target:
            m_strTgtPath = strPath;
            break;

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (   RT_SUCCESS(rc)
        && fOpen) /* Opening mode specified? */
    {
        LogFlowThisFunc(("enmType=%RU32, strPath=%s, fOpen=0x%x, enmType=%RU32, enmDest=%RU32\n",
                         enmType, strPath.c_str(), fOpen, enmType, enmDest));
        switch (enmType)
        {
            case File:
            {
                if (!m_fOpen)
                {
                    /*
                     * Open files on the source with RTFILE_O_DENY_WRITE to prevent races
                     * where the OS writes to the file while the destination side transfers
                     * it over.
                     */
                    LogFlowThisFunc(("Opening ...\n"));
                    rc = RTFileOpen(&u.m_hFile, strPath.c_str(), fOpen);
                    if (RT_SUCCESS(rc))
                        rc = RTFileGetSize(u.m_hFile, &m_cbSize);

                    if (RT_SUCCESS(rc))
                    {
                        if (   (fOpen & RTFILE_O_WRITE) /* Only set the file mode on write. */
                            &&  fMode                   /* Some file mode to set specified? */)
                        {
                            rc = RTFileSetMode(u.m_hFile, fMode);
                            if (RT_SUCCESS(rc))
                                m_fMode = fMode;
                        }
                        else if (fOpen & RTFILE_O_READ)
                        {
#if 0 /** @todo Enable this as soon as RTFileGetMode is implemented. */
                            rc = RTFileGetMode(u.m_hFile, &m_fMode);
#else
                            RTFSOBJINFO ObjInfo;
                            rc = RTFileQueryInfo(u.m_hFile, &ObjInfo, RTFSOBJATTRADD_NOTHING);
                            if (RT_SUCCESS(rc))
                                m_fMode = ObjInfo.Attr.fMode;
#endif
                        }
                    }

                    if (RT_SUCCESS(rc))
                    {
                        LogFlowThisFunc(("cbSize=%RU64, fMode=0x%x\n", m_cbSize, m_fMode));
                        m_cbProcessed = 0;
                    }
                }
                else
                    rc = VINF_SUCCESS;

                break;
            }

            case Directory:
                rc = VINF_SUCCESS;
                break;

            default:
                rc = VERR_NOT_IMPLEMENTED;
                break;
        }
    }

    if (RT_SUCCESS(rc))
    {
        m_Type  = enmType;
        m_fOpen = true;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* static */
/** @todo Put this into an own class like DnDURIPath : public RTCString? */
int DnDURIObject::RebaseURIPath(RTCString &strPath,
                                const RTCString &strBaseOld /* = "" */,
                                const RTCString &strBaseNew /* = "" */)
{
    char *pszPath = RTUriFilePath(strPath.c_str());
    if (!pszPath) /* No URI? */
         pszPath = RTStrDup(strPath.c_str());

    int rc;

    if (pszPath)
    {
        const char *pszPathStart = pszPath;
        const char *pszBaseOld = strBaseOld.c_str();
        if (   pszBaseOld
            && RTPathStartsWith(pszPath, pszBaseOld))
        {
            pszPathStart += strlen(pszBaseOld);
        }

        rc = VINF_SUCCESS;

        if (RT_SUCCESS(rc))
        {
            char *pszPathNew = RTPathJoinA(strBaseNew.c_str(), pszPathStart);
            if (pszPathNew)
            {
                char *pszPathURI = RTUriCreate("file" /* pszScheme */, "/" /* pszAuthority */,
                                               pszPathNew /* pszPath */,
                                               NULL /* pszQuery */, NULL /* pszFragment */);
                if (pszPathURI)
                {
                    LogFlowFunc(("Rebasing \"%s\" to \"%s\"\n", strPath.c_str(), pszPathURI));

                    strPath = RTCString(pszPathURI) + "\r\n";
                    RTStrFree(pszPathURI);
                }
                else
                    rc = VERR_INVALID_PARAMETER;

                RTStrFree(pszPathNew);
            }
            else
                rc = VERR_NO_MEMORY;
        }

        RTStrFree(pszPath);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

int DnDURIObject::Read(void *pvBuf, size_t cbBuf, uint32_t *pcbRead)
{
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf, VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    size_t cbRead = 0;

    int rc;
    switch (m_Type)
    {
        case File:
        {
            rc = OpenEx(m_strSrcPath, File, Source,
                        /* Use some sensible defaults. */
                        RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, 0 /* fFlags */);
            if (RT_SUCCESS(rc))
            {
                rc = RTFileRead(u.m_hFile, pvBuf, cbBuf, &cbRead);
                if (RT_SUCCESS(rc))
                {
                    m_cbProcessed += cbRead;
                    Assert(m_cbProcessed <= m_cbSize);

                    /* End of file reached or error occurred? */
                    if (   m_cbSize
                        && m_cbProcessed == m_cbSize)
                    {
                        rc = VINF_EOF;
                    }
                }
            }

            break;
        }

        case Directory:
        {
            rc = VINF_SUCCESS;
            break;
        }

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        if (pcbRead)
            *pcbRead = (uint32_t)cbRead;
    }

    LogFlowFunc(("Returning strSourcePath=%s, cbRead=%zu, rc=%Rrc\n", m_strSrcPath.c_str(), cbRead, rc));
    return rc;
}

void DnDURIObject::Reset(void)
{
    LogFlowThisFuncEnter();

    Close();

    m_Type        = Unknown;
    m_strSrcPath  = "";
    m_strTgtPath  = "";
    m_fMode       = 0;
    m_cbSize      = 0;
    m_cbProcessed = 0;
}

int DnDURIObject::Write(const void *pvBuf, size_t cbBuf, uint32_t *pcbWritten)
{
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf, VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    size_t cbWritten = 0;

    int rc;
    switch (m_Type)
    {
        case File:
        {
            rc = OpenEx(m_strTgtPath, File, Target,
                        /* Use some sensible defaults. */
                        RTFILE_O_OPEN_CREATE | RTFILE_O_DENY_WRITE | RTFILE_O_WRITE, 0 /* fFlags */);
            if (RT_SUCCESS(rc))
            {
                rc = RTFileWrite(u.m_hFile, pvBuf, cbBuf, &cbWritten);
                if (RT_SUCCESS(rc))
                    m_cbProcessed += cbWritten;
            }
            break;
        }

        case Directory:
        {
            rc = VINF_SUCCESS;
            break;
        }

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        if (pcbWritten)
            *pcbWritten = (uint32_t)cbWritten;
    }

    LogFlowThisFunc(("Returning strSourcePath=%s, cbWritten=%zu, rc=%Rrc\n", m_strSrcPath.c_str(), cbWritten, rc));
    return rc;
}

