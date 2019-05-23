/* $Id: UIVMCloseDialog.h $ */
/** @file
 * VBox Qt GUI - UIVMCloseDialog class declaration.
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

#ifndef __UIVMCloseDialog_h__
#define __UIVMCloseDialog_h__

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class CMachine;
class QLabel;
class QRadioButton;
class QCheckBox;

/* QIDialog extension to handle Runtime UI close-event: */
class UIVMCloseDialog : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIVMCloseDialog(QWidget *pParent, CMachine &machine,
                    bool fIsACPIEnabled, MachineCloseAction restictedCloseActions);

    /* API: Validation stuff: */
    bool isValid() const { return m_fValid; }

    /* API: Pixmap stuff: */
    void setPixmap(const QPixmap &pixmap);

private slots:

    /* Handler: Update stuff: */
    void sltUpdateWidgetAvailability();

    /* Handler: Accept stuff: */
    void accept();

private:

    /* API: Detach-button stuff: */
    void setDetachButtonEnabled(bool fEnabled);
    void setDetachButtonVisible(bool fVisible);
    /* API: Save-button stuff: */
    void setSaveButtonEnabled(bool fEnabled);
    void setSaveButtonVisible(bool fVisible);
    /* API: Shutdown-button stuff: */
    void setShutdownButtonEnabled(bool fEnabled);
    void setShutdownButtonVisible(bool fVisible);
    /* API: Power-off-button stuff: */
    void setPowerOffButtonEnabled(bool fEnabled);
    void setPowerOffButtonVisible(bool fVisible);
    /* API: Discard-check-box stuff: */
    void setDiscardCheckBoxVisible(bool fVisible);

    /* Helpers: Prepare stuff: */
    void prepare();
    void configure();

    /* Helper: Translate stuff: */
    void retranslateUi();

    /* Handler: Event-filtering stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Handler: Polish-event stuff: */
    void polishEvent(QShowEvent *pEvent);

    /* Widgets: */
    QLabel *m_pIcon;
    QLabel *m_pLabel;
    QLabel *m_pDetachIcon;
    QRadioButton *m_pDetachRadio;
    QLabel *m_pSaveIcon;
    QRadioButton *m_pSaveRadio;
    QLabel *m_pShutdownIcon;
    QRadioButton *m_pShutdownRadio;
    QLabel *m_pPowerOffIcon;
    QRadioButton *m_pPowerOffRadio;
    QCheckBox *m_pDiscardCheckBox;

    /* Variables: */
    CMachine &m_machine;
    const MachineCloseAction m_restictedCloseActions;
    bool m_fIsACPIEnabled;
    bool m_fValid;
    QString m_strDiscardCheckBoxText;
    MachineCloseAction m_lastCloseAction;
};

#endif // __UIVMCloseDialog_h__

