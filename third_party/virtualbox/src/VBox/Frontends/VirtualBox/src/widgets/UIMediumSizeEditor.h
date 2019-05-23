/* $Id: UIMediumSizeEditor.h $ */
/** @file
 * VBox Qt GUI - UIMediumSizeEditor class declaration.
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

#ifndef ___UIMediumSizeEditor_h___
#define ___UIMediumSizeEditor_h___

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QLabel;
class QSlider;
class QILineEdit;


/** Medium size editor widget. */
class UIMediumSizeEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about medium size changed. */
    void sigSizeChanged(qulonglong uSize);

public:

    /** Constructs medium size editor passing @a pParent to the base-class. */
    UIMediumSizeEditor(QWidget *pParent = 0);

    /** Returns the medium size. */
    qulonglong mediumSize() const { return m_uSize; }
    /** Defines the @a uSize. */
    void setMediumSize(qulonglong uSize);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles size slider change. */
    void sltSizeSliderChanged(int iValue);
    /** Handles size editor change. */
    void sltSizeEditorChanged(const QString &strValue);

private:

    /** Prepares all. */
    void prepare();

    /** Calculates slider scale according to passed @a uMaximumMediumSize. */
    static int calculateSliderScale(qulonglong uMaximumMediumSize);
    /** Returns log2 for passed @a uValue. */
    static int log2i(qulonglong uValue);
    /** Converts passed bytes @a uValue to slides scaled value using @a iSliderScale. */
    static int sizeMBToSlider(qulonglong uValue, int iSliderScale);
    /** Converts passed slider @a uValue to bytes unscaled value using @a iSliderScale. */
    static qulonglong sliderToSizeMB(int uValue, int iSliderScale);
    /** Updates slider/editor tool-tips. */
    void updateSizeToolTips(qulonglong uSize);

    /** Holds the minimum medium size. */
    const qulonglong  m_uSizeMin;
    /** Holds the maximum medium size. */
    const qulonglong  m_uSizeMax;
    /** Holds the slider scale. */
    const int         m_iSliderScale;
    /** Holds the current medium size. */
    qulonglong        m_uSize;

    /** Holds the size slider. */
    QSlider    *m_pSlider;
    /** Holds the minimum size label. */
    QLabel     *m_pLabelMinSize;
    /** Holds the maximum size label. */
    QLabel     *m_pLabelMaxSize;
    /** Holds the size editor. */
    QILineEdit *m_pEditor;
};

#endif /* !___UIMediumSizeEditor_h___ */

