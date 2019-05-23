/* $Id: VBoxDispMpLogger.h $ */
/** @file
 * VBox WDDM Display backdoor logger API
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

/* We're unable to use standard r3 vbgl-based backdoor logging API because win8 Metro apps
 * can not do CreateFile/Read/Write by default
 * this is why we use miniport escape functionality to issue backdoor log string to the miniport
 * and submit it to host via standard r0 backdoor logging api accordingly */

#ifndef ___VBoxDispMpLogger_h__
#define ___VBoxDispMpLogger_h__

#include <iprt/cdefs.h>

#if 0
/* enable this in case we include this in a dll*/
# ifdef IN_VBOXCRHGSMI
#  define VBOXDISPMPLOGGER_DECL(a_Type) DECLEXPORT(a_Type) RTCALL
# else
#  define VBOXDISPMPLOGGER_DECL(a_Type) DECLIMPORT(a_Type) RTCALL
# endif
#else
/*enable this in case we include this in a static lib*/
# define VBOXDISPMPLOGGER_DECL(a_Type) a_Type RTCALL
#endif

RT_C_DECLS_BEGIN

VBOXDISPMPLOGGER_DECL(int) VBoxDispMpLoggerInit(void);

VBOXDISPMPLOGGER_DECL(int) VBoxDispMpLoggerTerm(void);

VBOXDISPMPLOGGER_DECL(void) VBoxDispMpLoggerLog(const char *pszString);

VBOXDISPMPLOGGER_DECL(void) VBoxDispMpLoggerLogF(const char *pszString, ...);

DECLCALLBACK(void) VBoxWddmUmLog(const char *pszString);

RT_C_DECLS_END

#endif /* #ifndef ___VBoxDispMpLogger_h__ */

