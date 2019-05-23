/* $Id: D3DFeatureLevel.cpp $ */
/** @file
 * ????
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * This debugging tool asks D3D API regarding to maximum supported
 * D3D10 Feature Level and dumps it to stdout.
 */

#include <iprt/win/windows.h>
#include <D3D11.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    HRESULT rc;
    D3D_FEATURE_LEVEL iFeatureLevelMax = static_cast<D3D_FEATURE_LEVEL>(0);

    /* The list of feature levels we're selecting from. */
    const D3D_FEATURE_LEVEL aiFeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    rc = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, aiFeatureLevels,
        ARRAYSIZE(aiFeatureLevels), D3D11_SDK_VERSION, NULL, &iFeatureLevelMax, NULL);

    printf("Maximum supported feature level: 0x%X, hr=0x%X.\n", iFeatureLevelMax, rc);

    return rc;
}
