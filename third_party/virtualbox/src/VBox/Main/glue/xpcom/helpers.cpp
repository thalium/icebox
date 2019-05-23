/* $Id: helpers.cpp $ */
/** @file
 *
 * COM helper functions for XPCOM
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

#include "VBox/com/defs.h"

#include <nsMemory.h>

#include <iprt/string.h>


//
// OLE Automation string APIs
//

// Note: on Windows, every BSTR stores its length in the
// byte just before the pointer you get. If we do it like this,
// the caller cannot just use nsMemory::Free() on our strings.
// Therefore we'll try to implement it differently and hope that
// we don't run into problems.

/**
 * Copies a string into a new memory block including the terminating UCS2 NULL
 * @param sz source string to copy
 * @returns BSTR new string buffer
 */
BSTR SysAllocString(const OLECHAR* sz)
{
    if (!sz)
    {
        return NULL;
    }
    return SysAllocStringLen(sz, SysStringLen((BSTR)sz));
}

/**
 * Copies len OLECHARs of a string into a new memory block and
 * adds a terminating UCS2 NULL
 * @param psz source string to copy
 * @param len length of the source string in bytes
 * @returns BSTR new string buffer
 */
BSTR SysAllocStringByteLen(char *psz, unsigned int len)
{
    unsigned int *newBuffer;
    char *newString;

    newBuffer = (unsigned int*)nsMemory::Alloc(len + sizeof(OLECHAR));
    if (!newBuffer)
    {
        return NULL;
    }
    if (psz)
    {
        memcpy(newBuffer, psz, len);
    }
    // make sure there is a trailing UCS2 NULL
    newString = (char*)newBuffer;
    newString[len] = '\0';
    newString[len + 1] = '\0';
    return (BSTR)newString;
}

/**
 * Create a BSTR from the OLECHAR string with a given length in UCS2 characters
 * @param pch pointer to the source string
 * @param cch length of the source string in UCS2 characters
 * @returns BSTR new string buffer
 */
BSTR SysAllocStringLen(const OLECHAR *pch, unsigned int cch)
{
    unsigned int bufferSize;
    unsigned int *newBuffer;
    OLECHAR *newString;

    // add the trailing UCS2 NULL
    bufferSize = cch * sizeof(OLECHAR);
    newBuffer = (unsigned int*)nsMemory::Alloc(bufferSize + sizeof(OLECHAR));
    if (!newBuffer)
    {
        return NULL;
    }
    // copy the string, a NULL input string is allowed
    if (pch)
    {
        memcpy(newBuffer, pch, bufferSize);

    } else
    {
        memset(newBuffer, 0, bufferSize);
    }
    // make sure there is a trailing UCS2 NULL
    newString = (OLECHAR*)newBuffer;
    newString[cch] = L'\0';

    return (BSTR)newString;
}

/**
 * Frees the memory associated with the BSTR given
 * @param bstr source string to free
 */
void SysFreeString(BSTR bstr)
{
    if (bstr)
    {
        nsMemory::Free(bstr);
    }
}

/**
 * Reallocates a string by freeing the old string and copying
 * a new string into a new buffer.
 * @param pbstr old string to free
 * @param psz source string to copy into the new string
 * @returns success indicator
 */
int SysReAllocString(BSTR *pbstr, const OLECHAR *psz)
{
    if (!pbstr)
    {
        return 0;
    }
    SysFreeString(*pbstr);
    *pbstr = SysAllocString(psz);
    return 1;
}

#if 0
/* Does not work -- we ignore newBuffer! */
/**
 * Changes the length of a previous created BSTR
 * @param pbstr string to change the length of
 * @param psz source string to copy into the adjusted pbstr
 * @param cch length of the source string in UCS2 characters
 * @returns int success indicator
 */
int SysReAllocStringLen(BSTR *pbstr, const OLECHAR *psz, unsigned int cch)
{
    if (SysStringLen(*pbstr) > 0)
    {
        unsigned int newByteLen;
        unsigned int *newBuffer;
        newByteLen = cch * sizeof(OLECHAR);
        newBuffer = (unsigned int*)nsMemory::Realloc((void*)*pbstr,
                                                     newByteLen + sizeof(OLECHAR));
        if (psz)
        {
            memcpy(*pbstr, psz, newByteLen);
            *pbstr[cch] = 0;
        }
    } else
    {
        // allocate a new string
        *pbstr = SysAllocStringLen(psz, cch);
    }
    return 1;
}
#endif

/**
  * Returns the string length in bytes without the terminator
  * @returns unsigned int length in bytes
  * @param bstr source string
  */
unsigned int SysStringByteLen(BSTR bstr)
{
    return RTUtf16Len(bstr) * sizeof(OLECHAR);
}

/**
  * Returns the string length in OLECHARs without the terminator
  * @returns unsigned int length in OLECHARs
  * @param bstr source string
  */
unsigned int SysStringLen(BSTR bstr)
{
    return RTUtf16Len(bstr);
}
