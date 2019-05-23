/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server_dispatch.h"
#include "server.h"
#include "cr_error.h"
#include "state/cr_statetypes.h"


static const CRmatrix identity_matrix = { 
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
};


/*
 * Clip the rectangle q against the given image window.
 */
static void
crServerViewportClipToWindow( const CRrecti *imagewindow,	CRrecti *q) 
{
	if (q->x1 < imagewindow->x1) q->x1 = imagewindow->x1;
	if (q->x1 > imagewindow->x2) q->x1 = imagewindow->x2;

	if (q->x2 > imagewindow->x2) q->x2 = imagewindow->x2;
	if (q->x2 < imagewindow->x1) q->x2 = imagewindow->x1;
	
	if (q->y1 < imagewindow->y1) q->y1 = imagewindow->y1;
	if (q->y1 > imagewindow->y2) q->y1 = imagewindow->y2;

	if (q->y2 > imagewindow->y2) q->y2 = imagewindow->y2;
	if (q->y2 < imagewindow->y1) q->y2 = imagewindow->y1;
}


/*
 * Translate the rectangle q from the image window space to the outputwindow
 * space.
 */
static void
crServerConvertToOutput( const CRrecti *imagewindow,
												 const CRrecti *outputwindow,
												 CRrecti *q )
{
	q->x1 = q->x1 - imagewindow->x1 + outputwindow->x1;
	q->x2 = q->x2 - imagewindow->x2 + outputwindow->x2;
	q->y1 = q->y1 - imagewindow->y1 + outputwindow->y1;
	q->y2 = q->y2 - imagewindow->y2 + outputwindow->y2;
}


/*
 * Compute clipped image window, scissor, viewport and base projection
 * info for each tile in the mural.
 * Need to call this when either the viewport or mural is changed.
 */
void
crServerComputeViewportBounds(const CRViewportState *v, CRMuralInfo *mural)
{
#if 0
	static GLuint serialNo = 1;
	int i;

	for (i = 0; i < mural->numExtents; i++) {
		CRExtent *extent = &mural->extents[i];
		CRrecti q;

		/* If the scissor is disabled set it to the whole output.
		** We might as well use the actual scissorTest rather than
		** scissorValid - it never gets reset anyway.
		*/
		if (!v->scissorTest)
		{
			extent->scissorBox = extent->outputwindow;
		} 
		else 
		{
			q.x1 = v->scissorX;
			q.x2 = v->scissorX + v->scissorW;
			q.y1 = v->scissorY;
			q.y2 = v->scissorY + v->scissorH;

			crServerViewportClipToWindow(&(extent->imagewindow), &q);
			crServerConvertToOutput(&(extent->imagewindow),
															&(extent->outputwindow), &q);
			extent->scissorBox = q;
		}

		/* if the viewport is not valid,
		** set it to the entire output.
		*/
		if (!v->viewportValid) 
		{
			extent->clippedImagewindow = extent->imagewindow;
			extent->viewport = extent->outputwindow;
		}
		else
		{
			q.x1 = v->viewportX;
			q.x2 = v->viewportX + v->viewportW;
			q.y1 = v->viewportY;
			q.y2 = v->viewportY + v->viewportH;

			/* This is where the viewport gets clamped to the max size. */
			crServerViewportClipToWindow(&(extent->imagewindow), &q);

			extent->clippedImagewindow = q;

			crServerConvertToOutput(&(extent->imagewindow),
															&(extent->outputwindow), &q);

			extent->viewport = q;
		}

		/*
		** Now, compute the base projection.
		*/
		if (extent->clippedImagewindow.x1 == extent->clippedImagewindow.x2 ||
				extent->clippedImagewindow.y1 == extent->clippedImagewindow.y2) {
			/* zero-area extent, use identity matrix (doesn't really matter) */
			extent->baseProjection = identity_matrix;
		}
		else
		{
			const int vpx = v->viewportX;
			const int vpy = v->viewportY;
			const int vpw = v->viewportW;
			const int vph = v->viewportH;
			GLfloat xscale, yscale;
			GLfloat xtrans, ytrans;
			CRrectf p;

			/* 
			 * We need to take account of the current viewport parameters,
			 * and they are passed to this function as x, y, w, h.
			 * In the default case (from main.c) we pass the the
			 * full muralsize of 0, 0, width, height
			 */
			p.x1 = (GLfloat) (extent->clippedImagewindow.x1 - vpx) / vpw;
			p.y1 = (GLfloat) (extent->clippedImagewindow.y1 - vpy) / vph;
			p.x2 = (GLfloat) (extent->clippedImagewindow.x2 - vpx) / vpw;
			p.y2 = (GLfloat) (extent->clippedImagewindow.y2 - vpy) / vph;

			/* XXX not sure this clamping is really need anymore
			 */
			if (p.x1 < 0.0) { 
				p.x1 = 0.0;
				if (p.x2 > 1.0) p.x2 = 1.0;
			}

			if (p.y1 < 0.0) {
				p.y1 = 0.0; 
				if (p.y2 > 1.0) p.y2 = 1.0;
			}

			/* Rescale [0,1] -> [-1,1] */
			p.x1 = p.x1 * 2.0f - 1.0f;
			p.x2 = p.x2 * 2.0f - 1.0f;
			p.y1 = p.y1 * 2.0f - 1.0f;
			p.y2 = p.y2 * 2.0f - 1.0f;

			xscale = 2.0f / (p.x2 - p.x1);
			yscale = 2.0f / (p.y2 - p.y1);
			xtrans = -(p.x2 + p.x1) / 2.0f;
			ytrans = -(p.y2 + p.y1) / 2.0f;

			CRASSERT(xscale == xscale); /* NaN test */
			CRASSERT(yscale == yscale);

			extent->baseProjection = identity_matrix;
			extent->baseProjection.m00 = xscale;
			extent->baseProjection.m11 = yscale;
			extent->baseProjection.m30 = xtrans * xscale;
			extent->baseProjection.m31 = ytrans * yscale;
		}

		extent->serialNo = serialNo++;
	}
	mural->viewportValidated = GL_TRUE;
#endif
}


/*
 * Issue the glScissor, glViewport and projection matrix needed for
 * rendering the tile specified by extNum.  We computed the scissor,
 * viewport and projection parameters above in crServerComputeViewportBounds.
 */
void
crServerSetOutputBounds( const CRMuralInfo *mural, int extNum )
{
#if 0
	const CRExtent *extent = mural->extents + extNum;
	CRASSERT(mural->viewportValidated);

	/*
	 * Serial Number info:
	 * Everytime we compute new scissor, viewport, projection matrix info for
	 * a tile, we give that tile a new serial number.
	 * When we're about to render into a tile, we only update the scissor,
	 * viewport and projection matrix if the tile's serial number doesn't match
	 * the current serial number.  This avoids a _LOT_ of redundant calls to
	 * those three functions.
	 */
	if (extent->serialNo != cr_server.currentSerialNo) {
		cr_server.head_spu->dispatch_table.Scissor(extent->scissorBox.x1,
																 extent->scissorBox.y1,
																 extent->scissorBox.x2 - extent->scissorBox.x1,
																 extent->scissorBox.y2 - extent->scissorBox.y1);

		cr_server.head_spu->dispatch_table.Viewport(extent->viewport.x1,
																 extent->viewport.y1,
																 extent->viewport.x2 - extent->viewport.x1,
																 extent->viewport.y2 - extent->viewport.y1);

		crServerApplyBaseProjection(&(extent->baseProjection));
		cr_server.currentSerialNo = extent->serialNo;
	}		
#endif
}


/*
 * Pre-multiply the current projection matrix with the current client's
 * base projection.  I.e.  P' = b * P.  Note that OpenGL's glMultMatrix
 * POST-multiplies.
 */
void
crServerApplyBaseProjection(const CRmatrix *baseProj)
{
	const CRmatrix *projMatrix;
	if (cr_server.projectionOverride) {
		int eye = crServerGetCurrentEye();
		projMatrix = &cr_server.projectionMatrix[eye];
	}
	else
		projMatrix = cr_server.curClient->currentCtxInfo->pContext->transform.projectionStack.top;

	cr_server.head_spu->dispatch_table.PushAttrib( GL_TRANSFORM_BIT );
	cr_server.head_spu->dispatch_table.MatrixMode( GL_PROJECTION );
	cr_server.head_spu->dispatch_table.LoadMatrixf( (const GLfloat *) baseProj );
	cr_server.head_spu->dispatch_table.MultMatrixf( cr_server.alignment_matrix );
	cr_server.head_spu->dispatch_table.MultMatrixf( (const GLfloat *) projMatrix );
	cr_server.head_spu->dispatch_table.PopAttrib();
}


void
crServerApplyViewMatrix(const CRmatrix *view)
{
	const CRmatrix *modelview = cr_server.curClient->currentCtxInfo->pContext->transform.modelViewStack.top;

	cr_server.head_spu->dispatch_table.PushAttrib( GL_TRANSFORM_BIT );
	cr_server.head_spu->dispatch_table.MatrixMode( GL_MODELVIEW );
	cr_server.head_spu->dispatch_table.LoadMatrixf( (const GLfloat *) view );
	cr_server.head_spu->dispatch_table.MultMatrixf( (const GLfloat *) modelview );
	cr_server.head_spu->dispatch_table.PopAttrib();
}


/*
 * Called via unpacker module.
 * Note: when there's a tilesort SPU upstream, the viewport dimensions
 * will typically match the mural size.  That is, the viewport dimensions
 * probably won't be the same values that the application issues.
 */
void SERVER_DISPATCH_APIENTRY crServerDispatchViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
	CRMuralInfo *mural = cr_server.curClient->currentMural;
	CRContext *ctx = crStateGetCurrent();

	if (ctx->viewport.viewportX != x ||
			ctx->viewport.viewportY != y ||
			ctx->viewport.viewportW != width ||
			ctx->viewport.viewportH != height) {
		 /* Note -- If there are tiles, this will be overridden in the 
			* process of decoding the BoundsInfo packet, so no worries. */
		 crStateViewport( x, y, width, height );
	}

	/* always dispatch to be safe */
	cr_server.head_spu->dispatch_table.Viewport( x, y, width, height );
}
