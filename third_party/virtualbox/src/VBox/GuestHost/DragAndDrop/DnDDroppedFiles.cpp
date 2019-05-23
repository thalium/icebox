/* $Id: DnDDroppedFiles.cpp $ */
/** @file
 * DnD: Directory handling.
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

#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/GuestHost/DragAndDrop.h>

#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

DnDDroppedFiles::DnDDroppedFiles(void)
    : m_fOpen(0)
    , m_hDir(NULL) { }

DnDDroppedFiles::DnDDroppedFiles(const char *pszPath, uint32_t fFlags)
    : m_fOpen(0)
    , m_hDir(NULL)
{
    OpenEx(pszPath, fFlags);
}

DnDDroppedFiles::~DnDDroppedFiles(void)
{
    /* Only make sure to not leak any handles and stuff, don't delete any
     * directories / files here. */
    closeInternal();
}

int DnDDroppedFiles::AddFile(const char *pszFile)
{
    AssertPtrReturn(pszFile, VERR_INVALID_POINTER);

    if (!this->m_lstFiles.contains(pszFile))
        this->m_lstFiles.append(pszFile);
    return VINF_SUCCESS;
}

int DnDDroppedFiles::AddDir(const char *pszDir)
{
    AssertPtrReturn(pszDir, VERR_INVALID_POINTER);

    if (!this->m_lstDirs.contains(pszDir))
        this->m_lstDirs.append(pszDir);
    return VINF_SUCCESS;
}

int DnDDroppedFiles::closeInternal(void)
{
    int rc;
    if (this->m_hDir != NULL)
    {
        rc = RTDirClose(this->m_hDir);
        if (RT_SUCCESS(rc))
            this->m_hDir = NULL;
    }
    else
        rc = VINF_SUCCESS;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int DnDDroppedFiles::Close(void)
{
    return closeInternal();
}

const char *DnDDroppedFiles::GetDirAbs(void) const
{
    return this->m_strPathAbs.c_str();
}

bool DnDDroppedFiles::IsOpen(void) const
{
    return (this->m_hDir != NULL);
}

int DnDDroppedFiles::OpenEx(const char *pszPath, uint32_t fFlags)
{
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(fFlags == 0, VERR_INVALID_PARAMETER); /* Flags not supported yet. */

    int rc;

    do
    {
        char pszDropDir[RTPATH_MAX];
        RTStrPrintf(pszDropDir, sizeof(pszDropDir), "%s", pszPath);

        /** @todo On Windows we also could use the registry to override
         *        this path, on Posix a dotfile and/or a guest property
         *        can be used. */

        /* Append our base drop directory. */
        rc = RTPathAppend(pszDropDir, sizeof(pszDropDir), "VirtualBox Dropped Files"); /** @todo Make this tag configurable? */
        if (RT_FAILURE(rc))
            break;

        /* Create it when necessary. */
        if (!RTDirExists(pszDropDir))
        {
            rc = RTDirCreateFullPath(pszDropDir, RTFS_UNIX_IRWXU);
            if (RT_FAILURE(rc))
                break;
        }

        /* The actually drop directory consist of the current time stamp and a
         * unique number when necessary. */
        char pszTime[64];
        RTTIMESPEC time;
        if (!RTTimeSpecToString(RTTimeNow(&time), pszTime, sizeof(pszTime)))
        {
            rc = VERR_BUFFER_OVERFLOW;
            break;
        }

        rc = DnDPathSanitizeFilename(pszTime, sizeof(pszTime));
        if (RT_FAILURE(rc))
            break;

        rc = RTPathAppend(pszDropDir, sizeof(pszDropDir), pszTime);
        if (RT_FAILURE(rc))
            break;

        /* Create it (only accessible by the current user) */
        rc = RTDirCreateUniqueNumbered(pszDropDir, sizeof(pszDropDir), RTFS_UNIX_IRWXU, 3, '-');
        if (RT_SUCCESS(rc))
        {
            RTDIR hDir;
            rc = RTDirOpen(&hDir, pszDropDir);
            if (RT_SUCCESS(rc))
            {
                this->m_hDir       = hDir;
                this->m_strPathAbs = pszDropDir;
                this->m_fOpen      = fFlags;
            }
        }

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int DnDDroppedFiles::OpenTemp(uint32_t fFlags)
{
    AssertReturn(fFlags == 0, VERR_INVALID_PARAMETER); /* Flags not supported yet. */

    /*
     * Get the user's temp directory. Don't use the user's root directory (or
     * something inside it) because we don't know for how long/if the data will
     * be kept after the guest OS used it.
     */
    char szTemp[RTPATH_MAX];
    int rc = RTPathTemp(szTemp, sizeof(szTemp));
    if (RT_SUCCESS(rc))
        rc = OpenEx(szTemp, fFlags);

    return rc;
}

int DnDDroppedFiles::Reset(bool fRemoveDropDir)
{
    int rc = closeInternal();
    if (RT_SUCCESS(rc))
    {
        if (fRemoveDropDir)
        {
            rc = Rollback();
        }
        else
        {
            this->m_lstDirs.clear();
            this->m_lstFiles.clear();
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int DnDDroppedFiles::Reopen(void)
{
    if (this->m_strPathAbs.isEmpty())
        return VERR_NOT_FOUND;

    return OpenEx(this->m_strPathAbs.c_str(), this->m_fOpen);
}

int DnDDroppedFiles::Rollback(void)
{
    if (this->m_strPathAbs.isEmpty())
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;

    /* Rollback by removing any stuff created.
     * Note: Only remove empty directories, never ever delete
     *       anything recursive here! Steam (tm) knows best ... :-) */
    int rc2;
    for (size_t i = 0; i < this->m_lstFiles.size(); i++)
    {
        rc2 = RTFileDelete(this->m_lstFiles.at(i).c_str());
        if (RT_SUCCESS(rc2))
            this->m_lstFiles.removeAt(i);
        else if (RT_SUCCESS(rc))
           rc = rc2;
        /* Keep going. */
    }

    for (size_t i = 0; i < this->m_lstDirs.size(); i++)
    {
        rc2 = RTDirRemove(this->m_lstDirs.at(i).c_str());
        if (RT_SUCCESS(rc2))
            this->m_lstDirs.removeAt(i);
        else if (RT_SUCCESS(rc))
            rc = rc2;
        /* Keep going. */
    }

    if (RT_SUCCESS(rc))
    {
        Assert(this->m_lstFiles.isEmpty());
        Assert(this->m_lstDirs.isEmpty());

        rc2 = closeInternal();
        if (RT_SUCCESS(rc2))
        {
            /* Try to remove the empty root dropped files directory as well.
             * Might return VERR_DIR_NOT_EMPTY or similar. */
            rc2 = RTDirRemove(this->m_strPathAbs.c_str());
        }
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

