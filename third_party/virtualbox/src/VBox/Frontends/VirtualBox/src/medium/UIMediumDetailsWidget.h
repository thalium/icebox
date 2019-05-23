/* $Id: UIMediumDetailsWidget.h $ */
/** @file
 * VBox Qt GUI - UIMediumDetailsWidget class declaration.
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

#ifndef ___UIMediumDetailsWidget_h___
#define ___UIMediumDetailsWidget_h___

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIMediumDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QAbstractButton;
class QComboBox;
class QLabel;
class QLineEdit;
class QStackedLayout;
class QTextEdit;
class QWidget;
class QILabel;
class QITabWidget;
class QIToolButton;
class UIEnumerationProgressBar;
class UIMediumManagerWidget;
class UIMediumSizeEditor;


/** Virtual Media Manager: Medium options data structure. */
struct UIDataMediumOptions
{
    /** Constructs data. */
    UIDataMediumOptions()
        : m_enmType(KMediumType_Normal)
        , m_strLocation(QString())
        , m_strDescription(QString())
        , m_uLogicalSize(0)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataMediumOptions &other) const
    {
        return true
               && (m_enmType == other.m_enmType)
               && (m_strLocation == other.m_strLocation)
               && (m_strDescription == other.m_strDescription)
               && (m_uLogicalSize == other.m_uLogicalSize)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataMediumOptions &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataMediumOptions &other) const { return !equal(other); }

    /** Holds the type. */
    KMediumType m_enmType;
    /** Holds the location. */
    QString m_strLocation;
    /** Holds the description. */
    QString m_strDescription;
    /** Holds the logical size. */
    qulonglong m_uLogicalSize;
};


/** Virtual Media Manager: Medium details data structure. */
struct UIDataMediumDetails
{
    /** Constructs data. */
    UIDataMediumDetails()
        : m_aLabels(QStringList())
        , m_aFields(QStringList())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataMediumDetails &other) const
    {
        return true
               && (m_aLabels == other.m_aLabels)
               && (m_aFields == other.m_aFields)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataMediumDetails &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataMediumDetails &other) const { return !equal(other); }

    /** Holds the labels list. */
    QStringList m_aLabels;
    /** Holds the fields list. */
    QStringList m_aFields;
};


/** Virtual Media Manager: Medium data structure. */
struct UIDataMedium
{
    /** Constructs data. */
    UIDataMedium()
        : m_fValid(false)
        , m_enmType(UIMediumType_Invalid)
        , m_enmVariant(KMediumVariant_Max)
        , m_fHasChildren(false)
        , m_options(UIDataMediumOptions())
        , m_details(UIDataMediumDetails())
    {}

    /** Constructs data with passed @enmType. */
    UIDataMedium(UIMediumType enmType)
        : m_fValid(false)
        , m_enmType(enmType)
        , m_enmVariant(KMediumVariant_Max)
        , m_fHasChildren(false)
        , m_options(UIDataMediumOptions())
        , m_details(UIDataMediumDetails())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataMedium &other) const
    {
        return true
               && (m_fValid == other.m_fValid)
               && (m_enmType == other.m_enmType)
               && (m_enmVariant == other.m_enmVariant)
               && (m_fHasChildren == other.m_fHasChildren)
               && (m_options == other.m_options)
               && (m_details == other.m_details)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataMedium &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataMedium &other) const { return !equal(other); }

    /** Holds whether data is valid. */
    bool m_fValid;
    /** Holds the medium type. */
    UIMediumType m_enmType;
    /** Holds the medium variant. */
    KMediumVariant m_enmVariant;
    /** Holds whether medium has children. */
    bool m_fHasChildren;

    /** Holds the medium options. */
    UIDataMediumOptions m_options;
    /** Holds the details data. */
    UIDataMediumDetails m_details;
};


/** Virtual Media Manager: Virtual Media Manager details-widget. */
class UIMediumDetailsWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about accept is allowed. */
    void sigAcceptAllowed(bool fAllowed);
    /** Notifies listeners about reject is allowed. */
    void sigRejectAllowed(bool fAllowed);

    /** Notifies listeners about data change rejected and should be reseted. */
    void sigDataChangeRejected();
    /** Notifies listeners about data change accepted and should be applied. */
    void sigDataChangeAccepted();

public:

    /** Constructs medium details dialog passing @a pParent to the base-class.
      * @param  enmEmbedding  Brings embedding type. */
    UIMediumDetailsWidget(UIMediumManagerWidget *pParent, EmbedTo enmEmbedding);

    /** Defines the raised details @a enmType. */
    void setCurrentType(UIMediumType enmType);

    /** Returns the medium data. */
    const UIDataMedium &data() const { return m_newData; }
    /** Defines the @a data for passed @a enmType. */
    void setData(const UIDataMedium &data);

public slots:

    /** Defines whether the options tab is @a fEnabled. */
    void setOptionsEnabled(bool fEnabled);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** @name Options stuff.
      * @{ */
        /** Handles type change. */
        void sltTypeIndexChanged(int iIndex);
        /** Handles location change. */
        void sltLocationPathChanged(const QString &strPath);
        /** Handles request to choose location. */
        void sltChooseLocationPath();
        /** Handles description text change. */
        void sltDescriptionTextChanged();
        /** Handles size editor change. */
        void sltSizeValueChanged(qulonglong uSize);

        /** Handles button-box button click. */
        void sltHandleButtonBoxClick(QAbstractButton *pButton);
    /** @} */

private:

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares this. */
        void prepareThis();
        /** Prepares tab-widget. */
        void prepareTabWidget();
        /** Prepares 'Options' tab. */
        void prepareTabOptions();
        /** Prepares 'Details' tab. */
        void prepareTabDetails();
        /** Prepares information-container. */
        void prepareInformationContainer(UIMediumType enmType, int cFields);
    /** @} */

    /** @name Loading stuff.
      * @{ */
        /** Load options data. */
        void loadDataForOptions();
        /** Load details data. */
        void loadDataForDetails();
    /** @} */

    /** @name Options stuff.
      * @{ */
        /** Revalidates changes for passed @a pWidget. */
        void revalidate(QWidget *pWidget = 0);
        /** Retranslates validation for passed @a pWidget. */
        void retranslateValidation(QWidget *pWidget = 0);
        /** Updates button states. */
        void updateButtonStates();

        /** Returns tool-tip for passed medium @a enmType. */
        static QString mediumTypeTip(KMediumType enmType);
    /** @} */

    /** @name Details stuff.
      * @{ */
        /** Returns information-container for passed medium @a enmType. */
        QWidget *infoContainer(UIMediumType enmType) const;
        /** Returns information-label for passed medium @a enmType and @a iIndex. */
        QLabel *infoLabel(UIMediumType enmType, int iIndex) const;
        /** Returns information-field for passed medium @a enmType and @a iIndex. */
        QILabel *infoField(UIMediumType enmType, int iIndex) const;
    /** @} */

    /** @name General variables.
      * @{ */
        /** Holds the parent reference. */
        UIMediumManagerWidget *m_pParent;
        /** Holds the parent widget embedding type. */
        const EmbedTo m_enmEmbedding;

        /** Holds the old data copy. */
        UIDataMedium  m_oldData;
        /** Holds the new data copy. */
        UIDataMedium  m_newData;

        /** Holds the tab-widget. */
        QITabWidget *m_pTabWidget;
    /** @} */

    /** @name Options variables.
      * @{ */
        /** Holds the type label. */
        QLabel    *m_pLabelType;
        /** Holds the type combo-box. */
        QComboBox *m_pComboBoxType;
        /** Holds the type error pane. */
        QLabel    *m_pErrorPaneType;

        /** Holds the location label. */
        QLabel       *m_pLabelLocation;
        /** Holds the location editor. */
        QLineEdit    *m_pEditorLocation;
        /** Holds the location error pane. */
        QLabel       *m_pErrorPaneLocation;
        /** Holds the location button. */
        QIToolButton *m_pButtonLocation;

        /** Holds the description label. */
        QLabel    *m_pLabelDescription;
        /** Holds the description editor. */
        QTextEdit *m_pEditorDescription;
        /** Holds the description error pane. */
        QLabel    *m_pErrorPaneDescription;

        /** Holds the size label. */
        QLabel             *m_pLabelSize;
        /** Holds the size editor. */
        UIMediumSizeEditor *m_pEditorSize;
        /** Holds the size error pane. */
        QLabel             *m_pErrorPaneSize;

        /** Holds the button-box instance. */
        QIDialogButtonBox        *m_pButtonBox;
        /** Holds the progress-bar widget instance. */
        UIEnumerationProgressBar *m_pProgressBar;

        /** Holds whether options are valid. */
        bool m_fValid;
    /** @} */

    /** @name Details variables.
      * @{ */
        /** Holds the details layout: */
        QStackedLayout *m_pLayoutDetails;

        /** Holds the map of information-container instances. */
        QMap<UIMediumType, QWidget*>          m_aContainers;
        /** Holds the map of information-container label instances. */
        QMap<UIMediumType, QList<QLabel*> >   m_aLabels;
        /** Holds the information-container field instances. */
        QMap<UIMediumType, QList<QILabel*> >  m_aFields;
    /** @} */
};

#endif /* !___UIMediumDetailsWidget_h___ */

