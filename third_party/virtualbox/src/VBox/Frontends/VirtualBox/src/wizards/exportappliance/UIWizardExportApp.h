/* $Id: UIWizardExportApp.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportApp class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIWizardExportApp_h__
#define __UIWizardExportApp_h__

/* Local includes: */
#include "UIWizard.h"

/* Forward declarations: */
class CAppliance;

/* Export Appliance wizard: */
class UIWizardExportApp : public UIWizard
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
    UIWizardExportApp(QWidget *pParent, const QStringList &selectedVMNames = QStringList());

protected:

    /* Export appliance stuff: */
    bool exportAppliance();
    bool exportVMs(CAppliance &appliance);
    QString uri(bool fWithFile = true) const;

    /* Who will be able to export appliance: */
    friend class UIWizardExportAppPage4;
    friend class UIWizardExportAppPageBasic4;
    friend class UIWizardExportAppPageExpert;

private slots:

    /* Page change handler: */
    void sltCurrentIdChanged(int iId);
    /* Custom button 2 click handler: */
    void sltCustomButtonClicked(int iId);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Pages related stuff: */
    void prepare();

    /* Variables: */
    QStringList m_selectedVMNames;
};

#endif /* __UIWizardExportApp_h__ */

