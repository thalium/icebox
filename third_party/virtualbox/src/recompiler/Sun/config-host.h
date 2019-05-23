/* $Id: config-host.h $ */
/** @file
 * Sun host config - maintained by hand
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


#if defined(RT_ARCH_AMD64)
# define HOST_X86_64 1
# define HOST_LONG_BITS 64
#else
# define HOST_I386 1
# define HOST_LONG_BITS 32
#endif

#ifndef IPRT_NO_CRT
# ifdef RT_OS_WINDOWS
#  define CONFIG_WIN32 1
# elif defined(RT_OS_OS2)
#  define CONFIG_OS2
# elif defined(RT_OS_DARWIN)
#  define CONFIG_DARWIN
# elif defined(RT_OS_FREEBSD) || defined(RT_OS_NETBSD) || defined(RT_OS_OPENBSD)
#  define HAVE_MACHINE_BSWAP_H
/*#  define CONFIG_BSD*/
# elif defined(RT_OS_SOLARIS)
#  define CONFIG_SOLARIS
# else
#  define HAVE_BYTESWAP_H 1
# endif
#endif
#define QEMU_VERSION "0.13.0"
#define CONFIG_UNAME_RELEASE ""
#define CONFIG_QEMU_SHAREDIR "."

