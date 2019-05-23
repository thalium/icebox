/* $Id: QILabel.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QILabel class declaration.
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

/*
 * This class is based on the original QLabel implementation.
 */

#ifndef __QILabel_h__
#define __QILabel_h__

/* Global includes */
#include <QLabel>

class QILabel: public QLabel
{
    Q_OBJECT;

public:

    QILabel (QWidget *aParent = 0, Qt::WindowFlags aFlags = 0);
    QILabel (const QString &aText, QWidget *aParent = 0, Qt::WindowFlags aFlags = 0);

    /* Focusing extensions */
    bool fullSizeSelection() const;
    void setFullSizeSelection (bool aEnabled);

    /* Size-Hint extensions */
    void useSizeHintForWidth (int aWidthHint) const;
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    /* Default QLabel methods */
    QString text() const;

public slots:

    void clear();
    void setText (const QString &aText);
    void copy();

protected:

    void resizeEvent (QResizeEvent *aEvent);
    void mousePressEvent (QMouseEvent *aEvent);
    void mouseReleaseEvent (QMouseEvent *aEvent);
    void mouseMoveEvent (QMouseEvent *aEvent);
    void contextMenuEvent (QContextMenuEvent *aEvent);
    void focusInEvent (QFocusEvent *aEvent);
    void focusOutEvent (QFocusEvent *aEvent);
    void paintEvent (QPaintEvent *aEvent);

private:

    void init();

    void updateSizeHint() const;
    void setFullText (const QString &aText);
    void updateText();
    QString removeHtmlTags (QString aText) const;
    Qt::TextElideMode toTextElideMode (const QString& aStr) const;
    QString compressText (const QString &aText) const;

    QString mText;
    bool mFullSizeSelection;
    static const QRegExp mCopyRegExp;
    static QRegExp mElideRegExp;
    mutable bool mIsHintValid;
    mutable int mWidthHint;
    mutable QSize mOwnSizeHint;
    bool mStartDragging;
    QAction *mCopyAction;
};

#endif // __QILabel_h__

