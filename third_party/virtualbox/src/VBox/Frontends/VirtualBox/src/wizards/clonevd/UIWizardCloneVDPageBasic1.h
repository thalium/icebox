/* $Id: UIWizardCloneVDPageBasic1.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic1 class declaration.
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

#ifndef ___UIWizardCloneVDPageBasic1_h___
#define ___UIWizardCloneVDPageBasic1_h___

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Forward declarations: */
class VBoxMediaComboBox;
class QIToolButton;
class QIRichTextLabel;


/** 1st page of the Clone Virtual Disk Image wizard (base part): */
class UIWizardCloneVDPage1 : public UIWizardPageBase
{
protected:

    /** Constructs page basis. */
    UIWizardCloneVDPage1();

    /** Handles command to open source disk. */
    void onHandleOpenSourceDiskClick();

    /** Returns 'sourceVirtualDisk' field value. */
    CMedium sourceVirtualDisk() const;
    /** Defines 'sourceVirtualDisk' field value. */
    void setSourceVirtualDisk(const CMedium &comSourceVirtualDisk);

    /** Holds the source media combo-box instance. */
    VBoxMediaComboBox *m_pSourceDiskSelector;
    /** Holds the open-source-disk button instance. */
    QIToolButton      *m_pSourceDiskOpenButton;
};


/** 1st page of the Clone Virtual Disk Image wizard (basic extension): */
class UIWizardCloneVDPageBasic1 : public UIWizardPage, public UIWizardCloneVDPage1
{
    Q_OBJECT;
    Q_PROPERTY(CMedium sourceVirtualDisk READ sourceVirtualDisk WRITE setSourceVirtualDisk);

public:

    /** Constructs basic page.
      * @param  comSourceVirtualDisk  Brings the initial source disk to make copy from. */
    UIWizardCloneVDPageBasic1(const CMedium &comSourceVirtualDisk, KDeviceType enmDeviceType);

protected:

    /** Allows to access 'this' from base part. */
    UIWizardPage *thisImp() { return this; }

private slots:

    /** Handles command to open source disk. */
    void sltHandleOpenSourceDiskClick();

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Prepares the page. */
    virtual void initializePage() /* override */;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const /* override */;

    /** Holds the description label instance. */
    QIRichTextLabel *m_pLabel;
};

#endif /* !___UIWizardCloneVDPageBasic1_h___ */

