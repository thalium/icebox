/* $Id: QIArrowButtonPress.h $ */
/** @file
 * VBox Qt GUI - QIArrowButtonPress class declaration.
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

#ifndef ___QIArrowButtonPress_h___
#define ___QIArrowButtonPress_h___

/* GUI includes: */
#include "QIRichToolButton.h"
#include "QIWithRetranslateUI.h"

/** QIRichToolButton extension
  * representing arrow tool-button with text-label,
  * can be used as back/next buttons in various places. */
class QIArrowButtonPress : public QIWithRetranslateUI<QIRichToolButton>
{
    Q_OBJECT;

public:

    /** Button types. */
    enum ButtonType { ButtonType_Back, ButtonType_Next };

    /** Constructor, passes @a pParent to the QIRichToolButton constructor.
      * @param buttonType is used to define which type of the button it is. */
    QIArrowButtonPress(ButtonType buttonType, QWidget *pParent = 0);

protected:

    /** Retranslation routine. */
    virtual void retranslateUi();

    /** Key-press-event handler. */
    virtual void keyPressEvent(QKeyEvent *pEvent);

private:

    /** Holds the button-type. */
    ButtonType m_buttonType;
};

#endif /* !___QIArrowButtonPress_h___ */
