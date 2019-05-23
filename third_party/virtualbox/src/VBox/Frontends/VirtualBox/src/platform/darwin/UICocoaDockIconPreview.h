/* $Id: UICocoaDockIconPreview.h $ */
/** @file
 * VBox Qt GUI - UICocoaDockIconPreview class declaration.
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

#ifndef ___UICocoaDockIconPreview_h___
#define ___UICocoaDockIconPreview_h___

/* Qt includes */
#include "UIAbstractDockIconPreview.h"

class UICocoaDockIconPreviewPrivate;

class UICocoaDockIconPreview: public UIAbstractDockIconPreview
{
public:
    UICocoaDockIconPreview(UISession *pSession, const QPixmap& overlayImage);
    ~UICocoaDockIconPreview();

    virtual void updateDockOverlay();
    virtual void updateDockPreview(CGImageRef VMImage);
    virtual void updateDockPreview(UIFrameBuffer *pFrameBuffer);

    virtual void setOriginalSize(int aWidth, int aHeight);

private:
    UICocoaDockIconPreviewPrivate *d;
};

#endif /* ___UICocoaDockIconPreview_h___ */

