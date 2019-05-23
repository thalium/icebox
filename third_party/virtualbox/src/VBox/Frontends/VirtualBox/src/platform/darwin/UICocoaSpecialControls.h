/* $Id: UICocoaSpecialControls.h $ */
/** @file
 * VBox Qt GUI - VBoxCocoaSpecialControls class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___darwin_UICocoaSpecialControls_h__
#define ___darwin_UICocoaSpecialControls_h__

/* VBox includes */
#include "VBoxCocoaHelper.h"

/* Qt includes */
#include <QWidget>
#include <QMacCocoaViewContainer>

/* Add typedefs for Cocoa types */
ADD_COCOA_NATIVE_REF(NSButton);
ADD_COCOA_NATIVE_REF(NSSegmentedControl);
ADD_COCOA_NATIVE_REF(NSSearchField);

class UICocoaButton: public QMacCocoaViewContainer
{
    Q_OBJECT

public:
    enum CocoaButtonType
    {
        HelpButton,
        CancelButton,
        ResetButton
    };

    UICocoaButton(QWidget *pParent, CocoaButtonType type);
    ~UICocoaButton();

    QSize sizeHint() const;

    void setText(const QString& strText);
    void setToolTip(const QString& strTip);

    void onClicked();

signals:
    void clicked(bool fChecked = false);

private:
    NativeNSButtonRef nativeRef() const { return static_cast<NativeNSButtonRef>(cocoaView()); }
};

class UICocoaSegmentedButton: public QMacCocoaViewContainer
{
    Q_OBJECT

public:
    enum CocoaSegmentType
    {
        RoundRectSegment,
        TexturedRoundedSegment
    };

    UICocoaSegmentedButton(QWidget *pParent, int count, CocoaSegmentType type = RoundRectSegment);

    /** Returns the number of segments. */
    int count() const;

    /** Returns whether the @a iSegment is selected. */
    bool isSelected(int iSegment) const;

    /** Returns the @a iSegment description. */
    QString description(int iSegment) const;

    QSize sizeHint() const;

    void setTitle(int iSegment, const QString &strTitle);
    void setToolTip(int iSegment, const QString &strTip);
    void setIcon(int iSegment, const QIcon& icon);
    void setEnabled(int iSegment, bool fEnabled);

    void setSelected(int iSegment);
    void animateClick(int iSegment);
    void onClicked(int iSegment);

signals:
    void clicked(int iSegment, bool fChecked = false);

private:
    NativeNSSegmentedControlRef nativeRef() const { return static_cast<NativeNSSegmentedControlRef>(cocoaView()); }
};

class UICocoaSearchField: public QMacCocoaViewContainer
{
    Q_OBJECT

public:
    UICocoaSearchField(QWidget* pParent);
    ~UICocoaSearchField();

    QSize sizeHint() const;

    QString text() const;
    void insert(const QString &strText);
    void setToolTip(const QString &strTip);
    void selectAll();

    void markError();
    void unmarkError();

    void onTextChanged(const QString &strText);

signals:
    void textChanged(const QString& strText);

private:
    NativeNSSearchFieldRef nativeRef() const { return static_cast<NativeNSSearchFieldRef>(cocoaView()); }
};

#endif /* ___darwin_UICocoaSpecialControls_h__ */

