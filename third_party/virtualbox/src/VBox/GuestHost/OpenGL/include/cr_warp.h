/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_WARP_H
#define CR_WARP_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLEXPORT(void) crWarpPoint(const float *align, const float *point, float *result);

#ifdef __cplusplus
}
#endif

#endif /* CR_BBOX_H */
