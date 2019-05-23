/* $Id: UITakeSnapshotDialog.h $ */
/** @file
 * VBox Qt GUI - UITakeSnapshotDialog class declaration.
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

#ifndef ___UITakeSnapshotDialog_h___
#define ___UITakeSnapshotDialog_h___

/* GUI includes: */
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QLabel;
class QLineEdit;
class QTextEdit;
class QIDialogButtonBox;
class QILabel;
class CMachine;


/** QIDialog subclass for taking snapshot name/description. */
class UITakeSnapshotDialog : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /** Constructs take snapshot dialog passing @ pParent to the base-class.
      * @param  comMachine  Brings the machine to take snapshot for. */
    UITakeSnapshotDialog(QWidget *pParent, const CMachine &comMachine);

    /** Defines dialog @a pixmap. */
    void setPixmap(const QPixmap &pixmap);

    /** Defines snapshot @a strName. */
    void setName(const QString &strName);

    /** Returns snapshot name. */
    QString name() const;
    /** Returns snapshot description. */
    QString description() const;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles name change signal. */
    void sltHandleNameChanged(const QString &strName);

private:

    /** Prepares all. */
    void prepare();

    /** Holds the wrapper of machine to take snapshot for. */
    const CMachine &m_comMachine;

    /** Holds the amount of immutable attachments. */
    int m_cImmutableMediums;

    /** Holds the icon label instance. */
    QLabel *m_pLabelIcon;

    /** Holds the name label instance. */
    QLabel    *m_pLabelName;
    /** Holds the name editor instance. */
    QLineEdit *m_pEditorName;

    /** Holds the description label instance. */
    QLabel    *m_pLabelDescription;
    /** Holds the description editor instance. */
    QTextEdit *m_pEditorDescription;

    /** Holds the information label instance. */
    QILabel *m_pLabelInfo;

    /** Holds the dialog button-box instance. */
    QIDialogButtonBox *m_pButtonBox;
};

#endif /* !___UITakeSnapshotDialog_h___ */

