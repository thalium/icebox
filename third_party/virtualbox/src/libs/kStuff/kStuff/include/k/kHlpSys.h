/* $Id: kHlpSys.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpSys - System Call Prototypes.
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

#ifndef ___k_kHlpSys_h___
#define ___k_kHlpSys_h___

#include <k/kHlpDefs.h>
#include <k/kTypes.h>

/** @defgroup grp_kHlpSys kHlpSys - System Call Prototypes
 * @addtogroup grp_kHlp
 * @{*/

#ifdef __cplusplus
extern "C" {
#endif

/* common unix stuff. */
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
KSSIZE      kHlpSys_readlink(const char *pszPath, char *pszBuf, KSIZE cbBuf);
int         kHlpSys_open(const char *filename, int flags, int mode);
int         kHlpSys_close(int fd);
KFOFF       kHlpSys_lseek(int fd, int whench, KFOFF off);
KSSIZE      kHlpSys_read(int fd, void *pvBuf, KSIZE cbBuf);
KSSIZE      kHlpSys_write(int fd, const void *pvBuf, KSIZE cbBuf);
void       *kHlpSys_mmap(void *addr, KSIZE len, int prot, int flags, int fd, KI64 off);
int         kHlpSys_mprotect(void *addr, KSIZE len, int prot);
int         kHlpSys_munmap(void *addr, KSIZE len);
void        kHlpSys_exit(int rc);
#endif

/* specific */
#if K_OS == K_OS_DARWIN

#elif K_OS == K_OS_LINUX

#endif

#ifdef __cplusplus
}
#endif

/** @} */

#endif


