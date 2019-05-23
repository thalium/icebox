/** $Id: VBoxSFFind.cpp $ */
/** @file
 * VBoxSF - OS/2 Shared Folders, Find File IFS EPs.
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
FS32_FINDFIRST(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszName, USHORT iCurDirEnd, USHORT fAttr,
               PFSFSI pfsfsi, PVBOXSFFS pfsfsd, PBYTE pbData, USHORT cbData, PUSHORT pcMatch,
               USHORT uLevel, USHORT fFlags)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszName); NOREF(iCurDirEnd); NOREF(fAttr); NOREF(pfsfsi); NOREF(pfsfsd); NOREF(pbData);
    NOREF(cbData); NOREF(pcMatch); NOREF(uLevel); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FINDFROMNAME(PFSFSI pfsfsi, PVBOXSFFS pfsfsd, PBYTE pbData, USHORT cbData, PUSHORT pcMatch,
                  USHORT uLevel, ULONG position, PCSZ pszName, USHORT fFlags)
{
    NOREF(pfsfsi); NOREF(pfsfsd); NOREF(pbData); NOREF(cbData); NOREF(pcMatch); NOREF(uLevel); NOREF(position); NOREF(pszName);
    NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FINDNEXT(PFSFSI pfsfsi, PVBOXSFFS pfsfsd, PBYTE pbData, USHORT cbData, PUSHORT pcMatch,
              USHORT uLevel, USHORT fFlags)
{
    NOREF(pfsfsi); NOREF(pfsfsd); NOREF(pbData); NOREF(cbData); NOREF(pcMatch); NOREF(uLevel); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FINDCLOSE(PFSFSI pfsfsi, PVBOXSFFS pfsfsd)
{
    NOREF(pfsfsi); NOREF(pfsfsd);
    return ERROR_NOT_SUPPORTED;
}





DECLASM(int)
FS32_FINDNOTIFYFIRST(PCDFSI pcdfsi, PVBOXSFCD pcdfsd, PCSZ pszName, USHORT iCurDirEnd, USHORT fAtt,
                     PUSHORT phHandle, PBYTE pbData, USHORT cbData, PUSHORT pcMatch,
                     USHORT uLevel, USHORT fFlags)
{
    NOREF(pcdfsi); NOREF(pcdfsd); NOREF(pszName); NOREF(iCurDirEnd); NOREF(fAtt); NOREF(phHandle); NOREF(pbData); NOREF(cbData);
    NOREF(pcMatch); NOREF(uLevel); NOREF(fFlags);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FINDNOTIFYNEXT(USHORT hHandle, PBYTE pbData, USHORT cbData, PUSHORT pcMatch,
                    USHORT uLevel, ULONG cMsTimeout)
{
    NOREF(hHandle); NOREF(pbData); NOREF(cbData); NOREF(pcMatch); NOREF(uLevel); NOREF(cMsTimeout);
    return ERROR_NOT_SUPPORTED;
}


DECLASM(int)
FS32_FINDNOTIFYCLOSE(USHORT hHandle)
{
    NOREF(hHandle);
    return ERROR_NOT_SUPPORTED;
}

