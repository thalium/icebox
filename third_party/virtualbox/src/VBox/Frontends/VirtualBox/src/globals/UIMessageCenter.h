/* $Id: UIMessageCenter.h $ */
/** @file
 * VBox Qt GUI - UIMessageCenter class declaration.
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

#ifndef __UIMessageCenter_h__
#define __UIMessageCenter_h__

/* Qt includes: */
#include <QObject>

/* GUI includes: */
#include "QIMessageBox.h"
#include "UIMediumDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CProgress.h"

/* Forward declarations: */
class UIMedium;
struct StorageSlot;
#ifdef VBOX_WITH_DRAG_AND_DROP
class CGuest;
#endif /* VBOX_WITH_DRAG_AND_DROP */

/* Possible message types: */
enum MessageType
{
    MessageType_Info = 1,
    MessageType_Question,
    MessageType_Warning,
    MessageType_Error,
    MessageType_Critical,
    MessageType_GuruMeditation
};
Q_DECLARE_METATYPE(MessageType);

/* Global message-center object: */
class UIMessageCenter: public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Interthreading stuff: */
    void sigToShowMessageBox(QWidget *pParent, MessageType type,
                             const QString &strMessage, const QString &strDetails,
                             int iButton1, int iButton2, int iButton3,
                             const QString &strButtonText1, const QString &strButtonText2, const QString &strButtonText3,
                             const QString &strAutoConfirmId) const;

public:

    /* Static API: Create/destroy stuff: */
    static void create();
    static void destroy();

    /* API: Warning registration stuff: */
    bool warningShown(const QString &strWarningName) const;
    void setWarningShown(const QString &strWarningName, bool fWarningShown) const;

    /* API: Main message function, used directly only in exceptional cases: */
    int message(QWidget *pParent, MessageType type,
                const QString &strMessage,
                const QString &strDetails,
                const char *pcszAutoConfirmId = 0,
                int iButton1 = 0, int iButton2 = 0, int iButton3 = 0,
                const QString &strButtonText1 = QString(),
                const QString &strButtonText2 = QString(),
                const QString &strButtonText3 = QString()) const;

    /* API: Wrapper to 'message' function.
     * Provides single OK button: */
    void error(QWidget *pParent, MessageType type,
               const QString &strMessage,
               const QString &strDetails,
               const char *pcszAutoConfirmId = 0) const;

    /* API: Wrapper to 'message' function,
     * Error with question providing two buttons (OK and Cancel by default): */
    bool errorWithQuestion(QWidget *pParent, MessageType type,
                           const QString &strMessage,
                           const QString &strDetails,
                           const char *pcszAutoConfirmId = 0,
                           const QString &strOkButtonText = QString(),
                           const QString &strCancelButtonText = QString()) const;

    /* API: Wrapper to 'error' function.
     * Omits details: */
    void alert(QWidget *pParent, MessageType type,
               const QString &strMessage,
               const char *pcszAutoConfirmId = 0) const;

    /* API: Wrapper to 'message' function.
     * Omits details, provides two or three buttons: */
    int question(QWidget *pParent, MessageType type,
                 const QString &strMessage,
                 const char *pcszAutoConfirmId = 0,
                 int iButton1 = 0, int iButton2 = 0, int iButton3 = 0,
                 const QString &strButtonText1 = QString(),
                 const QString &strButtonText2 = QString(),
                 const QString &strButtonText3 = QString()) const;

    /* API: Wrapper to 'question' function,
     * Question providing two buttons (OK and Cancel by default): */
    bool questionBinary(QWidget *pParent, MessageType type,
                        const QString &strMessage,
                        const char *pcszAutoConfirmId = 0,
                        const QString &strOkButtonText = QString(),
                        const QString &strCancelButtonText = QString(),
                        bool fDefaultFocusForOk = true) const;

    /* API: Wrapper to 'question' function,
     * Question providing three buttons (Yes, No and Cancel by default): */
    int questionTrinary(QWidget *pParent, MessageType type,
                        const QString &strMessage,
                        const char *pcszAutoConfirmId = 0,
                        const QString &strChoice1ButtonText = QString(),
                        const QString &strChoice2ButtonText = QString(),
                        const QString &strCancelButtonText = QString()) const;

    /* API: One more main function: */
    int messageWithOption(QWidget *pParent, MessageType type,
                          const QString &strMessage,
                          const QString &strOptionText,
                          bool fDefaultOptionValue = true,
                          int iButton1 = 0, int iButton2 = 0, int iButton3 = 0,
                          const QString &strButtonText1 = QString(),
                          const QString &strButtonText2 = QString(),
                          const QString &strButtonText3 = QString()) const;

    /* API: Progress-dialog stuff: */
    bool showModalProgressDialog(CProgress &progress, const QString &strTitle,
                                 const QString &strImage = "", QWidget *pParent = 0,
                                 int cMinDuration = 2000);

    /* API: Main (startup) warnings: */
#ifdef RT_OS_LINUX
    void warnAboutWrongUSBMounted() const;
#endif /* RT_OS_LINUX */
    void cannotStartSelector() const;
    void showBetaBuildWarning() const;
    void showExperimentalBuildWarning() const;

    /* API: COM startup warnings: */
    void cannotInitUserHome(const QString &strUserHome) const;
    void cannotInitCOM(HRESULT rc) const;
    void cannotCreateVirtualBoxClient(const CVirtualBoxClient &client) const;
    void cannotAcquireVirtualBox(const CVirtualBoxClient &client) const;

    /* API: Global warnings: */
    void cannotFindLanguage(const QString &strLangId, const QString &strNlsPath) const;
    void cannotLoadLanguage(const QString &strLangFile) const;
    void cannotFindMachineByName(const CVirtualBox &vbox, const QString &strName) const;
    void cannotFindMachineById(const CVirtualBox &vbox, const QString &strId) const;
    void cannotOpenSession(const CSession &session) const;
    void cannotOpenSession(const CMachine &machine) const;
    void cannotOpenSession(const CProgress &progress, const QString &strMachineName) const;
    void cannotGetMediaAccessibility(const UIMedium &medium) const;
    void cannotOpenURL(const QString &strUrl) const;
    void cannotSetExtraData(const CVirtualBox &vbox, const QString &strKey, const QString &strValue);
    void cannotSetExtraData(const CMachine &machine, const QString &strKey, const QString &strValue);
    void warnAboutInvalidEncryptionPassword(const QString &strPasswordId, QWidget *pParent = 0);

    /* API: Selector warnings: */
    void cannotOpenMachine(const CVirtualBox &vbox, const QString &strMachinePath) const;
    void cannotReregisterExistingMachine(const QString &strMachinePath, const QString &strMachineName) const;
    void cannotResolveCollisionAutomatically(const QString &strCollisionName, const QString &strGroupName) const;
    bool confirmAutomaticCollisionResolve(const QString &strName, const QString &strGroupName) const;
    void cannotSetGroups(const CMachine &machine) const;
    bool confirmMachineItemRemoval(const QStringList &names) const;
    int confirmMachineRemoval(const QList<CMachine> &machines) const;
    void cannotRemoveMachine(const CMachine &machine) const;
    void cannotRemoveMachine(const CMachine &machine, const CProgress &progress) const;
    bool warnAboutInaccessibleMedia() const;
    bool confirmDiscardSavedState(const QString &strNames) const;
    bool confirmResetMachine(const QString &strNames) const;
    bool confirmACPIShutdownMachine(const QString &strNames) const;
    bool confirmPowerOffMachine(const QString &strNames) const;
    void cannotPauseMachine(const CConsole &console) const;
    void cannotResumeMachine(const CConsole &console) const;
    void cannotDiscardSavedState(const CMachine &machine) const;
    void cannotSaveMachineState(const CMachine &machine);
    void cannotSaveMachineState(const CProgress &progress, const QString &strMachineName);
    void cannotACPIShutdownMachine(const CConsole &console) const;
    void cannotPowerDownMachine(const CConsole &console) const;
    void cannotPowerDownMachine(const CProgress &progress, const QString &strMachineName) const;
    bool confirmStartMultipleMachines(const QString &strNames) const;

    /* API: Snapshot warnings: */
    int confirmSnapshotRestoring(const QString &strSnapshotName, bool fAlsoCreateNewSnapshot) const;
    bool confirmSnapshotRemoval(const QString &strSnapshotName) const;
    bool warnAboutSnapshotRemovalFreeSpace(const QString &strSnapshotName, const QString &strTargetImageName,
                                           const QString &strTargetImageMaxSize, const QString &strTargetFileSystemFree) const;
    void cannotTakeSnapshot(const CMachine &machine, const QString &strMachineName, QWidget *pParent = 0) const;
    void cannotTakeSnapshot(const CProgress &progress, const QString &strMachineName, QWidget *pParent = 0) const;
    bool cannotRestoreSnapshot(const CMachine &machine, const QString &strSnapshotName, const QString &strMachineName) const;
    bool cannotRestoreSnapshot(const CProgress &progress, const QString &strSnapshotName, const QString &strMachineName) const;
    void cannotRemoveSnapshot(const CMachine &machine, const QString &strSnapshotName, const QString &strMachineName) const;
    void cannotRemoveSnapshot(const CProgress &progress, const QString &strSnapshotName, const QString &strMachineName) const;
    void cannotChangeSnapshot(const CSnapshot &comSnapshot, const QString &strSnapshotName, const QString &strMachineName) const;
    void cannotFindSnapshotByName(const CMachine &comMachine, const QString &strName, QWidget *pParent = 0) const;
    void cannotFindSnapshotById(const CMachine &comMachine, const QString &strId, QWidget *pParent = 0) const;
    void cannotAcquireSnapshotAttributes(const CSnapshot &comSnapshot, QWidget *pParent = 0);

    /* API: Common settings warnings: */
    void cannotSaveSettings(const QString strDetails, QWidget *pParent = 0) const;

    /* API: Global settings warnings: */
    bool confirmNATNetworkRemoval(const QString &strName, QWidget *pParent = 0) const;
    void cannotSetSystemProperties(const CSystemProperties &properties, QWidget *pParent = 0) const;

    /* API: Machine settings warnings: */
    void warnAboutUnaccessibleUSB(const COMBaseWithEI &object, QWidget *pParent = 0) const;
    void warnAboutStateChange(QWidget *pParent = 0) const;
    bool confirmSettingsReloading(QWidget *pParent = 0) const;
    int confirmHardDiskAttachmentCreation(const QString &strControllerName, QWidget *pParent = 0) const;
    int confirmOpticalAttachmentCreation(const QString &strControllerName, QWidget *pParent = 0) const;
    int confirmFloppyAttachmentCreation(const QString &strControllerName, QWidget *pParent = 0) const;
    int confirmRemovingOfLastDVDDevice(QWidget *pParent = 0) const;
    void cannotAttachDevice(const CMachine &machine, UIMediumType type, const QString &strLocation, const StorageSlot &storageSlot, QWidget *pParent = 0);
    bool warnAboutIncorrectPort(QWidget *pParent = 0) const;
    bool warnAboutIncorrectAddress(QWidget *pParent = 0) const;
    bool warnAboutEmptyGuestAddress(QWidget *pParent = 0) const;
    bool warnAboutNameShouldBeUnique(QWidget *pParent = 0) const;
    bool warnAboutRulesConflict(QWidget *pParent = 0) const;
    bool confirmCancelingPortForwardingDialog(QWidget *pParent = 0) const;
    void cannotSaveMachineSettings(const CMachine &machine, QWidget *pParent = 0) const;

    /* API: Virtual Medium Manager warnings: */
    void cannotChangeMediumType(const CMedium &medium, KMediumType oldMediumType, KMediumType newMediumType, QWidget *pParent = 0) const;
    void cannotMoveMediumStorage(const CMedium &comMedium, const QString &strLocationOld, const QString &strLocationNew, QWidget *pParent = 0) const;
    void cannotMoveMediumStorage(const CProgress &comProgress, const QString &strLocationOld, const QString &strLocationNew, QWidget *pParent = 0) const;
    void cannotChangeMediumDescription(const CMedium &comMedium, const QString &strLocation, QWidget *pParent = 0) const;
    bool confirmMediumRelease(const UIMedium &medium, bool fInduced, QWidget *pParent = 0) const;
    bool confirmMediumRemoval(const UIMedium &medium, QWidget *pParent = 0) const;
    int confirmDeleteHardDiskStorage(const QString &strLocation, QWidget *pParent = 0) const;
    void cannotDeleteHardDiskStorage(const CMedium &medium, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotDeleteHardDiskStorage(const CProgress &progress, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotResizeHardDiskStorage(const CMedium &comMedium, const QString &strLocation, const QString &strSizeOld, const QString &strSizeNew, QWidget *pParent = 0) const;
    void cannotResizeHardDiskStorage(const CProgress &comProgress, const QString &strLocation, const QString &strSizeOld, const QString &strSizeNew, QWidget *pParent = 0) const;
    void cannotDetachDevice(const CMachine &machine, UIMediumType type, const QString &strLocation, const StorageSlot &storageSlot, QWidget *pParent = 0) const;
    bool cannotRemountMedium(const CMachine &machine, const UIMedium &medium, bool fMount, bool fRetry, QWidget *pParent = 0) const;
    void cannotOpenMedium(const CVirtualBox &vbox, UIMediumType type, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCloseMedium(const UIMedium &medium, const COMResult &rc, QWidget *pParent = 0) const;

    /* API: Host Network Manager warnings: */
    bool confirmHostOnlyInterfaceRemoval(const QString &strName, QWidget *pParent = 0) const;
    void cannotAcquireHostNetworkInterfaces(const CHost &comHost, QWidget *pParent = 0) const;
    void cannotFindHostNetworkInterface(const CHost &comHost, const QString &strInterfaceName, QWidget *pParent = 0) const;
    void cannotCreateHostNetworkInterface(const CHost &comHost, QWidget *pParent = 0) const;
    void cannotCreateHostNetworkInterface(const CProgress &progress, QWidget *pParent = 0) const;
    void cannotRemoveHostNetworkInterface(const CHost &comHost, const QString &strInterfaceName, QWidget *pParent = 0) const;
    void cannotRemoveHostNetworkInterface(const CProgress &progress, const QString &strInterfaceName, QWidget *pParent = 0) const;
    void cannotAcquireHostNetworkInterfaceParameter(const CHostNetworkInterface &comInterface, QWidget *pParent = 0) const;
    void cannotSaveHostNetworkInterfaceParameter(const CHostNetworkInterface &comInterface, QWidget *pParent = 0) const;
    void cannotCreateDHCPServer(const CVirtualBox &comVBox, const QString &strInterfaceName, QWidget *pParent = 0) const;
    void cannotRemoveDHCPServer(const CVirtualBox &comVBox, const QString &strInterfaceName, QWidget *pParent = 0) const;
    void cannotAcquireDHCPServerParameter(const CDHCPServer &comServer, QWidget *pParent = 0) const;
    void cannotSaveDHCPServerParameter(const CDHCPServer &comServer, QWidget *pParent = 0) const;

    /* API: Wizards warnings: */
    bool confirmHardDisklessMachine(QWidget *pParent = 0) const;
    void cannotCreateMachine(const CVirtualBox &vbox, QWidget *pParent = 0) const;
    void cannotRegisterMachine(const CVirtualBox &vbox, const QString &strMachineName, QWidget *pParent = 0) const;
    void cannotCreateClone(const CMachine &machine, QWidget *pParent = 0) const;
    void cannotCreateClone(const CProgress &progress, const QString &strMachineName, QWidget *pParent = 0) const;
    void cannotOverwriteHardDiskStorage(const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCreateHardDiskStorage(const CVirtualBox &vbox, const QString &strLocation,QWidget *pParent = 0) const;
    void cannotCreateHardDiskStorage(const CMedium &medium, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCreateHardDiskStorage(const CProgress &progress, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCreateMediumStorage(const CVirtualBox &comVBox, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCreateMediumStorage(const CMedium &comMedium, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotCreateMediumStorage(const CProgress &comProgress, const QString &strLocation, QWidget *pParent = 0) const;
    void cannotRemoveMachineFolder(const QString &strFolderName, QWidget *pParent = 0) const;
    void cannotRewriteMachineFolder(const QString &strFolderName, QWidget *pParent = 0) const;
    void cannotCreateMachineFolder(const QString &strFolderName, QWidget *pParent = 0) const;
    void cannotImportAppliance(CAppliance &appliance, QWidget *pParent = 0) const;
    void cannotImportAppliance(const CProgress &progress, const QString &strPath, QWidget *pParent = 0) const;
    void cannotCheckFiles(const CProgress &progress, QWidget *pParent = 0) const;
    void cannotRemoveFiles(const CProgress &progress, QWidget *pParent = 0) const;
    bool confirmExportMachinesInSaveState(const QStringList &machineNames, QWidget *pParent = 0) const;
    void cannotExportAppliance(const CAppliance &appliance, QWidget *pParent = 0) const;
    void cannotExportAppliance(const CMachine &machine, const QString &strPath, QWidget *pParent = 0) const;
    void cannotExportAppliance(const CProgress &progress, const QString &strPath, QWidget *pParent = 0) const;
    void cannotAddDiskEncryptionPassword(const CAppliance &appliance, QWidget *pParent = 0);

    /* API: Runtime UI warnings: */
    void showRuntimeError(const CConsole &console, bool fFatal, const QString &strErrorId, const QString &strErrorMsg) const;
    bool remindAboutGuruMeditation(const QString &strLogFolder);
    void warnAboutVBoxSVCUnavailable() const;
    bool warnAboutVirtExInactiveFor64BitsGuest(bool fHWVirtExSupported) const;
    bool warnAboutVirtExInactiveForRecommendedGuest(bool fHWVirtExSupported) const;
    bool cannotStartWithoutNetworkIf(const QString &strMachineName, const QString &strIfNames) const;
    void cannotStartMachine(const CConsole &console, const QString &strName) const;
    void cannotStartMachine(const CProgress &progress, const QString &strName) const;
    bool confirmInputCapture(bool &fAutoConfirmed) const;
    bool confirmGoingFullscreen(const QString &strHotKey) const;
    bool confirmGoingSeamless(const QString &strHotKey) const;
    bool confirmGoingScale(const QString &strHotKey) const;
    bool cannotEnterFullscreenMode(ULONG uWidth, ULONG uHeight, ULONG uBpp, ULONG64 uMinVRAM) const;
    void cannotEnterSeamlessMode(ULONG uWidth, ULONG uHeight, ULONG uBpp, ULONG64 uMinVRAM) const;
    bool cannotSwitchScreenInFullscreen(quint64 uMinVRAM) const;
    void cannotSwitchScreenInSeamless(quint64 uMinVRAM) const;
    void cannotAddDiskEncryptionPassword(const CConsole &console);

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* API: Network management warnings: */
    bool confirmCancelingAllNetworkRequests() const;
    void showUpdateSuccess(const QString &strVersion, const QString &strLink) const;
    void showUpdateNotFound() const;
    void askUserToDownloadExtensionPack(const QString &strExtPackName, const QString &strExtPackVersion, const QString &strVBoxVersion) const;

    /* API: Downloading warnings: */
    bool cannotFindGuestAdditions() const;
    bool confirmDownloadGuestAdditions(const QString &strUrl, qulonglong uSize) const;
    void cannotSaveGuestAdditions(const QString &strURL, const QString &strTarget) const;
    bool proposeMountGuestAdditions(const QString &strUrl, const QString &strSrc) const;
    void cannotValidateGuestAdditionsSHA256Sum(const QString &strUrl, const QString &strSrc) const;
    void cannotUpdateGuestAdditions(const CProgress &progress) const;
    bool cannotFindUserManual(const QString &strMissedLocation) const;
    bool confirmDownloadUserManual(const QString &strURL, qulonglong uSize) const;
    void cannotSaveUserManual(const QString &strURL, const QString &strTarget) const;
    void warnAboutUserManualDownloaded(const QString &strURL, const QString &strTarget) const;
    bool warAboutOutdatedExtensionPack(const QString &strExtPackName, const QString &strExtPackVersion) const;
    bool confirmDownloadExtensionPack(const QString &strExtPackName, const QString &strURL, qulonglong uSize) const;
    void cannotSaveExtensionPack(const QString &strExtPackName, const QString &strFrom, const QString &strTo) const;
    bool proposeInstallExtentionPack(const QString &strExtPackName, const QString &strFrom, const QString &strTo) const;
    void cannotValidateExtentionPackSHA256Sum(const QString &strExtPackName, const QString &strFrom, const QString &strTo) const;
    bool proposeDeleteExtentionPack(const QString &strTo) const;
    bool proposeDeleteOldExtentionPacks(const QStringList &strFiles) const;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

    /* API: Extension-pack warnings: */
    bool confirmInstallExtensionPack(const QString &strPackName, const QString &strPackVersion, const QString &strPackDescription, QWidget *pParent = 0) const;
    bool confirmReplaceExtensionPack(const QString &strPackName, const QString &strPackVersionNew, const QString &strPackVersionOld,
                                     const QString &strPackDescription, QWidget *pParent = 0) const;
    bool confirmRemoveExtensionPack(const QString &strPackName, QWidget *pParent = 0) const;
    void cannotOpenExtPack(const QString &strFilename, const CExtPackManager &extPackManager, QWidget *pParent = 0) const;
    void warnAboutBadExtPackFile(const QString &strFilename, const CExtPackFile &extPackFile, QWidget *pParent = 0) const;
    void cannotInstallExtPack(const CExtPackFile &extPackFile, const QString &strFilename, QWidget *pParent = 0) const;
    void cannotInstallExtPack(const CProgress &progress, const QString &strFilename, QWidget *pParent = 0) const;
    void cannotUninstallExtPack(const CExtPackManager &extPackManager, const QString &strPackName, QWidget *pParent = 0) const;
    void cannotUninstallExtPack(const CProgress &progress, const QString &strPackName, QWidget *pParent = 0) const;
    void warnAboutExtPackInstalled(const QString &strPackName, QWidget *pParent = 0) const;

#ifdef VBOX_WITH_DRAG_AND_DROP
    /* API: Drag and drop warnings: */
    void cannotDropDataToGuest(const CDnDTarget &dndTarget, QWidget *pParent = 0) const;
    void cannotDropDataToGuest(const CProgress &progress, QWidget *pParent = 0) const;
    void cannotCancelDropToGuest(const CDnDTarget &dndTarget, QWidget *pParent = 0) const;
    void cannotDropDataToHost(const CDnDSource &dndSource, QWidget *pParent = 0) const;
    void cannotDropDataToHost(const CProgress &progress, QWidget *pParent = 0) const;
#endif /* VBOX_WITH_DRAG_AND_DROP */

    /* API: License-viewer warnings: */
    void cannotOpenLicenseFile(const QString &strPath, QWidget *pParent = 0) const;

    /* API: File-dialog warnings: */
    bool confirmOverridingFile(const QString &strPath, QWidget *pParent = 0) const;
    bool confirmOverridingFiles(const QVector<QString> &strPaths, QWidget *pParent = 0) const;
    bool confirmOverridingFileIfExists(const QString &strPath, QWidget *pParent = 0) const;
    bool confirmOverridingFilesIfExists(const QVector<QString> &strPaths, QWidget *pParent = 0) const;

public slots:

    /* Handlers: Help menu stuff: */
    void sltShowHelpWebDialog();
    void sltShowBugTracker();
    void sltShowForums();
    void sltShowOracle();
    void sltShowHelpAboutDialog();
    void sltShowHelpHelpDialog();
    void sltResetSuppressedMessages();
    void sltShowUserManual(const QString &strLocation);

private slots:

    /* Handler: Interthreading stuff: */
    void sltShowMessageBox(QWidget *pParent, MessageType type,
                           const QString &strMessage, const QString &strDetails,
                           int iButton1, int iButton2, int iButton3,
                           const QString &strButtonText1, const QString &strButtonText2, const QString &strButtonText3,
                           const QString &strAutoConfirmId) const;

private:

    /* Constructor/destructor: */
    UIMessageCenter();
    ~UIMessageCenter();

    /* Helpers: Prepare/cleanup stuff: */
    void prepare();
    void cleanup();

    /* Helper: Message-box stuff: */
    int showMessageBox(QWidget *pParent, MessageType type,
                       const QString &strMessage, const QString &strDetails,
                       int iButton1, int iButton2, int iButton3,
                       const QString &strButtonText1, const QString &strButtonText2, const QString &strButtonText3,
                       const QString &strAutoConfirmId) const;

    /* Variables: */
    mutable QStringList m_warnings;

    /* API: Instance stuff: */
    static UIMessageCenter* m_spInstance;
    static UIMessageCenter* instance();
    friend UIMessageCenter& msgCenter();
};

/* Shortcut to the static UIMessageCenter::instance() method: */
inline UIMessageCenter& msgCenter() { return *UIMessageCenter::instance(); }

#endif // __UIMessageCenter_h__

