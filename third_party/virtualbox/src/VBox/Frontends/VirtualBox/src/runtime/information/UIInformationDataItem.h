/* $Id: UIInformationDataItem.h $ */
/** @file
 * VBox Qt GUI - UIInformationDataItem class declaration.
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

#ifndef ___UIInformationDataItem_h___
#define ___UIInformationDataItem_h___

/* Qt includes: */
#include <QIcon>
#include <QModelIndex>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"
#include "CConsole.h"

/* Forward declarations: */
class UIInformationModel;


/** QObject extension
  * used as data-item in information-model in session-information window. */
class UIInformationDataItem : public QObject
{
    Q_OBJECT;

public:

    /** Constructs information data-item of type @a enmType.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataItem(InformationElementType enmType, const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns type of information data-item. */
    InformationElementType elementType() const { return m_enmType; }

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;

protected:

    /** Holds the type of information data-item. */
    InformationElementType m_enmType;

    /** Holds the pixmap of information data-item. */
    QPixmap m_pixmap;

    /** Holds the name of information data-item. */
    QString m_strName;

    /** Holds the machine reference. */
    CMachine m_machine;

    /** Holds the machine console reference. */
    CConsole m_console;

    /** Holds the instance of model. */
    UIInformationModel *m_pModel;
};


/** UIInformationDataItem extension for the details-element type 'General'. */
class UIInformationDataGeneral : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataGeneral(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'System'. */
class UIInformationDataSystem : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataSystem(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'System'. */
class UIInformationDataDisplay : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataDisplay(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'Storage'. */
class UIInformationDataStorage : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataStorage(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'Audio'. */
class UIInformationDataAudio : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataAudio(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'Network'. */
class UIInformationDataNetwork : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataNetwork(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'Serial ports'. */
class UIInformationDataSerialPorts : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataSerialPorts(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'USB'. */
class UIInformationDataUSB : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataUSB(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'Shared folders'. */
class UIInformationDataSharedFolders : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataSharedFolders(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;

protected slots:

    /** Updates item data. */
    void updateData();
};


/** UIInformationDataItem extension for the details-element type 'runtime attributes'. */
class UIInformationDataRuntimeAttributes : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataRuntimeAttributes(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
};


/** UIInformationDataItem extension for the details-element type 'network statistics'. */
class UIInformationDataNetworkStatistics : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataNetworkStatistics(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;

private slots:

    /** Handles processing of statistics. */
    void sltProcessStatistics();

private:

    /** Returns parsed-data of statistics. */
    QString parseStatistics(const QString &strText);

    /** VM statistics counter data map. */
    typedef QMap<QString, QString> DataMapType;
    /** VM statistics counter links map. */
    typedef QMap<QString, QStringList> LinksMapType;
    /** VM statistics counter struct. */
    struct CounterElementType { QString type; DataMapType list; };

    /** Holds the VM statistics counter names. */
    DataMapType   m_names;
    /** Holds the VM statistics counter values. */
    DataMapType   m_values;
    /** Holds the VM statistics counter units. */
    DataMapType   m_units;
    /** Holds the VM statistics counter links. */
    LinksMapType  m_links;
    /** Holds the VM statistics update timer instance. */
    QTimer       *m_pTimer;
};


/** UIInformationDataItem extension for the details-element type 'storage statistics'. */
class UIInformationDataStorageStatistics : public UIInformationDataItem
{
    Q_OBJECT;

public:

    /** Constructs details-element object.
      * @param  machine  Brings the machine reference.
      * @param  console  Brings the machine console reference.
      * @param  pModel   Brings the information model this item belings to. */
    UIInformationDataStorageStatistics(const CMachine &machine, const CConsole &console, UIInformationModel *pModel);

    /** Returns data for item specified by @a index for the @a iRole. */
    virtual QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;

private slots:

    /** Handles processing of statistics. */
    void sltProcessStatistics();

private:

    /** Returns parsed-data of statistics. */
    QString parseStatistics(const QString &strText);

    /** Converts a given storage controller type to the string representation used in statistics. */
    const char *storCtrlType2Str(const KStorageControllerType enmCtrlType) const;

    /** VM statistics counter data map. */
    typedef QMap<QString, QString> DataMapType;
    /** VM statistics counter links map. */
    typedef QMap<QString, QStringList> LinksMapType;
    /** VM statistics counter struct. */
    struct CounterElementType { QString type; DataMapType list; };

    /** Holds the VM statistics counter names. */
    DataMapType   m_names;
    /** Holds the VM statistics counter values. */
    DataMapType   m_values;
    /** Holds the VM statistics counter units. */
    DataMapType   m_units;
    /** Holds the VM statistics counter links. */
    LinksMapType  m_links;
    /** Holds the VM statistics update timer instance. */
    QTimer       *m_pTimer;
};

#endif /* !___UIInformationDataItem_h___ */

