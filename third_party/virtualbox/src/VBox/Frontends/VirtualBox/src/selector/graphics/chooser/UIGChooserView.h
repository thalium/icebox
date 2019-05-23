/* $Id: UIGChooserView.h $ */
/** @file
 * VBox Qt GUI - UIGChooserView class declaration.
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

#ifndef __UIGChooserView_h__
#define __UIGChooserView_h__

/* GUI includes: */
#include "QIGraphicsView.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIGChooser;
class UIGChooserItem;

/* Graphics chooser-view: */
class UIGChooserView : public QIWithRetranslateUI<QIGraphicsView>
{
    Q_OBJECT;

signals:

    /* Notifier: Resize stuff: */
    void sigResized();

public:

    /** Constructs a chooser-view passing @a pParent to the base-class.
      * @param  pParent  Brings the chooser container to embed into. */
    UIGChooserView(UIGChooser *pParent);

    /** Returns the chooser reference. */
    UIGChooser *chooser() const { return m_pChooser; }

private slots:

    /* Handlers: Size-hint stuff: */
    void sltMinimumWidthHintChanged(int iMinimumWidthHint);
    void sltMinimumHeightHintChanged(int iMinimumHeightHint);

    /* Handler: Focus-item stuff: */
    void sltFocusChanged(UIGChooserItem *pFocusItem);

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /* Handler: Resize-event stuff: */
    void resizeEvent(QResizeEvent *pEvent);

    /* Helper: Update stuff: */
    void updateSceneRect();

    /** Holds the chooser reference. */
    UIGChooser *m_pChooser;

    /* Variables: */
    int m_iMinimumWidthHint;
    int m_iMinimumHeightHint;
};

#endif /* __UIGChooserView_h__ */

