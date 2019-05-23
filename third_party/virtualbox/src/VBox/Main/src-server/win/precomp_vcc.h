/* $Id: precomp_vcc.h $ */
/** @file
 * VirtualBox COM - Visual C++ precompiled header for VBoxSVC.
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


#include <iprt/cdefs.h>
#include <iprt/win/winsock2.h>
#include <iprt/win/windows.h>
#include <VBox/cdefs.h>
#include <iprt/types.h>
#include <iprt/cpp/list.h>
#include <iprt/cpp/meta.h>
#include <iprt/cpp/ministring.h>
#include <VBox/com/microatl.h>
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/Guid.h>
#include <VBox/com/string.h>

#include "VBox/com/VirtualBox.h"

#if defined(Log) || defined(LogIsEnabled)
# error "Log() from iprt/log.h cannot be defined in the precompiled header!"
#endif

