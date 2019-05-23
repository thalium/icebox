/* $Id: QIMessageBox.h $ */
/** @file
 * VBox Qt GUI - QIMessageBox class declaration.
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

#ifndef ___QIMessageBox_h___
#define ___QIMessageBox_h___

/* Qt includes: */
#include <QMessageBox>

/* GUI includes: */
#include "QIDialog.h"

/* Forward declarations: */
class QCheckBox;
class QIArrowSplitter;
class QIDialogButtonBox;
class QILabel;
class QLabel;
class QPushButton;

/** Button types. */
enum AlertButton
{
    AlertButton_NoButton      =  0x0,  /* 00000000 00000000 */
    AlertButton_Ok            =  0x1,  /* 00000000 00000001 */
    AlertButton_Cancel        =  0x2,  /* 00000000 00000010 */
    AlertButton_Choice1       =  0x4,  /* 00000000 00000100 */
    AlertButton_Choice2       =  0x8,  /* 00000000 00001000 */
    AlertButton_Copy          = 0x10,  /* 00000000 00010000 */
    AlertButtonMask           = 0xFF   /* 00000000 11111111 */
};

/** Button options. */
enum AlertButtonOption
{
    AlertButtonOption_Default = 0x100, /* 00000001 00000000 */
    AlertButtonOption_Escape  = 0x200, /* 00000010 00000000 */
    AlertButtonOptionMask     = 0x300  /* 00000011 00000000 */
};

/** Alert options. */
enum AlertOption
{
    AlertOption_AutoConfirmed = 0x400, /* 00000100 00000000 */
    AlertOption_CheckBox      = 0x800, /* 00001000 00000000 */
    AlertOptionMask           = 0xFC00 /* 11111100 00000000 */
};

/** Icon types. */
enum AlertIconType
{
    AlertIconType_NoIcon         = QMessageBox::NoIcon,
    AlertIconType_Information    = QMessageBox::Information,
    AlertIconType_Warning        = QMessageBox::Warning,
    AlertIconType_Critical       = QMessageBox::Critical,
    AlertIconType_Question       = QMessageBox::Question,
    AlertIconType_GuruMeditation
};

/** QIDialog extension
  * representing GUI alerts. */
class QIMessageBox : public QIDialog
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the QIDialog constructor.
      * @param strTitle   defines title,
      * @param strMessage defines message,
      * @param iconType   defines icon-type,
      * @param iButton1   specifies integer-code for the 1st button,
      * @param iButton2   specifies integer-code for the 2nd button,
      * @param iButton3   specifies integer-code for the 3rd button. */
    QIMessageBox(const QString &strTitle, const QString &strMessage, AlertIconType iconType,
                 int iButton1 = 0, int iButton2 = 0, int iButton3 = 0, QWidget *pParent = 0);

    /** Defines details-text. */
    void setDetailsText(const QString &strText);

    /** Returns whether flag is checked. */
    bool flagChecked() const;
    /** Defines whether flag is @a fChecked. */
    void setFlagChecked(bool fChecked);
    /** Defines @a strFlagText. */
    void setFlagText(const QString &strFlagText);

    /** Defines @a iButton @a strText. */
    void setButtonText(int iButton, const QString &strText);

private slots:

    /** Updates dialog size: */
    void sltUpdateSize();

    /** Copy details-text. */
    void sltCopy() const;

    /** Closes dialog like user would press the Cancel button. */
    virtual void reject();

    /** Closes dialog like user would press the 1st button. */
    void sltDone1() { m_fDone = true; done(m_iButton1 & AlertButtonMask); }
    /** Closes dialog like user would press the 2nd button. */
    void sltDone2() { m_fDone = true; done(m_iButton2 & AlertButtonMask); }
    /** Closes dialog like user would press the 3rd button. */
    void sltDone3() { m_fDone = true; done(m_iButton3 & AlertButtonMask); }

private:

    /** Prepares message-box. */
    void prepare();

    /** Prepares focus. */
    void prepareFocus();

    /** Push-button factory. */
    QPushButton* createButton(int iButton);

    /** Polish-event handler. */
    void polishEvent(QShowEvent *pPolishEvent);
    /** Close-event handler. */
    void closeEvent(QCloseEvent *pCloseEvent);

    /** Visibility update routine for details-container. */
    void updateDetailsContainer();
    /** Visibility update routine for check-box. */
    void updateCheckBox();

    /** Generates standard pixmap for passed @a iconType using @a pWidget as hint. */
    static QPixmap standardPixmap(AlertIconType iconType, QWidget *pWidget = 0);

    /** Holds the title. */
    QString m_strTitle;

    /** Holds the icon-type. */
    AlertIconType m_iconType;
    /** Holds the icon-label instance. */
    QLabel *m_pLabelIcon;

    /** Holds the message. */
    QString m_strMessage;
    /** Holds the message-label instance. */
    QILabel *m_pLabelText;

    /** Holds the flag check-box instance. */
    QCheckBox *m_pFlagCheckBox;

    /** Holds the flag details-container instance. */
    QIArrowSplitter *m_pDetailsContainer;

    /** Holds the integer-code for the 1st button. */
    int m_iButton1;
    /** Holds the integer-code for the 2nd button. */
    int m_iButton2;
    /** Holds the integer-code for the 3rd button. */
    int m_iButton3;
    /** Holds the integer-code of the cancel-button. */
    int m_iButtonEsc;
    /** Holds the 1st button instance. */
    QPushButton *m_pButton1;
    /** Holds the 2nd button instance. */
    QPushButton *m_pButton2;
    /** Holds the 3rd button instance. */
    QPushButton *m_pButton3;
    /** Holds the button-box instance. */
    QIDialogButtonBox *m_pButtonBox;

    /** Defines whether message was accepted. */
    bool m_fDone : 1;
};

#endif /* !___QIMessageBox_h___ */

