/** $Id: VBoxSFFile.cpp $ */
/** @file
 * VBoxSF - OS/2 Shared Folders, the file level IFS EPs.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT
#include "VBoxSFInternal.h"

#include <VBox/log.h>
#include <iprt/assert.h>



DECLASM(int)
FS32_OPENCREATE(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszName, USHORT iCurDirEnd,
                PSFFSI psffsi, PVBOXSFFSD psffsd, ULONG uOpenMode, USHORT fOpenFlag,
                PUSHORT puAction, USHORT fAttr, PBYTE pbEABuf, PUSHORT pfGenFlag)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszName); NOREF(iCurDirEnd); NOREF(psffsi); NOREF(psffsd); NOREF(uOpenMode);
    NOREF(fOpenFlag); NOREF(puAction); NOREF(fAttr); NOREF(pbEABuf); NOREF(pfGenFlag);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_CLOSE(ULONG uType, ULONG fIoFlags, PSFFSI psffsi, PVBOXSFFSD psffsd)
{
    NOREF(uType); NOREF(fIoFlags); NOREF(psffsi); NOREF(psffsd);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_COMMIT(ULONG uType, ULONG fIoFlags, PSFFSI psffsi, PVBOXSFFSD psffsd)
{
    NOREF(uType); NOREF(fIoFlags); NOREF(psffsi); NOREF(psffsd);
    return ERROR_NOT_SUPPORTED;
}


extern "C" APIRET APIENTRY
FS32_CHGFILEPTRL(PSFFSI psffsi, PVBOXSFFSD psffsd, LONGLONG off, ULONG uMethod, ULONG fIoFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(off); NOREF(uMethod); NOREF(fIoFlags);
    return ERROR_NOT_SUPPORTED;
}


/** Forwards the call to FS32_CHGFILEPTRL. */
extern "C" APIRET APIENTRY
FS32_CHGFILEPTR(PSFFSI psffsi, PVBOXSFFSD psffsd, LONG off, ULONG uMethod, ULONG fIoFlags)
{
    return FS32_CHGFILEPTRL(psffsi, psffsd, off, uMethod, fIoFlags);
}

DECLASM(int)
FS32_FILEINFO(ULONG fFlag, PSFFSI psffsi, PVBOXSFFSD psffsd, ULONG uLevel,
              PBYTE pbData, ULONG cbData, ULONG fIoFlags)
{
    NOREF(fFlag); NOREF(psffsi); NOREF(psffsd); NOREF(uLevel); NOREF(pbData); NOREF(cbData); NOREF(fIoFlags);
    return ERROR_NOT_SUPPORTED;
}

DECLASM(int)
FS32_NEWSIZEL(PSFFSI psffsi, PVBOXSFFSD psffsd, LONGLONG cbFile, ULONG fIoFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(cbFile); NOREF(fIoFlags);
    return ERROR_NOT_SUPPORTED;
}


extern "C" APIRET APIENTRY
FS32_READ(PSFFSI psffsi, PVBOXSFFSD psffsd, PVOID pvData, PULONG pcb, ULONG fIoFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pvData); NOREF(pcb); NOREF(fIoFlags);
    return ERROR_NOT_SUPPORTED;
}


extern "C" APIRET APIENTRY
FS32_WRITE(PSFFSI psffsi, PVBOXSFFSD psffsd, PVOID pvData, PULONG pcb, ULONG fIoFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pvData); NOREF(pcb); NOREF(fIoFlags);
    return ERROR_NOT_SUPPORTED;
}


extern "C" APIRET APIENTRY
FS32_READFILEATCACHE(PSFFSI psffsi, PVBOXSFFSD psffsd, ULONG fIoFlags, LONGLONG off, ULONG pcb, KernCacheList_t **ppCacheList)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(fIoFlags); NOREF(off); NOREF(pcb); NOREF(ppCacheList);
    return ERROR_NOT_SUPPORTED;
}


extern "C" APIRET APIENTRY
FS32_RETURNFILECACHE(KernCacheList_t *pCacheList)
{
    NOREF(pCacheList);
    return ERROR_NOT_SUPPORTED;
}


/* oddments */

DECLASM(int)
FS32_CANCELLOCKREQUESTL(PSFFSI psffsi, PVBOXSFFSD psffsd, struct filelockl *pLockRange)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pLockRange);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_CANCELLOCKREQUEST(PSFFSI psffsi, PVBOXSFFSD psffsd, struct filelock *pLockRange)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pLockRange);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FILELOCKSL(PSFFSI psffsi, PVBOXSFFSD psffsd, struct filelockl *pUnLockRange,
                struct filelockl *pLockRange, ULONG cMsTimeout, ULONG fFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pUnLockRange); NOREF(pLockRange); NOREF(cMsTimeout); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FILELOCKS(PSFFSI psffsi, PVBOXSFFSD psffsd, struct filelock *pUnLockRange,
               struct filelock *pLockRange, ULONG cMsTimeout, ULONG fFlags)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pUnLockRange); NOREF(pLockRange); NOREF(cMsTimeout); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_IOCTL(PSFFSI psffsi, PVBOXSFFSD psffsd, USHORT uCategory, USHORT uFunction,
           PVOID pvParm, USHORT cbParm, PUSHORT pcbParmIO,
           PVOID pvData, USHORT cbData, PUSHORT pcbDataIO)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(uCategory); NOREF(uFunction); NOREF(pvParm); NOREF(cbParm); NOREF(pcbParmIO);
    NOREF(pvData); NOREF(cbData); NOREF(pcbDataIO);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FILEIO(PSFFSI psffsi, PVBOXSFFSD psffsd, PBYTE pbCmdList, USHORT cbCmdList,
            PUSHORT poffError, USHORT fIoFlag)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pbCmdList); NOREF(cbCmdList); NOREF(poffError); NOREF(fIoFlag);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_NMPIPE(PSFFSI psffsi, PVBOXSFFSD psffsd, USHORT uOpType, union npoper *pOpRec,
            PBYTE pbData, PCSZ pszName)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(uOpType); NOREF(pOpRec); NOREF(pbData); NOREF(pszName);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_OPENPAGEFILE(PULONG pfFlags, PULONG pcMaxReq, PCSZ pszName, PSFFSI psffsi, PVBOXSFFSD psffsd,
                  USHORT uOpenMode, USHORT fOpenFlags, USHORT fAttr, ULONG uReserved)
{
    NOREF(pfFlags); NOREF(pcMaxReq); NOREF(pszName); NOREF(psffsi); NOREF(psffsd); NOREF(uOpenMode); NOREF(fOpenFlags);
    NOREF(fAttr); NOREF(uReserved);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_SETSWAP(PSFFSI psffsi, PVBOXSFFSD psffsd)
{
    NOREF(psffsi); NOREF(psffsd);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_ALLOCATEPAGESPACE(PSFFSI psffsi, PVBOXSFFSD psffsd, ULONG cb, USHORT cbWantContig)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(cb); NOREF(cbWantContig);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_DOPAGEIO(PSFFSI psffsi, PVBOXSFFSD psffsd, struct PageCmdHeader *pList)
{
    NOREF(psffsi); NOREF(psffsd); NOREF(pList);
    return ERROR_NOT_SUPPORTED;
}

