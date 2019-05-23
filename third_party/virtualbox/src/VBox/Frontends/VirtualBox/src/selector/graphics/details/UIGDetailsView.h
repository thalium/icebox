/* $Id: UIGDetailsView.h $ */
/** @file
 * VBox Qt GUI - UIGDetailsView class declaration.
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

#ifndef __UIGDetailsView_h__
#define __UIGDetailsView_h__

/* GUI includes: */
#include "QIGraphicsView.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIGDetails;

/* Graphics details-view: */
class UIGDetailsView : public QIWithRetranslateUI<QIGraphicsView>
{
    Q_OBJECT;

signals:

    /* Notifier: Resize stuff: */
    void sigResized();

public:

    /** Constructs a details-view passing @a pParent to the base-class.
      * @param  pParent  Brings the details container to embed into. */
    UIGDetailsView(UIGDetails *pParent);

    /** Returns the details reference. */
    UIGDetails *details() const { return m_pDetails; }

private slots:

    /* Handlers: Size-hint stuff: */
    void sltMinimumWidthHintChanged(int iMinimumWidthHint);
    void sltMinimumHeightHintChanged(int iMinimumHeightHint);

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /* Helper: Prepare stuff: */
    void preparePalette();

    /* Handler: Resize-event stuff: */
    void resizeEvent(QResizeEvent *pEvent);

    /* Helper: Update stuff: */
    void updateSceneRect();

    /** Holds the details reference. */
    UIGDetails *m_pDetails;

    /* Variables: */
    int m_iMinimumWidthHint;
    int m_iMinimumHeightHint;
};

#endif /* __UIGDetailsView_h__ */

