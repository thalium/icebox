/* $Id: UIMediumTypeChangeDialog.h $ */
/** @file
 * VBox Qt GUI - UIMediumTypeChangeDialog class declaration.
 */

/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMediumTypeChangeDialog_h___
#define ___UIMediumTypeChangeDialog_h___

/* GUI includes: */
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Forward declarations: */
class QGroupBox;
class QRadioButton;
class QVBoxLayout;
class QIDialogButtonBox;
class QILabel;


/** QIDialog extension
  * providing GUI with possibility to change particular medium type. */
class UIMediumTypeChangeDialog : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /** Constructs the dialog passing @a pParent to the base-class.
      * @param  strMediumID  Brings the ID of the medium to be modified. */
    UIMediumTypeChangeDialog(QWidget *pParent, const QString &strMediumID);

protected:

    /** Handles translation event. */
    void retranslateUi() /* override */;

protected slots:

    /** Accepts the dialog. */
    void sltAccept();
    /** Rejects the dialog. */
    void sltReject();

    /** Performes the dialog validation. */
    void sltValidate();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares medium-type radio-buttons. */
    void prepareMediumTypeButtons();
    /** Prepares radio-button for the passed @a mediumType. */
    void prepareMediumTypeButton(KMediumType mediumType);

    /** Updates the details-pane. */
    void updateDetailsPane();

    /** Holds the medium ID reference. */
    const QString &m_strMediumID;
    /** Holds the medium instance to be modified. */
    CMedium m_medium;
    /** Holds the old medium type. */
    KMediumType m_enmMediumTypeOld;
    /** Holds the new medium type. */
    KMediumType m_enmMediumTypeNew;

    /** Holds the description label instance. */
    QILabel *m_pLabel;
    /** Holds the group-box instance. */
    QGroupBox *m_pGroupBox;
    /** Holds the button layout instance. */
    QVBoxLayout *m_pButtonLayout;
    /** Holds the details-pane instance. */
    QILabel *m_pDetailsPane;
    /** Holds the button-box instance. */
    QIDialogButtonBox *m_pButtonBox;
};

#endif /* !___UIMediumTypeChangeDialog_h___ */

