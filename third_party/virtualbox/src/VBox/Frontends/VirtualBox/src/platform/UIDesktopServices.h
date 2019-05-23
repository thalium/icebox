/* $Id: UIDesktopServices.h $ */
/** @file
 * VBox Qt GUI - Desktop Services..
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

#ifndef ___UIDesktopServices_h___
#define ___UIDesktopServices_h___

/* Qt forward declarations */
class QString;

class UIDesktopServices
{
public:
    static bool createMachineShortcut(const QString &strSrcFile, const QString &strDstPath, const QString &strName, const QString &strUuid);
    static bool openInFileManager(const QString &strFile);
};

#endif /* !___UIDesktopServices_h___ */

