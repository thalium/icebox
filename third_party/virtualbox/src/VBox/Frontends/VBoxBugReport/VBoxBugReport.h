/* $Id: VBoxBugReport.h $ */
/** @file
 * VBoxBugReport - VirtualBox command-line diagnostics tool, internal header file.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___H_VBOXBUGREPORT
#define ___H_VBOXBUGREPORT

/*
 * Introduction.
 *
 * In the most general sense a bug report is a collection of data obtained from
 * the user's host system. It may include files common for all VMs, like the
 * VBoxSVC.log file, as well as files related to particular machines. It may
 * also contain the output of commands executed on the host, as well as data
 * collected via OS APIs.
 */

/* @todo not sure if using a separate namespace would be beneficial */

#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/tar.h>
#include <iprt/vfs.h>
#include <iprt/cpp/list.h>

#ifdef RT_OS_WINDOWS
#define VBOXMANAGE "VBoxManage.exe"
#else /* !RT_OS_WINDOWS */
#define VBOXMANAGE "VBoxManage"
#endif /* !RT_OS_WINDOWS */

/* Base */

inline void handleRtError(int rc, const char *pszMsgFmt, ...)
{
    if (RT_FAILURE(rc))
    {
        va_list va;
        va_start(va, pszMsgFmt);
        RTCString msgArgs(pszMsgFmt, va);
        va_end(va);
        RTCStringFmt msg("%s. %s (%d)\n", msgArgs.c_str(), RTErrGetFull(rc), rc);
        throw RTCError(msg.c_str());
    }
}

inline void handleComError(HRESULT hr, const char *pszMsgFmt, ...)
{
    if (FAILED(hr))
    {
        va_list va;
        va_start(va, pszMsgFmt);
        RTCString msgArgs(pszMsgFmt, va);
        va_end(va);
        RTCStringFmt msg("%s (hr=0x%x)\n", msgArgs.c_str(), hr);
        throw RTCError(msg.c_str());
    }
}

/*
 * An auxiliary class to facilitate in-place path joins.
 */
class PathJoin
{
public:
    PathJoin(const char *folder, const char *file) { m_path = RTPathJoinA(folder, file); }
    ~PathJoin() { RTStrFree(m_path); };
    operator char*() const { return m_path; };
private:
    char *m_path;
};


/*
 * An abstract class serving as the root of the bug report filter tree.
 * A child provides an implementation of the 'apply' method. A child
 * should modify the input buffer (provided via pvSource) in place, or
 * allocate a new buffer via 'allocateBuffer'. Allocated buffers are
 * released automatically when another buffer is allocated, which means
 * that NEXT CALL TO 'APPLY' INVALIDATES BUFFERS RETURNED IN PREVIOUS
 * CALLS!
 */
class BugReportFilter
{
public:
    BugReportFilter();
    virtual ~BugReportFilter();
    virtual void *apply(void *pvSource, size_t *pcbInOut) = 0;
protected:
    void *allocateBuffer(size_t cbNeeded);
private:
    void *m_pvBuffer;
    size_t m_cbBuffer;
};


/*
 * An abstract class serving as the root of the bug report item tree.
 */
class BugReportItem
{
public:
    BugReportItem(const char *pszTitle);
    virtual ~BugReportItem();
    virtual const char *getTitle(void);
    virtual PRTSTREAM getStream(void) = 0;
    void addFilter(BugReportFilter *filter);
    void *applyFilter(void *pvSource, size_t *pcbInOut);
private:
    char *m_pszTitle;
    BugReportFilter *m_filter;
};

/*
 * An abstract class to serve as a base class for all report types.
 */
class BugReport
{
public:
    BugReport(const char *pszFileName);
    virtual ~BugReport();

    void addItem(BugReportItem* item, BugReportFilter *filter = 0);
    int  getItemCount(void);
    void process();
    void *applyFilters(BugReportItem* item, void *pvSource, size_t *pcbInOut);

    virtual void processItem(BugReportItem* item) = 0;
    virtual void complete(void) = 0;

protected:
    char *m_pszFileName;
    RTCList<BugReportItem*> m_Items;
};

/*
 * An auxiliary class providing formatted output into a temporary file for item
 * classes that obtain data via host OS APIs.
 */
class BugReportStream : public BugReportItem
{
public:
    BugReportStream(const char *pszTitle);
    virtual ~BugReportStream();
    virtual PRTSTREAM getStream(void);
protected:
    int printf(const char *pszFmt, ...);
    int putStr(const char *pszString);
private:
    PRTSTREAM m_Strm;
    char m_szFileName[RTPATH_MAX];
};


/* Generic */

/*
 * This class reports everything into a single text file.
 */
class BugReportText : public BugReport
{
public:
    BugReportText(const char *pszFileName);
    virtual ~BugReportText();
    virtual void processItem(BugReportItem* item);
    virtual void complete(void) {};
private:
    PRTSTREAM m_StrmTxt;
};

/*
 * This class reports items as individual files archived into a single compressed TAR file.
 */
class BugReportTarGzip : public BugReport
{
public:
    BugReportTarGzip(const char *pszFileName);
    virtual ~BugReportTarGzip();
    virtual void processItem(BugReportItem* item);
    virtual void complete(void);
private:
    /*
     * Helper class to release handles going out of scope.
     */
    class VfsIoStreamHandle
    {
    public:
        VfsIoStreamHandle() : m_hVfsStream(NIL_RTVFSIOSTREAM) {};
        ~VfsIoStreamHandle() { release(); }
        PRTVFSIOSTREAM getPtr(void) { return &m_hVfsStream; };
        RTVFSIOSTREAM get(void) { return m_hVfsStream; };
        void release(void)
        {
            if (m_hVfsStream != NIL_RTVFSIOSTREAM)
                RTVfsIoStrmRelease(m_hVfsStream);
            m_hVfsStream = NIL_RTVFSIOSTREAM;
        };
    private:
        RTVFSIOSTREAM m_hVfsStream;
    };

    VfsIoStreamHandle m_hVfsGzip;

    RTTAR m_hTar;
    RTTARFILE m_hTarFile;
    char m_szTarName[RTPATH_MAX];
};


/*
 * BugReportFile adds a file as an item to a report.
 */
class BugReportFile : public BugReportItem
{
public:
    BugReportFile(const char *pszPath, const char *pcszName);
    virtual ~BugReportFile();
    virtual PRTSTREAM getStream(void);

private:
    char *m_pszPath;
    PRTSTREAM m_Strm;
};

/*
 * A base class for item classes that collect CLI output.
 */
class BugReportCommand : public BugReportItem
{
public:
    BugReportCommand(const char *pszTitle, const char *pszExec, ...);
    virtual ~BugReportCommand();
    virtual PRTSTREAM getStream(void);
private:
    PRTSTREAM m_Strm;
    char m_szFileName[RTPATH_MAX];
    char *m_papszArgs[32];
};

/*
 * A base class for item classes that provide temp output file to a command.
 */
class BugReportCommandTemp : public BugReportItem
{
public:
    BugReportCommandTemp(const char *pszTitle, const char *pszExec, ...);
    virtual ~BugReportCommandTemp();
    virtual PRTSTREAM getStream(void);
private:
    PRTSTREAM m_Strm;
    char m_szFileName[RTPATH_MAX];
    char m_szErrFileName[RTPATH_MAX];
    char *m_papszArgs[32];
};

/* Platform-specific */

void createBugReportOsSpecific(BugReport* report, const char *pszHome);

#endif /* !___H_VBOXBUGREPORT */
