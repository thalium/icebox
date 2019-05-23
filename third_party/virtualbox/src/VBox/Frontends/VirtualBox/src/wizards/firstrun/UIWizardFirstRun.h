/* $Id: UIWizardFirstRun.h $ */
/** @file
 * VBox Qt GUI - UIWizardFirstRun class declaration.
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

#ifndef __UIWizardFirstRun_h__
#define __UIWizardFirstRun_h__

/* GUI includes: */
#include "UIWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"

/* First Run wizard: */
class UIWizardFirstRun : public UIWizard
{
    Q_OBJECT;

public:

    /* Page IDs: */
    enum
    {
        Page
    };

    /* Constructor: */
    UIWizardFirstRun(QWidget *pParent, const CMachine &machine);

protected:

    /* Insert medium stuff: */
    bool insertMedium();

    /* Who will be able to start VM: */
    friend class UIWizardFirstRunPageBasic;

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Pages related stuff: */
    void prepare();

    /* Helping stuff: */
    static bool isBootHardDiskAttached(const CMachine &machine);

    /* Variables: */
    CMachine m_machine;
    bool m_fHardDiskWasSet;
};

#endif // __UIFirstRunWzd_h__

