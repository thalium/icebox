/* $Id: UIUpdateDefs.cpp $ */
/** @file
 * VBox Qt GUI - Update routine related implementations.
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

/* Global includes: */
# include <QCoreApplication>
# include <QStringList>

/* Local includes: */
# include "UIUpdateDefs.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static: */
VBoxUpdateDayList VBoxUpdateData::m_dayList = VBoxUpdateDayList();

/* static */
void VBoxUpdateData::populate()
{
    /* Clear list initially: */
    m_dayList.clear();

    /* To avoid re-translation complexity
     * all values will be retranslated separately: */

    /* Separately retranslate each day: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 day"),  "1 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 days"), "2 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 days"), "3 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "4 days"), "4 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "5 days"), "5 d");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "6 days"), "6 d");

    /* Separately retranslate each week: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 week"),  "1 w");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 weeks"), "2 w");
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 weeks"), "3 w");

    /* Separately retranslate each month: */
    m_dayList << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 month"), "1 m");
}

/* static */
QStringList VBoxUpdateData::list()
{
    QStringList result;
    for (int i = 0; i < m_dayList.size(); ++i)
        result << m_dayList[i].val;
    return result;
}

VBoxUpdateData::VBoxUpdateData(const QString &strData)
    : m_strData(strData)
    , m_periodIndex(Period1Day)
    , m_branchIndex(BranchStable)
{
    decode();
}

VBoxUpdateData::VBoxUpdateData(PeriodType periodIndex, BranchType branchIndex)
    : m_strData(QString())
    , m_periodIndex(periodIndex)
    , m_branchIndex(branchIndex)
{
    encode();
}

bool VBoxUpdateData::isNoNeedToCheck() const
{
    /* Return 'false' if Period == Never: */
    return m_periodIndex == PeriodNever;
}

bool VBoxUpdateData::isNeedToCheck() const
{
    /* Return 'false' if Period == Never: */
    if (isNoNeedToCheck())
        return false;

    /* Return 'true' if date of next check is today or missed: */
    if (QDate::currentDate() >= m_date)
        return true;

    /* Return 'true' if saved version value is NOT valid or NOT equal to current: */
    if (!version().isValid() || version() != UIVersion(vboxGlobal().vboxVersionStringNormalized()))
        return true;

    /* Return 'false' in all other cases: */
    return false;
}

QString VBoxUpdateData::data() const
{
    return m_strData;
}

VBoxUpdateData::PeriodType VBoxUpdateData::periodIndex() const
{
    return m_periodIndex;
}

QString VBoxUpdateData::date() const
{
    return isNoNeedToCheck() ? QCoreApplication::translate("UIUpdateManager", "Never") : m_date.toString(Qt::LocaleDate);
}

VBoxUpdateData::BranchType VBoxUpdateData::branchIndex() const
{
    return m_branchIndex;
}

QString VBoxUpdateData::branchName() const
{
    switch (m_branchIndex)
    {
        case BranchStable:
            return "stable";
        case BranchAllRelease:
            return "allrelease";
        case BranchWithBetas:
            return "withbetas";
    }
    return QString();
}

UIVersion VBoxUpdateData::version() const
{
    return m_version;
}

void VBoxUpdateData::decode()
{
    /* Parse standard values: */
    if (m_strData == "never")
        m_periodIndex = PeriodNever;
    /* Parse other values: */
    else
    {
        QStringList parser(m_strData.split(", ", QString::SkipEmptyParts));

        /* Parse 'period' value: */
        if (parser.size() > 0)
        {
            if (m_dayList.isEmpty())
                populate();
            PeriodType index = (PeriodType)m_dayList.indexOf(VBoxUpdateDay(QString(), parser[0]));
            m_periodIndex = index == PeriodUndefined ? Period1Day : index;
        }

        /* Parse 'date' value: */
        if (parser.size() > 1)
        {
            QDate date = QDate::fromString(parser[1], Qt::ISODate);
            m_date = date.isValid() ? date : QDate::currentDate();
        }

        /* Parse 'branch' value: */
        if (parser.size() > 2)
        {
            QString branch(parser[2]);
            m_branchIndex = branch == "withbetas" ? BranchWithBetas :
                            branch == "allrelease" ? BranchAllRelease : BranchStable;
        }

        /* Parse 'version' value: */
        if (parser.size() > 3)
        {
            m_version = UIVersion(parser[3]);
        }
    }
}

void VBoxUpdateData::encode()
{
    /* Encode standard values: */
    if (m_periodIndex == PeriodNever)
        m_strData = "never";
    /* Encode other values: */
    else
    {
        /* Encode 'period' value: */
        if (m_dayList.isEmpty())
            populate();
        QString remindPeriod = m_dayList[m_periodIndex].key;

        /* Encode 'date' value: */
        m_date = QDate::currentDate();
        QStringList parser(remindPeriod.split(' '));
        if (parser[1] == "d")
            m_date = m_date.addDays(parser[0].toInt());
        else if (parser[1] == "w")
            m_date = m_date.addDays(parser[0].toInt() * 7);
        else if (parser[1] == "m")
            m_date = m_date.addMonths(parser[0].toInt());
        QString remindDate = m_date.toString(Qt::ISODate);

        /* Encode 'branch' value: */
        QString branchValue = m_branchIndex == BranchWithBetas ? "withbetas" :
                              m_branchIndex == BranchAllRelease ? "allrelease" : "stable";

        /* Encode 'version' value: */
        QString versionValue = UIVersion(vboxGlobal().vboxVersionStringNormalized()).toString();

        /* Composite m_strData: */
        m_strData = QString("%1, %2, %3, %4").arg(remindPeriod, remindDate, branchValue, versionValue);
    }
}

