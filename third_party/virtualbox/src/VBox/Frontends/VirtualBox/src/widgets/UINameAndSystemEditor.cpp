/* $Id: UINameAndSystemEditor.cpp $ */
/** @file
 * VBox Qt GUI - UINameAndSystemEditor class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
# include <QGridLayout>
# include <QVBoxLayout>
# include <QLabel>
# include <QLineEdit>
# include <QComboBox>

/* GUI includes: */
# include "UINameAndSystemEditor.h"
# include "UIFilePathSelector.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/** Defines the VM OS type ID. */
enum
{
    TypeID = Qt::UserRole + 1
};


UINameAndSystemEditor::UINameAndSystemEditor(QWidget *pParent, bool fChooseLocation /* = false */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fChooseLocation(fChooseLocation)
    , m_fSupportsHWVirtEx(false)
    , m_fSupportsLongMode(false)
    , m_pLabelName(0)
    , m_pLabelFamily(0)
    , m_pLabelType(0)
    , m_pIconType(0)
    , m_pEditorName(0)
    , m_pEditorLocation(0)
    , m_pComboFamily(0)
    , m_pComboType(0)
{
    /* Prepare: */
    prepare();
}

void UINameAndSystemEditor::prepare()
{
    /* Prepare this: */
    prepareThis();
    /* Prepare widgets: */
    prepareWidgets();
    /* Prepare connections: */
    prepareConnections();
    /* Retranslate: */
    retranslateUi();
}

void UINameAndSystemEditor::prepareThis()
{
    /* Check if host supports (AMD-V or VT-x) and long mode: */
    CHost host = vboxGlobal().host();
    m_fSupportsHWVirtEx = host.GetProcessorFeature(KProcessorFeature_HWVirtEx);
    m_fSupportsLongMode = host.GetProcessorFeature(KProcessorFeature_LongMode);
}

void UINameAndSystemEditor::prepareWidgets()
{
    /* Create main-layout: */
    QGridLayout *pMainLayout = new QGridLayout(this);
    AssertPtrReturnVoid(pMainLayout);
    {
        /* Configure main-layout: */
        pMainLayout->setContentsMargins(0, 0, 0, 0);

        /* Create VM name label: */
        m_pLabelName = new QLabel;
        AssertPtrReturnVoid(m_pLabelName);
        {
            /* Configure VM name label: */
            m_pLabelName->setAlignment(Qt::AlignRight);
            m_pLabelName->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            /* Add VM name label into main-layout: */
            pMainLayout->addWidget(m_pLabelName, 0, 0);
        }

        if (!m_fChooseLocation)
        {
            /* Create VM name editor: */
            m_pEditorName = new QLineEdit;
            AssertPtrReturnVoid(m_pEditorName);
            {
                /* Configure VM name editor: */
                m_pEditorName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                m_pLabelName->setBuddy(m_pEditorName);
                /* Add VM name editor into main-layout: */
                pMainLayout->addWidget(m_pEditorName, 0, 1, 1, 2);
            }
        }
        else
        {
            /* Create VM location editor: */
            m_pEditorLocation = new UIFilePathSelector;
            AssertPtrReturnVoid(m_pEditorLocation);
            {
                /* Configure advanced VM name editor: */
                m_pEditorLocation->setResetEnabled(false);
                m_pEditorLocation->setMode(UIFilePathSelector::Mode_File_Save);
                m_pEditorLocation->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                m_pEditorLocation->setHomeDir(vboxGlobal().virtualBox().GetSystemProperties().GetDefaultMachineFolder());
                m_pLabelName->setBuddy(m_pEditorLocation);
                /* Add advanced VM name editor into main-layout: */
                pMainLayout->addWidget(m_pEditorLocation, 0, 1, 1, 2);
            }
        }

        /* Create VM OS family label: */
        m_pLabelFamily = new QLabel;
        AssertPtrReturnVoid(m_pLabelFamily);
        {
            /* Configure VM OS family label: */
            m_pLabelFamily->setAlignment(Qt::AlignRight);
            m_pLabelFamily->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            /* Add VM OS family label into main-layout: */
            pMainLayout->addWidget(m_pLabelFamily, 1, 0);
        }

        /* Create VM OS family combo: */
        m_pComboFamily = new QComboBox;
        AssertPtrReturnVoid(m_pComboFamily);
        {
            /* Configure VM OS family combo: */
            m_pComboFamily->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            m_pLabelFamily->setBuddy(m_pComboFamily);
            /* Add VM OS family combo into main-layout: */
            pMainLayout->addWidget(m_pComboFamily, 1, 1);
        }

        /* Create VM OS type label: */
        m_pLabelType = new QLabel;
        AssertPtrReturnVoid(m_pLabelType);
        {
            /* Configure VM OS type label: */
            m_pLabelType->setAlignment(Qt::AlignRight);
            m_pLabelType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            /* Add VM OS type label into main-layout: */
            pMainLayout->addWidget(m_pLabelType, 2, 0);
        }

        /* Create VM OS type combo: */
        m_pComboType = new QComboBox;
        AssertPtrReturnVoid(m_pComboType);
        {
            /* Configure VM OS type combo: */
            m_pComboType->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            m_pLabelType->setBuddy(m_pComboType);
            /* Add VM OS type combo into main-layout: */
            pMainLayout->addWidget(m_pComboType, 2, 1);
        }

        /* Create sub-layout: */
        QVBoxLayout *pLayoutIcon = new QVBoxLayout;
        AssertPtrReturnVoid(pLayoutIcon);
        {
            /* Create VM OS type icon: */
            m_pIconType = new QLabel;
            AssertPtrReturnVoid(m_pIconType);
            {
                /* Configure VM OS type icon: */
                m_pIconType->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                /* Add VM OS type icon into sub-layout: */
                pLayoutIcon->addWidget(m_pIconType);
            }

            /* Add stretch to sub-layout: */
            pLayoutIcon->addStretch();
            /* Add sub-layout into main-layout: */
            pMainLayout->addLayout(pLayoutIcon, 1, 2, 2, 1);
        }
    }

    /* Initialize VM OS family combo
     * after all widgets were created: */
    prepareFamilyCombo();
}

void UINameAndSystemEditor::prepareFamilyCombo()
{
    /* Populate VM OS family combo: */
    const QList<CGuestOSType> families = vboxGlobal().vmGuestOSFamilyList();
    for (int i = 0; i < families.size(); ++i)
    {
        const QString strFamilyName = families.at(i).GetFamilyDescription();
        m_pComboFamily->insertItem(i, strFamilyName);
        m_pComboFamily->setItemData(i, families.at(i).GetFamilyId(), TypeID);
    }

    /* Choose the 1st item to be the current: */
    m_pComboFamily->setCurrentIndex(0);
    /* And update the linked widgets accordingly: */
    sltFamilyChanged(m_pComboFamily->currentIndex());
}

void UINameAndSystemEditor::prepareConnections()
{
    /* Prepare connections: */
    if (!m_fChooseLocation)
        connect(m_pEditorName, SIGNAL(textChanged(const QString &)), this, SIGNAL(sigNameChanged(const QString &)));
    else
        connect(m_pEditorLocation, SIGNAL(pathChanged(const QString &)), this, SIGNAL(sigNameChanged(const QString &)));
    connect(m_pComboFamily, SIGNAL(currentIndexChanged(int)), this, SLOT(sltFamilyChanged(int)));
    connect(m_pComboType, SIGNAL(currentIndexChanged(int)), this, SLOT(sltTypeChanged(int)));
}

QString UINameAndSystemEditor::name() const
{
    if (!m_fChooseLocation)
        return m_pEditorName->text();
    else
        return m_pEditorLocation->path();
}

void UINameAndSystemEditor::setName(const QString &strName)
{
    if (!m_fChooseLocation)
        m_pEditorName->setText(strName);
    else
        m_pEditorLocation->setPath(strName);
}

CGuestOSType UINameAndSystemEditor::type() const
{
    return m_type;
}

void UINameAndSystemEditor::setType(const CGuestOSType &type)
{
    /** @todo We're getting here with a NULL type when creating new VMs.  Very
     *        annoying, so I've just shut it up for now.  Sergey and Santosh can try
     *        figure out why this happens now with Qt5. */
    if (type.isNotNull())
    {
        /* Initialize variables: */
        const QString strFamilyId = type.GetFamilyId();
        const QString strTypeId = type.GetId();

        /* Get/check family index: */
        const int iFamilyIndex = m_pComboFamily->findData(strFamilyId, TypeID);
        AssertMsg(iFamilyIndex != -1, ("Invalid family ID: '%s'", strFamilyId.toLatin1().constData()));
        if (iFamilyIndex != -1)
            m_pComboFamily->setCurrentIndex(iFamilyIndex);

        /* Get/check type index: */
        const int iTypeIndex = m_pComboType->findData(strTypeId, TypeID);
        AssertMsg(iTypeIndex != -1, ("Invalid type ID: '%s'", strTypeId.toLatin1().constData()));
        if (iTypeIndex != -1)
            m_pComboType->setCurrentIndex(iTypeIndex);
    }
}

void UINameAndSystemEditor::retranslateUi()
{
    m_pLabelName->setText(tr("N&ame:"));
    m_pLabelFamily->setText(tr("&Type:"));
    m_pLabelType->setText(tr("&Version:"));
    if (!m_fChooseLocation)
        m_pEditorName->setWhatsThis(tr("Holds the name of the virtual machine."));
    else
        m_pEditorLocation->setWhatsThis(tr("Holds the location of the virtual machine."));
    m_pComboFamily->setWhatsThis(tr("Selects the operating system family that "
                                    "you plan to install into this virtual machine."));
    m_pComboType->setWhatsThis(tr("Selects the operating system type that "
                                  "you plan to install into this virtual machine "
                                  "(called a guest operating system)."));
}

void UINameAndSystemEditor::sltFamilyChanged(int iIndex)
{
    /* Lock the signals of m_pComboType to prevent it's reaction on clearing: */
    m_pComboType->blockSignals(true);
    m_pComboType->clear();

    /* Populate combo-box with OS types related to currently selected family id: */
    const QString strFamilyId = m_pComboFamily->itemData(iIndex, TypeID).toString();
    const QList<CGuestOSType> types = vboxGlobal().vmGuestOSTypeList(strFamilyId);
    for (int i = 0; i < types.size(); ++i)
    {
        /* Skip 64bit OS types is hardware virtualization or long mode is not supported: */
        if (types.at(i).GetIs64Bit() && (!m_fSupportsHWVirtEx || !m_fSupportsLongMode))
            continue;
        const int iIndex = m_pComboType->count();
        m_pComboType->insertItem(iIndex, types[i].GetDescription());
        m_pComboType->setItemData(iIndex, types[i].GetId(), TypeID);
    }

    /* Select the most recently chosen item: */
    if (m_currentIds.contains(strFamilyId))
    {
        const QString strTypeId = m_currentIds.value(strFamilyId);
        const int iTypeIndex = m_pComboType->findData(strTypeId, TypeID);
        if (iTypeIndex != -1)
            m_pComboType->setCurrentIndex(iTypeIndex);
    }
    /* Or select Windows 7 item for Windows family as default: */
    else if (strFamilyId == "Windows")
    {
        QString strDefaultID = "Windows7";
        if (ARCH_BITS == 64 && m_fSupportsHWVirtEx && m_fSupportsLongMode)
            strDefaultID += "_64";
        const int iIndexWin7 = m_pComboType->findData(strDefaultID, TypeID);
        if (iIndexWin7 != -1)
            m_pComboType->setCurrentIndex(iIndexWin7);
    }
    /* Or select Ubuntu item for Linux family as default: */
    else if (strFamilyId == "Linux")
    {
        QString strDefaultID = "Ubuntu";
        if (ARCH_BITS == 64 && m_fSupportsHWVirtEx && m_fSupportsLongMode)
            strDefaultID += "_64";
        const int iIndexUbuntu = m_pComboType->findData(strDefaultID, TypeID);
        if (iIndexUbuntu != -1)
            m_pComboType->setCurrentIndex(iIndexUbuntu);
    }
    /* Else simply select the first one present: */
    else
        m_pComboType->setCurrentIndex(0);

    /* Update all the stuff: */
    sltTypeChanged(m_pComboType->currentIndex());

    /* Unlock the signals of m_pComboType: */
    m_pComboType->blockSignals(false);
}

void UINameAndSystemEditor::sltTypeChanged(int iIndex)
{
    /* Save the new selected OS type: */
    m_type = vboxGlobal().vmGuestOSType(m_pComboType->itemData(iIndex, TypeID).toString(),
                                        m_pComboFamily->itemData(m_pComboFamily->currentIndex(), TypeID).toString());
    m_pIconType->setPixmap(vboxGlobal().vmGuestOSTypePixmapDefault(m_type.GetId()));

    /* Save the most recently used item: */
    m_currentIds[m_type.GetFamilyId()] = m_type.GetId();

    /* Notifies listeners about OS type change: */
    emit sigOsTypeChanged();
}

