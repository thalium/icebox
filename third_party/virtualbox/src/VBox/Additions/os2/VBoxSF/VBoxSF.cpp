/** $Id: VBoxSF.cpp $ */
/** @file
 * VBoxSF - OS/2 Shared Folders, the FS and FSD level IFS EPs
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



DECLASM(void)
FS32_EXIT(ULONG uid, ULONG pid, ULONG pdb)
{
    NOREF(uid); NOREF(pid); NOREF(pdb);
}


DECLASM(int)
FS32_SHUTDOWN(ULONG type, ULONG reserved)
{
    NOREF(type); NOREF(reserved);
    return NO_ERROR;
}


DECLASM(int)
FS32_ATTACH(ULONG fFlags, PCSZ pszDev, PVBOXSFVP pvpfsd, PVBOXSFCD pcdfsd, PBYTE pszParm, PUSHORT pcbParm)
{
    NOREF(fFlags); NOREF(pszDev); NOREF(pvpfsd); NOREF(pcdfsd); NOREF(pszParm); NOREF(pcbParm);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FLUSHBUF(USHORT hVPB, ULONG fFlags)
{
    NOREF(hVPB); NOREF(fFlags);
    return NO_ERROR;
}


DECLASM(int)
FS32_FSINFO(ULONG fFlags, USHORT hVPB, PBYTE pbData, USHORT cbData, ULONG uLevel)
{
    NOREF(fFlags); NOREF(hVPB); NOREF(pbData); NOREF(cbData); NOREF(uLevel);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FSCTL(union argdat *pArgdat, ULONG iArgType, ULONG uFunction,
           PVOID pvParm, USHORT cbParm, PUSHORT pcbParmIO,
           PVOID pvData, USHORT cbData, PUSHORT pcbDataIO)
{
    NOREF(pArgdat); NOREF(iArgType); NOREF(uFunction); NOREF(pvParm); NOREF(cbParm); NOREF(pcbParmIO);
    NOREF(pvData); NOREF(cbData); NOREF(pcbDataIO);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_PROCESSNAME(PSZ pszName)
{
    NOREF(pszName);
    return NO_ERROR;
}


DECLASM(int)
FS32_CHDIR(ULONG fFlags, PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszDir, USHORT iCurDirEnd)
{
    NOREF(fFlags); NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszDir); NOREF(iCurDirEnd);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_MKDIR(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszDir, USHORT iCurDirEnd,
           PBYTE pbEABuf, ULONG fFlags)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszDir); NOREF(iCurDirEnd); NOREF(pbEABuf); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_RMDIR(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszDir, USHORT iCurDirEnd)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszDir); NOREF(iCurDirEnd);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_COPY(USHORT fFlags, PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszSrc, USHORT iSrcCurDirEnd,
          PCSZ pszDst, USHORT iDstCurDirEnd, USHORT uNameType)
{
    NOREF(fFlags); NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszSrc); NOREF(iSrcCurDirEnd);
    NOREF(pszDst); NOREF(iDstCurDirEnd); NOREF(uNameType);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_MOVE(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszSrc, USHORT iSrcCurDirEnd,
          PCSZ pszDst, USHORT iDstCurDirEnd, USHORT uNameType)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszSrc); NOREF(iSrcCurDirEnd); NOREF(pszDst); NOREF(iDstCurDirEnd); NOREF(uNameType);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_DELETE(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszFile, USHORT iCurDirEnd)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszFile); NOREF(iCurDirEnd);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FILEATTRIBUTE(ULONG fFlags, PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszName, USHORT iCurDirEnd, PUSHORT pfAttr)
{
    NOREF(fFlags); NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszName); NOREF(iCurDirEnd); NOREF(pfAttr);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_PATHINFO(USHORT fFlags, PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszName, USHORT iCurDirEnd,
              USHORT uLevel, PBYTE pbData, USHORT cbData)
{
    NOREF(fFlags); NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszName); NOREF(iCurDirEnd); NOREF(uLevel); NOREF(pbData); NOREF(cbData);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_MOUNT(USHORT fFlags, PVPFSI pvpfsi, PVBOXSFVP pvpfsd, USHORT hVPB, PCSZ pszBoot)
{
    NOREF(fFlags); NOREF(pvpfsi); NOREF(pvpfsd); NOREF(hVPB); NOREF(pszBoot);
    return ERROR_NOT_SUPPORTED;
}

