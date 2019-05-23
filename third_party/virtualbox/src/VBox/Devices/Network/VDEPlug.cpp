/** @file
 *
 * Module to dynamically load libvdeplug and load all symbols
 * which are needed by VirtualBox.
 */

/*
 * Copyright (C) 2008-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP RTLOGGROUP_LDR
/* Declarations of the functions that we need from libdbus-1 */
#define VDEPLUG_GENERATE_BODY
#include <VBox/VDEPlugSymDefs.h>
