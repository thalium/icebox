/* $Id: VBoxGlobal.h $ */
/** @file
 * VBox Qt GUI - VBoxGlobal class declaration.
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

#ifndef ___VBoxGlobal_h___
#define ___VBoxGlobal_h___

/* Qt includes: */
#include <QApplication>
#include <QLayout>
#include <QMenu>
#include <QStyle>
#include <QHash>
#include <QFileIconProvider>
#include <QReadWriteLock>
#ifdef VBOX_WS_MAC
# include <QSet>
#endif /* VBOX_WS_MAC */

/* GUI includes: */
#include "UIDefs.h"
#include "UIMediumDefs.h"
#ifdef VBOX_WS_X11
# include "VBoxX11Helper.h"
#endif /* VBOX_WS_X11 */

/* COM includes: */
#include "VBox/com/Guid.h"
#include "CHost.h"
#include "CVirtualBoxClient.h"
#include "CVirtualBox.h"
#include "CSession.h"
#include "CGuestOSType.h"

/* Other includes: */
#ifdef VBOX_WS_X11
# include <X11/Xdefs.h>
#endif /* VBOX_WS_X11 */

/* Forward declarations: */
class QAction;
class QLabel;
class QToolButton;
class UIMachine;
class CMachine;
class CMedium;
class CUSBDevice;
class CHostVideoInputDevice;
class QSpinBox;
class UIMediumEnumerator;
class UIMedium;
class UIIconPoolGeneral;
class UIThreadPool;

// VBoxGlobal class
////////////////////////////////////////////////////////////////////////////////

class VBoxUpdateDlg;

class VBoxGlobal : public QObject
{
    Q_OBJECT

public:

    /** VM launch modes. */
    enum LaunchMode
    {
        LaunchMode_Invalid,
        LaunchMode_Default,
        LaunchMode_Headless,
        LaunchMode_Separate
    };

    /** Whether to start the VM running. */
    enum StartRunning
    {
        StartRunning_Default,   /**< Default (depends on debug settings). */
        StartRunning_No,        /**< Start the VM paused. */
        StartRunning_Yes        /**< Start the VM running. */
    };

    /* Static API: Create/destroy stuff: */
    static VBoxGlobal* instance();
    static void create();
    static void destroy();

    bool isValid() { return mValid; }
    bool isCleaningUp() { return m_sfCleanupInProgress; }

    static QString qtRTVersionString();
    static uint qtRTVersion();
    /** Returns Qt runtime major version. */
    static uint qtRTMajorVersion();
    /** Returns Qt runtime minor version. */
    static uint qtRTMinorVersion();
    /** Returns Qt runtime revision number. */
    static uint qtRTRevisionNumber();
    static QString qtCTVersionString();
    static uint qtCTVersion();

    QString vboxVersionString() const;
    QString vboxVersionStringNormalized() const;

    QString versionString() const { return mVerString; }
    bool isBeta() const;

    /** Returns whether GUI is separate (from VM) process. */
    bool isSeparateProcess() const { return m_fSeparateProcess; }

#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns #MacOSXRelease determined using <i>uname</i> call. */
    static MacOSXRelease determineOsRelease();
    /** Mac OS X: Returns #MacOSXRelease determined during VBoxGlobal prepare routine. */
    MacOSXRelease osRelease() const { return m_osRelease; }
#endif /* VBOX_WS_MAC */

    /** Try to acquire COM cleanup protection token for reading. */
    bool comTokenTryLockForRead() { return m_comCleanupProtectionToken.tryLockForRead(); }
    /** Unlock previously acquired COM cleanup protection token. */
    void comTokenUnlock() { return m_comCleanupProtectionToken.unlock(); }

    /** Returns the copy of VirtualBox client wrapper. */
    CVirtualBoxClient virtualBoxClient() const { return m_client; }
    /** Returns the copy of VirtualBox object wrapper. */
    CVirtualBox virtualBox() const { return m_vbox; }
    /** Returns the copy of VirtualBox host-object wrapper. */
    CHost host() const { return m_host; }
    /** Returns the symbolic VirtualBox home-folder representation. */
    QString homeFolder() const { return m_strHomeFolder; }

    /** Returns the VBoxSVC availability value. */
    bool isVBoxSVCAvailable() const { return m_fVBoxSVCAvailable; }

    /** Returns the thread-pool instance. */
    UIThreadPool* threadPool() const { return m_pThreadPool; }

    /** Returns currently active virtual machine window. */
    QWidget* activeMachineWindow() const;

    bool is3DAvailableWorker() const;
    bool is3DAvailable() const { if (m3DAvailable < 0) return is3DAvailableWorker(); return m3DAvailable != 0; }

#ifdef VBOX_GUI_WITH_PIDFILE
    void createPidfile();
    void deletePidfile();
#endif

    /* branding */
    bool brandingIsActive (bool aForce = false);
    QString brandingGetKey (QString aKey);

    bool processArgs();

    bool switchToMachine(CMachine &machine);

    bool launchMachine(CMachine &machine, LaunchMode enmLaunchMode = LaunchMode_Default);

    bool isVMConsoleProcess() const { return !vmUuid.isNull(); }
    bool showStartVMErrors() const { return mShowStartVMErrors; }
    QString managedVMUuid() const { return vmUuid; }
    QList<QUrl> &argUrlList() { return m_ArgUrlList; }

    /** Returns whether the Extension Pack installation was requested at startup. */
    bool isEPInstallationRequested() const { return m_fEPInstallationRequested; }
    /** Defines whether the Extension Pack installation was @a fRequested at startup. */
    void setEPInstallationRequested(bool fRequested) { m_fEPInstallationRequested = fRequested; }

#ifdef VBOX_WS_X11
    /** X11: Returns whether the Window Manager we are running at is composition one. */
    bool isCompositingManagerRunning() const { return m_fCompositingManagerRunning; }
    /** X11: Returns the type of the Window Manager we are running under. */
    X11WMType typeOfWindowManager() const { return m_enmWindowManagerType; }
#endif /* VBOX_WS_X11 */

    /** Returns whether we should restore current snapshot before VM started. */
    bool shouldRestoreCurrentSnapshot() const { return mRestoreCurrentSnapshot; }
    /** Defines whether we should fRestore current snapshot before VM started. */
    void setShouldRestoreCurrentSnapshot(bool fRestore) { mRestoreCurrentSnapshot = fRestore; }

    bool hasFloppyImageToMount() const    { return !m_strFloppyImage.isEmpty(); }
    bool hasDvdImageToMount() const       { return !m_strDvdImage.isEmpty(); }
    QString const &getFloppyImage() const { return m_strFloppyImage; }
    QString const &getDvdImage() const    { return m_strDvdImage; }

    bool isPatmDisabled() const { return mDisablePatm; }
    bool isCsamDisabled() const { return mDisableCsam; }
    bool isSupervisorCodeExecedRecompiled() const { return mRecompileSupervisor; }
    bool isUserCodeExecedRecompiled()       const { return mRecompileUser; }
    bool areWeToExecuteAllInIem()           const { return mExecuteAllInIem; }
    bool isDefaultWarpPct() const { return mWarpPct == 100; }
    uint32_t getWarpPct()       const { return mWarpPct; }

#ifdef VBOX_WITH_DEBUGGER_GUI
    bool isDebuggerEnabled() const;
    bool isDebuggerAutoShowEnabled() const;
    bool isDebuggerAutoShowCommandLineEnabled() const;
    bool isDebuggerAutoShowStatisticsEnabled() const;

    RTLDRMOD getDebuggerModule() const { return m_hVBoxDbg; }
#endif /* VBOX_WITH_DEBUGGER_GUI */

    bool shouldStartPaused() const
    {
#ifdef VBOX_WITH_DEBUGGER_GUI
        return m_enmStartRunning == StartRunning_Default ? isDebuggerAutoShowEnabled() : m_enmStartRunning == StartRunning_No;
#else
        return false;
#endif
    }

    /* VBox enum to/from string/icon/color convertors */

    QList <CGuestOSType> vmGuestOSFamilyList() const;
    QList <CGuestOSType> vmGuestOSTypeList (const QString &aFamilyId) const;

    /** Returns icon defined for a passed @a comMachine. */
    QIcon vmUserIcon(const CMachine &comMachine) const;
    /** Returns pixmap of a passed @a size defined for a passed @a comMachine. */
    QPixmap vmUserPixmap(const CMachine &comMachine, const QSize &size) const;
    /** Returns pixmap defined for a passed @a comMachine.
      * In case if non-null @a pLogicalSize pointer provided, it will be updated properly. */
    QPixmap vmUserPixmapDefault(const CMachine &comMachine, QSize *pLogicalSize = 0) const;

    /** Returns pixmap corresponding to passed @a strOSTypeID. */
    QIcon vmGuestOSTypeIcon(const QString &strOSTypeID) const;
    /** Returns pixmap corresponding to passed @a strOSTypeID and @a size. */
    QPixmap vmGuestOSTypePixmap(const QString &strOSTypeID, const QSize &size) const;
    /** Returns pixmap corresponding to passed @a strOSTypeID.
      * In case if non-null @a pLogicalSize pointer provided, it will be updated properly. */
    QPixmap vmGuestOSTypePixmapDefault(const QString &strOSTypeID, QSize *pLogicalSize = 0) const;

    CGuestOSType vmGuestOSType (const QString &aTypeId,
                                const QString &aFamilyId = QString::null) const;
    QString vmGuestOSTypeDescription (const QString &aTypeId) const;

    static inline QString yearsToString (uint32_t cVal)
    {
        return tr("%n year(s)", "", cVal);
    }

    static inline QString monthsToString (uint32_t cVal)
    {
        return tr("%n month(s)", "", cVal);
    }

    static inline QString daysToString (uint32_t cVal)
    {
        return tr("%n day(s)", "", cVal);
    }

    static inline QString hoursToString (uint32_t cVal)
    {
        return tr("%n hour(s)", "", cVal);
    }

    static inline QString minutesToString (uint32_t cVal)
    {
        return tr("%n minute(s)", "", cVal);
    }

    static inline QString secondsToString (uint32_t cVal)
    {
        return tr("%n second(s)", "", cVal);
    }

    QStringList COMPortNames() const;
    QString toCOMPortName (ulong aIRQ, ulong aIOBase) const;
    bool toCOMPortNumbers (const QString &aName, ulong &aIRQ, ulong &aIOBase) const;

    QStringList LPTPortNames() const;
    QString toLPTPortName (ulong aIRQ, ulong aIOBase) const;
    bool toLPTPortNumbers (const QString &aName, ulong &aIRQ, ulong &aIOBase) const;

    static bool hasAllowedExtension(const QString &strExt, const QStringList &extList)
    {
        for (int i = 0; i < extList.size(); ++i)
            if (strExt.endsWith(extList.at(i), Qt::CaseInsensitive))
                return true;
        return false;
    }

    QIcon icon(QFileIconProvider::IconType type) { return m_globalIconProvider.icon(type); }
    QIcon icon(const QFileInfo &info) { return m_globalIconProvider.icon(info); }

    QPixmap warningIcon() const { return mWarningIcon; }
    QPixmap errorIcon() const { return mErrorIcon; }

    /* details generators */

    QString details(const CMedium &medium, bool fPredictDiff, bool fUseHtml = true);
        static void unsetCursor(QWidget *pWidget);
        static void unsetCursor(QGraphicsWidget *pWidget);

    QString details (const CUSBDevice &aDevice) const;
    QString toolTip (const CUSBDevice &aDevice) const;
    QString toolTip (const CUSBDeviceFilter &aFilter) const;
    QString toolTip(const CHostVideoInputDevice &webcam) const;

    /* VirtualBox helpers */

    CSession openSession(const QString &aId, KLockType aLockType = KLockType_Write);

    /** Shortcut to openSession (aId, true). */
    CSession openExistingSession(const QString &aId) { return openSession(aId, KLockType_Shared); }

    /* API: Medium-processing stuff: */
    void createMedium(const UIMedium &medium);
    void deleteMedium(const QString &strMediumID);
    QString openMediumWithFileOpenDialog(UIMediumType mediumType, QWidget *pParent = 0,
                                         const QString &strDefaultFolder = QString(), bool fUseLastFolder = true);
    QString openMedium(UIMediumType mediumType, QString strMediumLocation, QWidget *pParent = 0);

    /* API: Medium-enumeration stuff: */
    void startMediumEnumeration();
    bool agressiveCaching() const { return mAgressiveCaching; }
    bool isMediumEnumerationInProgress() const;
    UIMedium medium(const QString &strMediumID) const;
    QList<QString> mediumIDs() const;

    /** Prepares storage menu according passed parameters.
      * @param menu              QMenu being prepared.
      * @param pListener         Listener QObject, this menu being prepared for.
      * @param pszSlotName       SLOT in the @a pListener above, this menu will be handled with.
      * @param machine           CMachine object, this menu being prepared for.
      * @param strControllerName The name of the CStorageController in the @a machine above.
      * @param storageSlot       The StorageSlot of the CStorageController called @a strControllerName above. */
    void prepareStorageMenu(QMenu &menu,
                            QObject *pListener, const char *pszSlotName,
                            const CMachine &machine, const QString &strControllerName, const StorageSlot &storageSlot);
    /** Updates @a constMachine storage with data described by @a target. */
    void updateMachineStorage(const CMachine &constMachine, const UIMediumTarget &target);

    /* various helpers */

    QString languageName() const;
    QString languageCountry() const;
    QString languageNameEnglish() const;
    QString languageCountryEnglish() const;
    QString languageTranslators() const;

    void retranslateUi();

    /* public static stuff */

    static bool isDOSType (const QString &aOSTypeId);

    static QString languageId();
    static void loadLanguage (const QString &aLangId = QString::null);
    QString helpFile() const;

    static void setTextLabel (QToolButton *aToolButton, const QString &aTextLabel);

    static QRect normalizeGeometry (const QRect &aRectangle, const QRegion &aBoundRegion,
                                    bool aCanResize = true);
    static QRect getNormalized (const QRect &aRectangle, const QRegion &aBoundRegion,
                                bool aCanResize = true);
    static QRegion flip (const QRegion &aRegion);

    static void centerWidget (QWidget *aWidget, QWidget *aRelative,
                              bool aCanResize = true);

    static QChar decimalSep();
    static QString sizeRegexp();

    static quint64 parseSize (const QString &);
    static QString formatSize (quint64 aSize, uint aDecimal = 2, FormatSize aMode = FormatSize_Round);

    static quint64 requiredVideoMemory(const QString &strGuestOSTypeId, int cMonitors = 1);

    static QString locationForHTML (const QString &aFileName);

    static QString highlight (const QString &aStr, bool aToolTip = false);

    static QString replaceHtmlEntities(QString strText);
    static QString emphasize (const QString &aStr);

    static QString systemLanguageId();

    static bool activateWindow (WId aWId, bool aSwitchDesktop = true);
    /** Does some checks on certain platforms before calling QWidget::setCursor(...). */
    static void setCursor(QWidget *pWidget, const QCursor &cursor);
    static void setCursor(QGraphicsWidget *pWidget, const QCursor &cursor);

#ifdef VBOX_WS_X11
    /** X11: Test whether the current window manager supports full screen mode. */
    static bool supportsFullScreenMonitorsProtocolX11();
    /** X11: Performs mapping of the passed @a pWidget to host-screen with passed @a uScreenId. */
    static bool setFullScreenMonitorX11(QWidget *pWidget, ulong uScreenId);

    /** X11: Returns a list of current _NET_WM_STATE flags for passed @a pWidget. */
    static QVector<Atom> flagsNetWmState(QWidget *pWidget);
    /** X11: Check whether _NET_WM_STATE_FULLSCREEN flag is set for passed @a pWidget. */
    static bool isFullScreenFlagSet(QWidget *pWidget);
    /** X11: Sets _NET_WM_STATE_FULLSCREEN flag for passed @a pWidget. */
    static void setFullScreenFlag(QWidget *pWidget);
    /** X11: Sets _NET_WM_STATE_SKIP_TASKBAR flag for passed @a pWidget. */
    static void setSkipTaskBarFlag(QWidget *pWidget);
    /** X11: Sets _NET_WM_STATE_SKIP_PAGER flag for passed @a pWidget. */
    static void setSkipPagerFlag(QWidget *pWidget);
#endif /* VBOX_WS_X11 */

    static QString removeAccelMark (const QString &aText);

    static QString insertKeyToActionText (const QString &aText, const QString &aKey);

    static QPixmap joinPixmaps (const QPixmap &aPM1, const QPixmap &aPM2);

    static QWidget *findWidget (QWidget *aParent, const char *aName,
                                const char *aClassName = NULL,
                                bool aRecursive = false);

    static QList <QPair <QString, QString> > MediumBackends(KDeviceType enmDeviceType);
    static QList <QPair <QString, QString> > HDDBackends();
    static QList <QPair <QString, QString> > DVDBackends();
    static QList <QPair <QString, QString> > FloppyBackends();

    static QString documentsPath();

#ifdef VBOX_WITH_VIDEOHWACCEL
    static bool isAcceleration2DVideoAvailable();

    /** additional video memory required for the best 2D support performance
     *  total amount of VRAM required is thus calculated as requiredVideoMemory + required2DOffscreenVideoMemory  */
    static quint64 required2DOffscreenVideoMemory();
#endif

#ifdef VBOX_WITH_CRHGSMI
    static bool isWddmCompatibleOsType(const QString &strGuestOSTypeId);
#endif /* VBOX_WITH_CRHGSMI */

    /* Returns full medium-format name for the given base medium-format name: */
    static QString fullMediumFormatName(const QString &strBaseMediumFormatName);


#ifdef RT_OS_LINUX
    static void checkForWrongUSBMounted();
#endif /* RT_OS_LINUX */

    /* Shame on Qt it hasn't stuff for tuning
     * widget size suitable for reflecting content of desired size.
     * For example QLineEdit, QSpinBox and similar widgets should have a methods
     * to strict the minimum width to reflect at least [n] symbols. */
    static void setMinimumWidthAccordingSymbolCount(QSpinBox *pSpinBox, int cCount);

    /** Assigns top-level @a pWidget geometry passed as QRect coordinates.
      * @note  Take into account that this request may fail on X11. */
    static void setTopLevelGeometry(QWidget *pWidget, int x, int y, int w, int h);
    /** Assigns top-level @a pWidget geometry passed as @a rect.
      * @note  Take into account that this request may fail on X11. */
    static void setTopLevelGeometry(QWidget *pWidget, const QRect &rect);

#ifdef VBOX_WS_X11
    /** Assigns WM_CLASS property for passed @a pWidget. */
    static void setWMClass(QWidget *pWidget, const QString &strNameString, const QString &strClassString);
#endif

signals:

    /** Notifies listeners about the VBoxSVC availability change. */
    void sigVBoxSVCAvailabilityChange();

    /* Notifiers: Medium-processing stuff: */
    void sigMediumCreated(const QString &strMediumID);
    void sigMediumDeleted(const QString &strMediumID);

    /* Notifiers: Medium-enumeration stuff: */
    void sigMediumEnumerationStarted();
    void sigMediumEnumerated(const QString &strMediumID);
    void sigMediumEnumerationFinished();

public slots:

    bool openURL (const QString &aURL);

    void sltGUILanguageChange(QString strLang);

protected slots:

    /* Handlers: Prepare/cleanup stuff: */
    void prepare();
    void cleanup();

    /** Handles @a manager request for emergency session shutdown. */
    void sltHandleCommitDataRequest(QSessionManager &manager);

    /** Shows UI. */
    void showUI();

    /** Handles the VBoxSVC availability change. */
    void sltHandleVBoxSVCAvailabilityChange(bool fAvailable);

protected:

    bool eventFilter (QObject *, QEvent *);

private:

    /* Constructor/destructor: */
    VBoxGlobal();
    ~VBoxGlobal();

    /** Re-initializes COM wrappers and containers. */
    void comWrappersReinit();

#ifdef VBOX_WS_WIN
    /** Wraps WinAPI ShutdownBlockReasonCreate function.
      * @remark  This function defined starting from Vista only. */
    static BOOL ShutdownBlockReasonCreateAPI(HWND hWnd, LPCWSTR pwszReason);
#endif

#ifdef VBOX_WITH_DEBUGGER_GUI
    void initDebuggerVar(int *piDbgCfgVar, const char *pszEnvVar, const char *pszExtraDataName, bool fDefault = false);
    void setDebuggerVar(int *piDbgCfgVar, bool fState);
    bool isDebuggerWorker(int *piDbgCfgVar, const char *pszExtraDataName) const;
#endif

    bool mValid;

#ifdef VBOX_WS_MAC
    /** Mac OS X: Holds the #MacOSXRelease determined using <i>uname</i> call. */
    MacOSXRelease m_osRelease;
#endif /* VBOX_WS_MAC */

    /** COM cleanup protection token. */
    QReadWriteLock m_comCleanupProtectionToken;

    /** Holds the instance of VirtualBox client wrapper. */
    CVirtualBoxClient m_client;
    /** Holds the copy of VirtualBox object wrapper. */
    CVirtualBox m_vbox;
    /** Holds the copy of VirtualBox host-object wrapper. */
    CHost m_host;
    /** Holds the symbolic VirtualBox home-folder representation. */
    QString m_strHomeFolder;
    /** Holds the guest OS family IDs. */
    QList<QString> m_guestOSFamilyIDs;
    /** Holds the guest OS types for each family ID. */
    QList<QList<CGuestOSType> > m_guestOSTypes;

    /** Holds whether acquired COM wrappers are currently valid. */
    bool m_fWrappersValid;
    /** Holds whether VBoxSVC is currently available. */
    bool m_fVBoxSVCAvailable;

    /** Holds whether GUI is separate (from VM) process. */
    bool m_fSeparateProcess;

    QString vmUuid;
    QList<QUrl> m_ArgUrlList;

    /** Whether to show error message boxes for VM start errors. */
    bool mShowStartVMErrors;

    /* Variable: Medium-enumeration stuff: */
    UIMediumEnumerator *m_pMediumEnumerator;
    mutable QReadWriteLock m_mediumEnumeratorDtorRwLock;

    /** Holds whether the Extension Pack installation was requested at startup. */
    bool m_fEPInstallationRequested;

#ifdef VBOX_WS_X11
    /** X11: Holds whether the Window Manager we are running at is composition one. */
    bool m_fCompositingManagerRunning;
    /** X11: Holds the type of the Window Manager we are running under. */
    X11WMType m_enmWindowManagerType;
#endif /* VBOX_WS_X11 */

    /** The --aggressive-caching / --no-aggressive-caching option. */
    bool mAgressiveCaching;
    /** The --restore-current option. */
    bool mRestoreCurrentSnapshot;

    /** @name Ad-hoc VM reconfiguration.
     * @{ */
        /** Floppy image. */
        QString m_strFloppyImage;
        /** DVD image. */
        QString m_strDvdImage;
    /** @} */

    /** @name VMM options
     * @{ */
        /** The --disable-patm option. */
        bool mDisablePatm;
        /** The --disable-csam option. */
        bool mDisableCsam;
        /** The --recompile-supervisor option. */
        bool mRecompileSupervisor;
        /** The --recompile-user option. */
        bool mRecompileUser;
        /** The --execute-all-in-iem option. */
        bool mExecuteAllInIem;
        /** The --warp-factor option value. */
        uint32_t mWarpPct;
    /** @} */

#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Whether the debugger should be accessible or not.
     * Use --dbg, the env.var. VBOX_GUI_DBG_ENABLED,
     *     --debug or the env.var. VBOX_GUI_DBG_AUTO_SHOW to enable. */
    mutable int m_fDbgEnabled;
    /** Whether to show the debugger automatically with the console.
     * Use --debug or the env.var. VBOX_GUI_DBG_AUTO_SHOW to enable. */
    mutable int m_fDbgAutoShow;
    /** Whether to show the command line window when m_fDbgAutoShow is set. */
    mutable int m_fDbgAutoShowCommandLine;
    /** Whether to show the statistics window when m_fDbgAutoShow is set. */
    mutable int m_fDbgAutoShowStatistics;
    /** VBoxDbg module handle. */
    RTLDRMOD m_hVBoxDbg;

    /** Whether --start-running, --start-paused or nothing was given. */
    enum StartRunning m_enmStartRunning;
#endif

#if defined (VBOX_WS_WIN)
    DWORD dwHTMLHelpCookie;
#endif

    QString mVerString;
    QString mBrandingConfig;

    int m3DAvailable;

    QString mUserDefinedPortName;

    QPixmap mWarningIcon, mErrorIcon;

    QFileIconProvider m_globalIconProvider;

#ifdef VBOX_GUI_WITH_PIDFILE
    QString m_strPidfile;
#endif

    char mSettingsPw[256];
    bool mSettingsPwSet;

    /** General icon-pool. */
    UIIconPoolGeneral *m_pIconPool;
    /** Holds the thread-pool instance. */
    UIThreadPool *m_pThreadPool;

    /* API: Instance stuff: */
    static bool m_sfCleanupInProgress;
    static VBoxGlobal* m_spInstance;
    friend VBoxGlobal& vboxGlobal();
};

/* Shortcut to the static VBoxGlobal::instance() method: */
inline VBoxGlobal& vboxGlobal() { return *VBoxGlobal::instance(); }

#endif /* !___VBoxGlobal_h___ */
