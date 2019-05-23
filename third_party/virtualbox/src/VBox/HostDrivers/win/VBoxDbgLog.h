/* $Id: VBoxDbgLog.h $ */
/** @file
 * Logging helper
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

#ifndef ___VBoxDbgLog_h___
#define ___VBoxDbgLog_h___

#ifndef VBOX_DBG_LOG_NAME
# error VBOX_DBG_LOG_NAME should be defined!
#endif

/* Uncomment to show file/line info in the log */
/*#define VBOX_DBG_LOG_SHOWLINEINFO*/

#define VBOX_DBG_LOG_PREFIX_FMT VBOX_DBG_LOG_NAME"::"LOG_FN_FMT": "
#define VBOX_DBG_LOG_PREFIX_PARMS __PRETTY_FUNCTION__

#ifdef VBOX_DBG_LOG_SHOWLINEINFO
# define VBOX_DBG_LOG_SUFFIX_FMT " (%s:%d)\n"
# define VBOX_DBG_LOG_SUFFIX_PARMS ,__FILE__, __LINE__
#else
# define VBOX_DBG_LOG_SUFFIX_FMT "\n"
# define VBOX_DBG_LOG_SUFFIX_PARMS
#endif

#ifdef DEBUG_misha
# define BP_WARN() AssertFailed()
#else
# define BP_WARN() do { } while (0)
#endif

#define _LOGMSG_EXACT(_logger, _a)                                          \
    do                                                                      \
    {                                                                       \
        _logger(_a);                                                        \
    } while (0)

#define _LOGMSG(_logger, _a)                                                \
    do                                                                      \
    {                                                                       \
        _logger((VBOX_DBG_LOG_PREFIX_FMT, VBOX_DBG_LOG_PREFIX_PARMS));  \
        _logger(_a);                                                        \
        _logger((VBOX_DBG_LOG_SUFFIX_FMT  VBOX_DBG_LOG_SUFFIX_PARMS));  \
    } while (0)

/* we can not print paged strings to RT logger, do it this way */
#define _LOGMSG_STR(_logger, _a, _f) do {\
        int _i = 0; \
        _logger(("\"")); \
        for (;(_a)[_i];++_i) { \
            _logger(("%"_f, (_a)[_i])); \
        }\
        _logger(("\"\n")); \
    } while (0)

#define _LOGMSG_USTR(_logger, _a) do {\
        int _i = 0; \
        _logger(("\"")); \
        for (;_i<(_a)->Length/2;++_i) { \
            _logger(("%c", (_a)->Buffer[_i])); \
        }\
        _logger(("\"\n")); \
    } while (0)

#define WARN_NOBP(_a)                                                          \
    do                                                                            \
    {                                                                             \
        Log((VBOX_DBG_LOG_PREFIX_FMT"WARNING! ", VBOX_DBG_LOG_PREFIX_PARMS)); \
        Log(_a);                                                                  \
        Log((VBOX_DBG_LOG_SUFFIX_FMT VBOX_DBG_LOG_SUFFIX_PARMS));             \
    } while (0)

#define WARN(_a)                                                                  \
    do                                                                            \
    {                                                                             \
        WARN_NOBP(_a);                                                         \
        BP_WARN();                                                             \
    } while (0)

#define ASSERT_WARN(_a, _w) do {\
        if(!(_a)) { \
            WARN(_w); \
        }\
    } while (0)

#define LOG(_a) _LOGMSG(Log, _a)
#define LOGREL(_a) _LOGMSG(LogRel, _a)
#define LOGF(_a) _LOGMSG(LogFlow, _a)
#define LOGF_ENTER() LOGF(("ENTER"))
#define LOGF_LEAVE() LOGF(("LEAVE"))
#define LOG_EXACT(_a) _LOGMSG_EXACT(Log, _a)
#define LOGREL_EXACT(_a) _LOGMSG_EXACT(LogRel, _a)
/* we can not print paged strings to RT logger, do it this way */
#define LOG_STRA(_a) do {\
        _LOGMSG_STR(Log, _a, "c"); \
    } while (0)
#define LOG_STRW(_a) do {\
        _LOGMSG_STR(Log, _a, "c"); \
    } while (0)
#define LOG_USTR(_a) do {\
        _LOGMSG_USTR(Log, _a); \
    } while (0)
#define LOGREL_STRA(_a) do {\
        _LOGMSG_STR(LogRel, _a, "c"); \
    } while (0)
#define LOGREL_STRW(_a) do {\
        _LOGMSG_STR(LogRel, _a, "c"); \
    } while (0)
#define LOGREL_USTR(_a) do {\
        _LOGMSG_USTR(LogRel, _a); \
    } while (0)


#endif

