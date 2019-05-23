/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_HULL_H
#define CR_HULL_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLEXPORT(void) crHullInteriorBox(const double *pnts, int npnts, double *bbox);

#ifdef __cplusplus
}
#endif

#endif /* CR_BBOX_H */
