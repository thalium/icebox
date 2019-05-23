/* $Id: Logging.h $ */
/** @file
 * VirtualBox COM - logging macros and function definitions
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_LOGGING
#define ____H_LOGGING

/* @def LOG_GROUP_MAIN_OVERRIDE
 *
 * DEPRECATED: Define this macro to point to the desired log group before including
 * DEPRECATED: the |Logging.h| header if you want to use a group other than LOG_GROUP_MAIN
 * DEPRECATED: for logging from within Main source files.
 * DEPRECATED:
 * DEPRECATED: Example:
 * DEPRECATED: @code
 * DEPRECATED: #define LOG_GROUP_MAIN_OVERRIDE LOG_GROUP_HGCM
 * DEPRECATED: @endcode
 *
 * INSTEAD: Please define LOG_GROUP and include LoggingNew.h at the top of the source file!
 * INSTEAD: Please define LOG_GROUP and include LoggingNew.h at the top of the source file!
 * INSTEAD: Please define LOG_GROUP and include LoggingNew.h at the top of the source file!
 *
 */

/*
 *  We might be including the VBox logging subsystem before
 *  including this header file, so reset the logging group.
 */
#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#ifdef LOG_GROUP_MAIN_OVERRIDE
# define LOG_GROUP LOG_GROUP_MAIN_OVERRIDE
#else
# define LOG_GROUP LOG_GROUP_MAIN
#endif
#ifndef VBOXSVC_LOG_DEFAULT
# define VBOXSVC_LOG_DEFAULT "all"
#endif
#ifndef VBOXSDS_LOG_DEFAULT
# define VBOXSDS_LOG_DEFAULT "all"
#endif

#include <VBox/log.h>

#endif // !____H_LOGGING
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
