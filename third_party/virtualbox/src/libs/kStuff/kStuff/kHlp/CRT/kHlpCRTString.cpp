/* $Id: kHlpCRTString.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpString - String And Memory Routines, CRT based implementation.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpString.h>
#include <string.h>


#ifndef kHlpMemChr
void   *kHlpMemChr(const void *pv, int ch, KSIZE cb)
{
    return (void *)memchr(pv, ch, cb);
}
#endif


#ifndef kHlpMemComp
int     kHlpMemComp(const void *pv1, const void *pv2, KSIZE cb)
{
    return memcmp(pv1, pv2, cb);
}
#endif


#ifndef kHlpMemCopy
void   *kHlpMemCopy(void *pv1, const void *pv2, KSIZE cb)
{
    return memcpy(pv1, pv2, cb);
}
#endif


#ifndef kHlpMemPCopy
void   *kHlpMemPCopy(void *pv1, const void *pv2, KSIZE cb)
{
    return (KU8 *)memcpy(pv1, pv2, cb) + cb;
}
#endif


#ifndef kHlpMemMove
void   *kHlpMemMove(void *pv1, const void *pv2, KSIZE cb)
{
    return memmove(pv1, pv2, cb);
}
#endif


#ifndef kHlpMemPMove
void   *kHlpMemPMove(void *pv1, const void *pv2, KSIZE cb)
{
    return (KU8 *)memmove(pv1, pv2, cb) + cb;
}
#endif


#ifndef kHlpMemSet
void   *kHlpMemSet(void *pv1, int ch, KSIZE cb)
{
    return memset(pv1, ch, cb);
}
#endif


#ifndef kHlpMemPSet
void   *kHlpMemPSet(void *pv1, int ch, KSIZE cb)
{
    return (KU8 *)memset(pv1, ch, cb) + cb;
}
#endif


#ifndef kHlpStrCat
char   *kHlpStrCat(char *psz1, const char *psz2)
{
    return strcat(psz1, psz2);
}
#endif


#ifndef kHlpStrNCat
char   *kHlpStrNCat(char *psz1, const char *psz2, KSIZE cb)
{
    return strncat(psz1, psz2, cb);
}
#endif


#ifndef kHlpStrChr
char   *kHlpStrChr(const char *psz, int ch)
{
    return (char *)strchr(psz, ch);
}
#endif


#ifndef kHlpStrRChr
char   *kHlpStrRChr(const char *psz, int ch)
{
    return (char *)strrchr(psz, ch);
}
#endif


#ifndef kHlpStrComp
int     kHlpStrComp(const char *psz1, const char *psz2)
{
    return strcmp(psz1, psz2);
}
#endif


#ifndef kHlpStrNComp
int     kHlpStrNComp(const char *psz1, const char *psz2, KSIZE cch)
{
    return strncmp(psz1, psz2, cch);
}
#endif


#ifndef kHlpStrCopy
char   *kHlpStrCopy(char *psz1, const char *psz2)
{
    return strcpy(psz1, psz2);
}
#endif


#ifndef kHlpStrLen
KSIZE   kHlpStrLen(const char *psz1)
{
    return strlen(psz1);
}
#endif

