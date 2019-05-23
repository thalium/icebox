/* $Id: UIGChooserItemGroup.h $ */
/** @file
 * VBox Qt GUI - UIGChooserItemGroup class declaration.
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

#ifndef __UIGChooserItemGroup_h__
#define __UIGChooserItemGroup_h__

/* Qt includes: */
#include <QWidget>
#include <QPixmap>

/* GUI includes: */
#include "UIGChooserItem.h"

/* Forward declarations: */
class QGraphicsScene;
class QGraphicsProxyWidget;
class QLineEdit;
class UIGraphicsButton;
class UIGraphicsRotatorButton;
class UIGroupRenameEditor;

/* Graphics group-item
 * for graphics selector model/view architecture: */
class UIGChooserItemGroup : public UIGChooserItem
{
    Q_OBJECT;
    Q_PROPERTY(int additionalHeight READ additionalHeight WRITE setAdditionalHeight);

signals:

    /* Notifiers: Toggle stuff: */
    void sigToggleStarted();
    void sigToggleFinished();

public:

    /* Class-name used for drag&drop mime-data format: */
    static QString className();

    /* Graphics-item type: */
    enum { Type = UIGChooserItemType_Group };
    int type() const { return Type; }

    /* Constructor (main-root-item): */
    UIGChooserItemGroup(QGraphicsScene *pScene);
    /* Constructor (temporary main-root-item/root-item copy): */
    UIGChooserItemGroup(QGraphicsScene *pScene, UIGChooserItemGroup *pCopyFrom, bool fMainRoot);
    /* Constructor (new non-root-item): */
    UIGChooserItemGroup(UIGChooserItem *pParent, const QString &strName, bool fOpened = false, int iPosition  = -1);
    /* Constructor (new non-root-item copy): */
    UIGChooserItemGroup(UIGChooserItem *pParent, UIGChooserItemGroup *pCopyFrom, int iPosition = -1);
    /* Destructor: */
    ~UIGChooserItemGroup();

    /* API: Basic stuff: */
    QString name() const;
    QString description() const;
    QString fullName() const;
    QString definition() const;
    void setName(const QString &strName);
    bool isClosed() const;
    bool isOpened() const;
    void close(bool fAnimated = true);
    void open(bool fAnimated = true);

    /* API: Children stuff: */
    bool isContainsMachine(const QString &strId) const;
    bool isContainsLockedMachine();

private slots:

    /* Handler: Name editing stuff: */
    void sltNameEditingFinished();

    /* Handler: Toggle stuff: */
    void sltGroupToggleStart();
    void sltGroupToggleFinish(bool fToggled);

    /* Handlers: Indent root stuff: */
    void sltIndentRoot();
    void sltUnindentRoot();

private:

    /* Data enumerator: */
    enum GroupItemData
    {
        /* Layout hints: */
        GroupItemData_HorizonalMargin,
        GroupItemData_VerticalMargin,
        GroupItemData_MajorSpacing,
        GroupItemData_MinorSpacing,
        GroupItemData_RootIndent,
    };

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Helpers: Prepare stuff: */
    void prepare();
    static void copyContent(UIGChooserItemGroup *pFrom, UIGChooserItemGroup *pTo);

    /* Helpers: Update stuff: */
    void handleRootStatusChange();
    void updateVisibleName();
    void updateItemCountInfo();
    void updateMinimumHeaderSize();
    void updateToolTip();
    void updateToggleButtonToolTip();

    /* Helper: Translate stuff: */
    void retranslateUi();

    /* Helpers: Basic stuff: */
    void show();
    void hide();
    void startEditing();
    bool isMainRoot() const { return m_fMainRoot; }

    /* Helpers: Children stuff: */
    void addItem(UIGChooserItem *pItem, int iPosition);
    void removeItem(UIGChooserItem *pItem);
    void setItems(const QList<UIGChooserItem*> &items, UIGChooserItemType type);
    QList<UIGChooserItem*> items(UIGChooserItemType type = UIGChooserItemType_Any) const;
    bool hasItems(UIGChooserItemType type = UIGChooserItemType_Any) const;
    void clearItems(UIGChooserItemType type = UIGChooserItemType_Any);
    void updateAll(const QString &strId);
    void removeAll(const QString &strId);
    UIGChooserItem* searchForItem(const QString &strSearchTag, int iItemSearchFlags);
    UIGChooserItemMachine* firstMachineItem();
    void sortItems();

    /* Helpers: Layout stuff: */
    void updateLayout();
    int minimumWidthHint(bool fOpenedGroup) const;
    int minimumHeightHint(bool fOpenedGroup) const;
    int minimumWidthHint() const;
    int minimumHeightHint() const;
#ifdef VBOX_WS_MAC
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
    QSizeF minimumSizeHint(bool fOpenedGroup) const;
#ifdef VBOX_WS_MAC
# pragma clang diagnostic pop
#endif
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    /* Helpers: Drag&drop stuff: */
    QPixmap toPixmap();
    bool isDropAllowed(QGraphicsSceneDragDropEvent *pEvent, DragToken where) const;
    void processDrop(QGraphicsSceneDragDropEvent *pEvent, UIGChooserItem *pFromWho, DragToken where);
    void resetDragToken();
    QMimeData* createMimeData();

    /* Handler: Resize handling stuff: */
    void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

    /* Handlers: Hover handling stuff: */
    void hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);

    /* Helpers: Paint stuff: */
    void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);
    void paintBackground(QPainter *pPainter, const QRect &rect);
    void paintHeader(QPainter *pPainter, const QRect &rect);

    /* Helpers: Animation stuff: */
    void updateAnimationParameters();
    void setAdditionalHeight(int iAdditionalHeight);
    int additionalHeight() const;

    /* Helper: Color stuff: */
    int blackoutDarkness() const { return m_iBlackoutDarkness; }

    /* Variables: */
    bool m_fClosed;
    UIGraphicsRotatorButton *m_pToggleButton;
    UIGraphicsButton *m_pEnterButton;
    UIGraphicsButton *m_pExitButton;
    UIGroupRenameEditor *m_pNameEditorWidget;
    QGraphicsProxyWidget *m_pNameEditor;
    QList<UIGChooserItem*> m_groupItems;
    QList<UIGChooserItem*> m_machineItems;
    int m_iAdditionalHeight;
    int m_iCornerRadius;
    bool m_fMainRoot;
    int m_iBlackoutDarkness;
    /* Cached values: */
    QString m_strName;
    QString m_strDescription;
    QString m_strVisibleName;
    QString m_strInfoGroups;
    QString m_strInfoMachines;
    QSize m_visibleNameSize;
    QSize m_infoSizeGroups;
    QSize m_infoSizeMachines;
    QSize m_pixmapSizeGroups;
    QSize m_pixmapSizeMachines;
    QSize m_minimumHeaderSize;
    QSize m_toggleButtonSize;
    QSize m_enterButtonSize;
    QSize m_exitButtonSize;
    QFont m_nameFont;
    QFont m_infoFont;
    QPixmap m_groupsPixmap;
    QPixmap m_machinesPixmap;
};

class UIGroupRenameEditor : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifier: Editing stuff: */
    void sigEditingFinished();

public:

    /* Constructor: */
    UIGroupRenameEditor(const QString &strName, UIGChooserItem *pParent);

    /* API: Text stuff: */
    QString text() const;
    void setText(const QString &strText);

    /* API: Font stuff: */
    void setFont(const QFont &font);

public slots:

    /* API/Handler: Focus stuff: */
    void setFocus();

private:

    /* Handler: Event-filter: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Helper: Context-menu stuff: */
    void handleContextMenuEvent(QContextMenuEvent *pContextMenuEvent);

    /* Variables: */
    UIGChooserItem *m_pParent;
    QLineEdit *m_pLineEdit;
    QMenu *m_pTemporaryMenu;
};

#endif /* __UIGChooserItemGroup_h__ */

