/* $Id: QILabel.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: QILabel class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QApplication>
# include <QClipboard>
# include <QContextMenuEvent>
# include <QFocusEvent>
# include <QMenu>
# include <QMimeData>
# include <QMouseEvent>
# include <QPainter>
# include <QStyleOptionFocusRect>
# include <QDrag>

/* GUI includes: */
# include "QILabel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* Some constant regular expressions */
const QRegExp QILabel::mCopyRegExp = QRegExp ("<[^>]*>");
QRegExp QILabel::mElideRegExp = QRegExp ("(<compact\\s+elipsis=\"(start|middle|end)\"?>([^<]*)</compact>)");

#define HOR_PADDING 1

QILabel::QILabel (QWidget *aParent /* = 0 */, Qt::WindowFlags aFlags /* = 0 */)
    : QLabel (aParent, aFlags)
{
    init();
}

QILabel::QILabel (const QString &aText, QWidget *aParent /* = 0 */, Qt::WindowFlags aFlags /* = 0 */)
    : QLabel (aParent, aFlags)
{
    init();
    setFullText (aText);
}

bool QILabel::fullSizeSelection () const
{
    return mFullSizeSelection;
}

void QILabel::setFullSizeSelection (bool aEnabled)
{
    mFullSizeSelection = aEnabled;
    if (mFullSizeSelection)
    {
        /* Enable mouse interaction only */
        setTextInteractionFlags (Qt::LinksAccessibleByMouse);
        /* The label should be able to get the focus */
        setFocusPolicy (Qt::StrongFocus);
        /* Change the appearance in the focus state a little bit.
         * Note: Unfortunately QLabel, precisely the text of a QLabel isn't
         * styleable. The trolls have forgotten the simplest case ... So this
         * is done by changing the currently used palette in the In/Out-focus
         * events below. Next broken feature is drawing a simple dotted line
         * around the label. So this is done manually in the paintEvent. Not
         * sure if the stylesheet stuff is ready for production environments. */
        setStyleSheet (QString ("QLabel::focus {\
                                background-color: palette(highlight);\
                                }\
                                QLabel {\
                                padding: 0px %1px 0px %1px;\
                                }").arg (HOR_PADDING));
    }
    else
    {
        /* Text should be selectable/copyable */
        setTextInteractionFlags (Qt::TextBrowserInteraction);
        /* No Focus an the label */
        setFocusPolicy (Qt::NoFocus);
        /* No focus style change */
        setStyleSheet ("");
    }
}

void QILabel::useSizeHintForWidth (int aWidthHint) const
{
    mWidthHint = aWidthHint;
    updateSizeHint();
}

QSize QILabel::sizeHint() const
{
    if (!mIsHintValid)
        updateSizeHint();

    /* If there is an updated sizeHint() present - using it. */
    return mOwnSizeHint.isValid() ? mOwnSizeHint : QLabel::sizeHint();
}

QSize QILabel::minimumSizeHint() const
{
    if (!mIsHintValid)
        updateSizeHint();

    /* If there is an updated minimumSizeHint() present - using it. */
    return mOwnSizeHint.isValid() ? mOwnSizeHint : QLabel::minimumSizeHint();
}

QString QILabel::text() const
{
    return mText;
}

void QILabel::clear()
{
    QLabel::clear();
    setFullText ("");
}

void QILabel::setText (const QString &aText)
{
    setFullText (aText);

    /* If QILabel forced to be fixed vertically */
    if (minimumHeight() == maximumHeight())
    {
        /* Check if new text requires label growing */
        QSize sh (width(), heightForWidth (width()));
        if (sh.height() > minimumHeight())
            setFixedHeight (sh.height());
    }
}

void QILabel::copy()
{
    QString text = removeHtmlTags (mText);
    /* Copy the current text to the global and selection clipboard. */
    QApplication::clipboard()->setText (text, QClipboard::Clipboard);
    QApplication::clipboard()->setText (text, QClipboard::Selection);
}

void QILabel::resizeEvent (QResizeEvent *aEvent)
{
    QLabel::resizeEvent (aEvent);
    /* Recalculate the elipsis of the text after every resize. */
    updateText();
}

void QILabel::mousePressEvent (QMouseEvent *aEvent)
{
    if (aEvent->button() == Qt::LeftButton && geometry().contains (aEvent->pos()) && mFullSizeSelection)
        mStartDragging = true;
    else
        QLabel::mousePressEvent (aEvent);
}

void QILabel::mouseReleaseEvent (QMouseEvent *aEvent)
{
    mStartDragging = false;
    QLabel::mouseReleaseEvent (aEvent);
}

void QILabel::mouseMoveEvent (QMouseEvent *aEvent)
{
    if (mStartDragging)
    {
        mStartDragging = false;
        /* Create a drag object out of the given data. */
        QDrag *drag = new QDrag (this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText (removeHtmlTags (mText));
        drag->setMimeData (mimeData);
        /* Start the dragging */
        drag->exec();
    }
    else
        QLabel::mouseMoveEvent (aEvent);
}

void QILabel::contextMenuEvent (QContextMenuEvent *aEvent)
{
    if (mFullSizeSelection)
    {
        /* Create a context menu for the copy to clipboard action. */
        QMenu menu;
        mCopyAction->setText (tr ("&Copy"));
        menu.addAction (mCopyAction);
        menu.exec (aEvent->globalPos());
    }
    else
        QLabel::contextMenuEvent (aEvent);
}

void QILabel::focusInEvent (QFocusEvent * /* aEvent */)
{
    if (mFullSizeSelection)
    {
        /* Set the text color to the current used highlight text color. */
        QPalette pal = qApp->palette();
        pal.setBrush (QPalette::WindowText, pal.brush (QPalette::HighlightedText));
        setPalette (pal);
    }
}

void QILabel::focusOutEvent (QFocusEvent *aEvent)
{
    /* Reset to the default palette */
    if (mFullSizeSelection && aEvent->reason() != Qt::PopupFocusReason)
        setPalette (qApp->palette());
}

void QILabel::paintEvent (QPaintEvent *aEvent)
{
    QLabel::paintEvent (aEvent);

    if (mFullSizeSelection && hasFocus())
    {
        QPainter painter (this);
        /* Paint a focus rect based on the current style. */
        QStyleOptionFocusRect option;
        option.initFrom (this);
        style()->drawPrimitive (QStyle::PE_FrameFocusRect, &option, &painter, this);
    }
}

void QILabel::init()
{
    /* Initial setup */
    mIsHintValid = false;
    mWidthHint = -1;
    mStartDragging = false;
    setFullSizeSelection (false);
    setOpenExternalLinks (true);

    /* Create invisible copy action */
    mCopyAction = new QAction (this);
    addAction (mCopyAction);
    mCopyAction->setShortcut (QKeySequence (QKeySequence::Copy));
    mCopyAction->setShortcutContext (Qt::WidgetShortcut);
    connect(mCopyAction, &QAction::triggered, this, &QILabel::copy);
}

void QILabel::updateSizeHint() const
{
    mOwnSizeHint = mWidthHint == -1 ? QSize() : QSize (mWidthHint, heightForWidth (mWidthHint));
    mIsHintValid = true;
}

void QILabel::setFullText (const QString &aText)
{
    QSizePolicy sp = sizePolicy();
    sp.setHeightForWidth (wordWrap());
    setSizePolicy (sp);
    mIsHintValid = false;

    mText = aText;
    updateText();
}

void QILabel::updateText()
{
    QString comp = compressText (mText);

    QLabel::setText (comp);
    /* Only set the tooltip if the text is shortened in any way. */
    if (removeHtmlTags (comp) != removeHtmlTags (mText))
        setToolTip (removeHtmlTags (mText));
    else
        setToolTip ("");
}

QString QILabel::removeHtmlTags (QString aText) const
{
    /* Remove all HTML tags from the text and return it. */
    return QString(aText).remove (mCopyRegExp);
}

Qt::TextElideMode QILabel::toTextElideMode (const QString& aStr) const
{
    /* Converts a string to a Qt elide mode */
    Qt::TextElideMode mode = Qt::ElideNone;
    if (aStr == "start")
        mode = Qt::ElideLeft;
    else if (aStr == "middle")
        mode = Qt::ElideMiddle;
    else if (aStr == "end")
        mode  = Qt::ElideRight;
    return mode;
}

QString QILabel::compressText (const QString &aText) const
{
    QStringList strResult;
    QFontMetrics fm = fontMetrics();
    /* Split up any multi line text */
    QStringList strList = aText.split (QRegExp ("<br */?>"));
    foreach (QString text, strList)
    {
        /* Search for the compact tag */
        if (mElideRegExp.indexIn (text) > -1)
        {
            QString workStr = text;
            /* Grep out the necessary info of the regexp */
            QString compactStr = mElideRegExp.cap (1);
            QString elideModeStr = mElideRegExp.cap (2);
            QString elideStr = mElideRegExp.cap (3);
            /* Remove the whole compact tag (also the text) */
            QString flatStr = removeHtmlTags (QString (workStr).remove (compactStr));
            /* What size will the text have without the compact text */
            int flatWidth = fm.width (flatStr);
            /* Create the shortened text */
            QString newStr = fm.elidedText (elideStr, toTextElideMode (elideModeStr), width() - (2 * HOR_PADDING) - flatWidth);
            /* Replace the compact part with the shortened text in the initial string */
            text = QString (workStr).replace (compactStr, newStr);
        }
        strResult << text;
    }
    return strResult.join ("<br />");
}

