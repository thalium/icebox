/* $Id: UIWizardNewVD.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewVD class implementation.
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

/* Qt includes: */
# include <QVariant>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIWizardNewVD.h"
# include "UIWizardNewVDPageBasic1.h"
# include "UIWizardNewVDPageBasic2.h"
# include "UIWizardNewVDPageBasic3.h"
# include "UIWizardNewVDPageExpert.h"
# include "UIMessageCenter.h"
# include "UIMedium.h"

/* COM includes: */
# include "CMediumFormat.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardNewVD::UIWizardNewVD(QWidget *pParent,
                             const QString &strDefaultName, const QString &strDefaultPath,
                             qulonglong uDefaultSize,
                             WizardMode mode)
    : UIWizard(pParent, WizardType_NewVD, mode)
    , m_strDefaultName(strDefaultName)
    , m_strDefaultPath(strDefaultPath)
    , m_uDefaultSize(uDefaultSize)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    assignWatermark(":/vmw_new_harddisk.png");
#else /* VBOX_WS_MAC */
    /* Assign background image: */
    assignBackground(":/vmw_new_harddisk_bg.png");
#endif /* VBOX_WS_MAC */
}

bool UIWizardNewVD::createVirtualDisk()
{
    /* Gather attributes: */
    CMediumFormat mediumFormat = field("mediumFormat").value<CMediumFormat>();
    qulonglong uVariant = field("mediumVariant").toULongLong();
    QString strMediumPath = field("mediumPath").toString();
    qulonglong uSize = field("mediumSize").toULongLong();
    /* Check attributes: */
    AssertReturn(!strMediumPath.isNull(), false);
    AssertReturn(uSize > 0, false);

    /* Get VBox object: */
    CVirtualBox vbox = vboxGlobal().virtualBox();

    /* Create new virtual hard-disk: */
    CMedium virtualDisk = vbox.CreateMedium(mediumFormat.GetName(), strMediumPath, KAccessMode_ReadWrite, KDeviceType_HardDisk);
    if (!vbox.isOk())
    {
        msgCenter().cannotCreateHardDiskStorage(vbox, strMediumPath, this);
        return false;
    }

    /* Compose medium-variant: */
    QVector<KMediumVariant> variants(sizeof(qulonglong)*8);
    for (int i = 0; i < variants.size(); ++i)
    {
        qulonglong temp = uVariant;
        temp &= UINT64_C(1)<<i;
        variants[i] = (KMediumVariant)temp;
    }

    /* Create base storage for the new virtual-disk: */
    CProgress progress = virtualDisk.CreateBaseStorage(uSize, variants);
    if (!virtualDisk.isOk())
    {
        msgCenter().cannotCreateHardDiskStorage(virtualDisk, strMediumPath, this);
        return false;
    }

    /* Show creation progress: */
    msgCenter().showModalProgressDialog(progress, windowTitle(), ":/progress_media_create_90px.png", this);
    if (progress.GetCanceled())
        return false;
    if (!progress.isOk() || progress.GetResultCode() != 0)
    {
        msgCenter().cannotCreateHardDiskStorage(progress, strMediumPath, this);
        return false;
    }

    /* Remember created virtual-disk: */
    m_virtualDisk = virtualDisk;

    /* Inform VBoxGlobal about it: */
    vboxGlobal().createMedium(UIMedium(m_virtualDisk, UIMediumType_HardDisk, KMediumState_Created));

    return true;
}

void UIWizardNewVD::retranslateUi()
{
    /* Call to base-class: */
    UIWizard::retranslateUi();

    /* Translate wizard: */
    setWindowTitle(tr("Create Virtual Hard Disk"));
    setButtonText(QWizard::FinishButton, tr("Create"));
}

void UIWizardNewVD::prepare()
{
    /* Create corresponding pages: */
    switch (mode())
    {
        case WizardMode_Basic:
        {
            setPage(Page1, new UIWizardNewVDPageBasic1);
            setPage(Page2, new UIWizardNewVDPageBasic2);
            setPage(Page3, new UIWizardNewVDPageBasic3(m_strDefaultName, m_strDefaultPath, m_uDefaultSize));
            break;
        }
        case WizardMode_Expert:
        {
            setPage(PageExpert, new UIWizardNewVDPageExpert(m_strDefaultName, m_strDefaultPath, m_uDefaultSize));
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
    /* Call to base-class: */
    UIWizard::prepare();
}

