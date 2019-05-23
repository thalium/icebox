/* $Id: chk_stubs.c $ */
/** @file
 * glibc stubs for the VirtualBox Guest Addition X11 Client.
 */

/*
 * Copyright (C) 2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* If we want the binary to be usable with glibc 2.3, we have to prevent
 VBoxClient from containing later symbols.  This includes resolution of
 symbols from supc++ and gcc_eh. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

extern int __sprintf_chk(char *psz, int fFlags, size_t cb, const char *pszFormat, ...);
int __sprintf_chk(char *psz, int fFlags, size_t cb, const char *pszFormat, ...)
{
    int rc;
    va_list va;

    (void)fFlags;
    va_start(va, pszFormat);
    rc = vsnprintf(psz, cb, pszFormat, va);
    va_end(va);
    return rc;
}

extern void __stack_chk_fail(void);
void __stack_chk_fail(void)
{
    fprintf(stderr, "Stack check failed!\n");
    _exit(1);
}

#ifdef __x86_64
/* Furthermore, wrap references to memcpy to force them to go to the right
 * version.  We are forced to do it this way because the shared libraries
 * supc++ and gcc_eh contain references which we cannot change. */

extern void *__wrap_memcpy(void *dest, const void *src, size_t n);

asm (".symver memcpy, memcpy@GLIBC_2.2.5");
void *__wrap_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}
#endif
