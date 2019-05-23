/* $Id: VBoxMPCrUtil.cpp $ */
/** @file
 * VBox WDDM Miniport driver
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxMPWddm.h"

#include <iprt/cdefs.h>

RT_C_DECLS_BEGIN

DECLEXPORT(void *) crAlloc( unsigned int nbytes );
//extern DECLEXPORT(void *) crCalloc( unsigned int nbytes );
//extern DECLEXPORT(void) crRealloc( void **ptr, unsigned int bytes );
DECLEXPORT(void) crFree( void *ptr );
DECLEXPORT(void) crMemcpy( void *dst, const void *src, unsigned int bytes );
DECLEXPORT(void) crMemset( void *ptr, int value, unsigned int bytes );
DECLEXPORT(void) crMemZero( void *ptr, unsigned int bytes );
DECLEXPORT(int)  crMemcmp( const void *p1, const void *p2, unsigned int bytes );
DECLEXPORT(void) crDebug(const char *format, ... );
#ifndef DEBUG_misha
DECLEXPORT(void) crWarning(const char *format, ... );
#endif

DECLEXPORT(void) crInfo(const char *format, ... );
DECLEXPORT(void) crError(const char *format, ... );

RT_C_DECLS_END

DECLEXPORT(void *) crAlloc( unsigned int nbytes )
{
    return vboxWddmMemAllocZero(nbytes);
}
//extern DECLEXPORT(void *) crCalloc( unsigned int nbytes );
//extern DECLEXPORT(void) crRealloc( void **ptr, unsigned int bytes );
DECLEXPORT(void) crFree( void *ptr )
{
    vboxWddmMemFree(ptr);
}

DECLEXPORT(void) crMemcpy( void *dst, const void *src, unsigned int bytes )
{
    memcpy(dst, src, bytes);
}

DECLEXPORT(void) crMemset( void *ptr, int value, unsigned int bytes )
{
    memset(ptr, value, bytes);
}

DECLEXPORT(void) crMemZero( void *ptr, unsigned int bytes )
{
    memset(ptr, 0, bytes);
}

DECLEXPORT(int)  crMemcmp( const void *p1, const void *p2, unsigned int bytes )
{
    return memcmp(p1, p2, bytes);
}

DECLEXPORT(void) crDebug(const char *format, ... )
{
    RT_NOREF(format);
}

#ifndef DEBUG_misha
DECLEXPORT(void) crWarning(const char *format, ... )
{
    RT_NOREF(format);
}
#endif

DECLEXPORT(void) crInfo(const char *format, ... )
{
    RT_NOREF(format);
}

DECLEXPORT(void) crError(const char *format, ... )
{
    RT_NOREF(format);
}

