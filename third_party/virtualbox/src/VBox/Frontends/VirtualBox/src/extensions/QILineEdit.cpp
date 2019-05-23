/* $Id: QILineEdit.cpp $ */
/** @file
 * VirtualBox Qt GUI - QILineEdit class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "QILineEdit.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
#include <QStyleOptionFrame>


void QILineEdit::setMinimumWidthByText (const QString &aText)
{
    setMinimumWidth (featTextWidth (aText).width());
}

void QILineEdit::setFixedWidthByText (const QString &aText)
{
    setFixedWidth (featTextWidth (aText).width());
}

QSize QILineEdit::featTextWidth (const QString &aText) const
{
    QStyleOptionFrame sof;
    sof.initFrom (this);
    sof.rect = contentsRect();
    sof.lineWidth = hasFrame() ? style()->pixelMetric (QStyle::PM_DefaultFrameWidth) : 0;
    sof.midLineWidth = 0;
    sof.state |= QStyle::State_Sunken;

    /* The margins are based on qlineedit.cpp of Qt. Maybe they where changed
     * at some time in the future. */
    QSize sc (fontMetrics().width (aText) + 2*2,
              fontMetrics().xHeight()     + 2*1);
    QSize sa = style()->sizeFromContents (QStyle::CT_LineEdit, &sof, sc, this);

    return sa;
}

