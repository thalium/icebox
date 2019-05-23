
#include <stdlib.h>
#include <math.h>

#include "cr_mem.h"
#include "cr_hull.h"
#include "cr_error.h"

#ifdef WINDOWS
/* I know while(1) is constant */
#pragma warning( disable: 4127 )
#endif

/*==================================================
 *
 * Do S1 & S2 intersect? if so, return t for s1
 */
static double
_segment_segment_intersection(const double *s1a, const double *s1b,
															const double *s2a, const double *s2b)
{
	double r1[2], r2[2];
	double A, B, s, t;
	double epsilon = .0000000001;

	r1[0] = s1b[0] - s1a[0];
	r1[1] = s1b[1] - s1a[1];
	r2[0] = s2b[0] - s2a[0];
	r2[1] = s2b[1] - s2a[1];

	if (r1[0] == 0)
		return -1;

	A = s1a[1] - s2a[1] + (r1[1] / r1[0]) * (s2a[0] - s1a[0]);
	B = r2[1] - (r1[1] / r1[0]) * r2[0];

	if (B == 0)
		return -1;
	s = A / B;

	if ((s <= epsilon) || (s > 1. + epsilon))
		return -1;

	t = (s2a[0] - s1a[0] + s * r2[0]) / r1[0];
	if ((t < epsilon) || (t > 1. + epsilon))
		return -1;

	return t;
}

static int
_segment_hull_intersect(double *sa, double *sb, const double *pnts,
												int *hull, int hull_len, double *hits)
{
	int a, hitnum;
	double result;
	double d[2];

	hitnum = 0;
	for (a = 0; a < hull_len - 1; a++)
	{
		result = _segment_segment_intersection(sa, sb, pnts + 2 * hull[a],
																					 pnts + 2 * hull[a + 1]);

		if (result >= 0)
		{
			d[0] = sb[0] - sa[0];
			d[1] = sb[1] - sa[1];
			hits[2 * hitnum] = sa[0] + result * d[0];
			hits[2 * hitnum + 1] = sa[1] + result * d[1];
			hitnum++;
		}
	}

	return hitnum;
}

/* 
 * given 4 points, find a rectangle that lays w/i the quad
 */
static void
_four_point_box(double *points, double *min, double *max)
{
	int a, b, tmp, sort[4];
	double npnts[8], pnt[2], retval, d[2];

	for (a = 0; a < 4; a++)
		sort[a] = a;

	for (a = 0; a < 4; a++)
		for (b = a + 1; b < 4; b++)
		{
			if (points[2 * sort[a] + 1] > points[2 * sort[b] + 1])
			{
				tmp = sort[a];
				sort[a] = sort[b];
				sort[b] = tmp;
			}
		}

	npnts[0] = points[2 * sort[1]];
	npnts[1] = points[2 * sort[1] + 1];
	npnts[2] = points[2 * sort[2]];
	npnts[3] = points[2 * sort[2] + 1];

	/* now, intersect a horizontal ray from sort[1] with the quad */
	for (b = 0; b < 2; b++)
	{
		for (a = 0; a < 4; a++)
		{
			pnt[0] = points[2 * sort[b + 1]] + 10;
			pnt[1] = points[2 * sort[b + 1] + 1];

			retval = _segment_segment_intersection(points + 2 * sort[b + 1], pnt,
																						 points + 2 * a,
																						 points + 2 * ((a + 1) % 4));
			if (retval <= 0.001)
			{
				pnt[0] = points[2 * sort[b + 1]] - 10;
				retval = _segment_segment_intersection(points + 2 * sort[b + 1], pnt,
																							 points + 2 * a,
																							 points + 2 * ((a + 1) % 4));
			}

			if (retval > 0.001)
			{
				d[0] = pnt[0] - points[2 * sort[b + 1]];
				d[1] = pnt[1] - points[2 * sort[b + 1] + 1];
				npnts[2 * b + 4] = points[2 * sort[b + 1]] + retval * d[0];
				npnts[2 * b + 5] = points[2 * sort[b + 1] + 1] + retval * d[1];
			}
		}
	}

	min[1] = points[2 * sort[1] + 1];
	max[1] = points[2 * sort[2] + 1];

	/* finally, sort npnts by x */
	for (a = 0; a < 4; a++)
		sort[a] = a;

	for (a = 0; a < 4; a++)
		for (b = a + 1; b < 4; b++)
		{
			if (npnts[2 * sort[a]] > npnts[2 * sort[b]])
			{
				tmp = sort[a];
				sort[a] = sort[b];
				sort[b] = tmp;
			}
		}

	/* and grab the x coord of the box */
	min[0] = npnts[2 * sort[1]];
	max[0] = npnts[2 * sort[2]];

}

/* 
 * Given the convex hull, find a rectangle w/i it. 
 *
 * Finding the rect from 4 pnts isn't too bad. Find the bbox
 * of the CH, then intersect the diagonals of the bbox with
 * the CH. Use those 4 points to compute the box
 */
static void
_bound(const double *pnts, int npnts, int *hull, int hull_len, double *bbox)
{
	double max[2], min[2], cent[2], dir[2], pnt[2];
	double x, y, intr_pnts[8];
	int a, retval;

	(void) npnts;

	max[0] = max[1] = -9999;
	min[0] = min[1] = 9999;
	for (a = 0; a < hull_len; a++)
	{
		x = pnts[2 * hull[a]];
		y = pnts[2 * hull[a] + 1];
		if (x < min[0])
			min[0] = x;
		if (x > max[0])
			max[0] = x;
		if (y < min[1])
			min[1] = y;
		if (y > max[1])
			max[1] = y;
	}

	/* push the bbox out just a hair to make intersection
	 * a bit more sane */
	cent[0] = (min[0] + max[0]) / 2.;
	cent[1] = (min[1] + max[1]) / 2.;
	dir[0] = max[0] - cent[0];
	dir[1] = max[1] - cent[1];
	max[0] = cent[0] + 1.01 * dir[0];
	max[1] = cent[1] + 1.01 * dir[1];
	dir[0] = min[0] - cent[0];
	dir[1] = min[1] - cent[1];
	min[0] = cent[0] + 1.01 * dir[0];
	min[1] = cent[1] + 1.01 * dir[1];

	retval = _segment_hull_intersect(min, max, pnts, hull, hull_len, intr_pnts);
	if (retval != 2)
		crError("Bad hull intersection");

	dir[0] = min[0];
	dir[1] = max[1];
	pnt[0] = max[0];
	pnt[1] = min[1];
	retval =
		_segment_hull_intersect(dir, pnt, pnts, hull, hull_len, intr_pnts + 4);
	if (retval != 2)
		crError("Bad hull intersection");

	/* swap points to get them in some order */
	cent[0] = intr_pnts[2];
	cent[1] = intr_pnts[3];
	intr_pnts[2] = intr_pnts[4];
	intr_pnts[3] = intr_pnts[5];
	intr_pnts[4] = cent[0];
	intr_pnts[5] = cent[1];

	_four_point_box(intr_pnts, bbox, bbox + 2);
}

/*==================================================*/
void
crHullInteriorBox(const double *pnts, int npnts, double *bbox)
{
	int lowest, a;
	int *hull, idx, low_idx = 0;
	double low_dot;
	const double *p0, *p1;
	double v[2], A, B, vnew[2], vlow[2];

	vlow[0] = vlow[1] = 0.0;

	hull = (int *) crAlloc((npnts + 1) * sizeof(int));

	/* find the lowest */
	lowest = 0;
	for (a = 0; a < 2 * npnts; a += 2)
	{
		if (pnts[a + 1] < pnts[2 * lowest + 1])
			lowest = a / 2;
	}

	hull[0] = lowest;
	p0 = pnts + 2 * lowest;
	idx = 1;

	v[0] = 1;
	v[1] = 0;

	while (1)
	{
		low_dot = -10;

		for (a = 0; a < npnts; a++)
		{
			if (a == hull[idx - 1])
				continue;

			p1 = pnts + 2 * a;
			if (v[0] == 0.0)
				A = 0.0;
			else
				A = p0[1] - p1[1] + (v[1] / v[0]) * (p1[0] - p0[0]);
			if (v[0] == 0.0)
				B = 0.0;
			else
				B = v[1] * v[1] / v[0] + v[0];

			if (B != 0.0 && A / B > 0)
			{
				continue;
			}

			vnew[0] = p1[0] - p0[0];
			vnew[1] = p1[1] - p0[1];
			B = sqrt(vnew[0] * vnew[0] + vnew[1] * vnew[1]);

			vnew[0] /= B;
			vnew[1] /= B;

			A = vnew[0] * v[0] + vnew[1] * v[1];

			if (A > low_dot)
			{
				low_dot = A;
				low_idx = a;
				vlow[0] = vnew[0];
				vlow[1] = vnew[1];
			}
		}

		p0 = pnts + 2 * low_idx;
		hull[idx] = low_idx;
		idx++;

		v[0] = vlow[0];
		v[1] = vlow[1];

		if (low_idx == lowest)
		{
			break;
		}
	} 

	_bound(pnts, npnts, hull, idx, bbox);

	crFree(hull);
}
