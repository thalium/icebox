/* $Id: UIWizardCloneVD.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVD class declaration.
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

#ifndef __UIWizardCloneVD_h__
#define __UIWizardCloneVD_h__

/* GUI includes: */
#include "UIWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Clone Virtual Hard Drive wizard: */
class UIWizardCloneVD : public UIWizard
{
    Q_OBJECT;

public:

    /* Page IDs: */
    enum
    {
        Page1,
        Page2,
        Page3,
        Page4
    };

    /* Page IDs: */
    enum
    {
        PageExpert
    };

    /* Constructor: */
    UIWizardCloneVD(QWidget *pParent, const CMedium &sourceVirtualDisk);

    /* Returns virtual-disk: */
    CMedium virtualDisk() const { return m_virtualDisk; }

protected:

    /* Copy virtual-disk: */
    bool copyVirtualDisk();

    /* Who will be able to copy virtual-disk: */
    friend class UIWizardCloneVDPageBasic4;
    friend class UIWizardCloneVDPageExpert;

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Pages related stuff: */
    void prepare();

    /* Variables: */
    CMedium m_sourceVirtualDisk;
    CMedium m_virtualDisk;
};

#endif // __UIWizardCloneVD_h__

