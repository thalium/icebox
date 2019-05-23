/* $Id: precomp.h $*/
/** @file
 * VBox Qt GUI - Header used if VBOX_WITH_PRECOMPILED_HEADERS is active.
 *
 * This is the remnants of an obsoleted experiment!
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

/* To get usage counts for the include files run the following from the src directory:
  ( for inc in $(sed -e '/^# *include/!d' -e 's/^# *include *.//' -e 's/[">].*''//' precomp.h);
    do
        echo $( grep -rE "^ *# *include *.$inc." --exclude=precomp.h . | wc -l ) \
            "/" $( grep -rE "^ *# *include *.$inc." --exclude="*.h" . | wc -l ) \
            "- $inc";
    done ) | sort -n
 */


#define LOG_GROUP LOG_GROUP_GUI

/*
 * Qt
 */
#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractTableModel>
#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
//#include <QCleanlooksStyle> - only used once
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
//#include <QCommonStyle> -  only used once
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
//#include <QDesktopWidget> -  only used once
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDrag>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFocusEvent>
//#include <QFontDatabase> - only used once
//#include <QFontMetrics> - only used once
#include <QFrame>
//#include <QGLContext> - only used once
#include <QGLWidget>
//#include <QGraphicsLinearLayout> - only used once
//#include <QGraphicsProxyWidget> - only used once
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
//#include <QGraphicsSceneHoverEvent> - only used once
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QHelpEvent>
#include <QIcon>
#include <QImage>
//#include <QImageWriter> - only used once
#include <QIntValidator>
#include <QItemDelegate>
#include <QItemEditorFactory>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLibrary>
//#include <QLibraryInfo> - only used once
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QListWidget>
#include <QLocale>
#ifdef VBOX_WS_MAC
//# include <QMacCocoaViewContainer> - only used once / only used in Objective C++
#endif
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaEnum>
//#include <QMetaProperty> - only used once
#include <QMetaType>
#include <QMimeData>
#include <QMouseEvent>
//#include <QMouseEventTransition> - only used once
#include <QMutex>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPair>
//#include <QParallelAnimationGroup> - only used once
#include <QPixmap>
#include <QPixmapCache>
//#include <QPlastiqueStyle> - only used once
#include <QPoint>
#include <QPointer>
//#include <QPrintDialog> - only used once
//#include <QPrinter> - only used once
#include <QProcess>
#include <QProgressBar>
//#include <QProgressDialog> - only used once
#include <QPropertyAnimation>
#include <QPushButton>
#include <QQueue>
#include <QRadioButton>
//#include <QReadLocker> - only used once
#include <QReadWriteLock>
#include <QRect>
#include <QRegExp>
#include <QRegExpValidator>
#include <QRegion>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
//#include <QSettings> - only used once
//#include <QShortcut> - only used once
#include <QSignalMapper>
#include <QSignalTransition>
//#include <QSizeGrip> - only used once
#include <QSlider>
//#include <QSocketNotifier> - only used once
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QSpinBox>
#include <QSplitter>
//#include <QStackedLayout> - only used once.
#include <QStackedWidget>
//#include <QStandardItemModel> - only used once
#include <QStateMachine>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleOption>
#include <QStyleOptionFocusRect>
//#include <QStyleOptionFrame> - only used once
#include <QStyleOptionGraphicsItem>
//#include <QStyleOptionSlider> - only used once
//#include <QStyleOptionSpinBox> - only used once
#include <QStylePainter>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>
#include <QTextEdit>
//#include <QTextLayout> - only used once
#include <QTextStream>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QToolTip>
//#include <QTouchEvent> - only used once
#include <QTransform>
#include <QTranslator>
#include <QTreeView>
#include <QTreeWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QValidator>
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>
#include <QWaitCondition>
#include <QWidget>
//#include <QWindowsStyle> - only used twice
//#include <QWindowsVistaStyle> - only used once
#include <QWizard>
#include <QWizardPage>
#ifdef VBOX_WS_X11
# include <QX11Info>
#endif
//#include <QXmlStreamReader> - only used once
//#include <QXmlStreamWriter> - only used once


/*
 * System specific headers.
 */
#ifdef VBOX_WS_WIN
# include <iprt/win/shlobj.h>
# include <iprt/win/windows.h>
#endif


/*
 * IPRT
 */
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/buildconfig.h>
#include <iprt/cdefs.h>
#include <iprt/cidr.h>
#include <iprt/cpp/utils.h>
#include <iprt/critsect.h>
//#include <iprt/ctype.h> - only used once
#include <iprt/env.h>
//#include <iprt/err.h> - don't include err.h!
//#include <iprt/file.h> - only used once
//#include <iprt/http.h> - only used once
#include <iprt/initterm.h>
#include <iprt/ldr.h>
#include <iprt/list.h>
#include <iprt/log.h>
#include <iprt/mem.h>
//#include <iprt/memcache.h> - only used once
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/semaphore.h>
//#include <iprt/sha.h> - only used once
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/system.h>
#include <iprt/thread.h>
#include <iprt/time.h>
#include <iprt/types.h>
//#include <iprt/uri.h> - only used once
//#include <iprt/zip.h> - only used once


/*
 * VirtualBox COM API
 */
#ifdef VBOX_WITH_XPCOM
# include <VirtualBox_XPCOM.h>
#else
# include <VirtualBox.h>
#endif

/*
 * VBox headers.
 */
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/VBoxCocoa.h>
#include <VBox/VBoxGL2D.h>
//#include <VBox/VBoxKeyboard.h> - includes X11/X.h which causes trouble.
//#include <VBox/VBoxOGL.h> - only used once
//#include <VBoxVideo.h> - only used twice
#ifdef VBOX_WITH_VIDEOHWACCEL
//# include <VBoxVideo3D.h> - only used once
#endif
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/assert.h>
#include <VBox/com/com.h>
#include <VBox/com/listeners.h>
#include <VBox/dbggui.h>
//#include <VBox/err.h> - don't include err.h!
#include <VBox/log.h>
#include <VBox/sup.h>
//#include <VBox/vd.h> - only used once
#include <VBox/version.h>
#ifdef VBOX_WITH_VIDEOHWACCEL
//# include <VBox/vmm/ssm.h> - only used once
#endif


/*
 * VirtualBox Qt GUI - QI* headers.
 */
#include "QIAdvancedSlider.h"
#include "QIArrowButtonPress.h"
#include "QIArrowButtonSwitch.h"
#include "QIArrowSplitter.h"
#include "QIDialog.h"
#include "QIDialogButtonBox.h"
#include "QIFileDialog.h"
#include "QIGraphicsWidget.h"
#include "QILabel.h"
#include "QILabelSeparator.h"
#include "QILineEdit.h"
#include "QIMainDialog.h"
#include "QIMenu.h"
#include "QIMessageBox.h"
#include "QIProcess.h"
#include "QIRichTextLabel.h"
#include "QIRichToolButton.h"
#include "QISplitter.h"
#include "QIStatusBar.h"
#include "QIStatusBarIndicator.h"
#include "QITabWidget.h"
#include "QITableView.h"
#include "QIToolButton.h"
#include "QITreeView.h"
#include "QITreeWidget.h"
#include "QIWidgetValidator.h"
#include "QIWithRetranslateUI.h"


/*
 * VirtualBox Qt GUI - COM headers & Wrappers (later).
 */
/* Note! X.h and Xlib.h shall not be included as they may redefine None and
         Status respectively, both are user in VBox enums.  Don't bother
         undefining them, just prevent their inclusion! */
#include "COMDefs.h"
#include "COMEnums.h"

#include "CAppliance.h"
#include "CAudioAdapter.h"
#include "CBIOSSettings.h"
//#include "CCanShowWindowEvent.h" - only used once
#include "CConsole.h"
#include "CDHCPServer.h"
#include "CDisplay.h"
#include "CDisplaySourceBitmap.h"
#include "CDnDSource.h"
#include "CDnDTarget.h"
#include "CEmulatedUSB.h"
//#include "CEvent.h" - only used once
#include "CEventListener.h"
#include "CEventSource.h"
#include "CExtPack.h"
#include "CExtPackFile.h"
#include "CExtPackManager.h"
//#include "CExtraDataCanChangeEvent.h" - only used once
//#include "CExtraDataChangedEvent.h" - only used once
//#include "CFramebuffer.h" - only used once
#include "CGuest.h"
//#include "CGuestDnDSource.h" - only used once
//#include "CGuestDnDTarget.h" - only used once
//#include "CGuestMonitorChangedEvent.h" - only used once
#include "CGuestOSType.h"
#include "CHost.h"
#include "CHostNetworkInterface.h"
#include "CHostUSBDevice.h"
//#include "CHostUSBDeviceFilter.h" - only used once
#include "CHostVideoInputDevice.h"
#include "CIShared.h"
#include "CKeyboard.h"
//#include "CKeyboardLedsChangedEvent.h" - only used once
#include "CMachine.h"
//#include "CMachineDataChangedEvent.h" - only used once
#include "CMachineDebugger.h"
//#include "CMachineRegisteredEvent.h" - only used once
//#include "CMachineStateChangedEvent.h" - only used once
#include "CMedium.h"
#include "CMediumAttachment.h"
//#include "CMediumChangedEvent.h" - only used once
#include "CMediumFormat.h"
//#include "CMouse.h" - only used once
//#include "CMouseCapabilityChangedEvent.h" - only used once
//#include "CMousePointerShapeChangedEvent.h" - only used once
//#include "CNATEngine.h" - only used once
#include "CNATNetwork.h"
#include "CNetworkAdapter.h"
//#include "CNetworkAdapterChangedEvent.h" - only used once
#include "CProgress.h"
//#include "CRuntimeErrorEvent.h" - only used once
#include "CSerialPort.h"
#include "CSession.h"
//#include "CSessionStateChangedEvent.h" - only used once
#include "CSharedFolder.h"
//#include "CShowWindowEvent.h" - only used once
#include "CSnapshot.h"
//#include "CSnapshotChangedEvent.h" - only used once
//#include "CSnapshotDeletedEvent.h" - only used once
//#include "CSnapshotTakenEvent.h" - only used once
//#include "CStateChangedEvent.h" - only used once
#include "CStorageController.h"
#include "CSystemProperties.h"
#include "CUSBController.h"
#include "CUSBDevice.h"
#include "CUSBDeviceFilter.h"
#include "CUSBDeviceFilters.h"
//#include "CUSBDeviceStateChangedEvent.h" - only used once
//#include "CVFSExplorer.h" - only used once
#include "CVRDEServer.h"
//#include "CVRDEServerInfo.h" - only used once
#include "CVirtualBox.h"
#include "CVirtualBoxErrorInfo.h"
#include "CVirtualSystemDescription.h"


/*
 * VirtualBox Qt GUI - UI headers.
 */
#include "UIMessageCenter.h"
#include "UINetworkReply.h"
#ifdef RT_OS_DARWIN
# include "UIAbstractDockIconPreview.h"
#endif
#include "UIActionPool.h"
#include "UIActionPoolRuntime.h"
#include "UIActionPoolSelector.h"
#include "UIAnimationFramework.h"
#include "UIApplianceEditorWidget.h"
#include "UIApplianceExportEditorWidget.h"
#include "UIApplianceImportEditorWidget.h"
#include "UIBootTable.h"
#ifdef RT_OS_DARWIN
# include "UICocoaApplication.h"
# include "UICocoaDockIconPreview.h"
# include "UICocoaSpecialControls.h"
#endif
#include "UIConsoleEventHandler.h"
#include "UIConverter.h"
#include "UIConverterBackend.h"
#include "UIDefs.h"
#include "UIDesktopServices.h"
#ifdef RT_OS_DARWIN
# include "UIDesktopServices_darwin_p.h"
#endif
#include "UIDnDDrag.h"
#ifdef RT_OS_WINDOWS
# include "UIDnDDataObject_win.h"
# include "UIDnDDropSource_win.h"
# include "UIDnDEnumFormat_win.h"
#endif
#include "UIDnDHandler.h"
#include "UIDnDMIMEData.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UIDownloader.h"
# include "UIDownloaderAdditions.h"
# include "UIDownloaderExtensionPack.h"
# include "UIDownloaderUserManual.h"
#endif
#include "UIExtraDataDefs.h"
#include "UIExtraDataManager.h"
#include "UIFilmContainer.h"
#include "UIFrameBuffer.h"
#include "UIGChooser.h"
#include "UIGChooserHandlerKeyboard.h"
#include "UIGChooserHandlerMouse.h"
#include "UIGChooserItem.h"
#include "UIGChooserItemGroup.h"
#include "UIGChooserItemMachine.h"
#include "UIGChooserModel.h"
#include "UIGChooserView.h"
#include "UIGDetails.h"
#include "UIGDetailsElement.h"
#include "UIGDetailsElements.h"
#include "UIGDetailsGroup.h"
#include "UIGDetailsItem.h"
#include "UIGDetailsModel.h"
#include "UIGDetailsSet.h"
#include "UIGDetailsView.h"
#include "UIGMachinePreview.h"
#include "UIGlobalSettingsDisplay.h"
#include "UIGlobalSettingsExtension.h"
#include "UIGlobalSettingsGeneral.h"
#include "UIGlobalSettingsInput.h"
#include "UIGlobalSettingsLanguage.h"
#include "UIGlobalSettingsNetwork.h"
#include "UIGlobalSettingsNetworkDetailsNAT.h"
#include "UIGlobalSettingsPortForwardingDlg.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UIGlobalSettingsProxy.h"
# include "UIGlobalSettingsUpdate.h"
#endif
#include "UIGraphicsButton.h"
#include "UIGraphicsRotatorButton.h"
#include "UIGraphicsTextPane.h"
#include "UIGraphicsToolBar.h"
#include "UIGraphicsZoomButton.h"
#include "UIHostComboEditor.h"
#include "UIHotKeyEditor.h"
#include "UIIconPool.h"
#include "UIImageTools.h"
#include "UIIndicatorsPool.h"
#include "UIKeyboardHandler.h"
#include "UIKeyboardHandlerFullscreen.h"
#include "UIKeyboardHandlerNormal.h"
#include "UIKeyboardHandlerScale.h"
#include "UIKeyboardHandlerSeamless.h"
#include "UILineTextEdit.h"
#include "UIMachine.h"
#include "UIMachineDefs.h"
#include "UIMachineLogic.h"
#include "UIMachineLogicFullscreen.h"
#include "UIMachineLogicNormal.h"
#include "UIMachineLogicScale.h"
#include "UIMachineLogicSeamless.h"
#include "UIMachineSettingsAudio.h"
#include "UIMachineSettingsDisplay.h"
#include "UIMachineSettingsGeneral.h"
#include "UIMachineSettingsInterface.h"
#include "UIMachineSettingsNetwork.h"
#include "UIMachineSettingsPortForwardingDlg.h"
#include "UIMachineSettingsSF.h"
#include "UIMachineSettingsSFDetails.h"
#include "UIMachineSettingsSerial.h"
#include "UIMachineSettingsStorage.h"
#include "UIMachineSettingsSystem.h"
#include "UIMachineSettingsUSB.h"
#include "UIMachineSettingsUSBFilterDetails.h"
#include "UIMachineView.h"
#include "UIMachineViewFullscreen.h"
#include "UIMachineViewNormal.h"
#include "UIMachineViewScale.h"
#include "UIMachineViewSeamless.h"
#include "UIMachineWindow.h"
#include "UIMachineWindowFullscreen.h"
#include "UIMachineWindowNormal.h"
#include "UIMachineWindowScale.h"
#include "UIMachineWindowSeamless.h"
#include "UIMainEventListener.h"
#include "UIMedium.h"
#include "UIMediumDefs.h"
#include "UIMediumEnumerator.h"
#include "UIMediumManager.h"
#include "UIMenuBar.h"
#include "UIMenuBarEditorWindow.h"
#include "UIMessageCenter.h"
#include "UIMiniToolBar.h"
#include "UIModalWindowManager.h"
#include "UIMouseHandler.h"
#include "UIMultiScreenLayout.h"
#include "UINameAndSystemEditor.h"
#include "UINetworkCustomer.h"
#include "UINetworkDefs.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UINetworkManager.h"
# include "UINetworkManagerDialog.h"
# include "UINetworkManagerIndicator.h"
#endif
#include "UINetworkReply.h"
#include "UINetworkRequest.h"
#include "UINetworkRequestWidget.h"
#include "UIPopupBox.h"
#include "UIPopupCenter.h"
#include "UIPopupPane.h"
#include "UIPopupPaneButtonPane.h"
#include "UIPopupPaneTextPane.h"
#include "UIPopupStack.h"
#include "UIPopupStackViewport.h"
#include "UIPortForwardingTable.h"
#include "UIProgressDialog.h"
#include "UISelectorWindow.h"
#include "UISession.h"
#include "UISettingsDefs.h"
#include "UISettingsDialog.h"
#include "UISettingsDialogSpecific.h"
#include "UISettingsPage.h"
#include "UIShortcutPool.h"
#include "UISlidingToolBar.h"
#include "UISpecialControls.h"
#include "UIStatusBarEditorWindow.h"
#include "UIThreadPool.h"
#include "UIToolBar.h"
#include "UIUpdateDefs.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UIUpdateManager.h"
#endif
#include "UIVMCloseDialog.h"
#include "UIDesktopPane.h"
#include "UIVMItem.h"
#include "UIVMLogViewer.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIWarningPane.h"
#include "UIWindowMenuManager.h"
#include "UIWizard.h"
#include "UIWizardCloneVD.h"
#include "UIWizardCloneVDPageBasic1.h"
#include "UIWizardCloneVDPageBasic2.h"
#include "UIWizardCloneVDPageBasic3.h"
#include "UIWizardCloneVDPageBasic4.h"
#include "UIWizardCloneVDPageExpert.h"
#include "UIWizardCloneVM.h"
#include "UIWizardCloneVMPageBasic1.h"
#include "UIWizardCloneVMPageBasic2.h"
#include "UIWizardCloneVMPageBasic3.h"
#include "UIWizardCloneVMPageExpert.h"
#include "UIWizardExportApp.h"
#include "UIWizardExportAppDefs.h"
#include "UIWizardExportAppPageBasic1.h"
#include "UIWizardExportAppPageBasic2.h"
#include "UIWizardExportAppPageBasic3.h"
#include "UIWizardExportAppPageBasic4.h"
#include "UIWizardExportAppPageExpert.h"
#include "UIWizardFirstRun.h"
#include "UIWizardFirstRunPageBasic.h"
#include "UIWizardImportApp.h"
#include "UIWizardImportAppDefs.h"
#include "UIWizardImportAppPageBasic1.h"
#include "UIWizardImportAppPageBasic2.h"
#include "UIWizardImportAppPageExpert.h"
#include "UIWizardNewVD.h"
#include "UIWizardNewVDPageBasic1.h"
#include "UIWizardNewVDPageBasic2.h"
#include "UIWizardNewVDPageBasic3.h"
#include "UIWizardNewVDPageExpert.h"
#include "UIWizardNewVM.h"
#include "UIWizardNewVMPageBasic1.h"
#include "UIWizardNewVMPageBasic2.h"
#include "UIWizardNewVMPageBasic3.h"
#include "UIWizardNewVMPageExpert.h"
#include "UIWizardPage.h"


/*
 * VirtualBox Qt GUI - VBox* headers.
 */
#include "VBoxAboutDlg.h"
//#include "VBoxCocoaHelper.h"
//#include "VBoxCocoaSpecialControls.h"
#include "VBoxFBOverlay.h"
#include "VBoxFBOverlayCommon.h"
#include "UIFilePathSelector.h"
#include "VBoxGlobal.h"
#include "VBoxGuestRAMSlider.h"
//#include "VBoxIChatTheaterWrapper.h"
#include "VBoxLicenseViewer.h"
#include "VBoxMediaComboBox.h"
#include "VBoxOSTypeSelectorButton.h"
#include "UISettingsSelector.h"
#include "UISnapshotDetailsWidget.h"
#include "UISnapshotPane.h"
#include "UITakeSnapshotDialog.h"
#ifdef RT_OS_DARWIN
# include "VBoxUtils-darwin.h"
#endif
#ifdef RT_OS_WINDOWS
# include "VBoxUtils-win.h"
#endif
/* Note! X.h shall not be included as it defines KeyPress and KeyRelease that
         are used as enum constants in VBoxUtils.h.  Don't bother undefining
         the redefinitions, just prevent the inclusion of the header! */
#include "VBoxUtils.h"
#include "UIVersion.h"
#ifdef VBOX_WS_X11
# include "VBoxX11Helper.h"
#endif

