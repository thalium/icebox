/* $Id: UIDesktopServices_darwin.cpp $ */
/** @file
 * VBox Qt GUI - Qt GUI - Utility Classes and Functions specific to darwin..
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* VBox includes */
#include "UIDesktopServices.h"
#include "UIDesktopServices_darwin_p.h"
#include "VBoxUtils-darwin.h"

/* Qt includes */
#include <QString>

bool UIDesktopServices::createMachineShortcut(const QString &strSrcFile, const QString &strDstPath, const QString &strName, const QString &strUuid)
{
    return ::darwinCreateMachineShortcut(::darwinToNativeString(strSrcFile.toUtf8().constData()),
                                         ::darwinToNativeString(strDstPath.toUtf8().constData()),
                                         ::darwinToNativeString(strName.toUtf8().constData()),
                                         ::darwinToNativeString(strUuid.toUtf8().constData()));
}

bool UIDesktopServices::openInFileManager(const QString &strFile)
{
    return ::darwinOpenInFileManager(::darwinToNativeString(strFile.toUtf8().constData()));
}

