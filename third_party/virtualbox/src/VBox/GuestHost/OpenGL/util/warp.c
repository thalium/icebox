/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_warp.h"

void
crWarpPoint(const float *align, const float *point, float *result)
{
	float w, x, y;

	x = align[0] * point[0] + align[1] * point[1] + align[2];
	y = align[3] * point[0] + align[4] * point[1] + align[5];
	w = align[6] * point[0] + align[7] * point[1] + align[8];

	if (w == 0)
		crError("Crud in alignment matrix --> w == 0. bleh!");

	result[0] = x / w;
	result[1] = y / w;
}
