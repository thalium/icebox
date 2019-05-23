/* $Id: VBoxMediaComboBox.h $ */
/** @file
 * VBox Qt GUI - VBoxMediaComboBox class declaration.
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

#ifndef __VBoxMediaComboBox_h__
#define __VBoxMediaComboBox_h__

#include "VBoxGlobal.h"

#include <QComboBox>
#include <QPixmap>

class VBoxMediaComboBox : public QComboBox
{
    Q_OBJECT

public:

    typedef QMap <QString, QString> BaseToDiffMap;

    VBoxMediaComboBox (QWidget *aParent);

    void refresh();
    void repopulate();

    QString id (int = -1) const;
    QString location (int = -1) const;
    UIMediumType type() const { return mType; }

    void setCurrentItem (const QString &aItemId);
    void setType (UIMediumType aMediumType);
    void setMachineId (const QString &aMachineId = QString::null);
    void setNullItemPresent (bool aNullItemPresent);

    void setShowDiffs (bool aShowDiffs);
    bool showDiffs() const { return mShowDiffs; }

protected slots:

    /* Handlers: Medium-processing stuff: */
    void sltHandleMediumCreated(const QString &strMediumID);
    void sltHandleMediumEnumerated(const QString &strMediumID);
    void sltHandleMediumDeleted(const QString &strMediumID);

    /* Handler: Medium-enumeration stuff: */
    void sltHandleMediumEnumerationStart();

    void processActivated (int aIndex);
//    void processIndexChanged (int aIndex);

    void processOnItem (const QModelIndex &aIndex);

protected:

    void updateToolTip (int);

    void appendItem (const UIMedium &);
    void replaceItem (int, const UIMedium &);

    bool findMediaIndex (const QString &aId, int &aIndex);

    UIMediumType mType;

    /** Obtruncated UIMedium structure. */
    struct Medium
    {
        Medium() {}
        Medium (const QString &aId, const QString &aLocation,
                const QString aToolTip)
            : id (aId), location (aLocation), toolTip (aToolTip) {}

        QString id;
        QString location;
        QString toolTip;
    };

    typedef QVector <Medium> Media;
    Media mMedia;

    QString mLastId;

    bool mShowDiffs : 1;
    bool mShowNullItem : 1;

    QString mMachineId;
};

#endif /* __VBoxMediaComboBox_h__ */

