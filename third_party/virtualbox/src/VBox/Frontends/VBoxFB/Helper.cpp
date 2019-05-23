/** @file
 *
 * VBox frontends: Framebuffer (FB, DirectFB):
 * Helper routines
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxFB.h"
#include "Helper.h"

/**
 * Globals
 */
videoMode videoModes[MAX_VIDEOMODES] = {{0}};
uint32_t numVideoModes = 0;

/**
 * callback handler for populating the supported video modes
 *
 * @returns callback success indicator
 * @param   width        width in pixels of the current video mode
 * @param   height       height in pixels of the current video mode
 * @param   bpp          bits per pixel of the current video mode
 * @param   callbackdata user data pointer
 */
DFBEnumerationResult enumVideoModesHandler(int width, int height, int bpp, void *callbackdata)
{
    if (numVideoModes >= MAX_VIDEOMODES)
    {
        return DFENUM_CANCEL;
    }
    // don't take palette based modes
    if (bpp >= 16)
    {
        // don't take modes we already have (I have seen many cases where
        // DirectFB returns the same modes several times)
        int32_t existingMode = getBestVideoMode(width, height, bpp);
        if ((existingMode == -1) ||
            ((videoModes[existingMode].width != (uint32_t)width) ||
             (videoModes[existingMode].height != (uint32_t)height) ||
             (videoModes[existingMode].bpp != (uint32_t)bpp)))
        {
            videoModes[numVideoModes].width  = (uint32_t)width;
            videoModes[numVideoModes].height = (uint32_t)height;
            videoModes[numVideoModes].bpp    = (uint32_t)bpp;
            numVideoModes++;
        }
    }
    return DFENUM_OK;
}

/**
 * Returns the best fitting video mode for the given characteristics.
 *
 * @returns index of the best video mode, -1 if no suitable mode found
 * @param   width  requested width
 * @param   height requested height
 * @param   bpp    requested bit depth
 */
int32_t getBestVideoMode(uint32_t width, uint32_t height, uint32_t bpp)
{
    int32_t bestMode = -1;

    for (uint32_t i = 0; i < numVideoModes; i++)
    {
        // is this mode compatible?
        if ((videoModes[i].width >= width) && (videoModes[i].height >= height) &&
            (videoModes[i].bpp >= bpp))
        {
            // first suitable mode?
            if (bestMode == -1)
            {
                bestMode = i;
            } else
            {
                // is it better than the one we got before?
                if ((videoModes[i].width  < videoModes[bestMode].width) ||
                    (videoModes[i].height < videoModes[bestMode].height) ||
                    (videoModes[i].bpp    < videoModes[bestMode].bpp))
                {
                    bestMode = i;
                }
            }
        }
    }
    return bestMode;
}
