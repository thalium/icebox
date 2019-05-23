/* $Id: UIActionPoolSelector.h $ */
/** @file
 * VBox Qt GUI - UIActionPoolSelector class declaration.
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

#ifndef ___UIActionPoolSelector_h___
#define ___UIActionPoolSelector_h___

/* GUI includes: */
#include "UIActionPool.h"

/** Runtime action-pool index enum.
  * Naming convention is following:
  * 1. Every menu index prepended with 'M',
  * 2. Every simple-action index prepended with 'S',
  * 3. Every toggle-action index presended with 'T',
  * 4. Every polymorphic-action index presended with 'P',
  * 5. Every sub-index contains full parent-index name. */
enum UIActionIndexST
{
    /* 'File' menu actions: */
    UIActionIndexST_M_File = UIActionIndex_Max + 1,
    UIActionIndexST_M_File_S_ShowVirtualMediumManager,
    UIActionIndexST_M_File_S_ShowHostNetworkManager,
    UIActionIndexST_M_File_S_ImportAppliance,
    UIActionIndexST_M_File_S_ExportAppliance,
#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    UIActionIndexST_M_File_S_ShowExtraDataManager,
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    UIActionIndexST_M_File_S_Close,

    /* 'Group' menu actions: */
    UIActionIndexST_M_Group,
    UIActionIndexST_M_Group_S_New,
    UIActionIndexST_M_Group_S_Add,
    UIActionIndexST_M_Group_S_Rename,
    UIActionIndexST_M_Group_S_Remove,
    UIActionIndexST_M_Group_M_StartOrShow,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless,
    UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable,
    UIActionIndexST_M_Group_T_Pause,
    UIActionIndexST_M_Group_S_Reset,
    UIActionIndexST_M_Group_M_Close,
    UIActionIndexST_M_Group_M_Close_S_Detach,
    UIActionIndexST_M_Group_M_Close_S_SaveState,
    UIActionIndexST_M_Group_M_Close_S_Shutdown,
    UIActionIndexST_M_Group_M_Close_S_PowerOff,
    UIActionIndexST_M_Group_S_Discard,
    UIActionIndexST_M_Group_S_ShowLogDialog,
    UIActionIndexST_M_Group_S_Refresh,
    UIActionIndexST_M_Group_S_ShowInFileManager,
    UIActionIndexST_M_Group_S_CreateShortcut,
    UIActionIndexST_M_Group_S_Sort,

    /* 'Machine' menu actions: */
    UIActionIndexST_M_Machine,
    UIActionIndexST_M_Machine_S_New,
    UIActionIndexST_M_Machine_S_Add,
    UIActionIndexST_M_Machine_S_Settings,
    UIActionIndexST_M_Machine_S_Clone,
    UIActionIndexST_M_Machine_S_Remove,
    UIActionIndexST_M_Machine_S_AddGroup,
    UIActionIndexST_M_Machine_M_StartOrShow,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless,
    UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable,
    UIActionIndexST_M_Machine_T_Pause,
    UIActionIndexST_M_Machine_S_Reset,
    UIActionIndexST_M_Machine_M_Close,
    UIActionIndexST_M_Machine_M_Close_S_Detach,
    UIActionIndexST_M_Machine_M_Close_S_SaveState,
    UIActionIndexST_M_Machine_M_Close_S_Shutdown,
    UIActionIndexST_M_Machine_M_Close_S_PowerOff,
    UIActionIndexST_M_Machine_S_Discard,
    UIActionIndexST_M_Machine_S_ShowLogDialog,
    UIActionIndexST_M_Machine_S_Refresh,
    UIActionIndexST_M_Machine_S_ShowInFileManager,
    UIActionIndexST_M_Machine_S_CreateShortcut,
    UIActionIndexST_M_Machine_S_SortParent,

    /* Machine Tools actions: */
    UIActionIndexST_M_Tools_T_Machine,
    UIActionIndexST_M_Tools_M_Machine,
    UIActionIndexST_M_Tools_M_Machine_S_Details,
    UIActionIndexST_M_Tools_M_Machine_S_Snapshots,

    /* Global Tools actions: */
    UIActionIndexST_M_Tools_T_Global,
    UIActionIndexST_M_Tools_M_Global,
    UIActionIndexST_M_Tools_M_Global_S_VirtualMediaManager,
    UIActionIndexST_M_Tools_M_Global_S_HostNetworkManager,

    /* Maximum index: */
    UIActionIndexST_Max
};

/** UIActionPool extension
  * representing action-pool singleton for Selector UI. */
class UIActionPoolSelector : public UIActionPool
{
    Q_OBJECT;

protected:

    /** Constructor,
      * @param fTemporary is used to determine whether this action-pool is temporary,
      *                   which can be created to re-initialize shortcuts-pool. */
    UIActionPoolSelector(bool fTemporary = false);

    /** Prepare pool routine. */
    virtual void preparePool();
    /** Prepare connections routine. */
    virtual void prepareConnections();

    /** Update menus routine. */
    virtual void updateMenus();

    /** Update shortcuts. */
    virtual void updateShortcuts();

    /** Returns extra-data ID to save keyboard shortcuts under. */
    virtual QString shortcutsExtraDataID() const;

    /** Returns the list of Selector UI main menus. */
    virtual QList<QMenu*> menus() const { return QList<QMenu*>(); }

private:

    /* Enable factory in base-class: */
    friend class UIActionPool;
};

#endif /* !___UIActionPoolSelector_h___ */
