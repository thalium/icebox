/* $Id: UIWizardNewVD.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVD class declaration.
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

#ifndef __UIWizardNewVD_h__
#define __UIWizardNewVD_h__

/* GUI includes: */
#include "UIWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* New Virtual Hard Drive wizard: */
class UIWizardNewVD : public UIWizard
{
    Q_OBJECT;

public:

    /* Page IDs: */
    enum
    {
        Page1,
        Page2,
        Page3
    };

    /* Page IDs: */
    enum
    {
        PageExpert
    };

    /* Constructor: */
    UIWizardNewVD(QWidget *pParent,
                  const QString &strDefaultName, const QString &strDefaultPath,
                  qulonglong uDefaultSize,
                  WizardMode mode = WizardMode_Auto);

    /* Pages related stuff: */
    void prepare();

    /* Returns virtual-disk: */
    CMedium virtualDisk() const { return m_virtualDisk; }

protected:

    /* Creates virtual-disk: */
    bool createVirtualDisk();

    /* Who will be able to create virtual-disk: */
    friend class UIWizardNewVDPageBasic3;
    friend class UIWizardNewVDPageExpert;

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Variables: */
    QString m_strDefaultName;
    QString m_strDefaultPath;
    qulonglong m_uDefaultSize;
    CMedium m_virtualDisk;
};

typedef QPointer<UIWizardNewVD> UISafePointerWizardNewVD;

#endif // __UIWizardNewVD_h__

