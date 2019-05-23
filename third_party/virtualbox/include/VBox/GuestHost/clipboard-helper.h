/* $Id: clipboard-helper.h $ */
/** @file
 * Shared Clipboard - Some helper function for converting between the various EOLs.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_GuestHost_clipboard_helper_h
#define ___VBox_GuestHost_clipboard_helper_h

#include <iprt/string.h>

/** Constants needed for string conversions done by the Linux/Mac clipboard code. */
enum {
    /** In Linux, lines end with a linefeed character. */
    LINEFEED = 0xa,
    /** In Windows, lines end with a carriage return and a linefeed character. */
    CARRIAGERETURN = 0xd,
    /** Little endian "real" UTF-16 strings start with this marker. */
    UTF16LEMARKER = 0xfeff,
    /** Big endian "real" UTF-16 strings start with this marker. */
    UTF16BEMARKER = 0xfffe
};

/**
 * Get the size of the buffer needed to hold a UTF-16-LE zero terminated string
 * with Windows EOLs converted from a UTF-16 string with Linux EOLs.
 *
 * @returns VBox status code.
 *
 * @param   pwszSrc  The source UTF-16 string.
 * @param   cwcSrc   The length of the source string in RTUTF16 units.
 * @param   pcwcDst  The length of the destination string in RTUTF16 units.
 */
int vboxClipboardUtf16GetWinSize(PRTUTF16 pwszSrc, size_t cwcSrc, size_t *pcwcDst);

/**
 * Convert a UTF-16 text with Linux EOLs to null-terminated UTF-16-LE with
 * Windows EOLs.
 *
 * Does no checking for validity.
 *
 * @returns VBox status code
 *
 * @param   pwszSrc  Source UTF-16 text to convert.
 * @param   cwcSrc   Size of the source text int RTUTF16 units
 * @param   pwszDst  Buffer to store the converted text to.
 * @param   cwcDst   Size of the buffer for the converted text in RTUTF16 units.
 */
int vboxClipboardUtf16LinToWin(PRTUTF16 pwszSrc, size_t cwcSrc, PRTUTF16 pwszDst, size_t cwcDst);

/**
 * Get the size of the buffer needed to hold a zero-terminated UTF-16 string
 * with Linux EOLs converted from a UTF-16 string with Windows EOLs.
 *
 * @returns RT status code
 *
 * @param   pwszSrc  The source UTF-16 string
 * @param   cwcSrc   The length of the source string in RTUTF16 units.
 * @retval  pcwcDst  The length of the destination string in RTUTF16 units.
 */
int vboxClipboardUtf16GetLinSize(PRTUTF16 pwszSrc, size_t cwcSrc, size_t *pcwcDst);

/**
 * Convert UTF-16-LE text with Windows EOLs to zero-terminated UTF-16 with Linux
 * EOLs.  This function does not verify that the UTF-16 is valid.
 *
 * @returns VBox status code
 *
 * @param   pwszSrc  Text to convert
 * @param   cwcSrc   Size of the source text in RTUTF16 units.
 * @param   pwszDst  The buffer to store the converted text to
 * @param   cwcDst   The size of the buffer for the destination text in RTUTF16
 *                   chars.
 */
int vboxClipboardUtf16WinToLin(PRTUTF16 pwszSrc, size_t cwcSrc, PRTUTF16 pwszDst, size_t cwcDst);

#pragma pack(1)
/** @todo r=bird: Why duplicate these structures here, we've got them in
 *        DevVGA.cpp already! */
/**
 * Bitmap File Header. Official win32 name is BITMAPFILEHEADER
 * Always Little Endian.
 */
typedef struct BMFILEHEADER
{
/** @todo r=bird: this type centric prefixing is what give hungarian notation a bad name... */
    uint16_t    u16Type;
    uint32_t    u32Size;
    uint16_t    u16Reserved1;
    uint16_t    u16Reserved2;
    uint32_t    u32OffBits;
} BMFILEHEADER;
/** Pointer to a BMFILEHEADER structure. */
typedef BMFILEHEADER *PBMFILEHEADER;
/** BMP file magic number */
#define BITMAPHEADERMAGIC (RT_H2LE_U16_C(0x4d42))

/**
 * Bitmap Info Header. Official win32 name is BITMAPINFOHEADER
 * Always Little Endian.
 */
typedef struct BMINFOHEADER
{
/** @todo r=bird: this type centric prefixing is what give hungarian notation a bad name... */
    uint32_t    u32Size;
    uint32_t    u32Width;
    uint32_t    u32Height;
    uint16_t    u16Planes;
    uint16_t    u16BitCount;
    uint32_t    u32Compression;
    uint32_t    u32SizeImage;
    uint32_t    u32XBitsPerMeter;
    uint32_t    u32YBitsPerMeter;
    uint32_t    u32ClrUsed;
    uint32_t    u32ClrImportant;
} BMINFOHEADER;
/** Pointer to a BMINFOHEADER structure. */
typedef BMINFOHEADER *PBMINFOHEADER;
#pragma pack() /** @todo r=bird: Only BMFILEHEADER needs packing. The BMINFOHEADER is perfectly aligned. */

/**
 * Convert CF_DIB data to full BMP data by prepending the BM header.
 * Allocates with RTMemAlloc.
 *
 * @returns VBox status code
 *
 * @param   pvSrc         DIB data to convert
 * @param   cbSrc         Size of the DIB data to convert in bytes
 * @param   ppvDst        Where to store the pointer to the buffer for the
 *                        destination data
 * @param   pcbDst        Pointer to the size of the buffer for the destination
 *                        data in bytes.
 */
int vboxClipboardDibToBmp(const void *pvSrc, size_t cbSrc, void **ppvDst, size_t *pcbDst);

/**
 * Get the address and size of CF_DIB data in a full BMP data in the input buffer.
 * Does not do any allocation.
 *
 * @returns VBox status code
 *
 * @param   pvSrc         BMP data to convert
 * @param   cbSrc         Size of the BMP data to convert in bytes
 * @param   ppvDst        Where to store the pointer to the destination data
 * @param   pcbDst        Pointer to the size of the destination data in bytes
 */
int vboxClipboardBmpGetDib(const void *pvSrc, size_t cbSrc, const void **ppvDst, size_t *pcbDst);


#endif

