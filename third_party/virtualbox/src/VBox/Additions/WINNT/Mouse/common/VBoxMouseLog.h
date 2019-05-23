/* $Id: VBoxMouseLog.h $ */
/** @file
 * VBox Mouse drivers, logging helper
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
 */

#ifndef VBOXMOUSELOG_H
#define VBOXMOUSELOG_H

#include <VBox/log.h>
#include <iprt/assert.h>

#define VBOX_MOUSE_LOG_NAME "VBoxMouse"

/* Uncomment to show file/line info in the log */
/*#define VBOX_MOUSE_LOG_SHOWLINEINFO*/

#define VBOX_MOUSE_LOG_PREFIX_FMT VBOX_MOUSE_LOG_NAME"::"LOG_FN_FMT": "
#define VBOX_MOUSE_LOG_PREFIX_PARMS __PRETTY_FUNCTION__

#ifdef VBOX_MOUSE_LOG_SHOWLINEINFO
# define VBOX_MOUSE_LOG_SUFFIX_FMT " (%s:%d)\n"
# define VBOX_MOUSE_LOG_SUFFIX_PARMS ,__FILE__, __LINE__
#else
# define VBOX_MOUSE_LOG_SUFFIX_FMT "\n"
# define VBOX_MOUSE_LOG_SUFFIX_PARMS
#endif

#define _LOGMSG(_logger, _a)                                                \
    do                                                                      \
    {                                                                       \
        _logger((VBOX_MOUSE_LOG_PREFIX_FMT, VBOX_MOUSE_LOG_PREFIX_PARMS));  \
        _logger(_a);                                                        \
        _logger((VBOX_MOUSE_LOG_SUFFIX_FMT  VBOX_MOUSE_LOG_SUFFIX_PARMS));  \
    } while (0)

#if 1 /* Exclude yourself if you're not keen on this. */
# define BREAK_WARN() AssertFailed()
#else
# define BREAK_WARN() do {} while(0)
#endif

#define WARN(_a)                                                                  \
    do                                                                            \
    {                                                                             \
        Log((VBOX_MOUSE_LOG_PREFIX_FMT"WARNING! ", VBOX_MOUSE_LOG_PREFIX_PARMS)); \
        Log(_a);                                                                  \
        Log((VBOX_MOUSE_LOG_SUFFIX_FMT VBOX_MOUSE_LOG_SUFFIX_PARMS));             \
        BREAK_WARN(); \
    } while (0)

#define LOG(_a) _LOGMSG(Log, _a)
#define LOGREL(_a) _LOGMSG(LogRel, _a)
#define LOGF(_a) _LOGMSG(LogFlow, _a)
#define LOGF_ENTER() LOGF(("ENTER"))
#define LOGF_LEAVE() LOGF(("LEAVE"))

#endif /* !VBOXMOUSELOG_H */

