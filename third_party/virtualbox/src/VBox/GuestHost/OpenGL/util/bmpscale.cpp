/** @file
 * Image resampling code, used for snapshot thumbnails.
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

/*
 * Based on gdImageCopyResampled from libgd.
 * Original copyright notice follows:

     Portions copyright 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
     Pierre-Alain Joye (pierre@libgd.org).

     Permission has been granted to copy, distribute and modify gd in
     any context without fee, including a commercial application,
     provided that this notice is present in user-accessible supporting
     documentation.

     This does not affect your ownership of the derived work itself, and
     the intent is to assure proper credit for the authors of gd, not to
     interfere with your productive use of gd. If you have questions,
     ask. "Derived works" includes all programs that utilize the
     library. Credit must be given in user-accessible documentation.

     This software is provided "AS IS." The copyright holders disclaim
     all warranties, either express or implied, including but not
     limited to implied warranties of merchantability and fitness for a
     particular purpose, with respect to this code and accompanying
     documentation.
 */

/*
 *
 * @todo Simplify: Offsets of images are 0,0 => no dstX, dstY, srcX, srcY;
 *             Screenshot has no alpha channel => no processing of alpha byte.
 */

#include <cr_bmpscale.h>

/* 2.0.10: cast instead of floor() yields 35% performance improvement.
    Thanks to John Buckman. */

#define floor2(exp) ((long) exp)
/*#define floor2(exp) floor(exp)*/

typedef uint8_t *gdImagePtr;

DECLINLINE(int) gdImageGetTrueColorPixel (gdImagePtr im, int x, int y, int w)
{
    return *(int32_t *)(im + y * w * 4 + x * 4);
}

DECLINLINE(void) gdImageSetPixel (gdImagePtr im, int x, int y, int color, int cbLine)
{
    *(int32_t *)(im + y * cbLine + x * 4) = color;
}

#define gdAlphaMax 127
#define gdAlphaOpaque 0
#define gdAlphaTransparent 127
#define gdRedMax 255
#define gdGreenMax 255
#define gdBlueMax 255
#define gdTrueColorGetAlpha(c) (((c) & 0x7F000000) >> 24)
#define gdTrueColorGetRed(c) (((c) & 0xFF0000) >> 16)
#define gdTrueColorGetGreen(c) (((c) & 0x00FF00) >> 8)
#define gdTrueColorGetBlue(c) ((c) & 0x0000FF)
#define gdTrueColorAlpha(r, g, b, a) (((a) << 24) + \
    ((r) << 16) + \
    ((g) << 8) + \
    (b))

void gdImageCopyResampled (uint8_t *dst,
              uint8_t *src,
              int dstX, int dstY,
              int srcX, int srcY,
              int dstW, int dstH, int srcW, int srcH)
{
  int x, y;
  double sy1, sy2, sx1, sx2;
  for (y = dstY; (y < dstY + dstH); y++)
    {
      sy1 = ((double) y - (double) dstY) * (double) srcH / (double) dstH;
      sy2 = ((double) (y + 1) - (double) dstY) * (double) srcH /
    (double) dstH;
      for (x = dstX; (x < dstX + dstW); x++)
    {
      double sx, sy;
      double spixels = 0;
      double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
      sx1 = ((double) x - (double) dstX) * (double) srcW / dstW;
      sx2 = ((double) (x + 1) - (double) dstX) * (double) srcW / dstW;
      sy = sy1;
      do
        {
          double yportion;
          if (floor2 (sy) == floor2 (sy1))
        {
          yportion = 1.0 - (sy - floor2 (sy));
          if (yportion > sy2 - sy1)
            {
              yportion = sy2 - sy1;
            }
          sy = floor2 (sy);
        }
          else if (sy == floor2 (sy2))
        {
          yportion = sy2 - floor2 (sy2);
        }
          else
        {
          yportion = 1.0;
        }
          sx = sx1;
          do
        {
          double xportion;
          double pcontribution;
          int p;
          if (floor2 (sx) == floor2 (sx1))
            {
              xportion = 1.0 - (sx - floor2 (sx));
              if (xportion > sx2 - sx1)
            {
              xportion = sx2 - sx1;
            }
              sx = floor2 (sx);
            }
          else if (sx == floor2 (sx2))
            {
              xportion = sx2 - floor2 (sx2);
            }
          else
            {
              xportion = 1.0;
            }
          pcontribution = xportion * yportion;
          /* 2.08: previously srcX and srcY were ignored.
             Andrew Pattison */
          p = gdImageGetTrueColorPixel (src,
                        (int) sx + srcX,
                        (int) sy + srcY, srcW);
          red += gdTrueColorGetRed (p) * pcontribution;
          green += gdTrueColorGetGreen (p) * pcontribution;
          blue += gdTrueColorGetBlue (p) * pcontribution;
          alpha += gdTrueColorGetAlpha (p) * pcontribution;
          spixels += xportion * yportion;
          sx += 1.0;
        }
          while (sx < sx2);
          sy += 1.0;
        }
      while (sy < sy2);
      if (spixels != 0.0)
        {
          red /= spixels;
          green /= spixels;
          blue /= spixels;
          alpha /= spixels;
        }
      /* Clamping to allow for rounding errors above */
      if (red > 255.0)
        {
          red = 255.0;
        }
      if (green > 255.0)
        {
          green = 255.0;
        }
      if (blue > 255.0)
        {
          blue = 255.0;
        }
      if (alpha > gdAlphaMax)
        {
          alpha = gdAlphaMax;
        }
      gdImageSetPixel (dst,
               x, y,
               gdTrueColorAlpha ((int) red,
                         (int) green,
                         (int) blue, (int) alpha), dstW * 4);
    }
    }
}

/* Fast integer implementation for 32 bpp bitmap scaling.
 * Use fixed point values * 16.
 */
typedef int32_t FIXEDPOINT;
#define INT_TO_FIXEDPOINT(i) (FIXEDPOINT)((i) << 4)
#define FIXEDPOINT_TO_INT(v) (int)((v) >> 4)
#define FIXEDPOINT_FLOOR(v) ((v) & ~0xF)
#define FIXEDPOINT_FRACTION(v) ((v) & 0xF)

/* For 32 bit source only. */
VBOXBMPSCALEDECL(void) CrBmpScale32 (uint8_t *dst,
                        int iDstDeltaLine,
                        int dstW, int dstH,
                        const uint8_t *src,
                        int iSrcDeltaLine,
                        int srcW, int srcH)
{
    int x, y;

    for (y = 0; y < dstH; y++)
    {
        FIXEDPOINT sy1 = INT_TO_FIXEDPOINT(y * srcH) / dstH;
        FIXEDPOINT sy2 = INT_TO_FIXEDPOINT((y + 1) * srcH) / dstH;

        for (x = 0; x < dstW; x++)
        {
            FIXEDPOINT red = 0, green = 0, blue = 0;

            FIXEDPOINT sx1 = INT_TO_FIXEDPOINT(x * srcW) / dstW;
            FIXEDPOINT sx2 = INT_TO_FIXEDPOINT((x + 1) * srcW) / dstW;

            FIXEDPOINT spixels = (sx2 - sx1) * (sy2 - sy1);

            FIXEDPOINT sy = sy1;

            do
            {
                FIXEDPOINT yportion;
                if (FIXEDPOINT_FLOOR (sy) == FIXEDPOINT_FLOOR (sy1))
                {
                    yportion = INT_TO_FIXEDPOINT(1) - FIXEDPOINT_FRACTION(sy);
                    if (yportion > sy2 - sy1)
                    {
                        yportion = sy2 - sy1;
                    }
                    sy = FIXEDPOINT_FLOOR (sy);
                }
                else if (sy == FIXEDPOINT_FLOOR (sy2))
                {
                    yportion = FIXEDPOINT_FRACTION(sy2);
                }
                else
                {
                    yportion = INT_TO_FIXEDPOINT(1);
                }

                const uint8_t *pu8SrcLine = src + iSrcDeltaLine * FIXEDPOINT_TO_INT(sy);
                FIXEDPOINT sx = sx1;
                do
                {
                    FIXEDPOINT xportion;
                    FIXEDPOINT pcontribution;
                    int p;
                    if (FIXEDPOINT_FLOOR (sx) == FIXEDPOINT_FLOOR (sx1))
                    {
                        xportion = INT_TO_FIXEDPOINT(1) - FIXEDPOINT_FRACTION(sx);
                        if (xportion > sx2 - sx1)
                        {
                            xportion = sx2 - sx1;
                        }
                        pcontribution = xportion * yportion;
                        sx = FIXEDPOINT_FLOOR (sx);
                    }
                    else if (sx == FIXEDPOINT_FLOOR (sx2))
                    {
                        xportion = FIXEDPOINT_FRACTION(sx2);
                        pcontribution = xportion * yportion;
                    }
                    else
                    {
                        xportion = INT_TO_FIXEDPOINT(1);
                        pcontribution = xportion * yportion;
                    }
                    /* Color depth specific code begin */
                    p = *(uint32_t *)(pu8SrcLine + FIXEDPOINT_TO_INT(sx) * 4);
                    /* Color depth specific code end */
                    red += gdTrueColorGetRed (p) * pcontribution;
                    green += gdTrueColorGetGreen (p) * pcontribution;
                    blue += gdTrueColorGetBlue (p) * pcontribution;

                    sx += INT_TO_FIXEDPOINT(1);
                } while (sx < sx2);

                sy += INT_TO_FIXEDPOINT(1);
            } while (sy < sy2);

            if (spixels != 0)
            {
                red /= spixels;
                green /= spixels;
                blue /= spixels;
            }
            /* Clamping to allow for rounding errors above */
            if (red > 255)
            {
                red = 255;
            }
            if (green > 255)
            {
                green = 255;
            }
            if (blue > 255)
            {
                blue = 255;
            }
            gdImageSetPixel (dst,
                             x, y,
                             ( ((int) red) << 16) + (((int) green) << 8) + ((int) blue),
                             iDstDeltaLine);
        }
    }
}

