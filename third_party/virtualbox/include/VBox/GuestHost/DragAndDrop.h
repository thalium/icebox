/* $Id: DragAndDrop.h $ */
/** @file
 * DnD: Shared functions between host and guest.
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

#ifndef ___VBox_GuestHost_DragAndDrop_h
#define ___VBox_GuestHost_DragAndDrop_h

#include <iprt/assert.h>
#include <iprt/cdefs.h>
#include <iprt/dir.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/types.h>

#include <iprt/cpp/list.h>
#include <iprt/cpp/ministring.h>

/**
 * Class for maintaining a "dropped files" directory
 * on the host or guest. This will contain all received files & directories
 * for a single drag and drop operation.
 *
 * In case of a failed drag and drop operation this class can also
 * perform a gentle rollback if required.
 */
class DnDDroppedFiles
{

public:

    DnDDroppedFiles(void);
    DnDDroppedFiles(const char *pszPath, uint32_t fFlags);
    virtual ~DnDDroppedFiles(void);

public:

    int AddFile(const char *pszFile);
    int AddDir(const char *pszDir);
    int Close(void);
    bool IsOpen(void) const;
    int OpenEx(const char *pszPath, uint32_t fFlags);
    int OpenTemp(uint32_t fFlags);
    const char *GetDirAbs(void) const;
    int Reopen(void);
    int Reset(bool fDeleteContent);
    int Rollback(void);

protected:

    int closeInternal(void);

protected:

    /** Open flags. */
    uint32_t                     m_fOpen;
    /** Directory handle for drop directory. */
    RTDIR                        m_hDir;
    /** Absolute path to drop directory. */
    RTCString                    m_strPathAbs;
    /** List for holding created directories in the case of a rollback. */
    RTCList<RTCString>           m_lstDirs;
    /** List for holding created files in the case of a rollback. */
    RTCList<RTCString>           m_lstFiles;
};

bool DnDMIMEHasFileURLs(const char *pcszFormat, size_t cchFormatMax);
bool DnDMIMENeedsDropDir(const char *pcszFormat, size_t cchFormatMax);

int DnDPathSanitizeFilename(char *pszPath, size_t cbPath);
int DnDPathSanitize(char *pszPath, size_t cbPath);

/** No flags specified. */
#define DNDURILIST_FLAGS_NONE                   0
/** Keep the original paths, don't convert paths to relative ones. */
#define DNDURILIST_FLAGS_ABSOLUTE_PATHS         RT_BIT(0)
/** Resolve all symlinks. */
#define DNDURILIST_FLAGS_RESOLVE_SYMLINKS       RT_BIT(1)
/** Keep the files + directory entries open while
 *  being in this list. */
#define DNDURILIST_FLAGS_KEEP_OPEN              RT_BIT(2)
/** Lazy loading: Only enumerate sub directories when needed.
 ** @todo Implement lazy loading.  */
#define DNDURILIST_FLAGS_LAZY                   RT_BIT(3)

class DnDURIObject
{
public:

    enum Type
    {
        Unknown = 0,
        File,
        Directory,
        Type_32Bit_Hack = 0x7fffffff
    };

    enum Dest
    {
        Source = 0,
        Target,
        Dest_32Bit_Hack = 0x7fffffff
    };

    DnDURIObject(void);
    DnDURIObject(Type type,
                 const RTCString &strSrcPath = "",
                 const RTCString &strDstPath = "",
                 uint32_t fMode = 0, uint64_t cbSize = 0);
    virtual ~DnDURIObject(void);

public:

    const RTCString &GetSourcePath(void) const { return m_strSrcPath; }
    const RTCString &GetDestPath(void) const { return m_strTgtPath; }
    uint32_t GetMode(void) const { return m_fMode; }
    uint64_t GetProcessed(void) const { return m_cbProcessed; }
    uint64_t GetSize(void) const { return m_cbSize; }
    Type GetType(void) const { return m_Type; }

public:

    int SetSize(uint64_t uSize) { m_cbSize = uSize; return VINF_SUCCESS; }

public:

    void Close(void);
    bool IsComplete(void) const;
    bool IsOpen(void) const;
    int Open(Dest enmDest, uint64_t fOpen, uint32_t fMode = 0);
    int OpenEx(const RTCString &strPath, Type enmType, Dest enmDest, uint64_t fOpen = 0, uint32_t fMode = 0, uint32_t fFlags = 0);
    int Read(void *pvBuf, size_t cbBuf, uint32_t *pcbRead);
    void Reset(void);
    int Write(const void *pvBuf, size_t cbBuf, uint32_t *pcbWritten);

public:

    static int RebaseURIPath(RTCString &strPath, const RTCString &strBaseOld = "", const RTCString &strBaseNew = "");

protected:

    void closeInternal(void);

protected:

    Type      m_Type;
    RTCString m_strSrcPath;
    RTCString m_strTgtPath;
    /** Whether the object is in "opened" state. */
    bool      m_fOpen;
    /** Object (file/directory) mode. */
    uint32_t  m_fMode;
    /** Size (in bytes) to read/write. */
    uint64_t  m_cbSize;
    /** Bytes processed reading/writing. */
    uint64_t  m_cbProcessed;

    union
    {
        RTFILE m_hFile;
    } u;
};

class DnDURIList
{
public:

    DnDURIList(void);
    virtual ~DnDURIList(void);

public:

    int AppendNativePath(const char *pszPath, uint32_t fFlags);
    int AppendNativePathsFromList(const char *pszNativePaths, size_t cbNativePaths, uint32_t fFlags);
    int AppendNativePathsFromList(const RTCList<RTCString> &lstNativePaths, uint32_t fFlags);
    int AppendURIPath(const char *pszURI, uint32_t fFlags);
    int AppendURIPathsFromList(const char *pszURIPaths, size_t cbURIPaths, uint32_t fFlags);
    int AppendURIPathsFromList(const RTCList<RTCString> &lstURI, uint32_t fFlags);

    void Clear(void);
    DnDURIObject *First(void) { return m_lstTree.first(); }
    bool IsEmpty(void) const { return m_lstTree.isEmpty(); }
    void RemoveFirst(void);
    int RootFromURIData(const void *pvData, size_t cbData, uint32_t fFlags);
    RTCString RootToString(const RTCString &strPathBase = "", const RTCString &strSeparator = "\r\n") const;
    uint64_t RootCount(void) const { return m_lstRoot.size(); }
    uint64_t TotalCount(void) const { return m_cTotal; }
    uint64_t TotalBytes(void) const { return m_cbTotal; }

protected:

    int addEntry(const char *pcszSource, const char *pcszTarget, uint32_t fFlags);
    int appendPathRecursive(const char *pcszSrcPath, const char *pcszDstPath, const char *pcszDstBase, size_t cchDstBase, uint32_t fFlags);

protected:

    /** List of all top-level file/directory entries.
     *  Note: All paths are kept internally as UNIX paths for
     *        easier conversion/handling!  */
    RTCList<RTCString>      m_lstRoot;
    /** List of all URI objects added. The list's content
     *  might vary depending on how the objects are being
     *  added (lazy or not). */
    RTCList<DnDURIObject *> m_lstTree;
    /** Total number of all URI objects. */
    uint64_t                m_cTotal;
    /** Total size of all URI objects, that is, the file
     *  size of all objects (in bytes).
     *  Note: Do *not* size_t here, as we also want to support large files
     *        on 32-bit guests. */
    uint64_t                m_cbTotal;
};

#endif /* !___VBox_GuestHost_DragAndDrop_h */

