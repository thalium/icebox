/* $Id: VBoxServiceToolBox.h $ */
/** @file
 * VBoxService - Toolbox header for sharing defines between toolbox binary and VBoxService.
 */

/*
 * Copyright (C) 2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxServiceToolBox_h
#define ___VBoxServiceToolBox_h

#include <VBox/GuestHost/GuestControl.h>

RT_C_DECLS_BEGIN
extern bool                     VGSvcToolboxMain(int argc, char **argv, RTEXITCODE *prcExit);
extern int                      VGSvcToolboxExitCodeConvertToRc(const char *pszTool, RTEXITCODE rcExit);
RT_C_DECLS_END

#endif /* ___VBoxServiceToolBox_h */

