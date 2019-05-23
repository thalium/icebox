/* $Id: UIMediumDefs.h $ */
/** @file
 * VBox Qt GUI - UIMedium related declarations.
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

#ifndef ___UIMediumDefs_h___
#define ___UIMediumDefs_h___

/* Qt includes: */
#include <QString>

/* COM includes: */
#include "COMEnums.h"

/* Other VBox includes: */
#include <VBox/com/defs.h>


/** UIMediumDefs namespace. */
namespace UIMediumDefs
{
    /** UIMedium types. */
    enum UIMediumType
    {
        UIMediumType_HardDisk,
        UIMediumType_DVD,
        UIMediumType_Floppy,
        UIMediumType_All,
        UIMediumType_Invalid
    };

    /** Converts global medium type (KDeviceType) to local (UIMediumType). */
    UIMediumType mediumTypeToLocal(KDeviceType globalType);

    /** Convert local medium type (UIMediumType) to global (KDeviceType). */
    KDeviceType mediumTypeToGlobal(UIMediumType localType);
}
/* Using this namespace globally: */
using namespace UIMediumDefs;

/** Medium-target. */
struct UIMediumTarget
{
    /** Medium-target types. */
    enum UIMediumTargetType { UIMediumTargetType_WithID, UIMediumTargetType_WithLocation };

    /** Medium-target constructor. */
    UIMediumTarget(const QString &strName = QString(), LONG iPort = 0, LONG iDevice = 0,
                   UIMediumType aMediumType = UIMediumType_Invalid,
                   UIMediumTargetType aType = UIMediumTargetType_WithID, const QString &strData = QString())
        : name(strName), port(iPort), device(iDevice)
        , mediumType(aMediumType)
        , type(aType), data(strData)
    {}

    /** Determines controller name. */
    QString name;
    /** Determines controller port. */
    LONG port;
    /** Determines controller device. */
    LONG device;

    /** Determines medium-target medium-type. */
    UIMediumType mediumType;

    /** Determines medium-target type. */
    UIMediumTargetType type;
    /** Depending on medium-target type holds <i>ID</i> or <i>location</i>. */
    QString data;
};

/* Let QMetaType know about our types: */
Q_DECLARE_METATYPE(UIMediumType);
Q_DECLARE_METATYPE(UIMediumTarget);

#endif /* !___UIMediumDefs_h___ */
