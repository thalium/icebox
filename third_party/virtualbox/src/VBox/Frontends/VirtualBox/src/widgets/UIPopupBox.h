/* $Id: UIPopupBox.h $ */
/** @file
 * VBox Qt GUI - UIPopupBox/UIPopupBoxGroup classes declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIPopupBoxStuff_h__
#define __UIPopupBoxStuff_h__

/* Global includes: */
#include <QIcon>
#include <QWidget>

/* Forward declarations: */
class QLabel;

/* QWidget reimplementation,
 * wraps content-widget with nice/collapsable frame: */
class UIPopupBox : public QWidget
{
    Q_OBJECT;

signals:

    /* Signals: */
    void sigTitleClicked(const QString &strLink);
    void sigToggled(bool fOpened);
    void sigUpdateContentWidget();
    void sigGotHover();

public:

    /* Constructor/destructor: */
    UIPopupBox(QWidget *pParent);
    ~UIPopupBox();

    /* Title-icon stuff: */
    void setTitleIcon(const QIcon &icon);
    QIcon titleIcon() const;

    /* Warning-icon stuff: */
    void setWarningIcon(const QIcon &icon);
    QIcon warningIcon() const;

    /* Title stuff: */
    void setTitle(const QString &strTitle);
    QString title() const;

    /* Link stuff: */
    void setTitleLink(const QString &strLink);
    QString titleLink() const;
    void setTitleLinkEnabled(bool fEnabled);
    bool isTitleLinkEnabled() const;

    /* Content-widget stuff: */
    void setContentWidget(QWidget *pWidget);
    QWidget* contentWidget() const;

    /* Toggle stuff: */
    void setOpen(bool fOpen);
    void toggleOpen();
    bool isOpen() const;

    /* Update stuff: */
    void callForUpdateContentWidget() { emit sigUpdateContentWidget(); }

protected:

    /* Event filter: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Event handlers: */
    void resizeEvent(QResizeEvent *pEvent);
    void mouseDoubleClickEvent(QMouseEvent *pEvent);
    void paintEvent(QPaintEvent *pEvent);

private:

    /* Helpers: */
    void updateTitleIcon();
    void updateWarningIcon();
    void updateTitle();
    void updateHover();
    void revokeHover();
    void toggleHover(bool fHeaderHover);
    void recalc();

    /* Widgets: */
    QLabel *m_pTitleIcon;
    QLabel *m_pWarningIcon;
    QLabel *m_pTitleLabel;

    /* Variables: */
    QIcon m_titleIcon;
    QIcon m_warningIcon;
    QString m_strTitle;
    QString m_strLink;
    bool m_fLinkEnabled;
    QWidget *m_pContentWidget;
    bool m_fOpen;
    QPainterPath *m_pLabelPath;
    const int m_iArrowWidth;
    QPainterPath m_arrowPath;
    bool m_fHeaderHover;

    /* Friend class: */
    friend class UIPopupBoxGroup;
};

/* QObject reimplementation,
 * provides a container to organize groups of popup-boxes: */
class UIPopupBoxGroup : public QObject
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    UIPopupBoxGroup(QObject *pParent);
    ~UIPopupBoxGroup();

    /* Add popup-box: */
    void addPopupBox(UIPopupBox *pPopupBox);

private slots:

    /* Hover-change handler: */
    void sltHoverChanged();

private:

    /* Variables: */
    QList<UIPopupBox*> m_list;
};

#endif /* !__UIPopupBoxStuff_h__ */

