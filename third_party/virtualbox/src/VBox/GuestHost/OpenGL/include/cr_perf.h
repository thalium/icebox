/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_PERF_H
#define CR_PERF_H

#ifdef __cplusplus
extern "C" {
#endif


/** 
 * For the performance SPU. Allows application to obtain it's own
 * statistics, and reset etc. 
 */
typedef struct {
	int 	count;

	int 	v2d, v2f, v2i, v2s;
	int 	v2dv, v2fv, v2iv, v2sv;
	int 	v3d, v3f, v3i, v3s;
	int 	v3dv, v3fv, v3iv, v3sv;
	int 	v4d, v4f, v4i, v4s;
	int 	v4dv, v4fv, v4iv, v4sv;

	int	ipoints;	/**< Interpreted points */
	int	ilines; 	/**< Interpreted lines */
	int	itris;		/**< Interpreted tris */
	int	iquads; 	/**< Interpreted quads */
	int	ipolygons; 	/**< Interpreted polygons */
} PerfVertex;

/**
 * Primitives data
 */
typedef struct {
	PerfVertex points;
	PerfVertex lines;
	PerfVertex lineloop;
	PerfVertex linestrip;
	PerfVertex triangles;
	PerfVertex tristrip;
	PerfVertex trifan;
	PerfVertex quads;
	PerfVertex quadstrip;
	PerfVertex polygon;
} PerfPrim;

typedef struct {
	int draw_pixels;
	int read_pixels;

	int teximage1DBytes;  /**< bytes given to glTexImage1D */
	int teximage2DBytes;  /**< bytes given to glTexImage2D */
	int teximage3DBytes;  /**< bytes given to glTexImage3D */
	int texsubimage1DBytes;  /**< bytes given to glTexSubImage1D */
	int texsubimage2DBytes;  /**< bytes given to glTexSubImage2D */
	int texsubimage3DBytes;  /**< bytes given to glTexSubImage3D */
	int newLists;       /**< glNewList calls */
	int callLists;      /**< glCallList(s) calls */

	PerfVertex *cur_vertex;
	PerfVertex vertex_snapshot;
	PerfPrim vertex_data;
} PerfData;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_PERF_H */
