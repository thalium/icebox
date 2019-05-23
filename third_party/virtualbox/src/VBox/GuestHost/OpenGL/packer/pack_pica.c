/* Copyright (c) 2003, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"
#include "cr_string.h"

void PACK_APIENTRY crPackPicaListCompositors(const PICAuint *config, 
                     PICAint *numResults, 
                     PICAcompItem *results, PICAerror *return_value, int* writeback )
{
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) config;
     (void) numResults;
     (void) results;
     (void) writeback;
}
void PACK_APIENTRY crPackPicaListCompositorsSWAP(const PICAuint *config, 
                         PICAint *numResults, 
                         PICAcompItem *results, PICAerror *return_value, int* writeback )
{
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) config;
     (void) numResults;
     (void) results;
     (void) writeback;
}
void PACK_APIENTRY crPackPicaGetCompositorParamiv(PICAcompID compositor,
                          PICAparam pname,
                          PICAint *params, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaGetCompositorParamivSWAP(PICAcompID compositor,
                          PICAparam pname,
                          PICAint *params, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaGetCompositorParamfv(PICAcompID compositor,
                          PICAparam pname,
                          PICAfloat *params, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaGetCompositorParamfvSWAP(PICAcompID compositor,
                          PICAparam pname,
                          PICAfloat *params, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaGetCompositorParamcv(PICAcompID compositor,
                          PICAparam pname,
                          PICAchar **params, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaGetCompositorParamcvSWAP(PICAcompID compositor,
                          PICAparam pname,
                          PICAchar **params, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) pname;
     (void) params;
}
void PACK_APIENTRY crPackPicaListNodes(PICAcompID compositor, 
                   PICAint *num,
                   PICAnodeItem *results, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) num;
     (void) results;
}
void PACK_APIENTRY crPackPicaListNodesSWAP(PICAcompID compositor, 
                       PICAint *num,
                       PICAnodeItem *results, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) compositor;
     (void) num;
     (void) results;
}

void PACK_APIENTRY crPackPicaCreateContext(const PICAuint *config, 
                       const PICAnodeID *nodes, 
                       PICAuint numNodes, PICAcontextID *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = (PICAcontextID) -1;
     (void) writeback;
     (void) config;
     (void) nodes;
     (void) numNodes;
}
void PACK_APIENTRY crPackPicaCreateContextSWAP(const PICAuint *config, 
                       const PICAnodeID *nodes, 
                       PICAuint numNodes, PICAcontextID *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = (PICAcontextID) -1;
     (void) writeback;
     (void) config;
     (void) nodes;
     (void) numNodes;
}
void PACK_APIENTRY crPackPicaDestroyContext(PICAcontextID ctx, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
}
void PACK_APIENTRY crPackPicaDestroyContextSWAP(PICAcontextID ctx, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
}

void PACK_APIENTRY crPackPicaSetContextParami(PICAcontextID ctx,
                      PICAparam pname,
                      PICAint param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamiSWAP(PICAcontextID ctx,
                          PICAparam pname,
                          PICAint param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamiv(PICAcontextID ctx,
                       PICAparam pname,
                       const PICAint *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamivSWAP(PICAcontextID ctx,
                           PICAparam pname,
                           const PICAint *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamf(PICAcontextID ctx,
                      PICAparam pname,
                      PICAfloat param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamfSWAP(PICAcontextID ctx,
                          PICAparam pname,
                          PICAfloat param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamfv(PICAcontextID ctx,
                       PICAparam pname,
                       const PICAfloat *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamfvSWAP(PICAcontextID ctx,
                           PICAparam pname,
                           const PICAfloat *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamv(PICAcontextID ctx,
                      PICAparam pname,
                      const PICAvoid *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}
void PACK_APIENTRY crPackPicaSetContextParamvSWAP(PICAcontextID ctx,
                          PICAparam pname,
                          const PICAvoid *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}

void PACK_APIENTRY crPackPicaGetContextParamiv(PICAcontextID ctx,
                       PICAparam pname,
                       PICAint *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  

void PACK_APIENTRY crPackPicaGetContextParamivSWAP(PICAcontextID ctx,
                           PICAparam pname,
                           PICAint *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamfv(PICAcontextID ctx,
                       PICAparam pname,
                       PICAfloat *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamfvSWAP(PICAcontextID ctx,
                           PICAparam pname,
                           PICAfloat *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamcv(PICAcontextID ctx,
                       PICAparam pname,
                       PICAchar **param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamcvSWAP(PICAcontextID ctx,
                           PICAparam pname,
                           PICAchar **param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamv(PICAcontextID ctx,
                      PICAparam pname,
                      PICAvoid *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  
void PACK_APIENTRY crPackPicaGetContextParamvSWAP(PICAcontextID ctx,
                          PICAparam pname,
                          PICAvoid *param, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) pname;
     (void) param;
}  

void PACK_APIENTRY crPackPicaBindLocalContext(PICAcontextID globalCtx, 
                      PICAnodeID node, PICAcontextID *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = (PICAcontextID) -1;
     (void) writeback;
     (void) globalCtx;
     (void) node;
}

void PACK_APIENTRY crPackPicaBindLocalContextSWAP(PICAcontextID globalCtx, 
                          PICAnodeID node, PICAcontextID *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = (PICAcontextID) -1;
     (void) writeback;
     (void) globalCtx;
     (void) node;
}
void PACK_APIENTRY crPackPicaDestroyLocalContext(PICAcontextID lctx, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
}
void PACK_APIENTRY crPackPicaDestroyLocalContextSWAP(PICAcontextID lctx, PICAerror *return_value, int* writeback )
{

     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
}

void PACK_APIENTRY crPackPicaStartFrame(PICAcontextID lctx,
                    const PICAframeID *frameID,
                    PICAuint numLocalFramelets,
                    PICAuint numOrders,
                    const PICAuint *orderList,
                    const PICArect *srcRectList,
                    const PICApoint *dstList, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
     (void) frameID;
     (void) numLocalFramelets;
     (void) numOrders;
     (void) orderList;
     (void) srcRectList;
     (void) dstList;
}

void PACK_APIENTRY crPackPicaStartFrameSWAP(PICAcontextID lctx,
                    const PICAframeID *frameID,
                    PICAuint numLocalFramelets,
                    PICAuint numOrders,
                    const PICAuint *orderList,
                    const PICArect *srcRectList,
                    const PICApoint *dstList, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
     (void) frameID;
     (void) numLocalFramelets;
     (void) numOrders;
     (void) orderList;
     (void) srcRectList;
     (void) dstList;
}
void PACK_APIENTRY crPackPicaEndFrame(PICAcontextID lctx, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
}
void PACK_APIENTRY crPackPicaEndFrameSWAP(PICAcontextID lctx, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
}
void PACK_APIENTRY crPackPicaCancelFrame(PICAcontextID ctx, 
                     PICAframeID frameID, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) frameID;
}
void PACK_APIENTRY crPackPicaCancelFrameSWAP(PICAcontextID ctx, 
                     PICAframeID frameID, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) ctx;
     (void) frameID;
}
void PACK_APIENTRY crPackPicaQueryFrame(PICAcontextID ctx,
                    PICAframeID frameID,
                    PICAnodeID nodeID,
                    PICAfloat timeout, PICAstatus *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) ctx;
     (void) writeback;
     (void) frameID;
     (void) nodeID;
     (void) timeout;
}
void PACK_APIENTRY crPackPicaQueryFrameSWAP(PICAcontextID ctx,
                    PICAframeID frameID,
                    PICAnodeID nodeID,
                    PICAfloat timeout, PICAstatus *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) ctx;
     (void) writeback;
     (void) frameID;
     (void) nodeID;
     (void) timeout;
}
void PACK_APIENTRY crPackPicaAddGfxFramelet(PICAcontextID lctx,
                    const PICArect *srcRect,
                    const PICApoint *dstPos,
                    PICAuint order,
                    PICAint iVolatile, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) lctx;
     (void) writeback;
     (void) srcRect;
     (void) dstPos;
     (void) order;
     (void) iVolatile;
}
void PACK_APIENTRY crPackPicaAddGfxFrameletSWAP(PICAcontextID lctx,
                        const PICArect *srcRect,
                        const PICApoint *dstPos,
                        PICAuint order,
                        PICAint iVolatile, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) lctx;
     (void) writeback;
     (void) srcRect;
     (void) dstPos;
     (void) order;
     (void) iVolatile;
}
void PACK_APIENTRY crPackPicaAddMemFramelet(PICAcontextID lctx,
                    const PICAvoid *colorBuffer,
                    const PICAvoid *depthBuffer,
                    PICAuint span_x,
                    const PICArect *srcRect,
                    const PICApoint *dstPos,
                    PICAuint order,
                    PICAint iVolatile, PICAerror *return_value, int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) lctx;
     (void) writeback;
     (void) colorBuffer;
     (void) depthBuffer;
     (void) span_x;
     (void) srcRect;
     (void) dstPos;
     (void) order;
     (void) iVolatile;
}
void PACK_APIENTRY crPackPicaAddMemFrameletSWAP(PICAcontextID lctx,
                        const PICAvoid *colorBuffer,
                        const PICAvoid *depthBuffer,
                        PICAuint span_x,
                        const PICArect *srcRect,
                        const PICApoint *dstPos,
                        PICAuint order,
                        PICAint iVolatile, 
                        PICAerror *return_value, 
                        int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) lctx;
     (void) writeback;
     (void) colorBuffer;
     (void) depthBuffer;
     (void) span_x;
     (void) srcRect;
     (void) dstPos;
     (void) order;
     (void) iVolatile;
}
void PACK_APIENTRY crPackPicaReadFrame(PICAcontextID lctx,
                   PICAframeID frameID,
                   PICAuint format,
                   PICAvoid *colorbuffer,
                   PICAvoid *depthbuffer,
                   const PICArect *rect, 
                   PICAerror *return_value, 
                   int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
     (void) frameID;
     (void) format;
     (void) colorbuffer;
     (void) depthbuffer;
     (void) rect;
}
void PACK_APIENTRY crPackPicaReadFrameSWAP(PICAcontextID lctx,
                       PICAframeID frameID,
                       PICAuint format,
                       PICAvoid *colorbuffer,
                       PICAvoid *depthbuffer,
                       const PICArect *rect, 
                       PICAerror *return_value, 
                       int* writeback )
{
     
     crWarning("You can't really pack PICA calls!");
     *return_value = -1;
     (void) writeback;
     (void) lctx;
     (void) frameID;
     (void) format;
     (void) colorbuffer;
     (void) depthbuffer;
     (void) rect;
}
