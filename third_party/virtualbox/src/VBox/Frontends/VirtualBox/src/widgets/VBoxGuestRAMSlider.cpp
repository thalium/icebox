/* $Id: VBoxGuestRAMSlider.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: VBoxGuestRAMSlider class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "VBoxGuestRAMSlider.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


VBoxGuestRAMSlider::VBoxGuestRAMSlider (QWidget *aParent /* = 0 */)
  : QIAdvancedSlider (aParent)
  , mMinRAM (0)
  , mMaxRAMOpt (0)
  , mMaxRAMAlw (0)
  , mMaxRAM (0)
{
    init();
}

VBoxGuestRAMSlider::VBoxGuestRAMSlider (Qt::Orientation aOrientation, QWidget *aParent /* = 0 */)
  : QIAdvancedSlider (aOrientation, aParent)
  , mMinRAM (0)
  , mMaxRAMOpt (0)
  , mMaxRAMAlw (0)
  , mMaxRAM (0)
{
    init();
}

uint VBoxGuestRAMSlider::minRAM() const
{
    return mMinRAM;
}

uint VBoxGuestRAMSlider::maxRAMOpt() const
{
    return mMaxRAMOpt;
}

uint VBoxGuestRAMSlider::maxRAMAlw() const
{
    return mMaxRAMAlw;
}

uint VBoxGuestRAMSlider::maxRAM() const
{
    return mMaxRAM;
}

void VBoxGuestRAMSlider::init()
{
    ulong fullSize = vboxGlobal().host().GetMemorySize();
    CSystemProperties sys = vboxGlobal().virtualBox().GetSystemProperties();
    mMinRAM = sys.GetMinGuestRAM();
    mMaxRAM = RT_MIN (RT_ALIGN (fullSize, _1G / _1M), sys.GetMaxGuestRAM());

    /* Come up with some nice round percent boundaries relative to
     * the system memory. A max of 75% on a 256GB config is ridiculous,
     * even on an 8GB rig reserving 2GB for the OS is way to conservative.
     * The max numbers can be estimated using the following program:
     *
     *      double calcMaxPct(uint64_t cbRam)
     *      {
     *          double cbRamOverhead = cbRam * 0.0390625; // 160 bytes per page.
     *          double cbRamForTheOS = RT_MAX(RT_MIN(_512M, cbRam * 0.25), _64M);
     *          double OSPct  = (cbRamOverhead + cbRamForTheOS) * 100.0 / cbRam;
     *          double MaxPct = 100 - OSPct;
     *          return MaxPct;
     *      }
     *
     *      int main()
     *      {
     *          uint64_t cbRam = _1G;
     *          for (; !(cbRam >> 33); cbRam += _1G)
     *              printf("%8lluGB %.1f%% %8lluKB\n", cbRam >> 30, calcMaxPct(cbRam),
     *                     (uint64_t)(cbRam * calcMaxPct(cbRam) / 100.0) >> 20);
     *          for (; !(cbRam >> 51); cbRam <<= 1)
     *              printf("%8lluGB %.1f%% %8lluKB\n", cbRam >> 30, calcMaxPct(cbRam),
     *                     (uint64_t)(cbRam * calcMaxPct(cbRam) / 100.0) >> 20);
     *          return 0;
     *      }
     *
     * Note. We might wanna put these calculations somewhere global later. */

    /* System RAM amount test */
    mMaxRAMAlw  = (uint)(0.75 * fullSize);
    mMaxRAMOpt  = (uint)(0.50 * fullSize);
    if (fullSize < 3072)
        /* done */;
    else if (fullSize < 4096)   /* 3GB */
        mMaxRAMAlw = (uint)(0.80 * fullSize);
    else if (fullSize < 6144)   /* 4-5GB */
    {
        mMaxRAMAlw = (uint)(0.84 * fullSize);
        mMaxRAMOpt = (uint)(0.60 * fullSize);
    }
    else if (fullSize < 8192)   /* 6-7GB */
    {
        mMaxRAMAlw = (uint)(0.88 * fullSize);
        mMaxRAMOpt = (uint)(0.65 * fullSize);
    }
    else if (fullSize < 16384)  /* 8-15GB */
    {
        mMaxRAMAlw = (uint)(0.90 * fullSize);
        mMaxRAMOpt = (uint)(0.70 * fullSize);
    }
    else if (fullSize < 32768)  /* 16-31GB */
    {
        mMaxRAMAlw = (uint)(0.93 * fullSize);
        mMaxRAMOpt = (uint)(0.75 * fullSize);
    }
    else if (fullSize < 65536)  /* 32-63GB */
    {
        mMaxRAMAlw = (uint)(0.94 * fullSize);
        mMaxRAMOpt = (uint)(0.80 * fullSize);
    }
    else if (fullSize < 131072) /* 64-127GB */
    {
        mMaxRAMAlw = (uint)(0.95 * fullSize);
        mMaxRAMOpt = (uint)(0.85 * fullSize);
    }
    else                        /* 128GB- */
    {
        mMaxRAMAlw = (uint)(0.96 * fullSize);
        mMaxRAMOpt = (uint)(0.90 * fullSize);
    }
    /* Now check the calculated maximums are out of the range for the guest
     * RAM. If so change it accordingly. */
    mMaxRAMAlw  = RT_MIN (mMaxRAMAlw, mMaxRAM);
    mMaxRAMOpt  = RT_MIN (mMaxRAMOpt, mMaxRAM);

    setPageStep (calcPageStep (mMaxRAM));
    setSingleStep (pageStep() / 4);
    setTickInterval (pageStep());
    /* Setup the scale so that ticks are at page step boundaries */
    setMinimum ((mMinRAM / pageStep()) * pageStep());
    setMaximum (mMaxRAM);
    setSnappingEnabled (true);
    setOptimalHint (mMinRAM, mMaxRAMOpt);
    setWarningHint (mMaxRAMOpt, mMaxRAMAlw);
    setErrorHint (mMaxRAMAlw, mMaxRAM);
}

/**
 *  Calculates a suitable page step size for the given max value. The returned
 *  size is so that there will be no more than 32 pages. The minimum returned
 *  page size is 4.
 */
int VBoxGuestRAMSlider::calcPageStep (int aMax) const
{
    /* reasonable max. number of page steps is 32 */
    uint page = ((uint) aMax + 31) / 32;
    /* make it a power of 2 */
    uint p = page, p2 = 0x1;
    while ((p >>= 1))
        p2 <<= 1;
    if (page != p2)
        p2 <<= 1;
    if (p2 < 4)
        p2 = 4;
    return (int) p2;
}

