/* $Id: DnDMIME.cpp $ */
/** @file
 * DnD: Path list class.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/string.h>

#include <VBox/GuestHost/DragAndDrop.h>

bool DnDMIMEHasFileURLs(const char *pcszFormat, size_t cchFormatMax)
{
    /** @todo "text/uri" also an official variant? */
    return (   RTStrNICmp(pcszFormat, "text/uri-list", cchFormatMax)             == 0
            || RTStrNICmp(pcszFormat, "x-special/gnome-icon-list", cchFormatMax) == 0);
}

bool DnDMIMENeedsDropDir(const char *pcszFormat, size_t cchFormatMax)
{
    bool fNeedsDropDir = false;
    if (!RTStrNICmp(pcszFormat, "text/uri-list", cchFormatMax)) /** @todo Add "x-special/gnome-icon-list"? */
        fNeedsDropDir = true;

    return fNeedsDropDir;
}

