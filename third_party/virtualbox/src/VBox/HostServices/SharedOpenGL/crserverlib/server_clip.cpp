/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/*
 * This code contributed by Karl Rasche <rkarl@vr.clemson.edu>
 */


#include <math.h>

#include "cr_server.h"
#include "cr_mem.h"
#include "server.h"


static void
__find_intersection(double *s, double *e, double *clp, double *clp_next,
										double *intr)
{
	double v1[2], v2[2];
	double A, B, T;

	v1[0] = e[0] - s[0];
	v1[1] = e[1] - s[1];
	v2[0] = clp_next[0] - clp[0];
	v2[1] = clp_next[1] - clp[1];
	
	if ((v1[1]) && (v2[0]))
	{
		A =  (clp[1]-s[1])/v1[1] + (v2[1]/v1[1])*(s[0]-clp[0])/v2[0];
		B = 1.-(v2[1]/v1[1])*(v1[0]/v2[0]);
		if (B)
			T = A/B;
		else
		{
			T = 0;
		}

		intr[0] = s[0]+T*v1[0];
		intr[1] = s[1]+T*v1[1];
	}
	else
	if (v1[1])
	{
		/* clp -> clp_next is vertical */
		T = (clp[0]-s[0])/v1[0];

		intr[0] = s[0]+T*v1[0];
		intr[1] = s[1]+T*v1[1];
	}
	else
	{
		/* s -> e is horizontal */
		T = (s[1]-clp[1])/v2[1];

		intr[0] = clp[0]+T*v2[0];
		intr[1] = clp[1]+T*v2[1];
	}

}

static void
 __clip_one_side(double *poly, int npnts, double *clp, double *clp_next,
								 double *norm, 
								 double **new_poly_in, int *new_npnts_in,
								 double **new_poly_out, int *new_npnts_out)
{
	int a, sin, ein;
	double *s, *e, intr[2]; 
	
	*new_poly_in  = (double *)crAlloc(2*npnts*2*sizeof(double));
	*new_npnts_in = 0;

	*new_poly_out  = (double *)crAlloc(2*npnts*2*sizeof(double));
	*new_npnts_out = 0;

	s = poly;

	for (a=0; a<npnts; a++)
	{
		e = poly+2*((a+1)%npnts);

		if (((e[0]-clp[0])*norm[0]) + ((e[1]-clp[1])*norm[1]) >= 0)
			ein = 0;
		else
			ein = 1;

		if (((s[0]-clp[0])*norm[0]) + ((s[1]-clp[1])*norm[1]) >= 0)
			sin = 0;
		else
			sin = 1;
	
		if (sin && ein)
		{
			/* case 1: */
			crMemcpy(*new_poly_in+2*(*new_npnts_in), e, 2*sizeof(double));
			(*new_npnts_in)++;
		}
		else
		if (sin && (!ein))
		{
			/* case 2: */

			__find_intersection(s, e, clp, clp_next, intr);

			crMemcpy(*new_poly_in+2*(*new_npnts_in), intr, 2*sizeof(double));
			(*new_npnts_in)++;

			crMemcpy(*new_poly_out+2*(*new_npnts_out), intr, 2*sizeof(double));
			(*new_npnts_out)++;
			crMemcpy(*new_poly_out+2*(*new_npnts_out), e, 2*sizeof(double));
			(*new_npnts_out)++;
		}
		else
		if ((!sin) && ein)
		{
			/* case 4: */
			__find_intersection(s, e, clp, clp_next, intr);

			crMemcpy((*new_poly_in)+2*(*new_npnts_in), intr, 2*sizeof(double));
			(*new_npnts_in)++;
			crMemcpy((*new_poly_in)+2*(*new_npnts_in), e, 2*sizeof(double));
			(*new_npnts_in)++;

			crMemcpy(*new_poly_out+2*(*new_npnts_out), intr, 2*sizeof(double));
			(*new_npnts_out)++;
		}
		else
		{
			crMemcpy(*new_poly_out+2*(*new_npnts_out), e, 2*sizeof(double));
			(*new_npnts_out)++;
		}

		s =  e;
	}
}

/* 
 * Sutherland/Hodgman clipping for interior & exterior regions. 
 * length_of((*new_vert_out)[a]) == nclip_to_vert
 */
static void
__clip(double *poly, int nvert, double *clip_to_poly, int nclip_to_vert,
			 double **new_vert_in, int *nnew_vert_in,
			 double ***new_vert_out, int **nnew_vert_out)
{
	int a, side, *nout;
	double *clip_normals, *s, *e, *n, *new_vert_src; 
	double *norm, *clp, *clp_next;
 	double **out;
	
	*new_vert_out = (double **)crAlloc(nclip_to_vert*sizeof(double *));
	*nnew_vert_out = (int *)crAlloc(nclip_to_vert*sizeof(int));

	/* 
	 * First, compute normals for the clip poly. This 
	 * breaks for multiple (3+) adjacent colinear vertices
 	 */
	clip_normals = (double *)crAlloc(nclip_to_vert*2*sizeof(double));
	for (a=0; a<nclip_to_vert; a++)
	{
		s = clip_to_poly+2*a;
		e = clip_to_poly+2*((a+1)%nclip_to_vert);
		n = clip_to_poly+2*((a+2)%nclip_to_vert);

		norm = clip_normals+2*a;
		norm[0]   = e[1]-s[1];
		norm[1] = -1*(e[0]-s[0]);
		
		/* 
		 * if dot(norm, n-e) > 0), the normals are backwards,
		 * 	assuming the clip region is convex 
		 */ 
		if (norm[0]*(n[0]-e[0]) + norm[1]*(n[1]-e[1]) > 0)
		{
			norm[0] *= -1;
			norm[1] *= -1;
		}
	}

	new_vert_src  = (double *)crAlloc(nvert*nclip_to_vert*2*sizeof(double));
	crMemcpy(new_vert_src, poly, 2*nvert*sizeof(double));

	for (side=0; side<nclip_to_vert; side++)
	{
		clp      = clip_to_poly+2*side;
		clp_next = clip_to_poly+2*((side+1)%nclip_to_vert);
		norm = clip_normals+2*side;
		*nnew_vert_in = 0;

		nout = (*nnew_vert_out)+side;
		out  = (*new_vert_out)+side;

	 	__clip_one_side(new_vert_src, nvert, clp, clp_next, norm,
				new_vert_in, nnew_vert_in, 
				out, nout);

		crMemcpy(new_vert_src, (*new_vert_in), 2*(*nnew_vert_in)*sizeof(double));
		if (side != nclip_to_vert-1)
			crFree(*new_vert_in);
		nvert = *nnew_vert_in;
	}
}

/* 
 * Given a bitmap and a group of 'base' polygons [the quads we are testing],
 * perform the unions and differences specified by the map and return
 * the resulting geometry
 */
static void
__execute_combination(CRPoly **base, int n, int *mask, CRPoly **head)
{
	int a, b, got_intr;
	int nin, *nout, last;
	double *in, **out;
	CRPoly *intr, *diff, *p;

	*head = NULL;

	intr = (CRPoly *)crAlloc(sizeof(CRPoly));
	intr->next = NULL;

	got_intr = 0;

	/* first, intersect the first 2 polys marked */
	for (a=0; a<n; a++)
		if (mask[a]) break;
	for (b=a+1; b<n; b++)
		if (mask[b]) break;

	__clip(base[a]->points, base[a]->npoints, 
				base[b]->points, base[b]->npoints,
				&in, &nin, &out, &nout);
	last = b;

	crFree (nout);
	for (a=0; a<base[last]->npoints; a++)
		if (out[a])
			crFree(out[a]);
	crFree(out);
					

	if (nin)
	{
		intr->npoints = nin;
		intr->points = in;
		got_intr = 1;
	}

	while (1)
	{
		for (a=last+1; a<n; a++)
			if (mask[a]) break;

		if (a == n) break;

		if (got_intr)
		{
			__clip(base[a]->points, base[a]->npoints, 
					intr->points, intr->npoints,
					&in, &nin, &out, &nout);

			crFree (nout);
			for (b=0; b<intr->npoints; b++)
				if (out[b])
					crFree(out[b]);
			crFree(out);

			if (nin)
			{
				intr->npoints = nin;
				intr->points = in;
			}
			else
			{
				got_intr = 0;
				break;
			}
		}
		else
		{
			__clip(base[a]->points, base[a]->npoints, 
					base[last]->points, base[last]->npoints,
					&in, &nin, &out, &nout);
			
			crFree (nout);
			for (b=0; b<base[last]->npoints; b++)
			{
				if (out[b])
					crFree(out[b]);
			}
			crFree(out);


			if (nin)
			{
				intr->npoints = nin;
				intr->points = in;
				got_intr = 1;
			}
		}

		last = a;
		if (a == n) break;
	}

	/* can't subtract something from nothing! */
	if (got_intr)
		*head = intr;
	else
		return;
	
	/* find the first item to subtract */
	for (a=0; a<n; a++)
		if (!mask[a]) break;

	if (a == n) return;
	last = a;

	/* and subtract it */
	diff = NULL;
	__clip(intr->points, intr->npoints,
				base[last]->points, base[last]->npoints, 
				&in, &nin, &out, &nout);

	crFree(in);

	for (a=0; a<base[last]->npoints; a++)	
	{
		if (!nout[a]) continue;

		p = (CRPoly *)crAlloc(sizeof(CRPoly));
		p->npoints = nout[a];
		p->points  = out[a];
		p->next = diff;
		diff = p;
	}
	*head = diff;

	while (1)
	{
		intr = diff;
		diff = NULL;

		for (a=last+1; a<n; a++)
			if (!mask[a]) break;
		if (a == n) return;

		last = a;

		/* subtract mask[a] from everything in intr and
		 * plop it into diff */
		while (intr)
		{
			__clip(intr->points, intr->npoints,
				base[last]->points, base[last]->npoints, 
				&in, &nin, &out, &nout);

			crFree(in);

			for (a=0; a<base[last]->npoints; a++)	
			{
				if (!nout[a]) continue;

				p = (CRPoly *)crAlloc(sizeof(CRPoly));
				p->npoints = nout[a];
				p->points  = out[a];
				p->next = diff;
				diff = p;
			}

			intr = intr->next;
		}

		*head = diff;
	}	

}

/*
 * Here we generate all valid bitmaps to represent union/difference
 * combinations. Each bitmap is N elements long, where N is the 
 * number of polys [quads] that we are testing for overlap
 */
static void
__generate_masks(int n, int ***mask, int *nmasks)
{
	int a, b, c, d, e;
	int i, idx, isec_size, add;

	*mask = (int **)crAlloc((unsigned int)(1 << n)*sizeof(int));
	for (a=0; a<(1 << n); a++)
		(*mask)[a] = (int *)crAlloc(n*sizeof(int));

	/* compute combinations */
	idx = 0;
	for (isec_size=1; isec_size<n; isec_size++)
	{
		for (a=0; a<n; a++)
		{
			for (b=a+1; b<n; b++)
			{
				crMemset((*mask)[idx], 0, n*sizeof(int));
				(*mask)[idx][a] = 1;

				add = 1;
				for (c=0; c<isec_size; c++)
				{
					i = (b+c) % n;
					if (i == a) add = 0;
						
					(*mask)[idx][i] = 1;	
				}

				/* dup check */
				if ((add) && (idx))
				{
					for (d=0; d<idx; d++)
					{
						add = 0;
						for (e=0; e<n; e++)
						{
							if ((*mask)[idx][e] != (*mask)[d][e]) 
								add = 1;
						}

						if (!add)
							break;
					}
				}

				if (add) 
					idx++;
			}
		}
	}

	*nmasks = idx;
}

/* 
 * To compute the overlap between a series of quads (This should work 
 * for n-gons, but we'll only need quads..), first generate a series of 
 * bitmaps that represent which elements to union together, and which
 * to difference. This goes into 'mask'. We then evaluate each bitmap with
 * Sutherland-Hodgman clipping to find the interior (union) and exterior
 * (difference) regions.
 *
 * In the map, 1 == union, 0 == difference
 *
 * (*res)[a] is the head of a poly list for all the polys that convert 
 * regions of overlap between a+1 polys ((*res)[0] == NULL)
 */ 
void
crComputeOverlapGeom(double *quads, int nquad, CRPoly ***res)
{
	int a, b, idx, isec_size, **mask;
	CRPoly *p, *next, **base;
	
	base = (CRPoly **)crAlloc(nquad*sizeof(CRPoly *));
	for (a=0; a<nquad; a++)
	{
		p = (CRPoly *)crAlloc(sizeof(CRPoly));
		p->npoints = 4;
		p->points  = (double *)crAlloc(8*sizeof(double));
		for (b=0; b<8; b++)
		{
			p->points[b] = quads[8*a+b];
		}
		p->next = NULL;
		base[a] = p;
	}
	
	*res = (CRPoly **)crAlloc(nquad*sizeof(CRPoly *));
	for (a=0; a<nquad; a++)
		(*res)[a] = NULL;
	
	__generate_masks(nquad, &mask, &idx);

	for (a=0; a<idx; a++)
	{
		isec_size = 0;
		for (b=0; b<nquad; b++)
			if (mask[a][b]) isec_size++;
		isec_size--;

		__execute_combination(base, nquad, mask[a], &p);
	
		while (p)
		{
			next = p->next;
			
			p->next = (*res)[isec_size];
			(*res)[isec_size] = p;
			
			p = next;
		}
	}

	for (a=0; a<nquad; a++)
	{
		crFree(base[a]->points);
		crFree(base[a]);
	}
	crFree(base);		

}

/*
 * This is similar to ComputeOverlapGeom above, but for "knockout" 
 * edge blending. 
 *
 * my_quad_idx is an index of quads indicating which display tile 
 * we are computing geometry for. From this, we either generate
 * geometry, or not, such that all geometry can be drawn in black 
 * and only one tile will show through the blend as non-black.
 *
 * To add a combination to our set of geom, we must test that:
 * 		+ mask[a][my_quad_idx] is set
 * 		+ mask[a][my_quad_idx] is not the first element set in
 * 			mask[a].
 * If these conditions hold, execute mask[a] and draw the resulting
 * geometry in black
 *
 * Unlike ComputeOverlapGeom, res is just a list of polys to draw in black
 */ 	
void
crComputeKnockoutGeom(double *quads, int nquad, int my_quad_idx, CRPoly **res)
{
	int a, b, idx, first, **mask;
	CRPoly *p, *next, **base;
	
	base = (CRPoly **) crAlloc(nquad*sizeof(CRPoly *));
	for (a=0; a<nquad; a++)
	{
		p = (CRPoly *) crAlloc(sizeof(CRPoly));
		p->npoints = 4;
		p->points  = (double *) crAlloc(8*sizeof(double));
		for (b=0; b<8; b++)
		{
			p->points[b] = quads[8*a+b];
		}
		p->next = NULL;
		base[a] = p;
	}
	
	(*res) = NULL;
	
	__generate_masks(nquad, &mask, &idx);

	for (a=0; a<idx; a++)
	{
		/* test for above conditions */
		if (!mask[a][my_quad_idx]) continue;	

		first = -1;
		for (b=0; b<nquad; b++)
			if (mask[a][b]) 
			{
				first = b;
				break;
			}
		if (first == my_quad_idx) continue;


		__execute_combination(base, nquad, mask[a], &p);
	
		while (p)
		{
			next = p->next;
			
			p->next = *res;
			*res = p;
			
			p = next;
		}
	}

	for (a=0; a<nquad; a++)
	{
		crFree(base[a]->points);
		crFree(base[a]);
	}
	crFree(base);		
}
