/* $Id: UIGraphicsToolBar.h $ */
/** @file
 * VBox Qt GUI - UIGraphicsToolBar class declaration.
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

#ifndef __UIGraphicsToolBar_h__
#define __UIGraphicsToolBar_h__

/* GUI includes: */
#include "QIGraphicsWidget.h"

/* Forward declarations: */
class UIGraphicsButton;

/* Graphics tool-bar: */
class UIGraphicsToolBar : public QIGraphicsWidget
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIGraphicsToolBar(QIGraphicsWidget *pParent, int iRows, int iColumns);

    /* API: Margin stuff: */
    int toolBarMargin() const;
    void setToolBarMargin(int iMargin);

    /* API: Children stuff: */
    void insertItem(UIGraphicsButton *pButton, int iRow, int iColumn);

    /* API: Layout stuff: */
    void updateLayout();

protected:

    /* Typedefs: */
    typedef QPair<int, int> UIGraphicsToolBarIndex;

    /* Helpers: Layout stuff: */
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:

    /* Variables: */
    int m_iMargin;
    int m_iRows;
    int m_iColumns;
    QMap<UIGraphicsToolBarIndex, UIGraphicsButton*> m_buttons;
};

#endif /* __UIGraphicsToolBar_h__ */

