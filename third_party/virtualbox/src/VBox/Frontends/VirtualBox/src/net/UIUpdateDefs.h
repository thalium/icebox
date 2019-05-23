/* $Id: UIUpdateDefs.h $ */
/** @file
 * VBox Qt GUI - Update routine related declarations.
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

#ifndef ___UIUpdateDefs_h___
#define ___UIUpdateDefs_h___

/* Qt includes: */
#include <QDate>

/* GUI includes: */
#include "UIVersion.h"


/* This structure is used to store retranslated reminder values. */
struct VBoxUpdateDay
{
    VBoxUpdateDay(const QString &strVal, const QString &strKey)
        : val(strVal), key(strKey) {}

    bool operator==(const VBoxUpdateDay &other) { return val == other.val || key == other.key; }

    QString val;
    QString key;
};
typedef QList<VBoxUpdateDay> VBoxUpdateDayList;


/* This class is used to encode/decode update data. */
class VBoxUpdateData
{
public:

    /* Period types: */
    enum PeriodType
    {
        PeriodNever     = -2,
        PeriodUndefined = -1,
        Period1Day      =  0,
        Period2Days     =  1,
        Period3Days     =  2,
        Period4Days     =  3,
        Period5Days     =  4,
        Period6Days     =  5,
        Period1Week     =  6,
        Period2Weeks    =  7,
        Period3Weeks    =  8,
        Period1Month    =  9
    };

    /* Branch types: */
    enum BranchType
    {
        BranchStable     = 0,
        BranchAllRelease = 1,
        BranchWithBetas  = 2
    };

    /* Public static helpers: */
    static void populate();
    static QStringList list();

    /* Constructors: */
    VBoxUpdateData(const QString &strData);
    VBoxUpdateData(PeriodType periodIndex, BranchType branchIndex);

    /* Public helpers: */
    bool isNoNeedToCheck() const;
    bool isNeedToCheck() const;
    QString data() const;
    PeriodType periodIndex() const;
    QString date() const;
    BranchType branchIndex() const;
    QString branchName() const;
    UIVersion version() const;

private:

    /* Private helpers: */
    void decode();
    void encode();

    /* Private variables: */
    static VBoxUpdateDayList m_dayList;
    QString m_strData;
    PeriodType m_periodIndex;
    QDate m_date;
    BranchType m_branchIndex;
    UIVersion m_version;
};

#endif /* !___UIUpdateDefs_h___ */

