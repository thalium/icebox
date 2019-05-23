/* $Id: UIInformationModel.h $ */
/** @file
 * VBox Qt GUI - UIInformationModel class declaration.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIInformationModel_h___
#define ___UIInformationModel_h___

/* Qt includes: */
#include <QAbstractListModel>

/* COM includes: */
# include "COMEnums.h"
# include "CConsole.h"
# include "CMachine.h"

/* Forward declarations: */
class UIInformationDataItem;


/** QAbstractListModel extension
  * providing GUI with information-model for view in session-information window. */
class UIInformationModel : public QAbstractListModel
{
    Q_OBJECT;

public:

    /** Constructs information-model passing @a pParent to the base-class.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference. */
    UIInformationModel(QObject *pParent, const CMachine &machine, const CConsole &console);
    /** Destructs information-model. */
    ~UIInformationModel();

    /** Returns the row-count for item specified by the @a parentIndex. */
    virtual int rowCount(const QModelIndex &parentIndex = QModelIndex()) const /* override */;

    /** Returns the data for item specified by the @a index and the @a role. */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const /* override */;

    /** Adds the @a pItem into the model. */
    void addItem(UIInformationDataItem *pItem);

    /** Updates the data for item specified by the @a index. */
    void updateData(const QModelIndex &index);

public slots:

    /** Updates the data for the specified @a pItem. */
    void updateData(UIInformationDataItem *pItem);

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Returns the list of role-names supported by model. */
    QHash<int, QByteArray> roleNames() const;

    /** Holds the machine reference. */
    CMachine m_machine;
    /** Holds the machine console reference. */
    CConsole m_console;
    /** Holds the list of instances of information-data items. */
    QList<UIInformationDataItem*> m_list;
};

#endif /* !___UIInformationModel_h___ */

