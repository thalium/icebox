/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "chromium.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

#ifdef VBOXCR_LOGFPS
#include <iprt/timer.h>
#include <iprt/ctype.h>
typedef struct VBOXCRFPS
{
    uint64_t mPeriodSum;
    uint64_t *mpaPeriods;
    uint64_t mPrevTime;
    uint64_t mcFrames;
    uint32_t mcPeriods;
    uint32_t miPeriod;

    uint64_t mBytesSum;
    uint32_t *mpaBytes;

    uint64_t mBytesSentSum;
    uint32_t *mpaBytesSent;

    uint64_t mCallsSum;
    uint32_t *mpaCalls;

    uint64_t mOpsSum;
    uint32_t *mpaOps;

    uint64_t mTimeUsedSum;
    uint64_t *mpaTimes;
} VBOXCRFPS, *PVBOXCRFPS;

void vboxCrFpsInit(PVBOXCRFPS pFps, uint32_t cPeriods)
{
    crMemset(pFps, 0, sizeof (*pFps));
    pFps->mcPeriods = cPeriods;
    pFps->mpaPeriods = crCalloc(sizeof (pFps->mpaPeriods[0]) * cPeriods);
    pFps->mpaBytes = crCalloc(sizeof (pFps->mpaBytes[0]) * cPeriods);
    pFps->mpaBytesSent = crCalloc(sizeof (pFps->mpaBytesSent[0]) * cPeriods);
    pFps->mpaCalls = crCalloc(sizeof (pFps->mpaCalls[0]) * cPeriods);
    pFps->mpaOps = crCalloc(sizeof (pFps->mpaOps[0]) * cPeriods);
    pFps->mpaTimes = crCalloc(sizeof (pFps->mpaTimes[0]) * cPeriods);
}

void vboxCrFpsTerm(PVBOXCRFPS pFps)
{
    crFree(pFps->mpaPeriods);
    crFree(pFps->mpaBytes);
    crFree(pFps->mpaCalls);
}

void vboxCrFpsReportFrame(PVBOXCRFPS pFps)
{
    uint64_t cur = RTTimeNanoTS();
    uint64_t curBytes, curBytesSent, curCalls, curOps, curTimeUsed;
    int i;

    curBytes = 0;
    curBytesSent = 0;
    curCalls = 0;
    curOps = 0;
    curTimeUsed = 0;

    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn)
        {
            curBytes += cr_server.clients[i]->conn->total_bytes_recv;
            curBytesSent += cr_server.clients[i]->conn->total_bytes_sent;
            curCalls += cr_server.clients[i]->conn->recv_count;
            curOps += cr_server.clients[i]->conn->opcodes_count;
            curTimeUsed += cr_server.clients[i]->timeUsed;
            cr_server.clients[i]->conn->total_bytes_recv = 0;
            cr_server.clients[i]->conn->total_bytes_sent = 0;
            cr_server.clients[i]->conn->recv_count = 0;
            cr_server.clients[i]->conn->opcodes_count = 0;
            cr_server.clients[i]->timeUsed = 0;
        }
    }

    if(pFps->mPrevTime)
    {
        uint64_t curPeriod = cur - pFps->mPrevTime;

        pFps->mPeriodSum += curPeriod - pFps->mpaPeriods[pFps->miPeriod];
        pFps->mpaPeriods[pFps->miPeriod] = curPeriod;

        pFps->mBytesSum += curBytes - pFps->mpaBytes[pFps->miPeriod];
        pFps->mpaBytes[pFps->miPeriod] = curBytes;

        pFps->mBytesSentSum += curBytesSent - pFps->mpaBytesSent[pFps->miPeriod];
        pFps->mpaBytesSent[pFps->miPeriod] = curBytesSent;

        pFps->mCallsSum += curCalls - pFps->mpaCalls[pFps->miPeriod];
        pFps->mpaCalls[pFps->miPeriod] = curCalls;

        pFps->mOpsSum += curOps - pFps->mpaOps[pFps->miPeriod];
        pFps->mpaOps[pFps->miPeriod] = curOps;

        pFps->mTimeUsedSum += curTimeUsed - pFps->mpaTimes[pFps->miPeriod];
        pFps->mpaTimes[pFps->miPeriod] = curTimeUsed;

        ++pFps->miPeriod;
        pFps->miPeriod %= pFps->mcPeriods;
    }
    pFps->mPrevTime = cur;
    ++pFps->mcFrames;
}

uint64_t vboxCrFpsGetEveragePeriod(PVBOXCRFPS pFps)
{
    return pFps->mPeriodSum / pFps->mcPeriods;
}

double vboxCrFpsGetFps(PVBOXCRFPS pFps)
{
    return ((double)1000000000.0) / vboxCrFpsGetEveragePeriod(pFps);
}

double vboxCrFpsGetBps(PVBOXCRFPS pFps)
{
    return vboxCrFpsGetFps(pFps) * pFps->mBytesSum / pFps->mcPeriods;
}

double vboxCrFpsGetBpsSent(PVBOXCRFPS pFps)
{
    return vboxCrFpsGetFps(pFps) * pFps->mBytesSentSum / pFps->mcPeriods;
}

double vboxCrFpsGetCps(PVBOXCRFPS pFps)
{
    return vboxCrFpsGetFps(pFps) * pFps->mCallsSum / pFps->mcPeriods;
}

double vboxCrFpsGetOps(PVBOXCRFPS pFps)
{
    return vboxCrFpsGetFps(pFps) * pFps->mOpsSum / pFps->mcPeriods;
}

double vboxCrFpsGetTimeProcPercent(PVBOXCRFPS pFps)
{
    return 100.0*pFps->mTimeUsedSum/pFps->mPeriodSum;
}

uint64_t vboxCrFpsGetNumFrames(PVBOXCRFPS pFps)
{
    return pFps->mcFrames;
}

#endif


void SERVER_DISPATCH_APIENTRY crServerDispatchClear( GLenum mask )
{
	CRMuralInfo *mural = cr_server.curClient->currentMural;
	const RunQueue *q = cr_server.run_queue;

	if (cr_server.only_swap_once)
	{
		/* NOTE: we only do the clear for the _last_ client in the list.
		 * This is because in multi-threaded apps the zeroeth client may
		 * be idle and never call glClear at all.  See threadtest.c
		 * It's pretty likely that the last client will be active.
		 */
		if ((mask & GL_COLOR_BUFFER_BIT) &&
			(cr_server.curClient != cr_server.clients[cr_server.numClients - 1]))
		   return;
	}

	cr_server.head_spu->dispatch_table.Clear( mask );
}

static void __draw_poly(CRPoly *p)
{
	int b;

	cr_server.head_spu->dispatch_table.Begin(GL_POLYGON);
	for (b=0; b<p->npoints; b++)
		cr_server.head_spu->dispatch_table.Vertex2dv(p->points+2*b);
	cr_server.head_spu->dispatch_table.End();
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchSwapBuffers( GLint window, GLint flags )
{
  CRMuralInfo *mural;
  CRContext *ctx;

#ifdef VBOXCR_LOGFPS
  static VBOXCRFPS Fps;
  static bool bFpsInited = false;

  if (!bFpsInited)
  {
      vboxCrFpsInit(&Fps, 64 /* cPeriods */);
      bFpsInited = true;
  }
  vboxCrFpsReportFrame(&Fps);
  if(!(vboxCrFpsGetNumFrames(&Fps) % 31))
  {
      double fps = vboxCrFpsGetFps(&Fps);
      double bps = vboxCrFpsGetBps(&Fps);
      double bpsSent = vboxCrFpsGetBpsSent(&Fps);
      double cps = vboxCrFpsGetCps(&Fps);
      double ops = vboxCrFpsGetOps(&Fps);
      double tup = vboxCrFpsGetTimeProcPercent(&Fps);
      crDebug("fps: %f, rec Mbps: %.1f, send Mbps: %.1f, cps: %.1f, ops: %.0f, host %.1f%%", 
              fps, bps/(1024.0*1024.0), bpsSent/(1024.0*1024.0), cps, ops, tup);
  }
#endif
	mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
	if (!mural) {
		 return;
	}


	if (cr_server.only_swap_once)
	{
		/* NOTE: we only do the clear for the _last_ client in the list.
		 * This is because in multi-threaded apps the zeroeth client may
		 * be idle and never call glClear at all.  See threadtest.c
		 * It's pretty likely that the last client will be active.
		 */
		if (cr_server.curClient != cr_server.clients[cr_server.numClients - 1])
		{
			return;
		}
	}

#if 0
	if (cr_server.overlapBlending)
	{
		int a;
		CRPoly *p;
		GLboolean lighting, fog, blend, cull, tex[3];
		GLenum mm, blendSrc, blendDst;
		GLcolorf col;
		CRContext *ctx = crStateGetCurrent();
		const CRmatrix *baseProj;

		/* 
		 * I've probably missed some state here, or it
	 	 * might be easier just to push/pop it....
		 */
		lighting = ctx->lighting.lighting;
		fog		 = ctx->fog.enable;
		tex[0] = 0;
		for (a=0; a<CR_MAX_TEXTURE_UNITS; a++)
		{
			if (!ctx->texture.unit[a].enabled1D) continue;
	
			tex[0] 	 = 1;
			break;
		}
		tex[1] = 0;
		for	(a=0; a<CR_MAX_TEXTURE_UNITS; a++)
		{
			if (!ctx->texture.unit[a].enabled2D) continue;

			tex[1] 	 = 1;
			break;
		}
		tex[2] = 0;
		for (a=0; a<CR_MAX_TEXTURE_UNITS; a++)
		{	
			if (!ctx->texture.unit[a].enabled3D) continue;
	
			tex[2] 	 = 1;
			break;
		}

		cull	 = ctx->polygon.cullFace;
		blend	 = ctx->buffer.blend;
		blendSrc = ctx->buffer.blendSrcRGB;
		blendDst = ctx->buffer.blendDstRGB;
		mm = ctx->transform.matrixMode;
		col.r = ctx->current.vertexAttrib[VERT_ATTRIB_COLOR0][0];
		col.g = ctx->current.vertexAttrib[VERT_ATTRIB_COLOR0][1];
		col.b = ctx->current.vertexAttrib[VERT_ATTRIB_COLOR0][2];
		col.a = ctx->current.vertexAttrib[VERT_ATTRIB_COLOR0][3];

		baseProj = &(cr_server.curClient->currentMural->extents[0].baseProjection);
	
		switch(mm)
		{
				case GL_PROJECTION:
					cr_server.head_spu->dispatch_table.PushMatrix();
					cr_server.head_spu->dispatch_table.LoadMatrixf((GLfloat *) baseProj);
					cr_server.head_spu->dispatch_table.MultMatrixf(cr_server.unnormalized_alignment_matrix);
					cr_server.head_spu->dispatch_table.MatrixMode(GL_MODELVIEW);
					cr_server.head_spu->dispatch_table.PushMatrix();
					cr_server.head_spu->dispatch_table.LoadIdentity();
					break;
				
				default:
					cr_server.head_spu->dispatch_table.MatrixMode(GL_MODELVIEW);
					/* fall through */
	
				case GL_MODELVIEW:
					cr_server.head_spu->dispatch_table.PushMatrix();
					cr_server.head_spu->dispatch_table.LoadIdentity();
					cr_server.head_spu->dispatch_table.MatrixMode(GL_PROJECTION);
					cr_server.head_spu->dispatch_table.PushMatrix();
					cr_server.head_spu->dispatch_table.LoadMatrixf((GLfloat *) baseProj);
					cr_server.head_spu->dispatch_table.MultMatrixf(cr_server.unnormalized_alignment_matrix);
					break;	
		}
	
		/* fix state */
		if (lighting)
			cr_server.head_spu->dispatch_table.Disable(GL_LIGHTING);
		if (fog)
			cr_server.head_spu->dispatch_table.Disable(GL_FOG);
		if (tex[0])
			cr_server.head_spu->dispatch_table.Disable(GL_TEXTURE_1D);
		if (tex[1])
			cr_server.head_spu->dispatch_table.Disable(GL_TEXTURE_2D);
		if (tex[2])
			cr_server.head_spu->dispatch_table.Disable(GL_TEXTURE_3D);
		if (cull)
			cr_server.head_spu->dispatch_table.Disable(GL_CULL_FACE);

		/* Regular Blending */
		if (cr_server.overlapBlending == 1)
		{
			if (!blend)
				cr_server.head_spu->dispatch_table.Enable(GL_BLEND);
			if ((blendSrc != GL_ZERO) && (blendDst != GL_SRC_ALPHA))
				cr_server.head_spu->dispatch_table.BlendFunc(GL_ZERO, GL_SRC_ALPHA);

			/* draw the blends */
			for (a=1; a<cr_server.num_overlap_levels; a++)
			{
				if (a-1 < cr_server.num_overlap_intens)
				{
					cr_server.head_spu->dispatch_table.Color4f(0, 0, 0, 
											cr_server.overlap_intens[a-1]);
				}
				else
				{
					cr_server.head_spu->dispatch_table.Color4f(0, 0, 0, 1);
				}
		
				p = cr_server.overlap_geom[a];
				while (p)
				{
					/* hopefully this isnt concave... */
					__draw_poly(p);
					p = p->next;
				}	
			}

			if (!blend)
				cr_server.head_spu->dispatch_table.Disable(GL_BLEND);
			if ((blendSrc != GL_ZERO) && (blendDst != GL_SRC_ALPHA))
				cr_server.head_spu->dispatch_table.BlendFunc(blendSrc, blendDst);
		}
		else
		/* Knockout Blending */
		{
			cr_server.head_spu->dispatch_table.Color4f(0, 0, 0, 1);

			if (blend)
				cr_server.head_spu->dispatch_table.Disable(GL_BLEND);
			p = cr_server.overlap_knockout;
			while (p)
			{
				__draw_poly(p);
				p = p->next;
			}	
			if (blend)
				cr_server.head_spu->dispatch_table.Enable(GL_BLEND);
		}


		/* return things to normal */
		switch (mm)
		{
			case GL_PROJECTION:
				cr_server.head_spu->dispatch_table.PopMatrix();
				cr_server.head_spu->dispatch_table.MatrixMode(GL_PROJECTION);
				cr_server.head_spu->dispatch_table.PopMatrix();
				break;
			case GL_MODELVIEW:
				cr_server.head_spu->dispatch_table.PopMatrix();
				cr_server.head_spu->dispatch_table.MatrixMode(GL_MODELVIEW);
				cr_server.head_spu->dispatch_table.PopMatrix();
				break;
			default:
				cr_server.head_spu->dispatch_table.PopMatrix();
				cr_server.head_spu->dispatch_table.MatrixMode(GL_MODELVIEW);
				cr_server.head_spu->dispatch_table.PopMatrix();
				cr_server.head_spu->dispatch_table.MatrixMode(mm);
				break;
		}

		if (lighting)
			cr_server.head_spu->dispatch_table.Enable(GL_LIGHTING);
		if (fog)
			cr_server.head_spu->dispatch_table.Enable(GL_FOG);
		if (tex[0])
			cr_server.head_spu->dispatch_table.Enable(GL_TEXTURE_1D);
		if (tex[1])
			cr_server.head_spu->dispatch_table.Enable(GL_TEXTURE_2D);
		if (tex[2])
			cr_server.head_spu->dispatch_table.Enable(GL_TEXTURE_3D);
		if (cull)
			cr_server.head_spu->dispatch_table.Enable(GL_CULL_FACE);
	
		cr_server.head_spu->dispatch_table.Color4f(col.r, col.g, col.b, col.a);
	}
#endif

	/* Check if using a file network */
	if (!cr_server.clients[0]->conn->actual_network && window == MAGIC_OFFSET)
		window = 0;

	ctx = crStateGetCurrent();

	CRASSERT(cr_server.curClient && cr_server.curClient->currentMural == mural);

    if (ctx->framebufferobject.drawFB
            || (ctx->buffer.drawBuffer != GL_FRONT && ctx->buffer.drawBuffer != GL_FRONT_LEFT))
        mural->bFbDraw = GL_FALSE;

    CR_SERVER_DUMP_SWAPBUFFERS_ENTER();

    if (crServerIsRedirectedToFBO())
    {
        crServerMuralFBOSwapBuffers(mural);
        crServerPresentFBO(mural);
    }
    else
    {
        cr_server.head_spu->dispatch_table.SwapBuffers( mural->spuWindow, flags );
    }

    CR_SERVER_DUMP_SWAPBUFFERS_LEAVE();
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchFlush(void)
{
    CRContext *ctx = crStateGetCurrent();
    cr_server.head_spu->dispatch_table.Flush();

    if (cr_server.curClient && cr_server.curClient->currentMural)
    {
        CRMuralInfo *mural = cr_server.curClient->currentMural;
        if (mural->bFbDraw)
        {
            if (crServerIsRedirectedToFBO())
                crServerPresentFBO(mural);
        }

        if (ctx->framebufferobject.drawFB
                || (ctx->buffer.drawBuffer != GL_FRONT && ctx->buffer.drawBuffer != GL_FRONT_LEFT))
            mural->bFbDraw = GL_FALSE;
    }
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchFinish(void)
{
    CRContext *ctx = crStateGetCurrent();

    cr_server.head_spu->dispatch_table.Finish();

    if (cr_server.curClient && cr_server.curClient->currentMural)
    {
        CRMuralInfo *mural = cr_server.curClient->currentMural;
        if (mural->bFbDraw)
        {
            if (crServerIsRedirectedToFBO())
                crServerPresentFBO(mural);
        }

        if (ctx->framebufferobject.drawFB
                || (ctx->buffer.drawBuffer != GL_FRONT && ctx->buffer.drawBuffer != GL_FRONT_LEFT))
            mural->bFbDraw = GL_FALSE;
    }
}
