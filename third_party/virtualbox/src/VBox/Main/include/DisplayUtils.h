/* $Id: DisplayUtils.h $ */
/** @file
 * Display helper declarations
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBox/com/string.h"

using namespace com;

#define sSSMDisplayScreenshotVer 0x00010001
#define sSSMDisplayVer 0x00010001
#define sSSMDisplayVer2 0x00010002
#define sSSMDisplayVer3 0x00010003
#define sSSMDisplayVer4 0x00010004
#define sSSMDisplayVer5 0x00010005

int readSavedGuestScreenInfo(const Utf8Str &strStateFilePath, uint32_t u32ScreenId,
                             uint32_t *pu32OriginX, uint32_t *pu32OriginY,
                             uint32_t *pu32Width, uint32_t *pu32Height, uint16_t *pu16Flags);

int readSavedDisplayScreenshot(const Utf8Str &strStateFilePath, uint32_t u32Type, uint8_t **ppu8Data, uint32_t *pcbData, uint32_t *pu32Width, uint32_t *pu32Height);
void freeSavedDisplayScreenshot(uint8_t *pu8Data);

