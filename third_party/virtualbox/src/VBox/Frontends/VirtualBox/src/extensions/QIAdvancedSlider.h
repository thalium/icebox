/* $Id: QIAdvancedSlider.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIAdvancedSlider class implementation
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

#ifndef __QIAdvancedSlider_h__
#define __QIAdvancedSlider_h__

/* Qt includes */
#include <QSlider>

class CPrivateSlider;

class QIAdvancedSlider: public QWidget
{
    Q_OBJECT;
    Q_PROPERTY(int value READ value WRITE setValue);

public:
    QIAdvancedSlider(QWidget *pParent = 0);
    QIAdvancedSlider(Qt::Orientation fOrientation, QWidget *pParent = 0);

    int value() const;

    void setRange(int minV, int maxV);

    void setMaximum(int val);
    int maximum() const;

    void setMinimum(int val);
    int minimum() const;

    void setPageStep(int val);
    int pageStep() const;

    void setSingleStep(int val);
    int singelStep() const;

    void setTickInterval(int val);
    int tickInterval() const;

    Qt::Orientation orientation() const;

    void setSnappingEnabled(bool fOn);
    bool isSnappingEnabled() const;

    void setOptimalHint(int min, int max);
    void setWarningHint(int min, int max);
    void setErrorHint(int min, int max);

public slots:

    void setOrientation(Qt::Orientation fOrientation);
    void setValue(int val);

signals:
    void valueChanged(int);
    void sliderMoved(int);
    void sliderPressed();
    void sliderReleased();

private slots:

    void sltSliderMoved(int val);

private:

    void init(Qt::Orientation fOrientation = Qt::Horizontal);
    int snapValue(int val);

    /* Private member vars */
    CPrivateSlider *m_pSlider;
    bool m_fSnappingEnabled;
};

#endif /* __QIAdvancedSlider__ */

