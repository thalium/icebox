/* $Id: UIConverterBackend.h $ */
/** @file
 * VBox Qt GUI - UIConverterBackend declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIConverterBackend_h__
#define __UIConverterBackend_h__

/* Qt includes: */
#include <QString>
#include <QColor>
#include <QIcon>
#include <QPixmap>

/* GUI includes: */
#include "UIDefs.h"
#include "UIExtraDataDefs.h"

/* Other VBox includes: */
#include <iprt/assert.h>

/* Determines if 'Object of type X' can be converted to object of other type.
 * This function always returns 'false' until re-determined for specific object type. */
template<class X> bool canConvert() { return false; }

/* Converts passed 'Object X' to QColor.
 * This function returns null QColor for any object type until re-determined for specific one. */
template<class X> QColor toColor(const X & /* xobject */) { Assert(0); return QColor(); }

/* Converts passed 'Object X' to QIcon.
 * This function returns null QIcon for any object type until re-determined for specific one. */
template<class X> QIcon toIcon(const X & /* xobject */) { Assert(0); return QIcon(); }

/* Converts passed 'Object X' to QPixmap.
 * This function returns null QPixmap for any object type until re-determined for specific one. */
template<class X> QPixmap toWarningPixmap(const X & /* xobject */) { Assert(0); return QPixmap(); }

/* Converts passed 'Object of type X' to QString.
 * This function returns null QString for any object type until re-determined for specific one. */
template<class X> QString toString(const X & /* xobject */) { Assert(0); return QString(); }
/* Converts passed QString to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromString(const QString & /* strData */) { Assert(0); return X(); }

/* Converts passed 'Object of type X' to non-translated QString.
 * This function returns null QString for any object type until re-determined for specific one. */
template<class X> QString toInternalString(const X & /* xobject */) { Assert(0); return QString(); }
/* Converts passed non-translated QString to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromInternalString(const QString & /* strData */) { Assert(0); return X(); }

/* Converts passed 'Object of type X' to abstract integer.
 * This function returns 0 for any object type until re-determined for specific one. */
template<class X> int toInternalInteger(const X & /* xobject */) { Assert(0); return 0; }
/* Converts passed abstract integer to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromInternalInteger(const int & /* iData */) { Assert(0); return X(); }

/* Declare global canConvert specializations: */
template<> bool canConvert<SizeSuffix>();
template<> bool canConvert<StorageSlot>();
template<> bool canConvert<UIExtraDataMetaDefs::MenuType>();
template<> bool canConvert<UIExtraDataMetaDefs::MenuApplicationActionType>();
template<> bool canConvert<UIExtraDataMetaDefs::MenuHelpActionType>();
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>();
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuViewActionType>();
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuInputActionType>();
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>();
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> bool canConvert<UIExtraDataMetaDefs::MenuWindowActionType>();
#endif /* VBOX_WS_MAC */
template<> bool canConvert<ToolTypeMachine>();
template<> bool canConvert<ToolTypeGlobal>();
template<> bool canConvert<UIVisualStateType>();
template<> bool canConvert<DetailsElementType>();
template<> bool canConvert<PreviewUpdateIntervalType>();
template<> bool canConvert<EventHandlingType>();
template<> bool canConvert<GUIFeatureType>();
template<> bool canConvert<GlobalSettingsPageType>();
template<> bool canConvert<MachineSettingsPageType>();
template<> bool canConvert<WizardType>();
template<> bool canConvert<IndicatorType>();
template<> bool canConvert<MachineCloseAction>();
template<> bool canConvert<MouseCapturePolicy>();
template<> bool canConvert<GuruMeditationHandlerType>();
template<> bool canConvert<ScalingOptimizationType>();
template<> bool canConvert<HiDPIOptimizationType>();
#ifndef VBOX_WS_MAC
template<> bool canConvert<MiniToolbarAlignment>();
#endif /* !VBOX_WS_MAC */
template<> bool canConvert<InformationElementType>();
template<> bool canConvert<MaxGuestResolutionPolicy>();

/* Declare COM canConvert specializations: */
template<> bool canConvert<KMachineState>();
template<> bool canConvert<KSessionState>();
template<> bool canConvert<KParavirtProvider>();
template<> bool canConvert<KDeviceType>();
template<> bool canConvert<KClipboardMode>();
template<> bool canConvert<KDnDMode>();
template<> bool canConvert<KPointingHIDType>();
template<> bool canConvert<KMediumType>();
template<> bool canConvert<KMediumVariant>();
template<> bool canConvert<KNetworkAttachmentType>();
template<> bool canConvert<KNetworkAdapterType>();
template<> bool canConvert<KNetworkAdapterPromiscModePolicy>();
template<> bool canConvert<KPortMode>();
template<> bool canConvert<KUSBControllerType>();
template<> bool canConvert<KUSBDeviceState>();
template<> bool canConvert<KUSBDeviceFilterAction>();
template<> bool canConvert<KAudioDriverType>();
template<> bool canConvert<KAudioControllerType>();
template<> bool canConvert<KAuthType>();
template<> bool canConvert<KStorageBus>();
template<> bool canConvert<KStorageControllerType>();
template<> bool canConvert<KChipsetType>();
template<> bool canConvert<KNATProtocol>();

/* Declare global conversion specializations: */
template<> QString toString(const SizeSuffix &sizeSuffix);
template<> SizeSuffix fromString<SizeSuffix>(const QString &strSizeSuffix);
template<> QString toString(const StorageSlot &storageSlot);
template<> StorageSlot fromString<StorageSlot>(const QString &strStorageSlot);
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuType &menuType);
template<> UIExtraDataMetaDefs::MenuType fromInternalString<UIExtraDataMetaDefs::MenuType>(const QString &strMenuType);
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuApplicationActionType &menuApplicationActionType);
template<> UIExtraDataMetaDefs::MenuApplicationActionType fromInternalString<UIExtraDataMetaDefs::MenuApplicationActionType>(const QString &strMenuApplicationActionType);
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuHelpActionType &menuHelpActionType);
template<> UIExtraDataMetaDefs::MenuHelpActionType fromInternalString<UIExtraDataMetaDefs::MenuHelpActionType>(const QString &strMenuHelpActionType);
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuMachineActionType &runtimeMenuMachineActionType);
template<> UIExtraDataMetaDefs::RuntimeMenuMachineActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(const QString &strRuntimeMenuMachineActionType);
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuViewActionType &runtimeMenuViewActionType);
template<> UIExtraDataMetaDefs::RuntimeMenuViewActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuViewActionType>(const QString &strRuntimeMenuViewActionType);
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuInputActionType &runtimeMenuInputActionType);
template<> UIExtraDataMetaDefs::RuntimeMenuInputActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuInputActionType>(const QString &strRuntimeMenuInputActionType);
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType &runtimeMenuDevicesActionType);
template<> UIExtraDataMetaDefs::RuntimeMenuDevicesActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(const QString &strRuntimeMenuDevicesActionType);
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType &runtimeMenuDebuggerActionType);
template<> UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(const QString &strRuntimeMenuDebuggerActionType);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuWindowActionType &menuWindowActionType);
template<> UIExtraDataMetaDefs::MenuWindowActionType fromInternalString<UIExtraDataMetaDefs::MenuWindowActionType>(const QString &strMenuWindowActionType);
#endif /* VBOX_WS_MAC */
template<> QString toInternalString(const ToolTypeMachine &enmToolTypeMachine);
template<> ToolTypeMachine fromInternalString<ToolTypeMachine>(const QString &strToolTypeMachine);
template<> QString toInternalString(const ToolTypeGlobal &enmToolTypeGlobal);
template<> ToolTypeGlobal fromInternalString<ToolTypeGlobal>(const QString &strToolTypeGlobal);
template<> QString toInternalString(const UIVisualStateType &visualStateType);
template<> UIVisualStateType fromInternalString<UIVisualStateType>(const QString &strVisualStateType);
template<> QString toString(const DetailsElementType &detailsElementType);
template<> DetailsElementType fromString<DetailsElementType>(const QString &strDetailsElementType);
template<> QString toInternalString(const DetailsElementType &detailsElementType);
template<> DetailsElementType fromInternalString<DetailsElementType>(const QString &strDetailsElementType);
template<> QIcon toIcon(const DetailsElementType &detailsElementType);
template<> QString toInternalString(const PreviewUpdateIntervalType &previewUpdateIntervalType);
template<> PreviewUpdateIntervalType fromInternalString<PreviewUpdateIntervalType>(const QString &strPreviewUpdateIntervalType);
template<> int toInternalInteger(const PreviewUpdateIntervalType &previewUpdateIntervalType);
template<> PreviewUpdateIntervalType fromInternalInteger<PreviewUpdateIntervalType>(const int &iPreviewUpdateIntervalType);
template<> EventHandlingType fromInternalString<EventHandlingType>(const QString &strEventHandlingType);
template<> QString toInternalString(const GUIFeatureType &guiFeatureType);
template<> GUIFeatureType fromInternalString<GUIFeatureType>(const QString &strGuiFeatureType);
template<> QString toInternalString(const GlobalSettingsPageType &globalSettingsPageType);
template<> GlobalSettingsPageType fromInternalString<GlobalSettingsPageType>(const QString &strGlobalSettingsPageType);
template<> QPixmap toWarningPixmap(const GlobalSettingsPageType &globalSettingsPageType);
template<> QString toInternalString(const MachineSettingsPageType &machineSettingsPageType);
template<> MachineSettingsPageType fromInternalString<MachineSettingsPageType>(const QString &strMachineSettingsPageType);
template<> QPixmap toWarningPixmap(const MachineSettingsPageType &machineSettingsPageType);
template<> QString toInternalString(const WizardType &wizardType);
template<> WizardType fromInternalString<WizardType>(const QString &strWizardType);
template<> QString toInternalString(const IndicatorType &indicatorType);
template<> IndicatorType fromInternalString<IndicatorType>(const QString &strIndicatorType);
template<> QString toString(const IndicatorType &indicatorType);
template<> QIcon toIcon(const IndicatorType &indicatorType);
template<> QString toInternalString(const MachineCloseAction &machineCloseAction);
template<> MachineCloseAction fromInternalString<MachineCloseAction>(const QString &strMachineCloseAction);
template<> QString toInternalString(const MouseCapturePolicy &mouseCapturePolicy);
template<> MouseCapturePolicy fromInternalString<MouseCapturePolicy>(const QString &strMouseCapturePolicy);
template<> QString toInternalString(const GuruMeditationHandlerType &guruMeditationHandlerType);
template<> GuruMeditationHandlerType fromInternalString<GuruMeditationHandlerType>(const QString &strGuruMeditationHandlerType);
template<> QString toInternalString(const ScalingOptimizationType &optimizationType);
template<> ScalingOptimizationType fromInternalString<ScalingOptimizationType>(const QString &strOptimizationType);
template<> QString toInternalString(const HiDPIOptimizationType &optimizationType);
template<> HiDPIOptimizationType fromInternalString<HiDPIOptimizationType>(const QString &strOptimizationType);
#ifndef VBOX_WS_MAC
template<> QString toInternalString(const MiniToolbarAlignment &miniToolbarAlignment);
template<> MiniToolbarAlignment fromInternalString<MiniToolbarAlignment>(const QString &strMiniToolbarAlignment);
#endif /* !VBOX_WS_MAC */
template<> QString toString(const InformationElementType &informationElementType);
template<> InformationElementType fromString<InformationElementType>(const QString &strInformationElementType);
template<> QString toInternalString(const InformationElementType &informationElementType);
template<> InformationElementType fromInternalString<InformationElementType>(const QString &strInformationElementType);
template<> QIcon toIcon(const InformationElementType &informationElementType);
template<> QString toInternalString(const MaxGuestResolutionPolicy &enmMaxGuestResolutionPolicy);
template<> MaxGuestResolutionPolicy fromInternalString<MaxGuestResolutionPolicy>(const QString &strMaxGuestResolutionPolicy);

/* Declare COM conversion specializations: */
template<> QColor toColor(const KMachineState &state);
template<> QIcon toIcon(const KMachineState &state);
template<> QString toString(const KMachineState &state);
template<> QString toString(const KSessionState &state);
template<> QString toString(const KParavirtProvider &type);
template<> QString toString(const KDeviceType &type);
template<> QString toString(const KClipboardMode &mode);
template<> QString toString(const KDnDMode &mode);
template<> QString toString(const KPointingHIDType &type);
template<> QString toString(const KMediumType &type);
template<> QString toString(const KMediumVariant &variant);
template<> QString toString(const KNetworkAttachmentType &type);
template<> QString toString(const KNetworkAdapterType &type);
template<> QString toString(const KNetworkAdapterPromiscModePolicy &policy);
template<> QString toString(const KPortMode &mode);
template<> QString toString(const KUSBControllerType &type);
template<> QString toString(const KUSBDeviceState &state);
template<> QString toString(const KUSBDeviceFilterAction &action);
template<> QString toString(const KAudioDriverType &type);
template<> QString toString(const KAudioControllerType &type);
template<> QString toString(const KAuthType &type);
template<> QString toString(const KStorageBus &bus);
template<> QString toString(const KStorageControllerType &type);
template<> QString toString(const KChipsetType &type);
template<> QString toString(const KNATProtocol &protocol);
template<> QString toInternalString(const KNATProtocol &protocol);
template<> KNATProtocol fromInternalString<KNATProtocol>(const QString &strProtocol);
template<> KPortMode fromString<KPortMode>(const QString &strMode);
template<> KUSBDeviceFilterAction fromString<KUSBDeviceFilterAction>(const QString &strAction);
template<> KAudioDriverType fromString<KAudioDriverType>(const QString &strType);
template<> KAudioControllerType fromString<KAudioControllerType>(const QString &strType);
template<> KAuthType fromString<KAuthType>(const QString &strType);
template<> KStorageControllerType fromString<KStorageControllerType>(const QString &strType);

#endif /* __UIConverterBackend_h__ */

