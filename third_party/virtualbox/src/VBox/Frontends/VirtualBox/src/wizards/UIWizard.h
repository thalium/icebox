/* $Id: UIWizard.h $ */
/** @file
 * VBox Qt GUI - UIWizard class declaration.
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

#ifndef __UIWizard_h__
#define __UIWizard_h__

/* Qt includes: */
#include <QWizard>
#include <QPointer>

/* Local includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class UIWizardPage;

/* QWizard class reimplementation with extended funtionality. */
class UIWizard : public QIWithRetranslateUI<QWizard>
{
    Q_OBJECT;

public:

    /* Mode related stuff: */
    WizardMode mode() { return m_mode; }

    /* Page related methods: */
    virtual void prepare();

protected slots:

    /* Page change handler: */
    virtual void sltCurrentIdChanged(int iId);
    /* Custom button 1 click handler: */
    virtual void sltCustomButtonClicked(int iId);

protected:

    /* Constructor: */
    UIWizard(QWidget *pParent, WizardType type, WizardMode mode = WizardMode_Auto);

    /* Translation stuff: */
    void retranslateUi();
    void retranslatePages();

    /* Page related methods: */
    void setPage(int iId, UIWizardPage *pPage);
    void cleanup();

    /* Adjusting stuff: */
    void resizeToGoldenRatio();

    /* Design stuff: */
#ifndef VBOX_WS_MAC
    void assignWatermark(const QString &strWaterMark);
#else
    void assignBackground(const QString &strBackground);
#endif

    /* Show event: */
    void showEvent(QShowEvent *pShowEvent);

private:

    /* Helpers: */
    void configurePage(UIWizardPage *pPage);
    void resizeAccordingLabelWidth(int iLabelWidth);
    double ratio();
#ifndef VBOX_WS_MAC
    int proposedWatermarkHeight();
    void assignWatermarkHelper();
#endif /* !VBOX_WS_MAC */

    /* Variables: */
    WizardType m_type;
    WizardMode m_mode;
#ifndef VBOX_WS_MAC
    QString m_strWatermarkName;
#endif /* !VBOX_WS_MAC */
};

typedef QPointer<UIWizard> UISafePointerWizard;

#endif // __UIWizard_h__

