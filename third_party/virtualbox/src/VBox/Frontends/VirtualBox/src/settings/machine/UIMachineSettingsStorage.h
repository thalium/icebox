/* $Id: UIMachineSettingsStorage.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsStorage class declaration.
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

#ifndef ___UIMachineSettingsStorage_h___
#define ___UIMachineSettingsStorage_h___

/* Qt includes: */
#ifdef VBOX_WS_MAC
/* Somewhere Carbon.h includes AssertMacros.h which defines the macro "check".
 * In QItemDelegate a class method is called "check" also. As we not used the
 * macro undefine it here. */
# undef check
#endif /* VBOX_WS_MAC */
#include <QItemDelegate>
#include <QPointer>

/* GUI includes: */
#include "QITreeView.h"
#include "UISettingsPage.h"
#include "UIMachineSettingsStorage.gen.h"

/* Forward declarations: */
class AttachmentItem;
class ControllerItem;
class UIMediumIDHolder;
struct UIDataSettingsMachineStorage;
struct UIDataSettingsMachineStorageController;
struct UIDataSettingsMachineStorageAttachment;
typedef UISettingsCache<UIDataSettingsMachineStorageAttachment> UISettingsCacheMachineStorageAttachment;
typedef UISettingsCachePool<UIDataSettingsMachineStorageController, UISettingsCacheMachineStorageAttachment> UISettingsCacheMachineStorageController;
typedef UISettingsCachePool<UIDataSettingsMachineStorage, UISettingsCacheMachineStorageController> UISettingsCacheMachineStorage;

/* Internal Types */
typedef QList <StorageSlot> SlotsList;
typedef QList <KDeviceType> DeviceTypeList;
typedef QList <KStorageControllerType> ControllerTypeList;
Q_DECLARE_METATYPE (SlotsList);
Q_DECLARE_METATYPE (DeviceTypeList);
Q_DECLARE_METATYPE (ControllerTypeList);


/** Known item states. */
enum ItemState
{
    State_DefaultItem,
    State_CollapsedItem,
    State_ExpandedItem,
    State_MAX
};

/** Known pixmap types. */
enum PixmapType
{
    InvalidPixmap,

    ControllerAddEn,
    ControllerAddDis,
    ControllerDelEn,
    ControllerDelDis,

    AttachmentAddEn,
    AttachmentAddDis,
    AttachmentDelEn,
    AttachmentDelDis,

    IDEControllerNormal,
    IDEControllerExpand,
    IDEControllerCollapse,
    SATAControllerNormal,
    SATAControllerExpand,
    SATAControllerCollapse,
    SCSIControllerNormal,
    SCSIControllerExpand,
    SCSIControllerCollapse,
    USBControllerNormal,
    USBControllerExpand,
    USBControllerCollapse,
    NVMeControllerNormal,
    NVMeControllerExpand,
    NVMeControllerCollapse,
    FloppyControllerNormal,
    FloppyControllerExpand,
    FloppyControllerCollapse,

    IDEControllerAddEn,
    IDEControllerAddDis,
    SATAControllerAddEn,
    SATAControllerAddDis,
    SCSIControllerAddEn,
    SCSIControllerAddDis,
    USBControllerAddEn,
    USBControllerAddDis,
    NVMeControllerAddEn,
    NVMeControllerAddDis,
    FloppyControllerAddEn,
    FloppyControllerAddDis,

    HDAttachmentNormal,
    CDAttachmentNormal,
    FDAttachmentNormal,

    HDAttachmentAddEn,
    HDAttachmentAddDis,
    CDAttachmentAddEn,
    CDAttachmentAddDis,
    FDAttachmentAddEn,
    FDAttachmentAddDis,

    ChooseExistingEn,
    ChooseExistingDis,
    HDNewEn,
    HDNewDis,
    CDUnmountEnabled,
    CDUnmountDisabled,
    FDUnmountEnabled,
    FDUnmountDisabled,

    MaxIndex
};

/* Abstract Controller Type */
class AbstractControllerType
{
public:

    AbstractControllerType (KStorageBus aBusType, KStorageControllerType aCtrType);
    virtual ~AbstractControllerType() {}

    KStorageBus busType() const;
    KStorageControllerType ctrType() const;
    ControllerTypeList ctrTypes() const;
    PixmapType pixmap(ItemState aState) const;

    void setCtrType (KStorageControllerType aCtrType);

    DeviceTypeList deviceTypeList() const;

protected:

    virtual KStorageControllerType first() const = 0;
    virtual uint size() const = 0;

    KStorageBus mBusType;
    KStorageControllerType mCtrType;
    QList<PixmapType> mPixmaps;
};

/* IDE Controller Type */
class IDEControllerType : public AbstractControllerType
{
public:

    IDEControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* SATA Controller Type */
class SATAControllerType : public AbstractControllerType
{
public:

    SATAControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* SCSI Controller Type */
class SCSIControllerType : public AbstractControllerType
{
public:

    SCSIControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* Floppy Controller Type */
class FloppyControllerType : public AbstractControllerType
{
public:

    FloppyControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* SAS Controller Type */
class SASControllerType : public AbstractControllerType
{
public:

    SASControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* USB Controller Type */
class USBStorageControllerType : public AbstractControllerType
{
public:

    USBStorageControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* NVMe Controller Type */
class NVMeStorageControllerType : public AbstractControllerType
{
public:

    NVMeStorageControllerType (KStorageControllerType aSubType);

private:

    KStorageControllerType first() const;
    uint size() const;
};

/* Abstract Item */
class AbstractItem : public QITreeViewItem
{
    Q_OBJECT;

public:

    enum ItemType
    {
        Type_InvalidItem    = 0,
        Type_RootItem       = 1,
        Type_ControllerItem = 2,
        Type_AttachmentItem = 3
    };

    AbstractItem(QITreeView *pParent);
    AbstractItem(AbstractItem *pParentItem);
    virtual ~AbstractItem();

    AbstractItem* parent() const;
    QUuid id() const;
    QString machineId() const;

    void setMachineId (const QString &aMchineId);

    virtual ItemType rtti() const = 0;
    virtual AbstractItem* childItem (int aIndex) const = 0;
    virtual AbstractItem* childItemById (const QUuid &aId) const = 0;
    virtual int posOfChild (AbstractItem *aItem) const = 0;
    virtual QString tip() const = 0;
    virtual QPixmap pixmap (ItemState aState = State_DefaultItem) = 0;

protected:

    virtual void addChild (AbstractItem *aItem) = 0;
    virtual void delChild (AbstractItem *aItem) = 0;

    AbstractItem *m_pParentItem;
    QUuid         mId;
    QString       mMachineId;
};
Q_DECLARE_METATYPE (AbstractItem::ItemType);

/* Root Item */
class RootItem : public AbstractItem
{
public:

    RootItem(QITreeView *pParent);
   ~RootItem();

    ULONG childCount (KStorageBus aBus) const;

private:

    ItemType rtti() const;
    AbstractItem* childItem (int aIndex) const;
    AbstractItem* childItemById (const QUuid &aId) const;
    int posOfChild (AbstractItem *aItem) const;
    int childCount() const;
    QString text() const;
    QString tip() const;
    QPixmap pixmap (ItemState aState);
    void addChild (AbstractItem *aItem);
    void delChild (AbstractItem *aItem);

    QList <AbstractItem*> mControllers;
};

/* Controller Item */
class ControllerItem : public AbstractItem
{
public:

    ControllerItem (AbstractItem *aParent, const QString &aName, KStorageBus aBusType,
                    KStorageControllerType aControllerType);
   ~ControllerItem();

    KStorageBus ctrBusType() const;
    QString oldCtrName() const;
    QString ctrName() const;
    KStorageControllerType ctrType() const;
    ControllerTypeList ctrTypes() const;
    uint portCount();
    uint maxPortCount();
    bool ctrUseIoCache() const;

    void setCtrName (const QString &aCtrName);
    void setCtrType (KStorageControllerType aCtrType);
    void setPortCount (uint aPortCount);
    void setCtrUseIoCache (bool aUseIoCache);

    SlotsList ctrAllSlots() const;
    SlotsList ctrUsedSlots() const;
    DeviceTypeList ctrDeviceTypeList() const;

    void setAttachments(const QList<AbstractItem*> &attachments) { mAttachments = attachments; }

private:

    ItemType rtti() const;
    AbstractItem* childItem (int aIndex) const;
    AbstractItem* childItemById (const QUuid &aId) const;
    int posOfChild (AbstractItem *aItem) const;
    int childCount() const;
    QString text() const;
    QString tip() const;
    QPixmap pixmap (ItemState aState);
    void addChild (AbstractItem *aItem);
    void delChild (AbstractItem *aItem);

    QString mOldCtrName;
    QString mCtrName;
    AbstractControllerType *mCtrType;
    uint mPortCount;
    bool mUseIoCache;
    QList <AbstractItem*> mAttachments;
};

/* Attachment Item */
class AttachmentItem : public AbstractItem
{
public:

    AttachmentItem (AbstractItem *aParent, KDeviceType aDeviceType);

    StorageSlot attSlot() const;
    SlotsList attSlots() const;
    KDeviceType attDeviceType() const;
    DeviceTypeList attDeviceTypes() const;
    QString attMediumId() const;
    bool attIsHostDrive() const;
    bool attIsPassthrough() const;
    bool attIsTempEject() const;
    bool attIsNonRotational() const;
    bool attIsHotPluggable() const;

    void setAttSlot (const StorageSlot &aAttSlot);
    void setAttDevice (KDeviceType aAttDeviceType);
    void setAttMediumId (const QString &aAttMediumId);
    void setAttIsPassthrough (bool aPassthrough);
    void setAttIsTempEject (bool aTempEject);
    void setAttIsNonRotational (bool aNonRotational);
    void setAttIsHotPluggable(bool fIsHotPluggable);

    QString attSize() const;
    QString attLogicalSize() const;
    QString attLocation() const;
    QString attFormat() const;
    QString attDetails() const;
    QString attUsage() const;
    QString attEncryptionPasswordID() const;

private:

    void cache();

    ItemType rtti() const;
    AbstractItem* childItem (int aIndex) const;
    AbstractItem* childItemById (const QUuid &aId) const;
    int posOfChild (AbstractItem *aItem) const;
    int childCount() const;
    QString text() const;
    QString tip() const;
    QPixmap pixmap (ItemState aState);
    void addChild (AbstractItem *aItem);
    void delChild (AbstractItem *aItem);

    KDeviceType mAttDeviceType;

    StorageSlot mAttSlot;
    QString mAttMediumId;
    bool mAttIsHostDrive;
    bool mAttIsPassthrough;
    bool mAttIsTempEject;
    bool mAttIsNonRotational;
    bool m_fIsHotPluggable;

    QString mAttName;
    QString mAttTip;
    QPixmap mAttPixmap;

    QString mAttSize;
    QString mAttLogicalSize;
    QString mAttLocation;
    QString mAttFormat;
    QString mAttDetails;
    QString mAttUsage;
    QString m_strAttEncryptionPasswordID;
};

/* Storage Model */
class StorageModel : public QAbstractItemModel
{
    Q_OBJECT;

public:

    enum DataRole
    {
        R_ItemId = Qt::UserRole + 1,
        R_ItemPixmap,
        R_ItemPixmapRect,
        R_ItemName,
        R_ItemNamePoint,
        R_ItemType,
        R_IsController,
        R_IsAttachment,

        R_ToolTipType,
        R_IsMoreIDEControllersPossible,
        R_IsMoreSATAControllersPossible,
        R_IsMoreSCSIControllersPossible,
        R_IsMoreFloppyControllersPossible,
        R_IsMoreSASControllersPossible,
        R_IsMoreUSBControllersPossible,
        R_IsMoreNVMeControllersPossible,
        R_IsMoreAttachmentsPossible,

        R_CtrOldName,
        R_CtrName,
        R_CtrType,
        R_CtrTypes,
        R_CtrDevices,
        R_CtrBusType,
        R_CtrPortCount,
        R_CtrMaxPortCount,
        R_CtrIoCache,

        R_AttSlot,
        R_AttSlots,
        R_AttDevice,
        R_AttMediumId,
        R_AttIsShowDiffs,
        R_AttIsHostDrive,
        R_AttIsPassthrough,
        R_AttIsTempEject,
        R_AttIsNonRotational,
        R_AttIsHotPluggable,
        R_AttSize,
        R_AttLogicalSize,
        R_AttLocation,
        R_AttFormat,
        R_AttDetails,
        R_AttUsage,
        R_AttEncryptionPasswordID,

        R_Margin,
        R_Spacing,
        R_IconSize,

        R_HDPixmapEn,
        R_CDPixmapEn,
        R_FDPixmapEn,

        R_HDPixmapAddEn,
        R_HDPixmapAddDis,
        R_CDPixmapAddEn,
        R_CDPixmapAddDis,
        R_FDPixmapAddEn,
        R_FDPixmapAddDis,
        R_HDPixmapRect,
        R_CDPixmapRect,
        R_FDPixmapRect
    };

    enum ToolTipType
    {
        DefaultToolTip  = 0,
        ExpanderToolTip = 1,
        HDAdderToolTip  = 2,
        CDAdderToolTip  = 3,
        FDAdderToolTip  = 4
    };

    StorageModel(QITreeView *pParent);
   ~StorageModel();

    int rowCount (const QModelIndex &aParent = QModelIndex()) const;
    int columnCount (const QModelIndex &aParent = QModelIndex()) const;

    QModelIndex root() const;
    QModelIndex index (int aRow, int aColumn, const QModelIndex &aParent = QModelIndex()) const;
    QModelIndex parent (const QModelIndex &aIndex) const;

    QVariant data (const QModelIndex &aIndex, int aRole) const;
    bool setData (const QModelIndex &aIndex, const QVariant &aValue, int aRole);

    QModelIndex addController (const QString &aCtrName, KStorageBus aBusType, KStorageControllerType aCtrType);
    void delController (const QUuid &aCtrId);

    QModelIndex addAttachment (const QUuid &aCtrId, KDeviceType aDeviceType, const QString &strMediumId);
    void delAttachment (const QUuid &aCtrId, const QUuid &aAttId);

    void setMachineId (const QString &aMachineId);

    void sort(int iColumn = 0, Qt::SortOrder order = Qt::AscendingOrder);
    QModelIndex attachmentBySlot(QModelIndex controllerIndex, StorageSlot attachmentStorageSlot);

    KChipsetType chipsetType() const;
    void setChipsetType(KChipsetType type);

    /** Defines configuration access level. */
    void setConfigurationAccessLevel(ConfigurationAccessLevel newConfigurationAccessLevel);

    void clear();

    QMap<KStorageBus, int> currentControllerTypes() const;
    QMap<KStorageBus, int> maximumControllerTypes() const;

private:

    Qt::ItemFlags flags (const QModelIndex &aIndex) const;

    AbstractItem *mRootItem;

    QPixmap mPlusPixmapEn;
    QPixmap mPlusPixmapDis;

    QPixmap mMinusPixmapEn;
    QPixmap mMinusPixmapDis;

    ToolTipType mToolTipType;

    KChipsetType m_chipsetType;

    /** Holds configuration access level. */
    ConfigurationAccessLevel m_configurationAccessLevel;
};
Q_DECLARE_METATYPE (StorageModel::ToolTipType);

/* Storage Delegate */
class StorageDelegate : public QItemDelegate
{
    Q_OBJECT;

public:

    StorageDelegate (QObject *aParent);

private:

    void paint (QPainter *aPainter, const QStyleOptionViewItem &aOption, const QModelIndex &aIndex) const;
};


/** Machine settings: Storage page. */
class UIMachineSettingsStorage : public UISettingsPageMachine,
                                 public Ui::UIMachineSettingsStorage
{
    Q_OBJECT;

signals:

    /** Notifies listeners about storage changed. */
    void sigStorageChanged();

public:

    /** Constructs Storage settings page. */
    UIMachineSettingsStorage();
    /** Destructs Storage settings page. */
    ~UIMachineSettingsStorage();

    /** Defines chipset @a type. */
    void setChipsetType(KChipsetType enmType);

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const /* override */;

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void getFromCache() /* override */;

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void putToCache() /* override */;
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void saveFromCacheTo(QVariant &data) /* overrride */;

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) /* override */;

    /** Defines the configuration access @a enmLevel. */
    virtual void setConfigurationAccessLevel(ConfigurationAccessLevel enmLevel) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) /* override */;

private slots:

    /** Handles enumeration of medium with @a strMediumId. */
    void sltHandleMediumEnumerated(const QString &strMediumId);
    /** Handles removing of medium with @a strMediumId. */
    void sltHandleMediumDeleted(const QString &strMediumId);

    /** Handles command to add controller. */
    void sltAddController();
    /** Handles command to add IDE controller. */
    void sltAddControllerIDE();
    /** Handles command to add SATA controller. */
    void sltAddControllerSATA();
    /** Handles command to add SCSI controller. */
    void sltAddControllerSCSI();
    /** Handles command to add Floppy controller. */
    void sltAddControllerFloppy();
    /** Handles command to add SAS controller. */
    void sltAddControllerSAS();
    /** Handles command to add USB controller. */
    void sltAddControllerUSB();
    /** Handles command to add NVMe controller. */
    void sltAddControllerNVMe();
    /** Handles command to remove controller. */
    void sltRemoveController();

    /** Handles command to add attachment. */
    void sltAddAttachment();
    /** Handles command to add HD attachment. */
    void sltAddAttachmentHD();
    /** Handles command to add CD attachment. */
    void sltAddAttachmentCD();
    /** Handles command to add FD attachment. */
    void sltAddAttachmentFD();
    /** Handles command to remove attachment. */
    void sltRemoveAttachment();

    /** Loads information from model to widgets. */
    void sltGetInformation();
    /** Saves information from widgets to model. */
    void sltSetInformation();

    /** Prepares 'Open Medium' menu. */
    void sltPrepareOpenMediumMenu();
    /** Mounts newly created hard-drive. */
    void sltCreateNewHardDisk();
    /** Unmounts current device. */
    void sltUnmountDevice();
    /** Mounts existing medium. */
    void sltChooseExistingMedium();
    /** Mounts existing host-drive. */
    void sltChooseHostDrive();
    /** Mounts one of recent mediums. */
    void sltChooseRecentMedium();

    /** Updates action states. */
    void sltUpdateActionStates();

    /** Handles row insertion into @a parent on @a iPosition. */
    void sltHandleRowInsertion(const QModelIndex &parent, int iPosition);
    /** Handles row removal. */
    void sltHandleRowRemoval();

    /** Handles current item change. */
    void sltHandleCurrentItemChange();

    /** Handles context menu request for @a position. */
    void sltHandleContextMenuRequest(const QPoint &position);

    /** Handles item branch drawing with @a pPainter, within @a rect for item with @a index. */
    void sltHandleDrawItemBranches(QPainter *pPainter, const QRect &rect, const QModelIndex &index);

    /** Handles mouse-move @a pEvent. */
    void sltHandleMouseMove(QMouseEvent *pEvent);
    /** Handles mouse-click @a pEvent. */
    void sltHandleMouseClick(QMouseEvent *pEvent);

private:

    /** Prepares all. */
    void prepare();
    /** Prepares storage tree. */
    void prepareStorageTree();
    /** Prepares storage toolbar. */
    void prepareStorageToolbar();
    /** Prepares storage widgets. */
    void prepareStorageWidgets();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Adds controller with @a strName, @a enmBus and @a enmType. */
    void addControllerWrapper(const QString &strName, KStorageBus enmBus, KStorageControllerType enmType);
    /** Adds attachment with @a enmDevice. */
    void addAttachmentWrapper(KDeviceType enmDevice);

    /** Creates new hard-drive. */
    QString getWithNewHDWizard();

    /** Updates additions details according to passed @a enmType. */
    void updateAdditionalDetails(KDeviceType enmType);

    /** Generates unique controller name based on passed @a strTemplate. */
    QString generateUniqueControllerName(const QString &strTemplate) const;

    /** Returns current devices count for passed @a enmType. */
    uint32_t deviceCount(KDeviceType enmType) const;

    /** Adds 'Choose Existing Medium' action into passed @a pOpenMediumMenu under passed @a strActionName. */
    void addChooseExistingMediumAction(QMenu *pOpenMediumMenu, const QString &strActionName);
    /** Adds 'Choose Host Drive' actions into passed @a pOpenMediumMenu. */
    void addChooseHostDriveActions(QMenu *pOpenMediumMenu);
    /** Adds 'Choose Recent Medium' actions of passed @a enmRecentMediumType into passed @a pOpenMediumMenu. */
    void addRecentMediumActions(QMenu *pOpenMediumMenu, UIMediumType enmRecentMediumType);

    /** Saves existing storage data from the cache. */
    bool saveStorageData();
    /** Removes existing storage controller described by the @a controllerCache. */
    bool removeStorageController(const UISettingsCacheMachineStorageController &controllerCache);
    /** Creates existing storage controller described by the @a controllerCache. */
    bool createStorageController(const UISettingsCacheMachineStorageController &controllerCache);
    /** Updates existing storage controller described by the @a controllerCache. */
    bool updateStorageController(const UISettingsCacheMachineStorageController &controllerCache,
                                 bool fRemovingStep);
    /** Removes existing storage attachment described by the @a controllerCache and @a attachmentCache. */
    bool removeStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                 const UISettingsCacheMachineStorageAttachment &attachmentCache);
    /** Creates existing storage attachment described by the @a controllerCache and @a attachmentCache. */
    bool createStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                 const UISettingsCacheMachineStorageAttachment &attachmentCache);
    /** Updates existing storage attachment described by the @a controllerCache and @a attachmentCache. */
    bool updateStorageAttachment(const UISettingsCacheMachineStorageController &controllerCache,
                                 const UISettingsCacheMachineStorageAttachment &attachmentCache);
    /** Returns whether the controller described by the @a controllerCache could be updated or recreated otherwise. */
    bool isControllerCouldBeUpdated(const UISettingsCacheMachineStorageController &controllerCache) const;
    /** Returns whether the attachment described by the @a attachmentCache could be updated or recreated otherwise. */
    bool isAttachmentCouldBeUpdated(const UISettingsCacheMachineStorageAttachment &attachmentCache) const;

    /** Holds the machine ID. */
    QString  m_strMachineId;
    /** Holds the machine settings file-path. */
    QString  m_strMachineSettingsFilePath;
    /** Holds the machine guest OS type ID. */
    QString  m_strMachineGuestOSTypeId;

    /** Holds the storage-tree instance. */
    QITreeView   *m_pTreeStorage;
    /** Holds the storage-model instance. */
    StorageModel *m_pModelStorage;

    /** Holds the 'Add Controller' action instance. */
    QAction *m_pActionAddController;
    /** Holds the 'Remove Controller' action instance. */
    QAction *m_pActionRemoveController;
    /** Holds the 'Add IDE Controller' action instance. */
    QAction *m_pActionAddControllerIDE;
    /** Holds the 'Add SATA Controller' action instance. */
    QAction *m_pActionAddControllerSATA;
    /** Holds the 'Add SCSI Controller' action instance. */
    QAction *m_pActionAddControllerSCSI;
    /** Holds the 'Add SAS Controller' action instance. */
    QAction *m_pActionAddControllerSAS;
    /** Holds the 'Add Floppy Controller' action instance. */
    QAction *m_pActionAddControllerFloppy;
    /** Holds the 'Add USB Controller' action instance. */
    QAction *m_pActionAddControllerUSB;
    /** Holds the 'Add NVMe Controller' action instance. */
    QAction *m_pActionAddControllerNVMe;
    /** Holds the 'Add Attachment' action instance. */
    QAction *m_pActionAddAttachment;
    /** Holds the 'Remove Attachment' action instance. */
    QAction *m_pActionRemoveAttachment;
    /** Holds the 'Add HD Attachment' action instance. */
    QAction *m_pActionAddAttachmentHD;
    /** Holds the 'Add CD Attachment' action instance. */
    QAction *m_pActionAddAttachmentCD;
    /** Holds the 'Add FD Attachment' action instance. */
    QAction *m_pActionAddAttachmentFD;

    /** Holds the medium ID wrapper instance. */
    UIMediumIDHolder *m_pMediumIdHolder;

    /** Holds whether the page is polished. */
    bool  m_fPolished;
    /** Holds whether the loading is in progress. */
    bool  m_fLoadingInProgress;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineStorage *m_pCache;
};

#endif /* !___UIMachineSettingsStorage_h___ */

