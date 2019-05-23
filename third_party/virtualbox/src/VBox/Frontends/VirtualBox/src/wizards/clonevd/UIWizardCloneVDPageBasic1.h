/* $Id: UIWizardCloneVDPageBasic1.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic1 class declaration.
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

#ifndef __UIWizardCloneVDPageBasic1_h__
#define __UIWizardCloneVDPageBasic1_h__

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Forward declarations: */
class VBoxMediaComboBox;
class QIToolButton;
class QIRichTextLabel;

/* 1st page of the Clone Virtual Hard Drive wizard (base part): */
class UIWizardCloneVDPage1 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVDPage1();

    /* Handlers: */
    void onHandleOpenSourceDiskClick();

    /* Stuff for 'sourceVirtualDisk' field: */
    CMedium sourceVirtualDisk() const;
    void setSourceVirtualDisk(const CMedium &sourceVirtualDisk);

    /* Widgets: */
    VBoxMediaComboBox *m_pSourceDiskSelector;
    QIToolButton *m_pSourceDiskOpenButton;
};

/* 1st page of the Clone Virtual Hard Drive wizard (basic extension): */
class UIWizardCloneVDPageBasic1 : public UIWizardPage, public UIWizardCloneVDPage1
{
    Q_OBJECT;
    Q_PROPERTY(CMedium sourceVirtualDisk READ sourceVirtualDisk WRITE setSourceVirtualDisk);

public:

    /* Constructor: */
    UIWizardCloneVDPageBasic1(const CMedium &sourceVirtualDisk);

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }

private slots:

    /* Handlers for source virtual-disk change: */
    void sltHandleOpenSourceDiskClick();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVDPageBasic1_h__

