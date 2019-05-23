/* $Id: UIMediumDefs.cpp $ */
/** @file
 * VBox Qt GUI - UIMedium related implementations.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIMediumDefs.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* Convert global medium type (KDeviceType) to local (UIMediumType): */
UIMediumType UIMediumDefs::mediumTypeToLocal(KDeviceType globalType)
{
    switch (globalType)
    {
        case KDeviceType_HardDisk:
            return UIMediumType_HardDisk;
        case KDeviceType_DVD:
            return UIMediumType_DVD;
        case KDeviceType_Floppy:
            return UIMediumType_Floppy;
        default:
            break;
    }
    return UIMediumType_Invalid;
}

/* Convert local medium type (UIMediumType) to global (KDeviceType): */
KDeviceType UIMediumDefs::mediumTypeToGlobal(UIMediumType localType)
{
    switch (localType)
    {
        case UIMediumType_HardDisk:
            return KDeviceType_HardDisk;
        case UIMediumType_DVD:
            return KDeviceType_DVD;
        case UIMediumType_Floppy:
            return KDeviceType_Floppy;
        default:
            break;
    }
    return KDeviceType_Null;
}

