/* $Id: UIIconPool.cpp $ */
/** @file
 * VBox Qt GUI - UIIconPool class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QApplication>
# include <QFile>
# include <QStyle>
# include <QWidget>

/* GUI includes: */
# include "UIIconPool.h"
# include "UIExtraDataManager.h"

/* COM includes: */
# include "COMEnums.h"
# include "CMachine.h"

/* Other VBox includes: */
# include <iprt/assert.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
QPixmap UIIconPool::pixmap(const QString &strName)
{
    /* Reuse iconSet API: */
    QIcon icon = iconSet(strName);

    /* Return pixmap of first available size: */
    const int iHint = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    return icon.pixmap(icon.availableSizes().value(0, QSize(iHint, iHint)));
}

/* static */
QIcon UIIconPool::iconSet(const QString &strNormal,
                          const QString &strDisabled /* = QString() */,
                          const QString &strActive /* = QString() */)
{
    /* Prepare fallback icon: */
    static QIcon nullIcon;

    /* Prepare icon: */
    QIcon icon;

    /* Add 'normal' pixmap: */
    AssertReturn(!strNormal.isEmpty(), nullIcon);
    addName(icon, strNormal, QIcon::Normal);

    /* Add 'disabled' pixmap (if any): */
    if (!strDisabled.isEmpty())
        addName(icon, strDisabled, QIcon::Disabled);

    /* Add 'active' pixmap (if any): */
    if (!strActive.isEmpty())
        addName(icon, strActive, QIcon::Active);

    /* Return icon: */
    return icon;
}

/* static */
QIcon UIIconPool::iconSetOnOff(const QString &strNormal, const QString strNormalOff,
                               const QString &strDisabled /* = QString() */, const QString &strDisabledOff /* = QString() */,
                               const QString &strActive /* = QString() */, const QString &strActiveOff /* = QString() */)
{
    /* Prepare fallback icon: */
    static QIcon nullIcon;

    /* Prepare icon: */
    QIcon icon;

    /* Add 'normal' on/off pixmaps: */
    AssertReturn(!strNormal.isEmpty(), nullIcon);
    addName(icon, strNormal, QIcon::Normal, QIcon::On);
    AssertReturn(!strNormalOff.isEmpty(), nullIcon);
    addName(icon, strNormalOff, QIcon::Normal, QIcon::Off);

    /* Add 'disabled' on/off pixmaps (if any): */
    if (!strDisabled.isEmpty())
        addName(icon, strDisabled, QIcon::Disabled, QIcon::On);
    if (!strDisabledOff.isEmpty())
        addName(icon, strDisabledOff, QIcon::Disabled, QIcon::Off);

    /* Add 'active' on/off pixmaps (if any): */
    if (!strActive.isEmpty())
        addName(icon, strActive, QIcon::Active, QIcon::On);
    if (!strActiveOff.isEmpty())
        addName(icon, strActiveOff, QIcon::Active, QIcon::Off);

    /* Return icon: */
    return icon;
}

/* static */
QIcon UIIconPool::iconSetFull(const QString &strNormal, const QString &strSmall,
                              const QString &strNormalDisabled /* = QString() */, const QString &strSmallDisabled /* = QString() */,
                              const QString &strNormalActive /* = QString() */, const QString &strSmallActive /* = QString() */)
{
    /* Prepare fallback icon: */
    static QIcon nullIcon;

    /* Prepare icon: */
    QIcon icon;

    /* Add 'normal' & 'small normal' pixmaps: */
    AssertReturn(!strNormal.isEmpty(), nullIcon);
    addName(icon, strNormal, QIcon::Normal);
    AssertReturn(!strSmall.isEmpty(), nullIcon);
    addName(icon, strSmall, QIcon::Normal);

    /* Add 'disabled' & 'small disabled' pixmaps (if any): */
    if (!strNormalDisabled.isEmpty())
        addName(icon, strNormalDisabled, QIcon::Disabled);
    if (!strSmallDisabled.isEmpty())
        addName(icon, strSmallDisabled, QIcon::Disabled);

    /* Add 'active' & 'small active' pixmaps (if any): */
    if (!strNormalActive.isEmpty())
        addName(icon, strNormalActive, QIcon::Active);
    if (!strSmallActive.isEmpty())
        addName(icon, strSmallActive, QIcon::Active);

    /* Return icon: */
    return icon;
}

/* static */
QIcon UIIconPool::iconSet(const QPixmap &normal,
                          const QPixmap &disabled /* = QPixmap() */,
                          const QPixmap &active /* = QPixmap() */)
{
    QIcon iconSet;

    Assert(!normal.isNull());
    iconSet.addPixmap(normal, QIcon::Normal);

    if (!disabled.isNull())
        iconSet.addPixmap(disabled, QIcon::Disabled);

    if (!active.isNull())
        iconSet.addPixmap(active, QIcon::Active);

    return iconSet;
}

/* static */
QIcon UIIconPool::defaultIcon(UIDefaultIconType defaultIconType, const QWidget *pWidget /* = 0 */)
{
    QIcon icon;
    QStyle *pStyle = pWidget ? pWidget->style() : QApplication::style();
    switch (defaultIconType)
    {
        case UIDefaultIconType_MessageBoxInformation:
        {
            icon = pStyle->standardIcon(QStyle::SP_MessageBoxInformation, 0, pWidget);
            break;
        }
        case UIDefaultIconType_MessageBoxQuestion:
        {
            icon = pStyle->standardIcon(QStyle::SP_MessageBoxQuestion, 0, pWidget);
            break;
        }
        case UIDefaultIconType_MessageBoxWarning:
        {
#ifdef VBOX_WS_MAC
            /* At least in Qt 4.3.4/4.4 RC1 SP_MessageBoxWarning is the application
             * icon. So change this to the critical icon. (Maybe this would be
             * fixed in a later Qt version) */
            icon = pStyle->standardIcon(QStyle::SP_MessageBoxCritical, 0, pWidget);
#else /* VBOX_WS_MAC */
            icon = pStyle->standardIcon(QStyle::SP_MessageBoxWarning, 0, pWidget);
#endif /* !VBOX_WS_MAC */
            break;
        }
        case UIDefaultIconType_MessageBoxCritical:
        {
            icon = pStyle->standardIcon(QStyle::SP_MessageBoxCritical, 0, pWidget);
            break;
        }
        case UIDefaultIconType_DialogCancel:
        {
            icon = pStyle->standardIcon(QStyle::SP_DialogCancelButton, 0, pWidget);
            if (icon.isNull())
                icon = iconSet(":/cancel_16px.png");
            break;
        }
        case UIDefaultIconType_DialogHelp:
        {
            icon = pStyle->standardIcon(QStyle::SP_DialogHelpButton, 0, pWidget);
            if (icon.isNull())
                icon = iconSet(":/help_16px.png");
            break;
        }
        case UIDefaultIconType_ArrowBack:
        {
            icon = pStyle->standardIcon(QStyle::SP_ArrowBack, 0, pWidget);
            if (icon.isNull())
                icon = iconSet(":/list_moveup_16px.png",
                               ":/list_moveup_disabled_16px.png");
            break;
        }
        case UIDefaultIconType_ArrowForward:
        {
            icon = pStyle->standardIcon(QStyle::SP_ArrowForward, 0, pWidget);
            if (icon.isNull())
                icon = iconSet(":/list_movedown_16px.png",
                               ":/list_movedown_disabled_16px.png");
            break;
        }
        default:
        {
            AssertMsgFailed(("Unknown default icon type!"));
            break;
        }
    }
    return icon;
}

/* static */
void UIIconPool::addName(QIcon &icon, const QString &strName,
                         QIcon::Mode mode /* = QIcon::Normal */, QIcon::State state /* = QIcon::Off */)
{
    /* Prepare pixmap on the basis of passed value: */
    QPixmap pixmap(strName);
    /* Add pixmap: */
    icon.addPixmap(pixmap, mode, state);

#ifdef VBOX_WS_MAC
    /* Test if HiDPI icons are enabled: */
    if (!qApp->testAttribute(Qt::AA_UseHighDpiPixmaps))
        return;
#endif /* VBOX_WS_MAC */

    /* Parse name to prefix and suffix: */
    QString strPrefix = strName.section('.', 0, -2);
    QString strSuffix = strName.section('.', -1, -1);
    /* Prepare HiDPI pixmap on the basis of values above: */
    QPixmap pixmapHiDPI(strPrefix + "_hidpi." + strSuffix);
    /* Add HiDPI pixmap (if any): */
    if (!pixmapHiDPI.isNull())
        icon.addPixmap(pixmapHiDPI, mode, state);
}


UIIconPoolGeneral::UIIconPoolGeneral()
{
    /* Prepare OS type icon-name hash: */
    m_guestOSTypeIconNames.insert("Other",           ":/os_other.png");
    m_guestOSTypeIconNames.insert("Other_64",        ":/os_other_64.png");
    m_guestOSTypeIconNames.insert("DOS",             ":/os_dos.png");
    m_guestOSTypeIconNames.insert("Netware",         ":/os_netware.png");
    m_guestOSTypeIconNames.insert("L4",              ":/os_l4.png");
    m_guestOSTypeIconNames.insert("Windows31",       ":/os_win31.png");
    m_guestOSTypeIconNames.insert("Windows95",       ":/os_win95.png");
    m_guestOSTypeIconNames.insert("Windows98",       ":/os_win98.png");
    m_guestOSTypeIconNames.insert("WindowsMe",       ":/os_winme.png");
    m_guestOSTypeIconNames.insert("WindowsNT4",      ":/os_winnt4.png");
    m_guestOSTypeIconNames.insert("Windows2000",     ":/os_win2k.png");
    m_guestOSTypeIconNames.insert("WindowsXP",       ":/os_winxp.png");
    m_guestOSTypeIconNames.insert("WindowsXP_64",    ":/os_winxp_64.png");
    m_guestOSTypeIconNames.insert("Windows2003",     ":/os_win2k3.png");
    m_guestOSTypeIconNames.insert("Windows2003_64",  ":/os_win2k3_64.png");
    m_guestOSTypeIconNames.insert("WindowsVista",    ":/os_winvista.png");
    m_guestOSTypeIconNames.insert("WindowsVista_64", ":/os_winvista_64.png");
    m_guestOSTypeIconNames.insert("Windows2008",     ":/os_win2k8.png");
    m_guestOSTypeIconNames.insert("Windows2008_64",  ":/os_win2k8_64.png");
    m_guestOSTypeIconNames.insert("Windows7",        ":/os_win7.png");
    m_guestOSTypeIconNames.insert("Windows7_64",     ":/os_win7_64.png");
    m_guestOSTypeIconNames.insert("Windows8",        ":/os_win8.png");
    m_guestOSTypeIconNames.insert("Windows8_64",     ":/os_win8_64.png");
    m_guestOSTypeIconNames.insert("Windows81",       ":/os_win81.png");
    m_guestOSTypeIconNames.insert("Windows81_64",    ":/os_win81_64.png");
    m_guestOSTypeIconNames.insert("Windows2012_64",  ":/os_win2k12_64.png");
    m_guestOSTypeIconNames.insert("Windows10",       ":/os_win10.png");
    m_guestOSTypeIconNames.insert("Windows10_64",    ":/os_win10_64.png");
    m_guestOSTypeIconNames.insert("Windows2016_64",  ":/os_win2k16_64.png");
    m_guestOSTypeIconNames.insert("WindowsNT",       ":/os_win_other.png");
    m_guestOSTypeIconNames.insert("WindowsNT_64",    ":/os_win_other_64.png");
    m_guestOSTypeIconNames.insert("OS2Warp3",        ":/os_os2warp3.png");
    m_guestOSTypeIconNames.insert("OS2Warp4",        ":/os_os2warp4.png");
    m_guestOSTypeIconNames.insert("OS2Warp45",       ":/os_os2warp45.png");
    m_guestOSTypeIconNames.insert("OS2eCS",          ":/os_os2ecs.png");
    m_guestOSTypeIconNames.insert("OS21x",           ":/os_os2_other.png");
    m_guestOSTypeIconNames.insert("OS2",             ":/os_os2_other.png");
    m_guestOSTypeIconNames.insert("Linux22",         ":/os_linux22.png");
    m_guestOSTypeIconNames.insert("Linux24",         ":/os_linux24.png");
    m_guestOSTypeIconNames.insert("Linux24_64",      ":/os_linux24_64.png");
    m_guestOSTypeIconNames.insert("Linux26",         ":/os_linux26.png");
    m_guestOSTypeIconNames.insert("Linux26_64",      ":/os_linux26_64.png");
    m_guestOSTypeIconNames.insert("ArchLinux",       ":/os_archlinux.png");
    m_guestOSTypeIconNames.insert("ArchLinux_64",    ":/os_archlinux_64.png");
    m_guestOSTypeIconNames.insert("Debian",          ":/os_debian.png");
    m_guestOSTypeIconNames.insert("Debian_64",       ":/os_debian_64.png");
    m_guestOSTypeIconNames.insert("OpenSUSE",        ":/os_opensuse.png");
    m_guestOSTypeIconNames.insert("OpenSUSE_64",     ":/os_opensuse_64.png");
    m_guestOSTypeIconNames.insert("Fedora",          ":/os_fedora.png");
    m_guestOSTypeIconNames.insert("Fedora_64",       ":/os_fedora_64.png");
    m_guestOSTypeIconNames.insert("Gentoo",          ":/os_gentoo.png");
    m_guestOSTypeIconNames.insert("Gentoo_64",       ":/os_gentoo_64.png");
    m_guestOSTypeIconNames.insert("Mandriva",        ":/os_mandriva.png");
    m_guestOSTypeIconNames.insert("Mandriva_64",     ":/os_mandriva_64.png");
    m_guestOSTypeIconNames.insert("RedHat",          ":/os_redhat.png");
    m_guestOSTypeIconNames.insert("RedHat_64",       ":/os_redhat_64.png");
    m_guestOSTypeIconNames.insert("Turbolinux",      ":/os_turbolinux.png");
    m_guestOSTypeIconNames.insert("Turbolinux_64",   ":/os_turbolinux_64.png");
    m_guestOSTypeIconNames.insert("Ubuntu",          ":/os_ubuntu.png");
    m_guestOSTypeIconNames.insert("Ubuntu_64",       ":/os_ubuntu_64.png");
    m_guestOSTypeIconNames.insert("Xandros",         ":/os_xandros.png");
    m_guestOSTypeIconNames.insert("Xandros_64",      ":/os_xandros_64.png");
    m_guestOSTypeIconNames.insert("Oracle",          ":/os_oracle.png");
    m_guestOSTypeIconNames.insert("Oracle_64",       ":/os_oracle_64.png");
    m_guestOSTypeIconNames.insert("Linux",           ":/os_linux.png");
    m_guestOSTypeIconNames.insert("Linux_64",        ":/os_linux_64.png");
    m_guestOSTypeIconNames.insert("FreeBSD",         ":/os_freebsd.png");
    m_guestOSTypeIconNames.insert("FreeBSD_64",      ":/os_freebsd_64.png");
    m_guestOSTypeIconNames.insert("OpenBSD",         ":/os_openbsd.png");
    m_guestOSTypeIconNames.insert("OpenBSD_64",      ":/os_openbsd_64.png");
    m_guestOSTypeIconNames.insert("NetBSD",          ":/os_netbsd.png");
    m_guestOSTypeIconNames.insert("NetBSD_64",       ":/os_netbsd_64.png");
    m_guestOSTypeIconNames.insert("Solaris",         ":/os_solaris.png");
    m_guestOSTypeIconNames.insert("Solaris_64",      ":/os_solaris_64.png");
    m_guestOSTypeIconNames.insert("OpenSolaris",     ":/os_oraclesolaris.png");
    m_guestOSTypeIconNames.insert("OpenSolaris_64",  ":/os_oraclesolaris_64.png");
    m_guestOSTypeIconNames.insert("Solaris11_64",    ":/os_oraclesolaris_64.png");
    m_guestOSTypeIconNames.insert("QNX",             ":/os_qnx.png");
    m_guestOSTypeIconNames.insert("MacOS",           ":/os_macosx.png");
    m_guestOSTypeIconNames.insert("MacOS_64",        ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS106",        ":/os_macosx.png");
    m_guestOSTypeIconNames.insert("MacOS106_64",     ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS107_64",     ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS108_64",     ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS109_64",     ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS1010_64",    ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS1011_64",    ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS1012_64",    ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("MacOS1013_64",    ":/os_macosx_64.png");
    m_guestOSTypeIconNames.insert("JRockitVE",       ":/os_jrockitve.png");
    m_guestOSTypeIconNames.insert("VBoxBS_64",       ":/os_other_64.png");
}

QIcon UIIconPoolGeneral::userMachineIcon(const CMachine &comMachine) const
{
    /* Get machine ID: */
    const QString strMachineId = comMachine.GetId();
    AssertReturn(comMachine.isOk(), QPixmap());

    /* Prepare icon: */
    QIcon icon;

    /* 1. First, load icon from IMachine extra-data: */
    if (icon.isNull())
        foreach (const QString &strIconName, gEDataManager->machineWindowIconNames(strMachineId))
            if (!strIconName.isEmpty() && QFile::exists(strIconName))
                icon.addFile(strIconName);

    /* 2. Otherwise, load icon from IMachine interface itself: */
    if (icon.isNull())
    {
        const QVector<BYTE> byteVector = comMachine.GetIcon();
        AssertReturn(comMachine.isOk(), QPixmap());
        const QByteArray byteArray = QByteArray::fromRawData(reinterpret_cast<const char*>(byteVector.constData()), byteVector.size());
        const QImage image = QImage::fromData(byteArray);
        if (!image.isNull())
        {
            QPixmap pixmap = QPixmap::fromImage(image);
            const int iMinimumLength = qMin(pixmap.width(), pixmap.height());
            if (pixmap.width() != iMinimumLength || pixmap.height() != iMinimumLength)
                pixmap = pixmap.scaled(QSize(iMinimumLength, iMinimumLength), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            icon.addPixmap(pixmap);
        }
    }

    /* Return icon: */
    return icon;
}

QPixmap UIIconPoolGeneral::userMachinePixmap(const CMachine &comMachine, const QSize &size) const
{
    /* Acquire icon: */
    const QIcon icon = userMachineIcon(comMachine);

    /* Prepare pixmap: */
    QPixmap pixmap;

    /* Check whether we have valid icon: */
    if (!icon.isNull())
    {
        /* Get pixmap of requested size: */
        pixmap = icon.pixmap(size);
        /* And even scale it if size is not valid: */
        if (pixmap.size() != size)
            pixmap = pixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    /* Return pixmap: */
    return pixmap;
}

QPixmap UIIconPoolGeneral::userMachinePixmapDefault(const CMachine &comMachine, QSize *pLogicalSize /* = 0 */) const
{
    /* Acquire icon: */
    const QIcon icon = userMachineIcon(comMachine);

    /* Prepare pixmap: */
    QPixmap pixmap;

    /* Check whether we have valid icon: */
    if (!icon.isNull())
    {
        /* Determine desired icon size: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
        const QSize iconSize = QSize(iIconMetric, iIconMetric);

        /* Pass up logical size if necessary: */
        if (pLogicalSize)
            *pLogicalSize = iconSize;

        /* Get pixmap of requested size: */
        pixmap = icon.pixmap(iconSize);
    }

    /* Return pixmap: */
    return pixmap;
}

QIcon UIIconPoolGeneral::guestOSTypeIcon(const QString &strOSTypeID) const
{
    /* Prepare fallback icon: */
    static QPixmap nullIcon;

    /* If we do NOT have that 'guest OS type' icon cached already: */
    if (!m_guestOSTypeIcons.contains(strOSTypeID))
    {
        /* Compose proper icon if we have that 'guest OS type' known: */
        if (m_guestOSTypeIconNames.contains(strOSTypeID))
            m_guestOSTypeIcons[strOSTypeID] = iconSet(m_guestOSTypeIconNames[strOSTypeID]);
        /* Assign fallback icon if we do NOT have that 'guest OS type' known: */
        else
            m_guestOSTypeIcons[strOSTypeID] = iconSet(nullIcon);
    }

    /* Retrieve corresponding icon: */
    const QIcon &icon = m_guestOSTypeIcons[strOSTypeID];
    AssertMsgReturn(!icon.isNull(),
                    ("Undefined icon for type '%s'.", strOSTypeID.toLatin1().constData()),
                    nullIcon);

    /* Return icon: */
    return icon;
}

QPixmap UIIconPoolGeneral::guestOSTypePixmap(const QString &strOSTypeID, const QSize &size) const
{
    /* Acquire icon: */
    const QIcon icon = guestOSTypeIcon(strOSTypeID);

    /* Prepare pixmap: */
    QPixmap pixmap;

    /* Check whether we have valid icon: */
    if (!icon.isNull())
    {
        /* Get pixmap of requested size: */
        pixmap = icon.pixmap(size);
        /* And even scale it if size is not valid: */
        if (pixmap.size() != size)
            pixmap = pixmap.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    /* Return pixmap: */
    return pixmap;
}

QPixmap UIIconPoolGeneral::guestOSTypePixmapDefault(const QString &strOSTypeID, QSize *pLogicalSize /* = 0 */) const
{
    /* Acquire icon: */
    const QIcon icon = guestOSTypeIcon(strOSTypeID);

    /* Prepare pixmap: */
    QPixmap pixmap;

    /* Check whether we have valid icon: */
    if (!icon.isNull())
    {
        /* Determine desired icon size: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
        const QSize iconSize = QSize(iIconMetric, iIconMetric);

        /* Pass up logical size if necessary: */
        if (pLogicalSize)
            *pLogicalSize = iconSize;

        /* Get pixmap of requested size: */
        pixmap = icon.pixmap(iconSize);
    }

    /* Return pixmap: */
    return pixmap;
}

