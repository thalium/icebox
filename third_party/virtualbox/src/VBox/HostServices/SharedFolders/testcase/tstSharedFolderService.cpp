/* $Id: tstSharedFolderService.cpp $ */
/** @file
 * Testcase for the shared folder service vbsf API.
 *
 * Note that this is still very threadbare (there is an awful lot which should
 * really be tested, but it already took too long to produce this much).  The
 * idea is that anyone who makes changes to the shared folders service and who
 * cares about unit testing them should add tests to the skeleton framework to
 * exercise the bits they change before and after changing them.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include "tstSharedFolderService.h"
#include "vbsf.h"

#include <iprt/fs.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/symlink.h>
#include <iprt/stream.h>
#include <iprt/test.h>
#include <iprt/string.h>
#include <iprt/utf16.h>

#include "teststubs.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest = NIL_RTTEST;


/*********************************************************************************************************************************
*   Declarations                                                                                                                 *
*********************************************************************************************************************************/
extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable);


/*********************************************************************************************************************************
*   Helpers                                                                                                                      *
*********************************************************************************************************************************/

/** Simple call handle structure for the guest call completion callback */
struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code */
    int32_t rc;
};

/** Call completion callback for guest calls. */
static DECLCALLBACK(void) callComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    callHandle->rc = rc;
}

/**
 * Initialise the HGCM service table as much as we need to start the
 * service
 * @param  pTable the table to initialise
 */
void initTable(VBOXHGCMSVCFNTABLE *pTable, VBOXHGCMSVCHELPERS *pHelpers)
{
    pTable->cbSize              = sizeof (VBOXHGCMSVCFNTABLE);
    pTable->u32Version          = VBOX_HGCM_SVC_VERSION;
    pHelpers->pfnCallComplete   = callComplete;
    pTable->pHelpers            = pHelpers;
}

#define LLUIFY(a) ((unsigned long long)(a))

static void bufferFromString(void *pvDest, size_t cb, const char *pcszSrc)
{
    char *pchDest = (char *)pvDest;

    Assert((cb) > 0);
    strncpy((pchDest), (pcszSrc), (cb) - 1);
    (pchDest)[(cb) - 1] = 0;
}

static void bufferFromPath(void *pvDest, size_t cb, const char *pcszSrc)
{
    char *psz;

    bufferFromString(pvDest, cb, pcszSrc);
    for (psz = (char *)pvDest; psz && psz < (char *)pvDest + cb; ++psz)
        if (*psz == '\\')
            *psz = '/';
}

#define ARRAY_FROM_PATH(a, b) \
    do { \
        void *p=(a); NOREF(p); \
        Assert((a) == p); /* Constant parameter */ \
        Assert(sizeof((a)) > 0); \
        bufferFromPath(a, sizeof(a), b); \
    } while (0)


/*********************************************************************************************************************************
*   Stub functions and data                                                                                                      *
*********************************************************************************************************************************/
static bool g_fFailIfNotLowercase = false;

static RTDIR g_testRTDirClose_hDir = NIL_RTDIR;

extern int testRTDirClose(RTDIR hDir)
{
 /* RTPrintf("%s: hDir=%p\n", __PRETTY_FUNCTION__, hDir); */
    g_testRTDirClose_hDir = hDir;
    return VINF_SUCCESS;
}

static char testRTDirCreatePath[256];
//static RTFMODE testRTDirCreateMode; - unused

extern int testRTDirCreate(const char *pszPath, RTFMODE fMode, uint32_t fCreate)
{
    RT_NOREF2(fMode, fCreate);
 /* RTPrintf("%s: pszPath=%s, fMode=0x%llx\n", __PRETTY_FUNCTION__, pszPath,
             LLUIFY(fMode)); */
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszPath, "/\\")))
        return VERR_FILE_NOT_FOUND;
    ARRAY_FROM_PATH(testRTDirCreatePath, pszPath);
    return 0;
}

static char testRTDirOpenName[256];
static struct TESTDIRHANDLE
{
    int iEntry;
    int iDir;
} g_aTestDirHandles[4];
static int g_iNextDirHandle = 0;
static RTDIR testRTDirOpen_hDir;

extern int testRTDirOpen(RTDIR *phDir, const char *pszPath)
{
 /* RTPrintf("%s: pszPath=%s\n", __PRETTY_FUNCTION__, pszPath); */
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszPath, "/\\")))
        return VERR_FILE_NOT_FOUND;
    ARRAY_FROM_PATH(testRTDirOpenName, pszPath);
    *phDir = testRTDirOpen_hDir;
    testRTDirOpen_hDir = NIL_RTDIR;
    if (!*phDir && g_fFailIfNotLowercase)
        *phDir = (RTDIR)&g_aTestDirHandles[g_iNextDirHandle++ % RT_ELEMENTS(g_aTestDirHandles)];
    if (*phDir)
    {
        struct TESTDIRHANDLE *pRealDir = (struct TESTDIRHANDLE *)*phDir;
        pRealDir->iEntry = 0;
        pRealDir->iDir   = 0;
        const char *pszSlash = pszPath - 1;
        while ((pszSlash = strpbrk(pszSlash + 1, "\\/")) != NULL)
            pRealDir->iDir += 1;
        /*RTPrintf("opendir %s = %d \n", pszPath, pRealDir->iDir);*/
    }
    return VINF_SUCCESS;
}

/** @todo Do something useful with the last two arguments. */
extern int testRTDirOpenFiltered(RTDIR *phDir, const char *pszPath, RTDIRFILTER, uint32_t)
{
 /* RTPrintf("%s: pszPath=%s\n", __PRETTY_FUNCTION__, pszPath); */
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszPath, "/\\")))
        return VERR_FILE_NOT_FOUND;
    ARRAY_FROM_PATH(testRTDirOpenName, pszPath);
    *phDir = testRTDirOpen_hDir;
    testRTDirOpen_hDir = NIL_RTDIR;
    if (!*phDir && g_fFailIfNotLowercase)
        *phDir = (RTDIR)&g_aTestDirHandles[g_iNextDirHandle++ % RT_ELEMENTS(g_aTestDirHandles)];
    if (*phDir)
    {
        struct TESTDIRHANDLE *pRealDir = (struct TESTDIRHANDLE *)*phDir;
        pRealDir->iEntry = 0;
        pRealDir->iDir   = 0;
        const char *pszSlash = pszPath - 1;
        while ((pszSlash = strpbrk(pszSlash + 1, "\\/")) != NULL)
            pRealDir->iDir += 1;
        pRealDir->iDir -= 1;
        /*RTPrintf("openfiltered %s = %d\n", pszPath, pRealDir->iDir);*/
    }
    return VINF_SUCCESS;
}

static RTDIR g_testRTDirQueryInfo_hDir;
static RTTIMESPEC testRTDirQueryInfoATime;

extern int testRTDirQueryInfo(RTDIR hDir, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs)
{
    RT_NOREF1(enmAdditionalAttribs);
 /* RTPrintf("%s: hDir=%p, enmAdditionalAttribs=0x%llx\n", __PRETTY_FUNCTION__,
             hDir, LLUIFY(enmAdditionalAttribs)); */
    g_testRTDirQueryInfo_hDir = hDir;
    RT_ZERO(*pObjInfo);
    pObjInfo->AccessTime = testRTDirQueryInfoATime;
    RT_ZERO(testRTDirQueryInfoATime);
    return VINF_SUCCESS;
}

extern int testRTDirRemove(const char *pszPath)
{
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszPath, "/\\")))
        return VERR_FILE_NOT_FOUND;
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}

static RTDIR g_testRTDirReadEx_hDir;

extern int testRTDirReadEx(RTDIR hDir, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry,
                           RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags)
{
    RT_NOREF4(pDirEntry, pcbDirEntry, enmAdditionalAttribs, fFlags);
 /* RTPrintf("%s: hDir=%p, pcbDirEntry=%d, enmAdditionalAttribs=%llu, fFlags=0x%llx\n",
             __PRETTY_FUNCTION__, hDir, pcbDirEntry ? (int) *pcbDirEntry : -1,
             LLUIFY(enmAdditionalAttribs), LLUIFY(fFlags)); */
    g_testRTDirReadEx_hDir = hDir;
    if (g_fFailIfNotLowercase && hDir != NIL_RTDIR)
    {
        struct TESTDIRHANDLE *pRealDir = (struct TESTDIRHANDLE *)hDir;
        if (pRealDir->iDir == 2) /* /test/mapping/ */
        {
            if (pRealDir->iEntry == 0)
            {
                pRealDir->iEntry++;
                RT_ZERO(*pDirEntry);
                pDirEntry->Info.Attr.fMode = RTFS_TYPE_DIRECTORY | RTFS_DOS_DIRECTORY | RTFS_UNIX_IROTH | RTFS_UNIX_IXOTH;
                pDirEntry->cbName = 4;
                pDirEntry->cwcShortName = 4;
                strcpy(pDirEntry->szName, "test");
                RTUtf16CopyAscii(pDirEntry->wszShortName, RT_ELEMENTS(pDirEntry->wszShortName), "test");
                /*RTPrintf("readdir: 'test'\n");*/
                return VINF_SUCCESS;
            }
        }
        else if (pRealDir->iDir == 3) /* /test/mapping/test/ */
        {
            if (pRealDir->iEntry == 0)
            {
                pRealDir->iEntry++;
                RT_ZERO(*pDirEntry);
                pDirEntry->Info.Attr.fMode = RTFS_TYPE_FILE | RTFS_DOS_NT_NORMAL | RTFS_UNIX_IROTH | RTFS_UNIX_IXOTH;
                pDirEntry->cbName = 4;
                pDirEntry->cwcShortName = 4;
                strcpy(pDirEntry->szName, "file");
                RTUtf16CopyAscii(pDirEntry->wszShortName, RT_ELEMENTS(pDirEntry->wszShortName), "file");
                /*RTPrintf("readdir: 'file'\n");*/
                return VINF_SUCCESS;
            }
        }
        /*else RTPrintf("%s: iDir=%d\n", pRealDir->iDir);*/
    }
    return VERR_NO_MORE_FILES;
}

static RTTIMESPEC testRTDirSetTimesATime;

extern int testRTDirSetTimes(RTDIR hDir, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                             PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
    RT_NOREF4(hDir, pModificationTime, pChangeTime, pBirthTime);
 /* RTPrintf("%s: hDir=%p, *pAccessTime=%lli, *pModificationTime=%lli, *pChangeTime=%lli, *pBirthTime=%lli\n",
             __PRETTY_FUNCTION__, hDir,
             pAccessTime ? (long long)RTTimeSpecGetNano(pAccessTime) : -1,
               pModificationTime
             ? (long long)RTTimeSpecGetNano(pModificationTime) : -1,
             pChangeTime ? (long long)RTTimeSpecGetNano(pChangeTime) : -1,
             pBirthTime ? (long long)RTTimeSpecGetNano(pBirthTime) : -1); */
    if (pAccessTime)
        testRTDirSetTimesATime = *pAccessTime;
    else
        RT_ZERO(testRTDirSetTimesATime);
    return VINF_SUCCESS;
}

static RTFILE g_testRTFileCloseFile;

extern int  testRTFileClose(RTFILE File)
{
 /* RTPrintf("%s: File=%p\n", __PRETTY_FUNCTION__, File); */
    g_testRTFileCloseFile = File;
    return 0;
}

extern int  testRTFileDelete(const char *pszFilename)
{
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszFilename, "/\\")))
        return VERR_FILE_NOT_FOUND;
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}

static RTFILE g_testRTFileFlushFile;

extern int  testRTFileFlush(RTFILE File)
{
 /* RTPrintf("%s: File=%p\n", __PRETTY_FUNCTION__, File); */
    g_testRTFileFlushFile = File;
    return VINF_SUCCESS;
}

static RTFILE g_testRTFileLockFile;
static unsigned testRTFileLockfLock;
static int64_t testRTFileLockOffset;
static uint64_t testRTFileLockSize;

extern int  testRTFileLock(RTFILE hFile, unsigned fLock, int64_t offLock, uint64_t cbLock)
{
 /* RTPrintf("%s: hFile=%p, fLock=%u, offLock=%lli, cbLock=%llu\n",
             __PRETTY_FUNCTION__, hFile, fLock, (long long) offLock,
             LLUIFY(cbLock)); */
    g_testRTFileLockFile = hFile;
    testRTFileLockfLock = fLock;
    testRTFileLockOffset = offLock;
    testRTFileLockSize = cbLock;
    return VINF_SUCCESS;
}

static char testRTFileOpenName[256];
static uint64_t testRTFileOpenFlags;
static RTFILE testRTFileOpenpFile;

extern int  testRTFileOpen(PRTFILE pFile, const char *pszFilename, uint64_t fOpen)
{
 /* RTPrintf("%s, pszFilename=%s, fOpen=0x%llx\n", __PRETTY_FUNCTION__,
             pszFilename, LLUIFY(fOpen)); */
    ARRAY_FROM_PATH(testRTFileOpenName, pszFilename);
    testRTFileOpenFlags = fOpen;
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszFilename, "/\\")))
        return VERR_FILE_NOT_FOUND;
    *pFile = testRTFileOpenpFile;
    testRTFileOpenpFile = 0;
    return VINF_SUCCESS;
}

static RTFILE g_testRTFileQueryInfoFile;
static RTTIMESPEC testRTFileQueryInfoATime;
static uint32_t testRTFileQueryInfoFMode;

extern int  testRTFileQueryInfo(RTFILE hFile, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs)
{
    RT_NOREF1(enmAdditionalAttribs);
 /* RTPrintf("%s, hFile=%p, enmAdditionalAttribs=0x%llx\n",
             __PRETTY_FUNCTION__, hFile, LLUIFY(enmAdditionalAttribs)); */
    g_testRTFileQueryInfoFile = hFile;
    RT_ZERO(*pObjInfo);
    pObjInfo->AccessTime = testRTFileQueryInfoATime;
    RT_ZERO(testRTDirQueryInfoATime);
    pObjInfo->Attr.fMode = testRTFileQueryInfoFMode;
    testRTFileQueryInfoFMode = 0;
    return VINF_SUCCESS;
}

static const char *testRTFileReadData;

extern int  testRTFileRead(RTFILE File, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    RT_NOREF1(File);
 /* RTPrintf("%s : File=%p, cbToRead=%llu\n", __PRETTY_FUNCTION__, File,
             LLUIFY(cbToRead)); */
    bufferFromPath(pvBuf, cbToRead, testRTFileReadData);
    if (pcbRead)
        *pcbRead = RT_MIN(cbToRead, strlen(testRTFileReadData) + 1);
    testRTFileReadData = 0;
    return VINF_SUCCESS;
}

extern int testRTFileSeek(RTFILE hFile, int64_t offSeek, unsigned uMethod, uint64_t *poffActual)
{
    RT_NOREF3(hFile, offSeek, uMethod);
 /* RTPrintf("%s : hFile=%p, offSeek=%llu, uMethod=%u\n", __PRETTY_FUNCTION__,
             hFile, LLUIFY(offSeek), uMethod); */
    if (poffActual)
        *poffActual = 0;
    return VINF_SUCCESS;
}

static uint64_t testRTFileSetFMode;

extern int testRTFileSetMode(RTFILE File, RTFMODE fMode)
{
    RT_NOREF1(File);
 /* RTPrintf("%s: fMode=%llu\n", __PRETTY_FUNCTION__, LLUIFY(fMode)); */
    testRTFileSetFMode = fMode;
    return VINF_SUCCESS;
}

static RTFILE g_testRTFileSetSizeFile;
static RTFOFF testRTFileSetSizeSize;

extern int  testRTFileSetSize(RTFILE File, uint64_t cbSize)
{
 /* RTPrintf("%s: File=%llu, cbSize=%llu\n", __PRETTY_FUNCTION__, LLUIFY(File),
             LLUIFY(cbSize)); */
    g_testRTFileSetSizeFile = File;
    testRTFileSetSizeSize = (RTFOFF) cbSize; /* Why was this signed before? */
    return VINF_SUCCESS;
}

static RTTIMESPEC testRTFileSetTimesATime;

extern int testRTFileSetTimes(RTFILE File, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                              PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
    RT_NOREF4(File, pModificationTime, pChangeTime, pBirthTime);
 /* RTPrintf("%s: pFile=%p, *pAccessTime=%lli, *pModificationTime=%lli, *pChangeTime=%lli, *pBirthTime=%lli\n",
             __PRETTY_FUNCTION__,
             pAccessTime ? (long long)RTTimeSpecGetNano(pAccessTime) : -1,
               pModificationTime
             ? (long long)RTTimeSpecGetNano(pModificationTime) : -1,
             pChangeTime ? (long long)RTTimeSpecGetNano(pChangeTime) : -1,
             pBirthTime ? (long long)RTTimeSpecGetNano(pBirthTime) : -1); */
    if (pAccessTime)
        testRTFileSetTimesATime = *pAccessTime;
    else
        RT_ZERO(testRTFileSetTimesATime);
    return VINF_SUCCESS;
}

static RTFILE g_testRTFileUnlockFile;
static int64_t testRTFileUnlockOffset;
static uint64_t testRTFileUnlockSize;

extern int  testRTFileUnlock(RTFILE File, int64_t offLock, uint64_t cbLock)
{
 /* RTPrintf("%s: hFile=%p, ofLock=%lli, cbLock=%llu\n", __PRETTY_FUNCTION__,
             File, (long long) offLock, LLUIFY(cbLock)); */
    g_testRTFileUnlockFile = File;
    testRTFileUnlockOffset = offLock;
    testRTFileUnlockSize = cbLock;
    return VINF_SUCCESS;
}

static char testRTFileWriteData[256];

extern int  testRTFileWrite(RTFILE File, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten)
{
    RT_NOREF2(File, cbToWrite);
 /* RTPrintf("%s: File=%p, pvBuf=%.*s, cbToWrite=%llu\n", __PRETTY_FUNCTION__,
             File, cbToWrite, (const char *)pvBuf, LLUIFY(cbToWrite)); */
    ARRAY_FROM_PATH(testRTFileWriteData, (const char *)pvBuf);
    if (pcbWritten)
        *pcbWritten = strlen(testRTFileWriteData) + 1;
    return VINF_SUCCESS;
}

extern int testRTFsQueryProperties(const char *pszFsPath, PRTFSPROPERTIES pProperties)
{
    RT_NOREF1(pszFsPath);
 /* RTPrintf("%s, pszFsPath=%s\n", __PRETTY_FUNCTION__, pszFsPath);
    RT_ZERO(*pProperties); */
    pProperties->cbMaxComponent = 256;
    pProperties->fCaseSensitive = true;
    return VINF_SUCCESS;
}

extern int testRTFsQuerySerial(const char *pszFsPath, uint32_t *pu32Serial)
{
    RT_NOREF2(pszFsPath, pu32Serial);
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}
extern int testRTFsQuerySizes(const char *pszFsPath, PRTFOFF pcbTotal, RTFOFF *pcbFree, uint32_t *pcbBlock, uint32_t *pcbSector)
{
    RT_NOREF5(pszFsPath, pcbTotal, pcbFree, pcbBlock, pcbSector);
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}

extern int testRTPathQueryInfoEx(const char *pszPath, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags)
{
    RT_NOREF2(enmAdditionalAttribs, fFlags);
 /* RTPrintf("%s: pszPath=%s, enmAdditionalAttribs=0x%x, fFlags=0x%x\n",
             __PRETTY_FUNCTION__, pszPath, (unsigned) enmAdditionalAttribs,
             (unsigned) fFlags); */
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszPath, "/\\")))
        return VERR_FILE_NOT_FOUND;
    RT_ZERO(*pObjInfo);
    return VINF_SUCCESS;
}

extern int testRTSymlinkDelete(const char *pszSymlink, uint32_t fDelete)
{
    RT_NOREF2(pszSymlink, fDelete);
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszSymlink, "/\\")))
        return VERR_FILE_NOT_FOUND;
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}

extern int testRTSymlinkRead(const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead)
{
    if (g_fFailIfNotLowercase && !RTStrIsLowerCased(strpbrk(pszSymlink, "/\\")))
        return VERR_FILE_NOT_FOUND;
    RT_NOREF4(pszSymlink, pszTarget, cbTarget, fRead);
    RTPrintf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}


/*********************************************************************************************************************************
*   Tests                                                                                                                        *
*********************************************************************************************************************************/

/* Sub-tests for testMappingsQuery(). */
void testMappingsQuerySimple(RTTEST hTest) { RT_NOREF1(hTest); }
void testMappingsQueryTooFewBuffers(RTTEST hTest) { RT_NOREF1(hTest); }
void testMappingsQueryAutoMount(RTTEST hTest) { RT_NOREF1(hTest); }
void testMappingsQueryArrayWrongSize(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testMappingsQueryName(). */
void testMappingsQueryNameValid(RTTEST hTest) { RT_NOREF1(hTest); }
void testMappingsQueryNameInvalid(RTTEST hTest) { RT_NOREF1(hTest); }
void testMappingsQueryNameBadBuffer(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testMapFolder(). */
void testMapFolderValid(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderInvalid(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderTwice(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderDelimiter(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderCaseSensitive(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderCaseInsensitive(RTTEST hTest) { RT_NOREF1(hTest); }
void testMapFolderBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testUnmapFolder(). */
void testUnmapFolderValid(RTTEST hTest) { RT_NOREF1(hTest); }
void testUnmapFolderInvalid(RTTEST hTest) { RT_NOREF1(hTest); }
void testUnmapFolderBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testCreate(). */
void testCreateBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testClose(). */
void testCloseBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testRead(). */
void testReadBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testWrite(). */
void testWriteBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testLock(). */
void testLockBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testFlush(). */
void testFlushBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testDirList(). */
void testDirListBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testReadLink(). */
void testReadLinkBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testFSInfo(). */
void testFSInfoBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testRemove(). */
void testRemoveBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testRename(). */
void testRenameBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testSymlink(). */
void testSymlinkBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testMappingsAdd(). */
void testMappingsAddBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

/* Sub-tests for testMappingsRemove(). */
void testMappingsRemoveBadParameters(RTTEST hTest) { RT_NOREF1(hTest); }

union TESTSHFLSTRING
{
    SHFLSTRING string;
    char acData[256];
};

static void fillTestShflString(union TESTSHFLSTRING *pDest,
                               const char *pcszSource)
{
    const size_t cchSource = strlen(pcszSource);
    AssertRelease(  cchSource * 2 + 2
                  < sizeof(*pDest) - RT_UOFFSETOF(SHFLSTRING, String));
    pDest->string.u16Length = (uint16_t)(cchSource * sizeof(RTUTF16));
    pDest->string.u16Size   = pDest->string.u16Length + sizeof(RTUTF16);
    /* Copy pcszSource ASCIIZ, including the trailing 0, to the UTF16 pDest->string.String.ucs2. */
    for (unsigned i = 0; i <= cchSource; ++i)
        pDest->string.String.ucs2[i] = (uint16_t)pcszSource[i];
}

static SHFLROOT initWithWritableMapping(RTTEST hTest,
                                        VBOXHGCMSVCFNTABLE *psvcTable,
                                        VBOXHGCMSVCHELPERS *psvcHelpers,
                                        const char *pcszFolderName,
                                        const char *pcszMapping,
                                        bool fCaseSensitive = true)
{
    VBOXHGCMSVCPARM aParms[RT_MAX(SHFL_CPARMS_ADD_MAPPING,
                                  SHFL_CPARMS_MAP_FOLDER)];
    union TESTSHFLSTRING FolderName;
    union TESTSHFLSTRING Mapping;
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    int rc;

    initTable(psvcTable, psvcHelpers);
    AssertReleaseRC(VBoxHGCMSvcLoad(psvcTable));
    AssertRelease(  psvcTable->pvService
                  = RTTestGuardedAllocTail(hTest, psvcTable->cbClient));
    RT_BZERO(psvcTable->pvService, psvcTable->cbClient);
    fillTestShflString(&FolderName, pcszFolderName);
    fillTestShflString(&Mapping, pcszMapping);
    aParms[0].setPointer(&FolderName,   RT_UOFFSETOF(SHFLSTRING, String)
                                      + FolderName.string.u16Size);
    aParms[1].setPointer(&Mapping,   RT_UOFFSETOF(SHFLSTRING, String)
                                   + Mapping.string.u16Size);
    aParms[2].setUInt32(1);
    rc = psvcTable->pfnHostCall(psvcTable->pvService, SHFL_FN_ADD_MAPPING,
                                SHFL_CPARMS_ADD_MAPPING, aParms);
    AssertReleaseRC(rc);
    aParms[0].setPointer(&Mapping,   RT_UOFFSETOF(SHFLSTRING, String)
                                   + Mapping.string.u16Size);
    aParms[1].setUInt32(0);  /* root */
    aParms[2].setUInt32('/');  /* delimiter */
    aParms[3].setUInt32(fCaseSensitive);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_MAP_FOLDER,
                       SHFL_CPARMS_MAP_FOLDER, aParms);
    AssertReleaseRC(callHandle.rc);
    return aParms[1].u.uint32;
}

/** @todo Mappings should be automatically removed by unloading the service,
 *        but unloading is currently a no-op! */
static void unmapAndRemoveMapping(RTTEST hTest, VBOXHGCMSVCFNTABLE *psvcTable,
                                  SHFLROOT root, const char *pcszFolderName)
{
    RT_NOREF1(hTest);
    VBOXHGCMSVCPARM aParms[RT_MAX(SHFL_CPARMS_UNMAP_FOLDER,
                                  SHFL_CPARMS_REMOVE_MAPPING)];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    union TESTSHFLSTRING FolderName;
    int rc;

    aParms[0].setUInt32(root);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_UNMAP_FOLDER,
                       SHFL_CPARMS_UNMAP_FOLDER, aParms);
    AssertReleaseRC(callHandle.rc);
    fillTestShflString(&FolderName, pcszFolderName);
    aParms[0].setPointer(&FolderName,   RT_UOFFSETOF(SHFLSTRING, String)
                                      + FolderName.string.u16Size);
    rc = psvcTable->pfnHostCall(psvcTable->pvService, SHFL_FN_REMOVE_MAPPING,
                                SHFL_CPARMS_REMOVE_MAPPING, aParms);
    AssertReleaseRC(rc);
}

static int createFile(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT Root,
                      const char *pcszFilename, uint32_t fCreateFlags,
                      SHFLHANDLE *pHandle, SHFLCREATERESULT *pResult)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_CREATE];
    union TESTSHFLSTRING Path;
    SHFLCREATEPARMS CreateParms;
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    fillTestShflString(&Path, pcszFilename);
    RT_ZERO(CreateParms);
    CreateParms.CreateFlags = fCreateFlags;
    aParms[0].setUInt32(Root);
    aParms[1].setPointer(&Path,   RT_UOFFSETOF(SHFLSTRING, String)
                                + Path.string.u16Size);
    aParms[2].setPointer(&CreateParms, sizeof(CreateParms));
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_CREATE,
                       RT_ELEMENTS(aParms), aParms);
    if (RT_FAILURE(callHandle.rc))
        return callHandle.rc;
    if (pHandle)
        *pHandle = CreateParms.Handle;
    if (pResult)
        *pResult = CreateParms.Result;
    return VINF_SUCCESS;
}

static int readFile(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT Root,
                    SHFLHANDLE hFile, uint64_t offSeek, uint32_t cbRead,
                    uint32_t *pcbRead, void *pvBuf, uint32_t cbBuf)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_READ];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(Root);
    aParms[1].setUInt64((uint64_t) hFile);
    aParms[2].setUInt64(offSeek);
    aParms[3].setUInt32(cbRead);
    aParms[4].setPointer(pvBuf, cbBuf);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_READ,
                       RT_ELEMENTS(aParms), aParms);
    if (pcbRead)
        *pcbRead = aParms[3].u.uint32;
    return callHandle.rc;
}

static int writeFile(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT Root,
                     SHFLHANDLE hFile, uint64_t offSeek, uint32_t cbWrite,
                     uint32_t *pcbWritten, const void *pvBuf, uint32_t cbBuf)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_WRITE];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(Root);
    aParms[1].setUInt64((uint64_t) hFile);
    aParms[2].setUInt64(offSeek);
    aParms[3].setUInt32(cbWrite);
    aParms[4].setPointer((void *)pvBuf, cbBuf);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_WRITE,
                       RT_ELEMENTS(aParms), aParms);
    if (pcbWritten)
        *pcbWritten = aParms[3].u.uint32;
    return callHandle.rc;
}

static int flushFile(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT root,
                     SHFLHANDLE handle)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_FLUSH];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(root);
    aParms[1].setUInt64(handle);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_FLUSH,
                       SHFL_CPARMS_FLUSH, aParms);
    return callHandle.rc;
}

static int listDir(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT root,
                   SHFLHANDLE handle, uint32_t fFlags,
                   const char *pcszPath, void *pvBuf, uint32_t cbBuf,
                   uint32_t resumePoint, uint32_t *pcFiles)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_LIST];
    union TESTSHFLSTRING Path;
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(root);
    aParms[1].setUInt64(handle);
    aParms[2].setUInt32(fFlags);
    aParms[3].setUInt32(cbBuf);
    if (pcszPath)
    {
        fillTestShflString(&Path, pcszPath);
        aParms[4].setPointer(&Path,   RT_UOFFSETOF(SHFLSTRING, String)
                                    + Path.string.u16Size);
    }
    else
        aParms[4].setPointer(NULL, 0);
    aParms[5].setPointer(pvBuf, cbBuf);
    aParms[6].setUInt32(resumePoint);
    aParms[7].setUInt32(0);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_LIST,
                       RT_ELEMENTS(aParms), aParms);
    if (pcFiles)
        *pcFiles = aParms[7].u.uint32;
    return callHandle.rc;
}

static int sfInformation(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT root,
                         SHFLHANDLE handle, uint32_t fFlags, uint32_t cb,
                         SHFLFSOBJINFO *pInfo)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_INFORMATION];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(root);
    aParms[1].setUInt64(handle);
    aParms[2].setUInt32(fFlags);
    aParms[3].setUInt32(cb);
    aParms[4].setPointer(pInfo, cb);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_INFORMATION,
                       RT_ELEMENTS(aParms), aParms);
    return callHandle.rc;
}

static int lockFile(VBOXHGCMSVCFNTABLE *psvcTable, SHFLROOT root,
                    SHFLHANDLE handle, int64_t offLock, uint64_t cbLock,
                    uint32_t fFlags)
{
    VBOXHGCMSVCPARM aParms[SHFL_CPARMS_LOCK];
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };

    aParms[0].setUInt32(root);
    aParms[1].setUInt64(handle);
    aParms[2].setUInt64(offLock);
    aParms[3].setUInt64(cbLock);
    aParms[4].setUInt32(fFlags);
    psvcTable->pfnCall(psvcTable->pvService, &callHandle, 0,
                       psvcTable->pvService, SHFL_FN_LOCK,
                       RT_ELEMENTS(aParms), aParms);
    return callHandle.rc;
}

void testCreateFileSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    SHFLCREATERESULT Result;
    int rc;

    RTTestSub(hTest, "Create file simple");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ, NULL,
                    &Result);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,
                     !strcmp(&testRTFileOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0],
                             "/test/mapping/test/file"),
                     (hTest, "pszFilename=%s\n", &testRTFileOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0]));
    RTTEST_CHECK_MSG(hTest, testRTFileOpenFlags == 0x181,
                     (hTest, "fOpen=%llu\n", LLUIFY(testRTFileOpenFlags)));
    RTTEST_CHECK_MSG(hTest, Result == SHFL_FILE_CREATED,
                     (hTest, "Result=%d\n", (int) Result));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile,
                     (hTest, "File=%u\n", (uintptr_t)g_testRTFileCloseFile));
}

void testCreateFileSimpleCaseInsensitive(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    SHFLCREATERESULT Result;
    int rc;

    g_fFailIfNotLowercase = true;

    RTTestSub(hTest, "Create file case insensitive");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname", false /*fCaseSensitive*/);
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/TesT/FilE", SHFL_CF_ACCESS_READ, NULL,
                    &Result);
    RTTEST_CHECK_RC_OK(hTest, rc);

    RTTEST_CHECK_MSG(hTest,
                     !strcmp(&testRTFileOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0],
                             "/test/mapping/test/file"),
                     (hTest, "pszFilename=%s\n", &testRTFileOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0]));
    RTTEST_CHECK_MSG(hTest, testRTFileOpenFlags == 0x181,
                     (hTest, "fOpen=%llu\n", LLUIFY(testRTFileOpenFlags)));
    RTTEST_CHECK_MSG(hTest, Result == SHFL_FILE_CREATED,
                     (hTest, "Result=%d\n", (int) Result));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile,
                     (hTest, "File=%u\n", (uintptr_t)g_testRTFileCloseFile));

    g_fFailIfNotLowercase = false;
}

void testCreateDirSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    RTDIR hDir = (RTDIR)&g_aTestDirHandles[g_iNextDirHandle++ % RT_ELEMENTS(g_aTestDirHandles)];
    SHFLCREATERESULT Result;
    int rc;

    RTTestSub(hTest, "Create directory simple");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTDirOpen_hDir = hDir;
    rc = createFile(&svcTable, Root, "test/dir",
                    SHFL_CF_DIRECTORY | SHFL_CF_ACCESS_READ, NULL, &Result);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,
                     !strcmp(&testRTDirCreatePath[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0],
                             "/test/mapping/test/dir"),
                     (hTest, "pszPath=%s\n", &testRTDirCreatePath[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0]));
    RTTEST_CHECK_MSG(hTest,
                     !strcmp(&testRTDirOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0],
                             "/test/mapping/test/dir"),
                     (hTest, "pszFilename=%s\n", &testRTDirOpenName[RTPATH_STYLE == RTPATH_STR_F_STYLE_DOS ? 2 : 0]));
    RTTEST_CHECK_MSG(hTest, Result == SHFL_FILE_CREATED,
                     (hTest, "Result=%d\n", (int) Result));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTDirClose_hDir == hDir, (hTest, "hDir=%p\n", g_testRTDirClose_hDir));
}

void testReadFileSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    SHFLHANDLE Handle;
    const char *pcszReadData = "Data to read";
    char acBuf[sizeof(pcszReadData) + 10];
    uint32_t cbRead;
    int rc;

    RTTestSub(hTest, "Read file simple");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    testRTFileReadData = pcszReadData;
    rc = readFile(&svcTable, Root, Handle, 0, (uint32_t)strlen(pcszReadData) + 1,
                  &cbRead, acBuf, (uint32_t)sizeof(acBuf));
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,
                     !strncmp(acBuf, pcszReadData, sizeof(acBuf)),
                     (hTest, "pvBuf=%.*s\n", sizeof(acBuf), acBuf));
    RTTEST_CHECK_MSG(hTest, cbRead == strlen(pcszReadData) + 1,
                     (hTest, "cbRead=%llu\n", LLUIFY(cbRead)));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
}

void testWriteFileSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    SHFLHANDLE Handle;
    const char *pcszWrittenData = "Data to write";
    uint32_t cbToWrite = (uint32_t)strlen(pcszWrittenData) + 1;
    uint32_t cbWritten;
    int rc;

    RTTestSub(hTest, "Write file simple");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    rc = writeFile(&svcTable, Root, Handle, 0, cbToWrite, &cbWritten,
                   pcszWrittenData, cbToWrite);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,
                     !strcmp(testRTFileWriteData, pcszWrittenData),
                     (hTest, "pvBuf=%s\n", testRTFileWriteData));
    RTTEST_CHECK_MSG(hTest, cbWritten == cbToWrite,
                     (hTest, "cbWritten=%llu\n", LLUIFY(cbWritten)));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
}

void testFlushFileSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    SHFLHANDLE Handle;
    int rc;

    RTTestSub(hTest, "Flush file simple");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    rc = flushFile(&svcTable, Root, Handle);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, g_testRTFileFlushFile == hcFile, (hTest, "File=%u\n", g_testRTFileFlushFile));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
}

void testDirListEmpty(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    RTDIR hDir = (RTDIR)&g_aTestDirHandles[g_iNextDirHandle++ % RT_ELEMENTS(g_aTestDirHandles)];
    SHFLHANDLE Handle;
    union
    {
        SHFLDIRINFO DirInfo;
        uint8_t     abBuffer[sizeof(SHFLDIRINFO) + 2 * sizeof(RTUTF16)];
    } Buf;
    uint32_t cFiles;
    int rc;

    RTTestSub(hTest, "List empty directory");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTDirOpen_hDir = hDir;
    rc = createFile(&svcTable, Root, "test/dir",
                    SHFL_CF_DIRECTORY | SHFL_CF_ACCESS_READ, &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    rc = listDir(&svcTable, Root, Handle, 0, NULL, &Buf.DirInfo, sizeof(Buf), 0, &cFiles);
    RTTEST_CHECK_RC(hTest, rc, VERR_NO_MORE_FILES);
    RTTEST_CHECK_MSG(hTest, g_testRTDirReadEx_hDir == hDir, (hTest, "Dir=%p\n", g_testRTDirReadEx_hDir));
    RTTEST_CHECK_MSG(hTest, cFiles == 0,
                     (hTest, "cFiles=%llu\n", LLUIFY(cFiles)));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTDirClose_hDir == hDir, (hTest, "hDir=%p\n", g_testRTDirClose_hDir));
}

void testFSInfoQuerySetFMode(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    const uint32_t fMode = 0660;
    SHFLFSOBJINFO Info;
    int rc;

    RTTestSub(hTest, "Query and set file size");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    SHFLHANDLE Handle = SHFL_HANDLE_NIL;
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK_RETV(hTest, rc);

    RT_ZERO(Info);
    testRTFileQueryInfoFMode = fMode;
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_FILE, sizeof(Info),
                       &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, g_testRTFileQueryInfoFile == hcFile, (hTest, "File=%u\n", g_testRTFileQueryInfoFile));
    RTTEST_CHECK_MSG(hTest, Info.Attr.fMode == fMode,
                     (hTest, "cbObject=%llu\n", LLUIFY(Info.cbObject)));
    RT_ZERO(Info);
    Info.Attr.fMode = fMode;
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_SET | SHFL_INFO_FILE,
                       sizeof(Info), &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, testRTFileSetFMode == fMode,
                     (hTest, "Size=%llu\n", LLUIFY(testRTFileSetFMode)));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
}

void testFSInfoQuerySetDirATime(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTDIR hDir = (RTDIR)&g_aTestDirHandles[g_iNextDirHandle++ % RT_ELEMENTS(g_aTestDirHandles)];
    const int64_t ccAtimeNano = 100000;
    SHFLFSOBJINFO Info;
    SHFLHANDLE Handle;
    int rc;

    RTTestSub(hTest, "Query and set directory atime");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTDirOpen_hDir = hDir;
    rc = createFile(&svcTable, Root, "test/dir",
                    SHFL_CF_DIRECTORY | SHFL_CF_ACCESS_READ, &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RT_ZERO(Info);
    RTTimeSpecSetNano(&testRTDirQueryInfoATime, ccAtimeNano);
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_FILE, sizeof(Info),
                       &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, g_testRTDirQueryInfo_hDir == hDir, (hTest, "Dir=%p\n", g_testRTDirQueryInfo_hDir));
    RTTEST_CHECK_MSG(hTest, RTTimeSpecGetNano(&Info.AccessTime) == ccAtimeNano,
                     (hTest, "ATime=%llu\n",
                      LLUIFY(RTTimeSpecGetNano(&Info.AccessTime))));
    RT_ZERO(Info);
    RTTimeSpecSetNano(&Info.AccessTime, ccAtimeNano);
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_SET | SHFL_INFO_FILE,
                       sizeof(Info), &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,    RTTimeSpecGetNano(&testRTDirSetTimesATime)
                            == ccAtimeNano,
                     (hTest, "ATime=%llu\n",
                      LLUIFY(RTTimeSpecGetNano(&testRTDirSetTimesATime))));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTDirClose_hDir == hDir, (hTest, "hDir=%p\n", g_testRTDirClose_hDir));
}

void testFSInfoQuerySetFileATime(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    const int64_t ccAtimeNano = 100000;
    SHFLFSOBJINFO Info;
    SHFLHANDLE Handle;
    int rc;

    RTTestSub(hTest, "Query and set file atime");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RT_ZERO(Info);
    RTTimeSpecSetNano(&testRTFileQueryInfoATime, ccAtimeNano);
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_FILE, sizeof(Info),
                       &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, g_testRTFileQueryInfoFile == hcFile, (hTest, "File=%u\n", g_testRTFileQueryInfoFile));
    RTTEST_CHECK_MSG(hTest, RTTimeSpecGetNano(&Info.AccessTime) == ccAtimeNano,
                     (hTest, "ATime=%llu\n",
                      LLUIFY(RTTimeSpecGetNano(&Info.AccessTime))));
    RT_ZERO(Info);
    RTTimeSpecSetNano(&Info.AccessTime, ccAtimeNano);
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_SET | SHFL_INFO_FILE,
                       sizeof(Info), &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest,    RTTimeSpecGetNano(&testRTFileSetTimesATime)
                            == ccAtimeNano,
                     (hTest, "ATime=%llu\n",
                      LLUIFY(RTTimeSpecGetNano(&testRTFileSetTimesATime))));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
}

void testFSInfoQuerySetEndOfFile(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    const RTFOFF cbNew = 50000;
    SHFLFSOBJINFO Info;
    SHFLHANDLE Handle;
    int rc;

    RTTestSub(hTest, "Set end of file position");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RT_ZERO(Info);
    Info.cbObject = cbNew;
    rc = sfInformation(&svcTable, Root, Handle, SHFL_INFO_SET | SHFL_INFO_SIZE,
                       sizeof(Info), &Info);
    RTTEST_CHECK_RC_OK(hTest, rc);
    RTTEST_CHECK_MSG(hTest, g_testRTFileSetSizeFile == hcFile, (hTest, "File=%u\n", g_testRTFileSetSizeFile));
    RTTEST_CHECK_MSG(hTest, testRTFileSetSizeSize == cbNew,
                     (hTest, "Size=%llu\n", LLUIFY(testRTFileSetSizeSize)));
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
}

void testLockFileSimple(RTTEST hTest)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    SHFLROOT Root;
    const RTFILE hcFile = (RTFILE) 0x10000;
    const int64_t offLock = 50000;
    const uint64_t cbLock = 4000;
    SHFLHANDLE Handle;
    int rc;

    RTTestSub(hTest, "Simple file lock and unlock");
    Root = initWithWritableMapping(hTest, &svcTable, &svcHelpers,
                                   "/test/mapping", "testname");
    testRTFileOpenpFile = hcFile;
    rc = createFile(&svcTable, Root, "/test/file", SHFL_CF_ACCESS_READ,
                    &Handle, NULL);
    RTTEST_CHECK_RC_OK(hTest, rc);
    rc = lockFile(&svcTable, Root, Handle, offLock, cbLock, SHFL_LOCK_SHARED);
    RTTEST_CHECK_RC_OK(hTest, rc);
#ifdef RT_OS_WINDOWS  /* Locking is a no-op elsewhere. */
    RTTEST_CHECK_MSG(hTest, g_testRTFileLockFile == hcFile, (hTest, "File=%u\n", g_testRTFileLockFile));
    RTTEST_CHECK_MSG(hTest, testRTFileLockfLock == 0,
                     (hTest, "fLock=%u\n", testRTFileLockfLock));
    RTTEST_CHECK_MSG(hTest, testRTFileLockOffset == offLock,
                     (hTest, "Offs=%llu\n", (long long) testRTFileLockOffset));
    RTTEST_CHECK_MSG(hTest, testRTFileLockSize == cbLock,
                     (hTest, "Size=%llu\n", LLUIFY(testRTFileLockSize)));
#endif
    rc = lockFile(&svcTable, Root, Handle, offLock, cbLock, SHFL_LOCK_CANCEL);
    RTTEST_CHECK_RC_OK(hTest, rc);
#ifdef RT_OS_WINDOWS
    RTTEST_CHECK_MSG(hTest, g_testRTFileUnlockFile == hcFile, (hTest, "File=%u\n", g_testRTFileUnlockFile));
    RTTEST_CHECK_MSG(hTest, testRTFileUnlockOffset == offLock,
                     (hTest, "Offs=%llu\n",
                      (long long) testRTFileUnlockOffset));
    RTTEST_CHECK_MSG(hTest, testRTFileUnlockSize == cbLock,
                     (hTest, "Size=%llu\n", LLUIFY(testRTFileUnlockSize)));
#endif
    unmapAndRemoveMapping(hTest, &svcTable, Root, "testname");
    AssertReleaseRC(svcTable.pfnDisconnect(NULL, 0, svcTable.pvService));
    AssertReleaseRC(svcTable.pfnUnload(NULL));
    RTTestGuardedFree(hTest, svcTable.pvService);
    RTTEST_CHECK_MSG(hTest, g_testRTFileCloseFile == hcFile, (hTest, "File=%u\n", g_testRTFileCloseFile));
}


/*********************************************************************************************************************************
*   Main code                                                                                                                    *
*********************************************************************************************************************************/

static void testAPI(RTTEST hTest)
{
    testMappingsQuery(hTest);
    testMappingsQueryName(hTest);
    testMapFolder(hTest);
    testUnmapFolder(hTest);
    testCreate(hTest);
    testClose(hTest);
    testRead(hTest);
    testWrite(hTest);
    testLock(hTest);
    testFlush(hTest);
    testDirList(hTest);
    testReadLink(hTest);
    testFSInfo(hTest);
    testRemove(hTest);
    testRename(hTest);
    testSymlink(hTest);
    testMappingsAdd(hTest);
    testMappingsRemove(hTest);
    /* testSetStatusLed(hTest); */
}

int main(int argc, char **argv)
{
    RT_NOREF1(argc);
    RTEXITCODE rcExit = RTTestInitAndCreate(RTPathFilename(argv[0]), &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(g_hTest);
    testAPI(g_hTest);
    return RTTestSummaryAndDestroy(g_hTest);
}
