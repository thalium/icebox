/* $Id: UIVMCloseDialog.cpp $ */
/** @file
 * VBox Qt GUI - UIVMCloseDialog class implementation.
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
# include <QVBoxLayout>
# include <QHBoxLayout>
# include <QGridLayout>
# include <QLabel>
# include <QRadioButton>
# include <QCheckBox>
# include <QPushButton>

/* GUI includes: */
# include "UIIconPool.h"
# include "UIVMCloseDialog.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "UIConverter.h"
# include "VBoxGlobal.h"
# include "QIDialogButtonBox.h"

/* COM includes: */
# include "CMachine.h"
# include "CSession.h"
# include "CConsole.h"
# include "CSnapshot.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIVMCloseDialog::UIVMCloseDialog(QWidget *pParent, CMachine &machine,
                                 bool fIsACPIEnabled, MachineCloseAction restictedCloseActions)
    : QIWithRetranslateUI<QIDialog>(pParent)
    , m_pIcon(0), m_pLabel(0)
    , m_pDetachIcon(0), m_pDetachRadio(0)
    , m_pSaveIcon(0), m_pSaveRadio(0)
    , m_pShutdownIcon(0), m_pShutdownRadio(0)
    , m_pPowerOffIcon(0), m_pPowerOffRadio(0), m_pDiscardCheckBox(0)
    , m_machine(machine)
    , m_restictedCloseActions(restictedCloseActions)
    , m_fIsACPIEnabled(fIsACPIEnabled)
    , m_fValid(false)
    , m_lastCloseAction(MachineCloseAction_Invalid)
{
    /* Prepare: */
    prepare();

    /* Configure: */
    configure();

    /* Retranslate: */
    retranslateUi();
}

void UIVMCloseDialog::setPixmap(const QPixmap &pixmap)
{
    /* Make sure pixmap is valid: */
    if (pixmap.isNull())
        return;

    /* Assign new pixmap: */
    m_pIcon->setPixmap(pixmap);
}

void UIVMCloseDialog::sltUpdateWidgetAvailability()
{
    /* Discard option should be enabled only on power-off action: */
    m_pDiscardCheckBox->setEnabled(m_pPowerOffRadio->isChecked());
}

void UIVMCloseDialog::accept()
{
    /* Calculate result: */
    if (m_pDetachRadio->isChecked())
        setResult(MachineCloseAction_Detach);
    else if (m_pSaveRadio->isChecked())
        setResult(MachineCloseAction_SaveState);
    else if (m_pShutdownRadio->isChecked())
        setResult(MachineCloseAction_Shutdown);
    else if (m_pPowerOffRadio->isChecked())
    {
        if (!m_pDiscardCheckBox->isChecked() || !m_pDiscardCheckBox->isVisible())
            setResult(MachineCloseAction_PowerOff);
        else
            setResult(MachineCloseAction_PowerOff_RestoringSnapshot);
    }

    /* Memorize the last user's choice for the given VM: */
    MachineCloseAction newCloseAction = static_cast<MachineCloseAction>(result());
    /* But make sure 'Shutdown' is preserved if temporary unavailable: */
    if (newCloseAction == MachineCloseAction_PowerOff &&
        m_lastCloseAction == MachineCloseAction_Shutdown && !m_fIsACPIEnabled)
        newCloseAction = MachineCloseAction_Shutdown;
    gEDataManager->setLastMachineCloseAction(newCloseAction, vboxGlobal().managedVMUuid());

    /* Hide the dialog: */
    hide();
}

void UIVMCloseDialog::setDetachButtonEnabled(bool fEnabled)
{
    m_pDetachIcon->setEnabled(fEnabled);
    m_pDetachRadio->setEnabled(fEnabled);
}

void UIVMCloseDialog::setDetachButtonVisible(bool fVisible)
{
    m_pDetachIcon->setVisible(fVisible);
    m_pDetachRadio->setVisible(fVisible);
}

void UIVMCloseDialog::setSaveButtonEnabled(bool fEnabled)
{
    m_pSaveIcon->setEnabled(fEnabled);
    m_pSaveRadio->setEnabled(fEnabled);
}

void UIVMCloseDialog::setSaveButtonVisible(bool fVisible)
{
    m_pSaveIcon->setVisible(fVisible);
    m_pSaveRadio->setVisible(fVisible);
}

void UIVMCloseDialog::setShutdownButtonEnabled(bool fEnabled)
{
    m_pShutdownIcon->setEnabled(fEnabled);
    m_pShutdownRadio->setEnabled(fEnabled);
}

void UIVMCloseDialog::setShutdownButtonVisible(bool fVisible)
{
    m_pShutdownIcon->setVisible(fVisible);
    m_pShutdownRadio->setVisible(fVisible);
}

void UIVMCloseDialog::setPowerOffButtonEnabled(bool fEnabled)
{
    m_pPowerOffIcon->setEnabled(fEnabled);
    m_pPowerOffRadio->setEnabled(fEnabled);
}

void UIVMCloseDialog::setPowerOffButtonVisible(bool fVisible)
{
    m_pPowerOffIcon->setVisible(fVisible);
    m_pPowerOffRadio->setVisible(fVisible);
}

void UIVMCloseDialog::setDiscardCheckBoxVisible(bool fVisible)
{
    m_pDiscardCheckBox->setVisible(fVisible);
}

void UIVMCloseDialog::prepare()
{
    /* Prepare 'main' layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        /* Configure layout: */
#ifdef VBOX_WS_MAC
        pMainLayout->setContentsMargins(40, 20, 40, 20);
        pMainLayout->setSpacing(15);
#else
        pMainLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) * 2);
#endif

        /* Prepare 'top' layout: */
        QHBoxLayout *pTopLayout = new QHBoxLayout;
        {
            /* Configure layout: */
#ifdef VBOX_WS_MAC
            pTopLayout->setSpacing(20);
#else
            pTopLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) * 2);
#endif

            /* Prepare 'top-left' layout: */
            QVBoxLayout *pTopLeftLayout = new QVBoxLayout;
            {
                /* Prepare 'icon': */
                m_pIcon = new QLabel(this);
                {
                    /* Configure icon: */
                    m_pIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
                    const QIcon icon = UIIconPool::iconSet(":/os_unknown.png");
                    m_pIcon->setPixmap(icon.pixmap(iIconMetric, iIconMetric));
                }

                /* Add into layout: */
                pTopLeftLayout->addWidget(m_pIcon);
                pTopLeftLayout->addStretch();
            }
            /* Prepare 'top-right' layout: */
            QVBoxLayout *pTopRightLayout = new QVBoxLayout;
            {
                /* Configure layout: */
#ifdef VBOX_WS_MAC
                pTopRightLayout->setSpacing(10);
#else
                pTopRightLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));
#endif

                /* Prepare 'text' label: */
                m_pLabel = new QLabel(this);
                /* Prepare 'choice' layout: */
                QGridLayout *pChoiceLayout = new QGridLayout;
                {
                    /* Configure layout: */
#ifdef VBOX_WS_MAC
                    pChoiceLayout->setSpacing(10);
#else
                    pChoiceLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));
#endif

                    /* Prepare icon metric: */
                    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
                    /* Prepare 'detach' icon: */
                    m_pDetachIcon = new QLabel(this);
                    {
                        /* Configure icon: */
                        m_pDetachIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                        const QIcon icon = UIIconPool::iconSet(":/vm_create_shortcut_16px.png");
                        m_pDetachIcon->setPixmap(icon.pixmap(iIconMetric, iIconMetric));
                    }
                    /* Prepare 'detach' radio-button: */
                    m_pDetachRadio = new QRadioButton(this);
                    {
                        /* Configure button: */
                        m_pDetachRadio->installEventFilter(this);
                        connect(m_pDetachRadio, SIGNAL(toggled(bool)), this, SLOT(sltUpdateWidgetAvailability()));
                    }
                    /* Prepare 'save' icon: */
                    m_pSaveIcon = new QLabel(this);
                    {
                        /* Configure icon: */
                        m_pSaveIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                        const QIcon icon = UIIconPool::iconSet(":/vm_save_state_16px.png");
                        m_pSaveIcon->setPixmap(icon.pixmap(iIconMetric, iIconMetric));
                    }
                    /* Prepare 'save' radio-button: */
                    m_pSaveRadio = new QRadioButton(this);
                    {
                        /* Configure button: */
                        m_pSaveRadio->installEventFilter(this);
                        connect(m_pSaveRadio, SIGNAL(toggled(bool)), this, SLOT(sltUpdateWidgetAvailability()));
                    }
                    /* Prepare 'shutdown' icon: */
                    m_pShutdownIcon = new QLabel(this);
                    {
                        /* Configure icon: */
                        m_pShutdownIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                        const QIcon icon = UIIconPool::iconSet(":/vm_shutdown_16px.png");
                        m_pShutdownIcon->setPixmap(icon.pixmap(iIconMetric, iIconMetric));
                    }
                    /* Prepare 'shutdown' radio-button: */
                    m_pShutdownRadio = new QRadioButton(this);
                    {
                        /* Configure button: */
                        m_pShutdownRadio->installEventFilter(this);
                        connect(m_pShutdownRadio, SIGNAL(toggled(bool)), this, SLOT(sltUpdateWidgetAvailability()));
                    }
                    /* Prepare 'power-off' icon: */
                    m_pPowerOffIcon = new QLabel(this);
                    {
                        /* Configure icon: */
                        m_pPowerOffIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                        const QIcon icon = UIIconPool::iconSet(":/vm_poweroff_16px.png");
                        m_pPowerOffIcon->setPixmap(icon.pixmap(iIconMetric, iIconMetric));
                    }
                    /* Prepare 'shutdown' radio-button: */
                    m_pPowerOffRadio = new QRadioButton(this);
                    {
                        /* Configure button: */
                        m_pPowerOffRadio->installEventFilter(this);
                        connect(m_pPowerOffRadio, SIGNAL(toggled(bool)), this, SLOT(sltUpdateWidgetAvailability()));
                    }
                    /* Prepare 'discard' check-box: */
                    m_pDiscardCheckBox = new QCheckBox(this);

                    /* Add into layout: */
                    pChoiceLayout->addWidget(m_pDetachIcon, 0, 0);
                    pChoiceLayout->addWidget(m_pDetachRadio, 0, 1);
                    pChoiceLayout->addWidget(m_pSaveIcon, 1, 0);
                    pChoiceLayout->addWidget(m_pSaveRadio, 1, 1);
                    pChoiceLayout->addWidget(m_pShutdownIcon, 2, 0);
                    pChoiceLayout->addWidget(m_pShutdownRadio, 2, 1);
                    pChoiceLayout->addWidget(m_pPowerOffIcon, 3, 0);
                    pChoiceLayout->addWidget(m_pPowerOffRadio, 3, 1);
                    pChoiceLayout->addWidget(m_pDiscardCheckBox, 4, 1);
                }

                /* Add into layout: */
                pTopRightLayout->addWidget(m_pLabel);
                pTopRightLayout->addItem(pChoiceLayout);
            }

            /* Add into layout: */
            pTopLayout->addItem(pTopLeftLayout);
            pTopLayout->addItem(pTopRightLayout);
        }

        /* Prepare button-box: */
        QIDialogButtonBox *pButtonBox = new QIDialogButtonBox(this);
        {
            /* Configure button-box: */
            pButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::NoButton | QDialogButtonBox::Ok);
            connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
            connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
            connect(pButtonBox, SIGNAL(helpRequested()), &msgCenter(), SLOT(sltShowHelpHelpDialog()));
        }

        /* Add into layout: */
        pMainLayout->addItem(pTopLayout);
        pMainLayout->addWidget(pButtonBox);
    }
    /* Prepare size-grip token: */
    setSizeGripEnabled(false);
}

void UIVMCloseDialog::configure()
{
    /* Get actual machine-state: */
    KMachineState machineState = m_machine.GetState();

    /* Check which close-actions are resticted: */
    bool fIsDetachAllowed = vboxGlobal().isSeparateProcess() && !(m_restictedCloseActions & MachineCloseAction_Detach);
    bool fIsStateSavingAllowed = !(m_restictedCloseActions & MachineCloseAction_SaveState);
    bool fIsACPIShutdownAllowed = !(m_restictedCloseActions & MachineCloseAction_Shutdown);
    bool fIsPowerOffAllowed = !(m_restictedCloseActions & MachineCloseAction_PowerOff);
    bool fIsPowerOffAndRestoreAllowed = fIsPowerOffAllowed && !(m_restictedCloseActions & MachineCloseAction_PowerOff_RestoringSnapshot);

    /* Make 'Detach' button visible/hidden depending on restriction: */
    setDetachButtonVisible(fIsDetachAllowed);
    /* Make 'Detach' button enabled/disabled depending on machine-state: */
    setDetachButtonEnabled(machineState != KMachineState_Stuck);

    /* Make 'Save state' button visible/hidden depending on restriction: */
    setSaveButtonVisible(fIsStateSavingAllowed);
    /* Make 'Save state' button enabled/disabled depending on machine-state: */
    setSaveButtonEnabled(machineState != KMachineState_Stuck);

    /* Make 'Shutdown' button visible/hidden depending on restriction: */
    setShutdownButtonVisible(fIsACPIShutdownAllowed);
    /* Make 'Shutdown' button enabled/disabled depending on console and machine-state: */
    setShutdownButtonEnabled(m_fIsACPIEnabled && machineState != KMachineState_Stuck);

    /* Make 'Power off' button visible/hidden depending on restriction: */
    setPowerOffButtonVisible(fIsPowerOffAllowed);
    /* Make the Restore Snapshot checkbox visible/hidden depending on snapshot count & restrictions: */
    setDiscardCheckBoxVisible(fIsPowerOffAndRestoreAllowed && m_machine.GetSnapshotCount() > 0);
    /* Assign Restore Snapshot checkbox text: */
    if (!m_machine.GetCurrentSnapshot().isNull())
        m_strDiscardCheckBoxText = m_machine.GetCurrentSnapshot().GetName();

    /* Check which radio-button should be initially chosen: */
    QRadioButton *pRadioButtonToChoose = 0;
    /* If choosing 'last choice' is possible: */
    m_lastCloseAction = gEDataManager->lastMachineCloseAction(vboxGlobal().managedVMUuid());
    if (m_lastCloseAction == MachineCloseAction_Detach && fIsDetachAllowed)
    {
        pRadioButtonToChoose = m_pDetachRadio;
    }
    else if (m_lastCloseAction == MachineCloseAction_SaveState && fIsStateSavingAllowed)
    {
        pRadioButtonToChoose = m_pSaveRadio;
    }
    else if (m_lastCloseAction == MachineCloseAction_Shutdown && fIsACPIShutdownAllowed && m_fIsACPIEnabled)
    {
        pRadioButtonToChoose = m_pShutdownRadio;
    }
    else if (m_lastCloseAction == MachineCloseAction_PowerOff && fIsPowerOffAllowed)
    {
        pRadioButtonToChoose = m_pPowerOffRadio;
    }
    else if (m_lastCloseAction == MachineCloseAction_PowerOff_RestoringSnapshot && fIsPowerOffAndRestoreAllowed)
    {
        pRadioButtonToChoose = m_pPowerOffRadio;
        m_pDiscardCheckBox->setChecked(true);
    }
    /* Else 'default choice' will be used: */
    else
    {
        if (fIsDetachAllowed)
            pRadioButtonToChoose = m_pDetachRadio;
        else if (fIsStateSavingAllowed)
            pRadioButtonToChoose = m_pSaveRadio;
        else if (fIsACPIShutdownAllowed && m_fIsACPIEnabled)
            pRadioButtonToChoose = m_pShutdownRadio;
        else if (fIsPowerOffAllowed)
            pRadioButtonToChoose = m_pPowerOffRadio;
    }

    /* If some radio-button chosen: */
    if (pRadioButtonToChoose)
    {
        /* Check and focus it: */
        pRadioButtonToChoose->setChecked(true);
        pRadioButtonToChoose->setFocus();
        sltUpdateWidgetAvailability();
        m_fValid = true;
    }
}

void UIVMCloseDialog::retranslateUi()
{
    /* Translate title: */
    setWindowTitle(tr("Close Virtual Machine"));

    /* Translate 'text' label: */
    m_pLabel->setText(tr("You want to:"));

    /* Translate radio-buttons: */
    m_pDetachRadio->setText(tr("&Continue running in the background"));
    m_pDetachRadio->setWhatsThis(tr("<p>Close the virtual machine windows but keep the virtual machine running.</p>"
                                    "<p>You can use the VirtualBox Manager to return to running the virtual machine in a window.</p>"));
    m_pSaveRadio->setText(tr("&Save the machine state"));
    m_pSaveRadio->setWhatsThis(tr("<p>Saves the current execution state of the virtual machine to the physical hard disk of the host PC.</p>"
                                  "<p>Next time this machine is started, it will be restored from the saved state and continue execution "
                                  "from the same place you saved it at, which will let you continue your work immediately.</p>"
                                  "<p>Note that saving the machine state may take a long time, depending on the guest operating "
                                  "system type and the amount of memory you assigned to the virtual machine.</p>"));
    m_pShutdownRadio->setText(tr("S&end the shutdown signal"));
    m_pShutdownRadio->setWhatsThis(tr("<p>Sends the ACPI Power Button press event to the virtual machine.</p>"
                                      "<p>Normally, the guest operating system running inside the virtual machine will detect this event "
                                      "and perform a clean shutdown procedure. This is a recommended way to turn off the virtual machine "
                                      "because all applications running inside it will get a chance to save their data and state.</p>"
                                      "<p>If the machine doesn't respond to this action then the guest operating system may be misconfigured "
                                      "or doesn't understand ACPI Power Button events at all. In this case you should select the "
                                      "<b>Power off the machine</b> action to stop virtual machine execution.</p>"));
    m_pPowerOffRadio->setText(tr("&Power off the machine"));
    m_pPowerOffRadio->setWhatsThis(tr("<p>Turns off the virtual machine.</p>"
                                      "<p>Note that this action will stop machine execution immediately so that the guest operating system "
                                      "running inside it will not be able to perform a clean shutdown procedure which may result in "
                                      "<i>data loss</i> inside the virtual machine. Selecting this action is recommended only if the "
                                      "virtual machine does not respond to the <b>Send the shutdown signal</b> action.</p>"));
    m_pDiscardCheckBox->setText(tr("&Restore current snapshot '%1'").arg(m_strDiscardCheckBoxText));
    m_pDiscardCheckBox->setToolTip(tr("Restore the machine state stored in the current snapshot"));
    m_pDiscardCheckBox->setWhatsThis(tr("<p>When checked, the machine will be returned to the state stored in the current snapshot after "
                                        "it is turned off. This is useful if you are sure that you want to discard the results of your "
                                        "last sessions and start again at that snapshot.</p>"));
}

bool UIVMCloseDialog::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* For now we are interested in double-click events only: */
    if (pEvent->type() == QEvent::MouseButtonDblClick)
    {
        /* Make sure its one of the radio-buttons
         * which has this event-filter installed: */
        if (qobject_cast<QRadioButton*>(pWatched))
        {
            /* Since on double-click the button will be also selected
             * we are just calling for the *accept* slot: */
            accept();
        }
    }

    /* Call to base-class: */
    return QIWithRetranslateUI<QIDialog>::eventFilter(pWatched, pEvent);
}

void UIVMCloseDialog::polishEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIDialog::polishEvent(pEvent);

    /* Make the dialog-size fixed: */
    setFixedSize(size());
}

