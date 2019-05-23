/* $Id: UIExtraDataDefs.cpp $ */
/** @file
 * VBox Qt GUI - Extra-data related definitions.
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

/* GUI includes: */
# include "UIExtraDataDefs.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* General: */
const char* UIExtraDataDefs::GUI_EventHandlingType = "GUI/EventHandlingType";

/* Messaging: */
const char* UIExtraDataDefs::GUI_SuppressMessages = "GUI/SuppressMessages";
const char* UIExtraDataDefs::GUI_InvertMessageOption = "GUI/InvertMessageOption";
#if !defined(VBOX_BLEEDING_EDGE) && !defined(DEBUG)
const char* UIExtraDataDefs::GUI_PreventBetaWarning = "GUI/PreventBetaWarning";
#endif /* !defined(VBOX_BLEEDING_EDGE) && !defined(DEBUG) */

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
/* Application Update: */
const char* UIExtraDataDefs::GUI_PreventApplicationUpdate = "GUI/PreventApplicationUpdate";
const char* UIExtraDataDefs::GUI_UpdateDate = "GUI/UpdateDate";
const char* UIExtraDataDefs::GUI_UpdateCheckCount = "GUI/UpdateCheckCount";
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

/* Progress: */
const char* UIExtraDataDefs::GUI_Progress_LegacyMode = "GUI/Progress/LegacyMode";

/* Settings: */
const char* UIExtraDataDefs::GUI_Customizations = "GUI/Customizations";
const char* UIExtraDataDefs::GUI_RestrictedGlobalSettingsPages = "GUI/RestrictedGlobalSettingsPages";
const char* UIExtraDataDefs::GUI_RestrictedMachineSettingsPages = "GUI/RestrictedMachineSettingsPages";

/* Settings: General: */
const char* UIExtraDataDefs::GUI_HostScreenSaverDisabled = "GUI/HostScreenSaverDisabled";

/* Settings: Language: */
const char* UIExtraDataDefs::GUI_LanguageID = "GUI/LanguageID";

/* Settings: Display: */
const char* UIExtraDataDefs::GUI_MaxGuestResolution = "GUI/MaxGuestResolution";
const char* UIExtraDataDefs::GUI_ActivateHoveredMachineWindow = "GUI/ActivateHoveredMachineWindow";

/* Settings: Keyboard: */
const char* UIExtraDataDefs::GUI_Input_SelectorShortcuts = "GUI/Input/SelectorShortcuts";
const char* UIExtraDataDefs::GUI_Input_MachineShortcuts = "GUI/Input/MachineShortcuts";
const char* UIExtraDataDefs::GUI_Input_HostKeyCombination = "GUI/Input/HostKeyCombination";
const char* UIExtraDataDefs::GUI_Input_AutoCapture = "GUI/Input/AutoCapture";
const char* UIExtraDataDefs::GUI_RemapScancodes = "GUI/RemapScancodes";

/* Settings: Proxy: */
const char* UIExtraDataDefs::GUI_ProxySettings = "GUI/ProxySettings";

/* Settings: Storage: */
const char* UIExtraDataDefs::GUI_RecentFolderHD = "GUI/RecentFolderHD";
const char* UIExtraDataDefs::GUI_RecentFolderCD = "GUI/RecentFolderCD";
const char* UIExtraDataDefs::GUI_RecentFolderFD = "GUI/RecentFolderFD";
const char* UIExtraDataDefs::GUI_RecentListHD = "GUI/RecentListHD";
const char* UIExtraDataDefs::GUI_RecentListCD = "GUI/RecentListCD";
const char* UIExtraDataDefs::GUI_RecentListFD = "GUI/RecentListFD";

/* VirtualBox Manager: */
const char* UIExtraDataDefs::GUI_LastSelectorWindowPosition = "GUI/LastWindowPosition";
const char* UIExtraDataDefs::GUI_SplitterSizes = "GUI/SplitterSizes";
const char* UIExtraDataDefs::GUI_Toolbar = "GUI/Toolbar";
const char* UIExtraDataDefs::GUI_Toolbar_Text = "GUI/Toolbar/Text";
const char* UIExtraDataDefs::GUI_Toolbar_MachineTools_Order = "GUI/Toolbar/MachineTools/Order";
const char* UIExtraDataDefs::GUI_Toolbar_GlobalTools_Order = "GUI/Toolbar/GlobalTools/Order";
const char* UIExtraDataDefs::GUI_Statusbar = "GUI/Statusbar";
const char* UIExtraDataDefs::GUI_GroupDefinitions = "GUI/GroupDefinitions";
const char* UIExtraDataDefs::GUI_LastItemSelected = "GUI/LastItemSelected";
const char* UIExtraDataDefs::GUI_DetailsPageBoxes = "GUI/DetailsPageBoxes";
const char* UIExtraDataDefs::GUI_PreviewUpdate = "GUI/PreviewUpdate";

/* Snapshot Manager: */
const char* UIExtraDataDefs::GUI_SnapshotManager_Details_Expanded = "GUI/SnapshotManager/Details/Expanded";

/* Virtual Media Manager: */
const char* UIExtraDataDefs::GUI_VirtualMediaManager_Details_Expanded = "GUI/VirtualMediaManager/Details/Expanded";

/* Host Network Manager: */
const char* UIExtraDataDefs::GUI_HostNetworkManager_Details_Expanded = "GUI/HostNetworkManager/Details/Expanded";

/* Wizards: */
const char* UIExtraDataDefs::GUI_HideDescriptionForWizards = "GUI/HideDescriptionForWizards";

/* Virtual Machine: */
const char* UIExtraDataDefs::GUI_HideFromManager = "GUI/HideFromManager";
const char* UIExtraDataDefs::GUI_HideDetails = "GUI/HideDetails";
const char* UIExtraDataDefs::GUI_PreventReconfiguration = "GUI/PreventReconfiguration";
const char* UIExtraDataDefs::GUI_PreventSnapshotOperations = "GUI/PreventSnapshotOperations";
const char* UIExtraDataDefs::GUI_FirstRun = "GUI/FirstRun";
const char* UIExtraDataDefs::GUI_MachineWindowIcons = "GUI/MachineWindowIcons";
#ifndef VBOX_WS_MAC
const char* UIExtraDataDefs::GUI_MachineWindowNamePostfix = "GUI/MachineWindowNamePostfix";
#endif /* !VBOX_WS_MAC */
const char* UIExtraDataDefs::GUI_LastNormalWindowPosition = "GUI/LastNormalWindowPosition";
const char* UIExtraDataDefs::GUI_LastScaleWindowPosition = "GUI/LastScaleWindowPosition";
const char* UIExtraDataDefs::GUI_Geometry_State_Max = "max";
#ifndef VBOX_WS_MAC
const char* UIExtraDataDefs::GUI_MenuBar_Enabled = "GUI/MenuBar/Enabled";
#endif /* !VBOX_WS_MAC */
const char* UIExtraDataDefs::GUI_MenuBar_ContextMenu_Enabled = "GUI/MenuBar/ContextMenu/Enabled";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeMenus = "GUI/RestrictedRuntimeMenus";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeApplicationMenuActions = "GUI/RestrictedRuntimeApplicationMenuActions";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeMachineMenuActions = "GUI/RestrictedRuntimeMachineMenuActions";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeViewMenuActions = "GUI/RestrictedRuntimeViewMenuActions";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeInputMenuActions = "GUI/RestrictedRuntimeInputMenuActions";
const char* UIExtraDataDefs::GUI_RestrictedRuntimeDevicesMenuActions = "GUI/RestrictedRuntimeDevicesMenuActions";
#ifdef VBOX_WITH_DEBUGGER_GUI
const char* UIExtraDataDefs::GUI_RestrictedRuntimeDebuggerMenuActions = "GUI/RestrictedRuntimeDebuggerMenuActions";
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
const char* UIExtraDataDefs::GUI_RestrictedRuntimeWindowMenuActions = "GUI/RestrictedRuntimeWindowMenuActions";
#endif /* VBOX_WS_MAC */
const char* UIExtraDataDefs::GUI_RestrictedRuntimeHelpMenuActions = "GUI/RestrictedRuntimeHelpMenuActions";
const char* UIExtraDataDefs::GUI_RestrictedVisualStates = "GUI/RestrictedVisualStates";
const char* UIExtraDataDefs::GUI_Fullscreen = "GUI/Fullscreen";
const char* UIExtraDataDefs::GUI_Seamless = "GUI/Seamless";
const char* UIExtraDataDefs::GUI_Scale = "GUI/Scale";
#ifdef VBOX_WS_X11
const char* UIExtraDataDefs::GUI_Fullscreen_LegacyMode = "GUI/Fullscreen/LegacyMode";
const char* UIExtraDataDefs::GUI_DistinguishMachineWindowGroups = "GUI/DistinguishMachineWindowGroups";
#endif /* VBOX_WS_X11 */
const char* UIExtraDataDefs::GUI_AutoresizeGuest = "GUI/AutoresizeGuest";
const char* UIExtraDataDefs::GUI_LastVisibilityStatusForGuestScreen = "GUI/LastVisibilityStatusForGuestScreen";
const char* UIExtraDataDefs::GUI_LastGuestSizeHint = "GUI/LastGuestSizeHint";
const char* UIExtraDataDefs::GUI_VirtualScreenToHostScreen = "GUI/VirtualScreenToHostScreen";
const char* UIExtraDataDefs::GUI_AutomountGuestScreens = "GUI/AutomountGuestScreens";
#ifdef VBOX_WITH_VIDEOHWACCEL
const char* UIExtraDataDefs::GUI_Accelerate2D_StretchLinear = "GUI/Accelerate2D/StretchLinear";
const char* UIExtraDataDefs::GUI_Accelerate2D_PixformatYV12 = "GUI/Accelerate2D/PixformatYV12";
const char* UIExtraDataDefs::GUI_Accelerate2D_PixformatUYVY = "GUI/Accelerate2D/PixformatUYVY";
const char* UIExtraDataDefs::GUI_Accelerate2D_PixformatYUY2 = "GUI/Accelerate2D/PixformatYUY2";
const char* UIExtraDataDefs::GUI_Accelerate2D_PixformatAYUV = "GUI/Accelerate2D/PixformatAYUV";
#endif /* VBOX_WITH_VIDEOHWACCEL */
const char* UIExtraDataDefs::GUI_HiDPI_UnscaledOutput = "GUI/HiDPI/UnscaledOutput";
const char* UIExtraDataDefs::GUI_HiDPI_Optimization = "GUI/HiDPI/Optimization";
#ifndef VBOX_WS_MAC
const char* UIExtraDataDefs::GUI_ShowMiniToolBar = "GUI/ShowMiniToolBar";
const char* UIExtraDataDefs::GUI_MiniToolBarAutoHide = "GUI/MiniToolBarAutoHide";
const char* UIExtraDataDefs::GUI_MiniToolBarAlignment = "GUI/MiniToolBarAlignment";
#endif /* !VBOX_WS_MAC */
const char* UIExtraDataDefs::GUI_StatusBar_Enabled = "GUI/StatusBar/Enabled";
const char* UIExtraDataDefs::GUI_StatusBar_ContextMenu_Enabled = "GUI/StatusBar/ContextMenu/Enabled";
const char* UIExtraDataDefs::GUI_RestrictedStatusBarIndicators = "GUI/RestrictedStatusBarIndicators";
const char* UIExtraDataDefs::GUI_StatusBar_IndicatorOrder = "GUI/StatusBar/IndicatorOrder";
#ifdef VBOX_WS_MAC
const char* UIExtraDataDefs::GUI_RealtimeDockIconUpdateEnabled = "GUI/RealtimeDockIconUpdateEnabled";
const char* UIExtraDataDefs::GUI_RealtimeDockIconUpdateMonitor = "GUI/RealtimeDockIconUpdateMonitor";
const char* UIExtraDataDefs::GUI_DockIconDisableOverlay = "GUI/DockIconDisableOverlay";
#endif /* VBOX_WS_MAC */
const char* UIExtraDataDefs::GUI_PassCAD = "GUI/PassCAD";
const char* UIExtraDataDefs::GUI_MouseCapturePolicy = "GUI/MouseCapturePolicy";
const char* UIExtraDataDefs::GUI_GuruMeditationHandler = "GUI/GuruMeditationHandler";
const char* UIExtraDataDefs::GUI_HidLedsSync = "GUI/HidLedsSync";
const char* UIExtraDataDefs::GUI_ScaleFactor = "GUI/ScaleFactor";
const char* UIExtraDataDefs::GUI_Scaling_Optimization = "GUI/Scaling/Optimization";

/* Virtual Machine: Information dialog: */
const char* UIExtraDataDefs::GUI_InformationWindowGeometry = "GUI/InformationWindowGeometry";
const char* UIExtraDataDefs::GUI_InformationWindowElements = "GUI/InformationWindowElements";

/* Virtual Machine: Close dialog: */
const char* UIExtraDataDefs::GUI_DefaultCloseAction = "GUI/DefaultCloseAction";
const char* UIExtraDataDefs::GUI_RestrictedCloseActions = "GUI/RestrictedCloseActions";
const char* UIExtraDataDefs::GUI_LastCloseAction = "GUI/LastCloseAction";
const char* UIExtraDataDefs::GUI_CloseActionHook = "GUI/CloseActionHook";

#ifdef VBOX_WITH_DEBUGGER_GUI
/* Virtual Machine: Debug UI: */
const char* UIExtraDataDefs::GUI_Dbg_Enabled = "GUI/Dbg/Enabled";
const char* UIExtraDataDefs::GUI_Dbg_AutoShow = "GUI/Dbg/AutoShow";
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
/* VirtualBox: Extra-data Manager window: */
const char* UIExtraDataDefs::GUI_ExtraDataManager_Geometry = "GUI/ExtraDataManager/Geometry";
const char* UIExtraDataDefs::GUI_ExtraDataManager_SplitterHints = "GUI/ExtraDataManager/SplitterHints";
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */

/* Virtual Machine: Log dialog: */
const char* UIExtraDataDefs::GUI_LogWindowGeometry = "GUI/LogWindowGeometry";

