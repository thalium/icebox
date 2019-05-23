/* $Id: VBoxGuestRAMSlider.h $ */
/** @file
 * VBox Qt GUI - VBoxGuestRAMSlider class declaration.
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


#ifndef __VBoxGuestRAMSlider_h__
#define __VBoxGuestRAMSlider_h__

/* VBox includes */
#include "QIAdvancedSlider.h"

class VBoxGuestRAMSlider: public QIAdvancedSlider
{
public:
    VBoxGuestRAMSlider (QWidget *aParent = 0);
    VBoxGuestRAMSlider (Qt::Orientation aOrientation, QWidget *aParent = 0);

    uint minRAM() const;
    uint maxRAMOpt() const;
    uint maxRAMAlw() const;
    uint maxRAM() const;

private:
    /* Private methods */
    void init();
    int calcPageStep (int aMax) const;

    /* Private member vars */
    uint mMinRAM;
    uint mMaxRAMOpt;
    uint mMaxRAMAlw;
    uint mMaxRAM;
};

#endif /* __VBoxGuestRAMSlider_h__ */

