/* $Id: UIPopupCenter.h $ */
/** @file
 * VBox Qt GUI - UIPopupCenter class declaration.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIPopupCenter_h__
#define __UIPopupCenter_h__

/* Qt includes: */
#include <QMap>
#include <QObject>
#include <QPointer>

/* GUI includes: */
#include "UIMediumDefs.h"

/* Forward declaration: */
class QWidget;
class UIPopupStack;
class CAudioAdapter;
class CConsole;
class CEmulatedUSB;
class CMachine;
class CNetworkAdapter;
class CVirtualBox;
class CVirtualBoxErrorInfo;
class CVRDEServer;

/* Popup-stack types: */
enum UIPopupStackType
{
    UIPopupStackType_Embedded,
    UIPopupStackType_Separate
};

/* Popup-stack orientations: */
enum UIPopupStackOrientation
{
    UIPopupStackOrientation_Top,
    UIPopupStackOrientation_Bottom
};

/* Popup-center singleton: */
class UIPopupCenter: public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Popup-pane stuff: */
    void sigPopupPaneDone(QString strPopupPaneID, int iResultCode);

public:

    /* Static API: Create/destroy stuff: */
    static void create();
    static void destroy();

    /* API: Popup-stack stuff: */
    void showPopupStack(QWidget *pParent);
    void hidePopupStack(QWidget *pParent);
    void setPopupStackType(QWidget *pParent, UIPopupStackType newStackType);
    void setPopupStackOrientation(QWidget *pParent, UIPopupStackOrientation newStackOrientation);

    /* API: Main message function.
     * Provides up to two buttons.
     * Its interface, do NOT use it directly! */
    void message(QWidget *pParent, const QString &strPopupPaneID,
                 const QString &strMessage, const QString &strDetails,
                 const QString &strButtonText1 = QString(),
                 const QString &strButtonText2 = QString(),
                 bool fProposeAutoConfirmation = false);

    /* API: Wrapper to 'message' function.
     * Omits details, provides no buttons: */
    void popup(QWidget *pParent, const QString &strPopupPaneID,
               const QString &strMessage);

    /* API: Wrapper to 'message' function.
     * Omits details, provides one button: */
    void alert(QWidget *pParent, const QString &strPopupPaneID,
               const QString &strMessage,
               bool fProposeAutoConfirmation = false);

    /* API: Wrapper to 'message' function.
     * Provides one button: */
    void alertWithDetails(QWidget *pParent, const QString &strPopupPaneID,
                          const QString &strMessage,
                          const QString &strDetails,
                          bool fProposeAutoConfirmation = false);

    /* API: Wrapper to 'message' function.
     * Omits details, provides up to two buttons: */
    void question(QWidget *pParent, const QString &strPopupPaneID,
                  const QString &strMessage,
                  const QString &strButtonText1 = QString(),
                  const QString &strButtonText2 = QString(),
                  bool fProposeAutoConfirmation = false);

    /* API: Recall function,
     * Close corresponding popup of passed parent: */
    void recall(QWidget *pParent, const QString &strPopupPaneID);

    /* API: Runtime UI stuff: */
    void cannotSendACPIToMachine(QWidget *pParent);
    void remindAboutAutoCapture(QWidget *pParent);
    void remindAboutMouseIntegration(QWidget *pParent, bool fSupportsAbsolute);
    void remindAboutPausedVMInput(QWidget *pParent);
    void forgetAboutPausedVMInput(QWidget *pParent);
    void remindAboutWrongColorDepth(QWidget *pParent, ulong uRealBPP, ulong uWantedBPP);
    void forgetAboutWrongColorDepth(QWidget *pParent);
    void cannotAttachUSBDevice(QWidget *pParent, const CConsole &comConsole, const QString &strDevice);
    void cannotAttachUSBDevice(QWidget *pParent, const CVirtualBoxErrorInfo &comErrorInfo,
                               const QString &strDevice, const QString &strMachineName);
    void cannotDetachUSBDevice(QWidget *pParent, const CConsole &comConsole, const QString &strDevice);
    void cannotDetachUSBDevice(QWidget *pParent, const CVirtualBoxErrorInfo &comErrorInfo,
                               const QString &strDevice, const QString &strMachineName);
    void cannotAttachWebCam(QWidget *pParent, const CEmulatedUSB &comDispatcher,
                            const QString &strWebCamName, const QString &strMachineName);
    void cannotDetachWebCam(QWidget *pParent, const CEmulatedUSB &comDispatcher,
                            const QString &strWebCamName, const QString &strMachineName);
    void cannotToggleVideoCapture(QWidget *pParent, const CMachine &comMachine, bool fEnable);
    void cannotToggleVRDEServer(QWidget *pParent,  const CVRDEServer &comServer,
                                const QString &strMachineName, bool fEnable);
    void cannotToggleNetworkAdapterCable(QWidget *pParent, const CNetworkAdapter &comAdapter,
                                         const QString &strMachineName, bool fConnect);
    void remindAboutGuestAdditionsAreNotActive(QWidget *pParent);
    void cannotToggleAudioOutput(QWidget *pParent, const CAudioAdapter &comAdapter,
                                 const QString &strMachineName, bool fEnable);
    void cannotToggleAudioInput(QWidget *pParent, const CAudioAdapter &comAdapter,
                                const QString &strMachineName, bool fEnable);
    void cannotMountImage(QWidget *pParent, const QString &strMachineName, const QString &strMediumName);
    void cannotOpenMedium(QWidget *pParent, const CVirtualBox &comVBox, UIMediumType enmType, const QString &strLocation);
    void cannotSaveMachineSettings(QWidget *pParent, const CMachine &comMachine);

private slots:

    /* Handler: Popup-pane stuff: */
    void sltPopupPaneDone(QString strPopupPaneID, int iResultCode);

    /* Handler: Popup-stack stuff: */
    void sltRemovePopupStack(QString strPopupStackID);

private:

    /* Constructor/destructor: */
    UIPopupCenter();
    ~UIPopupCenter();

    /* Helpers: Prepare/cleanup stuff: */
    void prepare();
    void cleanup();

    /* Helpers: Popup-pane stuff: */
    void showPopupPane(QWidget *pParent, const QString &strPopupPaneID,
                       const QString &strMessage, const QString &strDetails,
                       QString strButtonText1 = QString(), QString strButtonText2 = QString(),
                       bool fProposeAutoConfirmation = false);
    void hidePopupPane(QWidget *pParent, const QString &strPopupPaneID);

    /* Static helper: Popup-stack stuff: */
    static QString popupStackID(QWidget *pParent);
    static void assignPopupStackParent(UIPopupStack *pPopupStack, QWidget *pParent, UIPopupStackType stackType);
    static void unassignPopupStackParent(UIPopupStack *pPopupStack, QWidget *pParent);

    /* Variables: Popup-stack stuff: */
    QMap<QString, UIPopupStackType> m_stackTypes;
    QMap<QString, UIPopupStackOrientation> m_stackOrientations;
    QMap<QString, QPointer<UIPopupStack> > m_stacks;

    /* Instance stuff: */
    static UIPopupCenter* m_spInstance;
    static UIPopupCenter* instance();
    friend UIPopupCenter& popupCenter();
};

/* Shortcut to the static UIPopupCenter::instance() method: */
inline UIPopupCenter& popupCenter() { return *UIPopupCenter::instance(); }

#endif /* __UIPopupCenter_h__ */
