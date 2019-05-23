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

#ifndef __H_HELPER
#define __H_HELPER

#define MAX_VIDEOMODES 64
struct videoMode
{
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
};
extern videoMode videoModes[];
extern uint32_t numVideoModes;

DFBEnumerationResult enumVideoModesHandler(int width, int height, int bpp, void *callbackdata);
int32_t getBestVideoMode(uint32_t width, uint32_t height, uint32_t bpp);


#endif // __H_HELPER
