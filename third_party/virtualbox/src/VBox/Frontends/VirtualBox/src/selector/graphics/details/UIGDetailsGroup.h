/* $Id: UIGDetailsGroup.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsGroup class declaration.
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

#ifndef __UIGDetailsGroup_h__
#define __UIGDetailsGroup_h__

/* GUI includes: */
#include "UIGDetailsItem.h"

/* Forward declarations: */
class UIVMItem;
class QGraphicsScene;

/* Details group
 * for graphics details model/view architecture: */
class UIGDetailsGroup : public UIGDetailsItem
{
    Q_OBJECT;

signals:

    /* Notifiers: Size-hint stuff: */
    void sigMinimumWidthHintChanged(int iMinimumWidthHint);
    void sigMinimumHeightHintChanged(int iMinimumHeightHint);

public:

    /* Graphics-item type: */
    enum { Type = UIGDetailsItemType_Group };
    int type() const { return Type; }

    /* Constructor/destructor: */
    UIGDetailsGroup(QGraphicsScene *pParent);
    ~UIGDetailsGroup();

    /* API: Build stuff: */
    void buildGroup(const QList<UIVMItem*> &machineItems);
    void rebuildGroup();
    void stopBuildingGroup();

private slots:

    /* Handler: Build stuff: */
    void sltBuildStep(QString strStepId, int iStepNumber);

private:

    /* Data enumerator: */
    enum GroupItemData
    {
        /* Layout hints: */
        GroupData_Margin,
        GroupData_Spacing
    };

    /** Returns the description of the item. */
    virtual QString description() const /* override */ { return QString(); }

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Hidden API: Children stuff: */
    void addItem(UIGDetailsItem *pItem);
    void removeItem(UIGDetailsItem *pItem);
    QList<UIGDetailsItem*> items(UIGDetailsItemType type = UIGDetailsItemType_Set) const;
    bool hasItems(UIGDetailsItemType type = UIGDetailsItemType_Set) const;
    void clearItems(UIGDetailsItemType type = UIGDetailsItemType_Set);

    /* Helpers: Prepare stuff: */
    void prepareConnections();

    /* Helpers: Layout stuff: */
    void updateGeometry();
    int minimumWidthHint() const;
    int minimumHeightHint() const;
    void updateLayout();

    /* Variables: */
    int m_iPreviousMinimumWidthHint;
    int m_iPreviousMinimumHeightHint;
    QList<UIGDetailsItem*> m_items;
    QList<UIVMItem*> m_machineItems;
    UIBuildStep *m_pBuildStep;
    QString m_strGroupId;

    /* Friends: */
    friend class UIGDetailsModel;
};

#endif /* __UIGDetailsGroup_h__ */

