/* $Id: UIWizardCloneVD.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVD class implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
# include "UIWizardCloneVD.h"
# include "UIWizardCloneVDPageBasic1.h"
# include "UIWizardCloneVDPageBasic2.h"
# include "UIWizardCloneVDPageBasic3.h"
# include "UIWizardCloneVDPageBasic4.h"
# include "UIWizardCloneVDPageExpert.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"

/* COM includes: */
# include "CMediumFormat.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardCloneVD::UIWizardCloneVD(QWidget *pParent, const CMedium &sourceVirtualDisk)
    : UIWizard(pParent, WizardType_CloneVD)
    , m_sourceVirtualDisk(sourceVirtualDisk)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    assignWatermark(":/vmw_new_harddisk.png");
#else /* VBOX_WS_MAC */
    /* Assign background image: */
    assignBackground(":/vmw_new_harddisk_bg.png");
#endif /* VBOX_WS_MAC */
}

bool UIWizardCloneVD::copyVirtualDisk()
{
    /* Gather attributes: */
    CMedium sourceVirtualDisk = field("sourceVirtualDisk").value<CMedium>();
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
        temp &= Q_UINT64_C(1) << i;
        variants[i] = (KMediumVariant)temp;
    }

    /* Copy existing virtual-disk to the new virtual-disk: */
    CProgress progress = sourceVirtualDisk.CloneTo(virtualDisk, variants, CMedium());
    if (!sourceVirtualDisk.isOk())
    {
        msgCenter().cannotCreateHardDiskStorage(sourceVirtualDisk, strMediumPath, this);
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

    /* Just close the created medium, it is not necessary yet: */
    m_virtualDisk.Close();

    return true;
}

void UIWizardCloneVD::retranslateUi()
{
    /* Call to base-class: */
    UIWizard::retranslateUi();

    /* Translate wizard: */
    setWindowTitle(tr("Copy Virtual Hard Disk"));
    setButtonText(QWizard::FinishButton, tr("Copy"));
}

void UIWizardCloneVD::prepare()
{
    /* Create corresponding pages: */
    switch (mode())
    {
        case WizardMode_Basic:
        {
            setPage(Page1, new UIWizardCloneVDPageBasic1(m_sourceVirtualDisk));
            setPage(Page2, new UIWizardCloneVDPageBasic2);
            setPage(Page3, new UIWizardCloneVDPageBasic3);
            setPage(Page4, new UIWizardCloneVDPageBasic4);
            break;
        }
        case WizardMode_Expert:
        {
            setPage(PageExpert, new UIWizardCloneVDPageExpert(m_sourceVirtualDisk));
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

