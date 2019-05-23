/* $Id: UIActionPoolSelector.cpp $ */
/** @file
 * VBox Qt GUI - UIActionPoolSelector class implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

/* Local includes: */
# include "UIActionPoolSelector.h"
# include "UIExtraDataDefs.h"
# include "UIShortcutPool.h"
# include "UIDefs.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* TEMPORARY! */
#if defined(_MSC_VER) && !defined(RT_ARCH_AMD64)
# pragma optimize("g", off)
#endif

/* Namespaces: */
using namespace UIExtraDataDefs;


class UIActionMenuFile : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuFile(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    void retranslateUi()
    {
#ifdef VBOX_WS_MAC
        setName(QApplication::translate("UIActionPool", "&File", "Mac OS X version"));
#else /* VBOX_WS_MAC */
        setName(QApplication::translate("UIActionPool", "&File", "Non Mac OS X version"));
#endif /* !VBOX_WS_MAC */
    }
};

class UIActionSimpleVirtualMediumManagerDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleVirtualMediumManagerDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/diskimage_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("VirtualMediaManager");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+D");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Virtual Media Manager..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the Virtual Media Manager window"));
    }
};

class UIActionSimpleHostNetworkManagerDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleHostNetworkManagerDialog(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/host_iface_manager_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("HostNetworkManager");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+W");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Host Network Manager..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the Host Network Manager window"));
    }
};

class UIActionSimpleImportApplianceWizard : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleImportApplianceWizard(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/import_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ImportAppliance");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+I");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Import Appliance..."));
        setStatusTip(QApplication::translate("UIActionPool", "Import an appliance into VirtualBox"));
    }
};

class UIActionSimpleExportApplianceWizard : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleExportApplianceWizard(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/export_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ExportAppliance");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+E");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Export Appliance..."));
        setStatusTip(QApplication::translate("UIActionPool", "Export one or more VirtualBox virtual machines as an appliance"));
    }
};

#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
class UIActionSimpleExtraDataManagerWindow : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleExtraDataManagerWindow(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/edataman_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ExtraDataManager");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+X");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "E&xtra Data Manager..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the Extra Data Manager window"));
    }
};
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */

class UIActionSimpleExit : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleExit(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/exit_16px.png")
    {
        setMenuRole(QAction::QuitRole);
    }

protected:

    QString shortcutExtraDataID() const
    {
        return QString("Exit");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+Q");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "E&xit"));
        setStatusTip(QApplication::translate("UIActionPool", "Close application"));
    }
};


class UIActionMenuGroup : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuGroup(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Group"));
    }
};

class UIActionSimpleGroupNew : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleGroupNew(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_new_32px.png", ":/vm_new_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("NewVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+N");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&New Machine..."));
        setStatusTip(QApplication::translate("UIActionPool", "Create new virtual machine"));
        setToolTip(text().remove('&').remove('.') +
                   (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
    }
};

class UIActionSimpleGroupAdd : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleGroupAdd(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_add_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("AddVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+A");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Add Machine..."));
        setStatusTip(QApplication::translate("UIActionPool", "Add existing virtual machine"));
    }
};

class UIActionSimpleGroupRename : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleGroupRename(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_group_name_16px.png", ":/vm_group_name_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("RenameVMGroup");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+M");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Rena&me Group..."));
        setStatusTip(QApplication::translate("UIActionPool", "Rename selected virtual machine group"));
    }
};

class UIActionSimpleGroupRemove : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleGroupRemove(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_group_remove_16px.png", ":/vm_group_remove_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("AddVMGroup");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+U");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Ungroup"));
        setStatusTip(QApplication::translate("UIActionPool", "Ungroup items of selected virtual machine group"));
    }
};

class UIActionSimpleGroupSort : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleGroupSort(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/sort_16px.png", ":/sort_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("SortGroup");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Sort"));
        setStatusTip(QApplication::translate("UIActionPool", "Sort items of selected virtual machine group alphabetically"));
    }
};


class UIActionMenuMachineSelector : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuMachineSelector(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Machine"));
    }
};

class UIActionSimpleMachineNew : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineNew(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_new_32px.png", ":/vm_new_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("NewVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+N");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&New..."));
        setStatusTip(QApplication::translate("UIActionPool", "Create new virtual machine"));
        setToolTip(text().remove('&').remove('.') +
                   (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
    }
};

class UIActionSimpleMachineAdd : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineAdd(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_add_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("AddVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+A");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Add..."));
        setStatusTip(QApplication::translate("UIActionPool", "Add existing virtual machine"));
    }
};

class UIActionSimpleMachineAddGroup : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineAddGroup(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_group_create_16px.png", ":/vm_group_create_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("AddVMGroup");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+U");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Gro&up"));
        setStatusTip(QApplication::translate("UIActionPool", "Add new group based on selected virtual machines"));
    }
};

class UIActionSimpleMachineSettings : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineSettings(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/vm_settings_32px.png", ":/vm_settings_16px.png",
                         ":/vm_settings_disabled_32px.png", ":/vm_settings_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("SettingsVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+S");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Settings..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the virtual machine settings window"));
        setToolTip(text().remove('&').remove('.') +
                   (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
    }
};

class UIActionSimpleMachineClone : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineClone(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_clone_16px.png", ":/vm_clone_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("CloneVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+O");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Cl&one..."));
        setStatusTip(QApplication::translate("UIActionPool", "Clone selected virtual machine"));
    }
};

class UIActionSimpleMachineRemove : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineRemove(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/vm_delete_32px.png", ":/vm_delete_16px.png",
                         ":/vm_delete_disabled_32px.png", ":/vm_delete_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("RemoveVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+R");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Remove..."));
        setStatusTip(QApplication::translate("UIActionPool", "Remove selected virtual machines"));
    }
};


class UIActionStateCommonStartOrShow : public UIActionPolymorphicMenu
{
    Q_OBJECT;

public:

    UIActionStateCommonStartOrShow(UIActionPool *pParent)
        : UIActionPolymorphicMenu(pParent,
                                  ":/vm_start_32px.png", ":/vm_start_16px.png",
                                  ":/vm_start_disabled_32px.png", ":/vm_start_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("StartVM");
    }

    void retranslateUi()
    {
        switch (state())
        {
            case 0:
            {
                showMenu();
                setName(QApplication::translate("UIActionPool", "S&tart"));
                setStatusTip(QApplication::translate("UIActionPool", "Start selected virtual machines"));
                setToolTip(text().remove('&').remove('.') +
                           (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
                break;
            }
            case 1:
            {
                hideMenu();
                setName(QApplication::translate("UIActionPool", "S&how"));
                setStatusTip(QApplication::translate("UIActionPool", "Switch to the windows of selected virtual machines"));
                setToolTip(text().remove('&').remove('.') +
                           (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
                break;
            }
            default:
                break;
        }
    }
};

class UIActionSimpleStartNormal : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleStartNormal(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_start_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("StartVMNormal");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Normal Start"));
        setStatusTip(QApplication::translate("UIActionPool", "Start selected virtual machines"));
    }
};

class UIActionSimpleStartHeadless : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleStartHeadless(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_start_headless_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("StartVMHeadless");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Headless Start"));
        setStatusTip(QApplication::translate("UIActionPool", "Start selected virtual machines in the background"));
    }
};

class UIActionSimpleStartDetachable : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleStartDetachable(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_start_separate_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("StartVMDetachable");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Detachable Start"));
        setStatusTip(QApplication::translate("UIActionPool", "Start selected virtual machines with option of continuing in background"));
    }
};

class UIActionToggleCommonPauseAndResume : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleCommonPauseAndResume(UIActionPool *pParent)
        : UIActionToggle(pParent,
                         ":/vm_pause_on_16px.png", ":/vm_pause_16px.png",
                         ":/vm_pause_on_disabled_16px.png", ":/vm_pause_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("PauseVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+P");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Pause"));
        setStatusTip(QApplication::translate("UIActionPool", "Suspend execution of selected virtual machines"));
    }
};

class UIActionSimpleCommonReset : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonReset(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_reset_16px.png", ":/vm_reset_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ResetVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+T");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Reset"));
        setStatusTip(QApplication::translate("UIActionPool", "Reset selected virtual machines"));
    }
};

class UIActionSimpleCommonDiscard : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonDiscard(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/vm_discard_32px.png", ":/vm_discard_16px.png",
                         ":/vm_discard_disabled_32px.png", ":/vm_discard_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("DiscardVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+J");
    }

    void retranslateUi()
    {
        setIconText(QApplication::translate("UIActionPool", "Discard"));
        setName(QApplication::translate("UIActionPool", "D&iscard Saved State..."));
        setStatusTip(QApplication::translate("UIActionPool", "Discard saved state of selected virtual machines"));
        setToolTip(text().remove('&').remove('.') +
                   (shortcut().toString().isEmpty() ? "" : QString(" (%1)").arg(shortcut().toString())));
    }
};

class UIActionSimpleCommonShowLogDialog : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonShowLogDialog(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/vm_show_logs_32px.png", ":/vm_show_logs_16px.png",
                         ":/vm_show_logs_disabled_32px.png", ":/vm_show_logs_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ShowVMLog");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+L");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Show &Log..."));
        setStatusTip(QApplication::translate("UIActionPool", "Show log files of selected virtual machines"));
    }
};

class UIActionSimpleCommonRefresh : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonRefresh(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/refresh_32px.png", ":/refresh_16px.png",
                         ":/refresh_disabled_32px.png", ":/refresh_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("RefreshVM");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Re&fresh"));
        setStatusTip(QApplication::translate("UIActionPool", "Refresh accessibility state of selected virtual machines"));
    }
};

class UIActionSimpleCommonShowInFileManager : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonShowInFileManager(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_open_filemanager_16px.png", ":/vm_open_filemanager_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ShowVMInFileManager");
    }

    void retranslateUi()
    {
#if defined(VBOX_WS_MAC)
        setName(QApplication::translate("UIActionPool", "S&how in Finder"));
        setStatusTip(QApplication::translate("UIActionPool", "Show the VirtualBox Machine Definition files in Finder"));
#elif defined(VBOX_WS_WIN)
        setName(QApplication::translate("UIActionPool", "S&how in Explorer"));
        setStatusTip(QApplication::translate("UIActionPool", "Show the VirtualBox Machine Definition files in Explorer"));
#else
        setName(QApplication::translate("UIActionPool", "S&how in File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Show the VirtualBox Machine Definition files in the File Manager"));
#endif
    }
};

class UIActionSimpleCommonCreateShortcut : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleCommonCreateShortcut(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_create_shortcut_16px.png", ":/vm_create_shortcut_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("CreateVMAlias");
    }

    void retranslateUi()
    {
#if defined(VBOX_WS_MAC)
        setName(QApplication::translate("UIActionPool", "Cr&eate Alias on Desktop"));
        setStatusTip(QApplication::translate("UIActionPool", "Create alias files to the VirtualBox Machine Definition files on your desktop"));
#else
        setName(QApplication::translate("UIActionPool", "Cr&eate Shortcut on Desktop"));
        setStatusTip(QApplication::translate("UIActionPool", "Create shortcut files to the VirtualBox Machine Definition files on your desktop"));
#endif
    }
};

class UIActionSimpleMachineSortParent : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleMachineSortParent(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/sort_16px.png", ":/sort_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("SortGroup");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Sort"));
        setStatusTip(QApplication::translate("UIActionPool", "Sort group of first selected virtual machine alphabetically"));
    }
};


class UIActionToggleToolsMachine : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleToolsMachine(UIActionPool *pParent)
        : UIActionToggle(pParent, ":/tools_machine_32px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsMachine");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Machine Tools"));
        setStatusTip(QApplication::translate("UIActionPool", "Switch to machine tools"));
    }
};

class UIActionMenuToolsMachine : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuToolsMachine(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsMachineMenu");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Machine Tools Menu"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the machine tools menu"));
    }
};

class UIActionSimpleToolsMachineDetails : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleToolsMachineDetails(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/machine_details_manager_22px.png", ":/machine_details_manager_16px.png",
                         ":/machine_details_manager_22px.png", ":/machine_details_manager_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsMachineDetails");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Details"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the machine details pane"));
    }
};

class UIActionSimpleToolsMachineSnapshots : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleToolsMachineSnapshots(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/snapshot_manager_22px.png", ":/snapshot_manager_16px.png",
                         ":/snapshot_manager_22px.png", ":/snapshot_manager_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsMachineSnapshots");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Snapshots"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the machine snapshots pane"));
    }
};


class UIActionToggleToolsGlobal : public UIActionToggle
{
    Q_OBJECT;

public:

    UIActionToggleToolsGlobal(UIActionPool *pParent)
        : UIActionToggle(pParent, ":/tools_global_32px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsGlobal");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Global Tools"));
        setStatusTip(QApplication::translate("UIActionPool", "Switch to global tools"));
    }
};

class UIActionMenuToolsGlobal : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuToolsGlobal(UIActionPool *pParent)
        : UIActionMenu(pParent) {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsGlobalMenu");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Global Tools Menu"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the global tools menu"));
    }
};

class UIActionSimpleToolsGlobalVirtualMediaManager : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleToolsGlobalVirtualMediaManager(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/diskimage_22px.png", ":/diskimage_16px.png",
                         ":/diskimage_22px.png", ":/diskimage_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsGlobalVirtualMediaManager");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Virtual Media Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the Virtual Media Manager"));
    }
};

class UIActionSimpleToolsGlobalHostNetworkManager : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleToolsGlobalHostNetworkManager(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/host_iface_manager_22px.png", ":/host_iface_manager_16px.png",
                         ":/host_iface_manager_22px.png", ":/host_iface_manager_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ToolsGlobalHostNetworkManager");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Host Network Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open the Host Network Manager"));
    }
};


class UIActionMenuClose : public UIActionMenu
{
    Q_OBJECT;

public:

    UIActionMenuClose(UIActionPool *pParent)
        : UIActionMenu(pParent, ":/exit_16px.png") {}

protected:

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Close"));
    }
};

class UIActionSimpleDetach : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleDetach(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_create_shortcut_16px.png", ":/vm_create_shortcut_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("DetachUIVM");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Detach GUI"));
        setStatusTip(QApplication::translate("UIActionPool", "Detach the GUI from headless VM"));
    }
};

class UIActionSimpleSave : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleSave(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_save_state_16px.png", ":/vm_save_state_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("SaveVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+V");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "&Save State"));
        setStatusTip(QApplication::translate("UIActionPool", "Save state of selected virtual machines"));
    }
};

class UIActionSimpleACPIShutdown : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimpleACPIShutdown(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_shutdown_16px.png", ":/vm_shutdown_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("ACPIShutdownVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+H");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "ACPI Sh&utdown"));
        setStatusTip(QApplication::translate("UIActionPool", "Send ACPI Shutdown signal to selected virtual machines"));
    }
};

class UIActionSimplePowerOff : public UIActionSimple
{
    Q_OBJECT;

public:

    UIActionSimplePowerOff(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/vm_poweroff_16px.png", ":/vm_poweroff_disabled_16px.png") {}

protected:

    QString shortcutExtraDataID() const
    {
        return QString("PowerOffVM");
    }

    QKeySequence defaultShortcut(UIActionPoolType) const
    {
        return QKeySequence("Ctrl+F");
    }

    void retranslateUi()
    {
        setName(QApplication::translate("UIActionPool", "Po&wer Off"));
        setStatusTip(QApplication::translate("UIActionPool", "Power off selected virtual machines"));
    }
};


UIActionPoolSelector::UIActionPoolSelector(bool fTemporary /* = false */)
    : UIActionPool(UIActionPoolType_Selector, fTemporary)
{
}

void UIActionPoolSelector::preparePool()
{
    /* 'File' actions: */
    m_pool[UIActionIndexST_M_File] = new UIActionMenuFile(this);
    m_pool[UIActionIndexST_M_File_S_ShowVirtualMediumManager] = new UIActionSimpleVirtualMediumManagerDialog(this);
    m_pool[UIActionIndexST_M_File_S_ShowHostNetworkManager] = new UIActionSimpleHostNetworkManagerDialog(this);
    m_pool[UIActionIndexST_M_File_S_ImportAppliance] = new UIActionSimpleImportApplianceWizard(this);
    m_pool[UIActionIndexST_M_File_S_ExportAppliance] = new UIActionSimpleExportApplianceWizard(this);
#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    m_pool[UIActionIndexST_M_File_S_ShowExtraDataManager] = new UIActionSimpleExtraDataManagerWindow(this);
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    m_pool[UIActionIndexST_M_File_S_Close] = new UIActionSimpleExit(this);

    /* 'Group' actions: */
    m_pool[UIActionIndexST_M_Group] = new UIActionMenuGroup(this);
    m_pool[UIActionIndexST_M_Group_S_New] = new UIActionSimpleGroupNew(this);
    m_pool[UIActionIndexST_M_Group_S_Add] = new UIActionSimpleGroupAdd(this);
    m_pool[UIActionIndexST_M_Group_S_Rename] = new UIActionSimpleGroupRename(this);
    m_pool[UIActionIndexST_M_Group_S_Remove] = new UIActionSimpleGroupRemove(this);
    m_pool[UIActionIndexST_M_Group_M_StartOrShow] = new UIActionStateCommonStartOrShow(this);
    m_pool[UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal] = new UIActionSimpleStartNormal(this);
    m_pool[UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless] = new UIActionSimpleStartHeadless(this);
    m_pool[UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable] = new UIActionSimpleStartDetachable(this);
    m_pool[UIActionIndexST_M_Group_T_Pause] = new UIActionToggleCommonPauseAndResume(this);
    m_pool[UIActionIndexST_M_Group_S_Reset] = new UIActionSimpleCommonReset(this);
    m_pool[UIActionIndexST_M_Group_M_Close] = new UIActionMenuClose(this);
    m_pool[UIActionIndexST_M_Group_M_Close_S_Detach] = new UIActionSimpleDetach(this);
    m_pool[UIActionIndexST_M_Group_M_Close_S_SaveState] = new UIActionSimpleSave(this);
    m_pool[UIActionIndexST_M_Group_M_Close_S_Shutdown] = new UIActionSimpleACPIShutdown(this);
    m_pool[UIActionIndexST_M_Group_M_Close_S_PowerOff] = new UIActionSimplePowerOff(this);
    m_pool[UIActionIndexST_M_Group_S_Discard] = new UIActionSimpleCommonDiscard(this);
    m_pool[UIActionIndexST_M_Group_S_ShowLogDialog] = new UIActionSimpleCommonShowLogDialog(this);
    m_pool[UIActionIndexST_M_Group_S_Refresh] = new UIActionSimpleCommonRefresh(this);
    m_pool[UIActionIndexST_M_Group_S_ShowInFileManager] = new UIActionSimpleCommonShowInFileManager(this);
    m_pool[UIActionIndexST_M_Group_S_CreateShortcut] = new UIActionSimpleCommonCreateShortcut(this);
    m_pool[UIActionIndexST_M_Group_S_Sort] = new UIActionSimpleGroupSort(this);

    /* 'Machine' actions: */
    m_pool[UIActionIndexST_M_Machine] = new UIActionMenuMachineSelector(this);
    m_pool[UIActionIndexST_M_Machine_S_New] = new UIActionSimpleMachineNew(this);
    m_pool[UIActionIndexST_M_Machine_S_Add] = new UIActionSimpleMachineAdd(this);
    m_pool[UIActionIndexST_M_Machine_S_Settings] = new UIActionSimpleMachineSettings(this);
    m_pool[UIActionIndexST_M_Machine_S_Clone] = new UIActionSimpleMachineClone(this);
    m_pool[UIActionIndexST_M_Machine_S_Remove] = new UIActionSimpleMachineRemove(this);
    m_pool[UIActionIndexST_M_Machine_S_AddGroup] = new UIActionSimpleMachineAddGroup(this);
    m_pool[UIActionIndexST_M_Machine_M_StartOrShow] = new UIActionStateCommonStartOrShow(this);
    m_pool[UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal] = new UIActionSimpleStartNormal(this);
    m_pool[UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless] = new UIActionSimpleStartHeadless(this);
    m_pool[UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable] = new UIActionSimpleStartDetachable(this);
    m_pool[UIActionIndexST_M_Machine_T_Pause] = new UIActionToggleCommonPauseAndResume(this);
    m_pool[UIActionIndexST_M_Machine_S_Reset] = new UIActionSimpleCommonReset(this);
    m_pool[UIActionIndexST_M_Machine_M_Close] = new UIActionMenuClose(this);
    m_pool[UIActionIndexST_M_Machine_M_Close_S_Detach] = new UIActionSimpleDetach(this);
    m_pool[UIActionIndexST_M_Machine_M_Close_S_SaveState] = new UIActionSimpleSave(this);
    m_pool[UIActionIndexST_M_Machine_M_Close_S_Shutdown] = new UIActionSimpleACPIShutdown(this);
    m_pool[UIActionIndexST_M_Machine_M_Close_S_PowerOff] = new UIActionSimplePowerOff(this);
    m_pool[UIActionIndexST_M_Machine_S_Discard] = new UIActionSimpleCommonDiscard(this);
    m_pool[UIActionIndexST_M_Machine_S_ShowLogDialog] = new UIActionSimpleCommonShowLogDialog(this);
    m_pool[UIActionIndexST_M_Machine_S_Refresh] = new UIActionSimpleCommonRefresh(this);
    m_pool[UIActionIndexST_M_Machine_S_ShowInFileManager] = new UIActionSimpleCommonShowInFileManager(this);
    m_pool[UIActionIndexST_M_Machine_S_CreateShortcut] = new UIActionSimpleCommonCreateShortcut(this);
    m_pool[UIActionIndexST_M_Machine_S_SortParent] = new UIActionSimpleMachineSortParent(this);

    /* Machine Tools actions: */
    m_pool[UIActionIndexST_M_Tools_T_Machine] = new UIActionToggleToolsMachine(this);
    m_pool[UIActionIndexST_M_Tools_M_Machine] = new UIActionMenuToolsMachine(this);
    m_pool[UIActionIndexST_M_Tools_M_Machine_S_Details] = new UIActionSimpleToolsMachineDetails(this);
    m_pool[UIActionIndexST_M_Tools_M_Machine_S_Snapshots] = new UIActionSimpleToolsMachineSnapshots(this);

    /* Global Tools actions: */
    m_pool[UIActionIndexST_M_Tools_T_Global] = new UIActionToggleToolsGlobal(this);
    m_pool[UIActionIndexST_M_Tools_M_Global] = new UIActionMenuToolsGlobal(this);
    m_pool[UIActionIndexST_M_Tools_M_Global_S_VirtualMediaManager] = new UIActionSimpleToolsGlobalVirtualMediaManager(this);
    m_pool[UIActionIndexST_M_Tools_M_Global_S_HostNetworkManager] = new UIActionSimpleToolsGlobalHostNetworkManager(this);

    /* Call to base-class: */
    UIActionPool::preparePool();
}

void UIActionPoolSelector::prepareConnections()
{
    /* Prepare connections: */
    connect(gShortcutPool, SIGNAL(sigSelectorShortcutsReloaded()), this, SLOT(sltApplyShortcuts()));
    connect(gShortcutPool, SIGNAL(sigMachineShortcutsReloaded()), this, SLOT(sltApplyShortcuts()));

    /* Call to base-class: */
    UIActionPool::prepareConnections();
}

void UIActionPoolSelector::updateMenus()
{
    /* 'Help' menu: */
    updateMenuHelp();
}

void UIActionPoolSelector::updateShortcuts()
{
    /* Call to base-class: */
    UIActionPool::updateShortcuts();
    /* Create temporary Runtime UI pool to do the same: */
    if (!m_fTemporary)
        UIActionPool::createTemporary(UIActionPoolType_Runtime);
}

QString UIActionPoolSelector::shortcutsExtraDataID() const
{
    return GUI_Input_SelectorShortcuts;
}

#include "UIActionPoolSelector.moc"

