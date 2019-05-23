/* $Id: icd_drv.h $ */
/** @file
 * VirtualBox Windows NT/2000/XP guest OpenGL ICD header
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___ICDDRV_H___
#define ___ICDDRV_H___

#include <iprt/win/windows.h>

typedef struct ICDTABLE
{
    DWORD size;
    PROC  table[336];
} ICDTABLE, *PICDTABLE;

void APIENTRY DrvReleaseContext(HGLRC hglrc);
BOOL APIENTRY DrvValidateVersion(DWORD version);
PICDTABLE APIENTRY DrvSetContext(HDC hdc, HGLRC hglrc, void *callback);
BOOL APIENTRY DrvSetPixelFormat(HDC hdc, int iPixelFormat);
HGLRC APIENTRY DrvCreateContext(HDC hdc);
HGLRC APIENTRY DrvCreateLayerContext(HDC hdc, int iLayerPlane);
BOOL APIENTRY DrvDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane, UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd);
int APIENTRY DrvGetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       COLORREF *pcr);
int APIENTRY DrvDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pfd);
BOOL APIENTRY DrvDeleteContext(HGLRC hglrc);
BOOL APIENTRY DrvCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask);
BOOL APIENTRY DrvShareLists(HGLRC hglrc1, HGLRC hglrc2);
int APIENTRY DrvSetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       CONST COLORREF *pcr);
BOOL APIENTRY DrvRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize);
BOOL APIENTRY DrvSwapLayerBuffers(HDC hdc, UINT fuPlanes);
BOOL APIENTRY DrvSwapBuffers(HDC hdc);

#endif /* !___ICDDRV_H___ */
