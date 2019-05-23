/** @file
 * IPRT - Include Everything.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___iprt_runtime_h
#define ___iprt_runtime_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/param.h>
#include <iprt/initterm.h>

#if !defined(IN_RC) && !defined(IN_RING0_AGNOSTIC)
# include <iprt/alloca.h>
#endif
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/crc.h>
#include <iprt/critsect.h>
#include <iprt/dir.h>
#include <iprt/err.h>
#ifndef IN_RC
# include <iprt/file.h>
# include <iprt/fs.h>
#endif
#include <iprt/getopt.h>
#include <iprt/ldr.h>
#include <iprt/log.h>
#include <iprt/md5.h>
#ifndef IN_RC
# include <iprt/mem.h>
# include <iprt/mp.h>
#endif
#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/spinlock.h>
#include <iprt/stdarg.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/table.h>
#ifndef IN_RC
# include <iprt/thread.h>
#endif
#include <iprt/time.h>
#include <iprt/timer.h>
#include <iprt/uni.h>
#include <iprt/uuid.h>
#include <iprt/zip.h>

#ifdef IN_RING3
# include <iprt/stream.h>
# include <iprt/tcp.h>
# include <iprt/ctype.h>
# include <iprt/alloca.h>  /** @todo iprt/alloca.h should be made available in R0 and GC too! */
# include <iprt/process.h> /** @todo iprt/process.h should be made available in R0 too (partly). */
#endif

#ifdef IN_RING0
# include <iprt/memobj.h>
#endif


#endif

