/* $Id: server_rpw.cpp $ */

/** @file
 * VBox crOpenGL: Read Pixels worker
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#include "server.h"
#include "cr_string.h"
#include "cr_mem.h"
#include "cr_vreg.h"
#include "render/renderspu.h"

static void crServerRpwWorkerGpuSubmit(PRTLISTNODE pWorkList)
{
    CR_SERVER_RPW_ENTRY *pCurEntry;
    RTListForEach(pWorkList, pCurEntry, CR_SERVER_RPW_ENTRY, WorkerWorkEntry)
    {
        cr_server.head_spu->dispatch_table.BindTexture(GL_TEXTURE_2D, CR_SERVER_RPW_ENTRY_TEX(pCurEntry, Worker));

        if (CR_SERVER_RPW_ENTRY_PBO_IS_ACTIVE(pCurEntry))
        {
            cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, CR_SERVER_RPW_ENTRY_PBO_CUR(pCurEntry));
            /*read the texture, note pixels are NULL for PBO case as it's offset in the buffer*/
            cr_server.head_spu->dispatch_table.GetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
            CR_SERVER_RPW_ENTRY_PBO_FLIP(pCurEntry);
        }
        else
        {
            void *pvData = crAlloc(4*pCurEntry->Size.cx*pCurEntry->Size.cy);
            if (pvData)
            {
                cr_server.head_spu->dispatch_table.GetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pvData);

                pCurEntry->pfnData(pCurEntry, pvData);

                crFree(pvData);
            }
            else
            {
                crWarning("crAlloc failed");
            }
        }

        cr_server.head_spu->dispatch_table.BindTexture(GL_TEXTURE_2D, 0);
    }
}

static void crServerRpwWorkerGpuComplete(PRTLISTNODE pGpuSubmitedList)
{
    CR_SERVER_RPW_ENTRY *pCurEntry;
    RTListForEach(pGpuSubmitedList, pCurEntry, CR_SERVER_RPW_ENTRY, GpuSubmittedEntry)
    {
        Assert(CR_SERVER_RPW_ENTRY_PBO_IS_ACTIVE(pCurEntry));

        cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, CR_SERVER_RPW_ENTRY_PBO_COMPLETED(pCurEntry));

        void *pvData = cr_server.head_spu->dispatch_table.MapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);

        pCurEntry->pfnData(pCurEntry, pvData);

        cr_server.head_spu->dispatch_table.UnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);

        cr_server.head_spu->dispatch_table.BufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, pCurEntry->Size.cx*pCurEntry->Size.cy*4, 0, GL_STREAM_READ_ARB);

        cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }
}

static void crServerRpwWorkerGpuMarkGpuCompletedSubmitedLocked(PRTLISTNODE pGpuSubmitedList, PRTLISTNODE pWorkList)
{
    CR_SERVER_RPW_ENTRY *pCurEntry, *pNextEntry;
    RTListForEachSafe(pGpuSubmitedList, pCurEntry, pNextEntry, CR_SERVER_RPW_ENTRY, GpuSubmittedEntry)
    {
        CR_SERVER_RPW_ENTRY_TEX_INVALIDATE(pCurEntry, Gpu);
        RTListNodeRemove(&pCurEntry->GpuSubmittedEntry);
    }

    Assert(RTListIsEmpty(pGpuSubmitedList));

    RTListForEachSafe(pWorkList, pCurEntry, pNextEntry, CR_SERVER_RPW_ENTRY, WorkerWorkEntry)
    {
        Assert(CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pCurEntry, Worker));
        RTListNodeRemove(&pCurEntry->WorkerWorkEntry);
        if (CR_SERVER_RPW_ENTRY_PBO_IS_ACTIVE(pCurEntry))
        {
            /* PBO mode, put to the GPU submitted queue*/
            RTListAppend(pGpuSubmitedList, &pCurEntry->GpuSubmittedEntry);
            CR_SERVER_RPW_ENTRY_TEX_PROMOTE(pCurEntry, Worker, Gpu);
        }
        else
        {
            /* no PBO, we are already done entry data processing, free it right away */
            Assert(!CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pCurEntry, Gpu));
            CR_SERVER_RPW_ENTRY_TEX_INVALIDATE(pCurEntry, Worker);
        }
    }
}

static void crServerRpwWorkerGetWorkLocked(CR_SERVER_RPW *pWorker, PRTLISTNODE pWorkList)
{
    CR_SERVER_RPW_ENTRY *pCurEntry, *pNextEntry;
    RTListForEachSafe(&pWorker->WorkList, pCurEntry, pNextEntry, CR_SERVER_RPW_ENTRY, WorkEntry)
    {
        RTListNodeRemove(&pCurEntry->WorkEntry);
        RTListAppend(pWorkList, &pCurEntry->WorkerWorkEntry);
        CR_SERVER_RPW_ENTRY_TEX_PROMOTE(pCurEntry, Submitted, Worker);
    }
}

static DECLCALLBACK(int) crServerRpwWorkerThread(RTTHREAD ThreadSelf, void *pvUser)
{
    CR_SERVER_RPW *pWorker = (CR_SERVER_RPW *)pvUser;
    RTMSINTERVAL cWaitMillis = RT_INDEFINITE_WAIT;
    RTLISTNODE WorkList, GpuSubmittedList;
    CR_SERVER_RPW_CTL_TYPE enmCtlType = CR_SERVER_RPW_CTL_TYPE_UNDEFINED;
    CR_SERVER_RPW_ENTRY *pCtlEntry = NULL;
    CRMuralInfo *pDummyMural = crServerGetDummyMural(pWorker->ctxVisBits);
    bool fExit = false;
    bool fForceComplete = false;
    bool fNotifyCmdCompleted = false;

    CRASSERT(pDummyMural);

    int rc = RTSemEventSignal(pWorker->Ctl.hCompleteEvent);
    if (!RT_SUCCESS(rc))
    {
        crWarning("RTSemEventSignal failed rc %d", rc);
        return rc;
    }

    RTListInit(&WorkList);
    RTListInit(&GpuSubmittedList);

    cr_server.head_spu->dispatch_table.MakeCurrent(pDummyMural->spuWindow, 0, pWorker->ctxId);

    rc = RTCritSectEnter(&pWorker->CritSect);
    if (!RT_SUCCESS(rc))
    {
        crWarning("RTCritSectEnter failed, rc %d", rc);
        goto end;
    }

    for (;;)
    {
        /* the crit sect is locked here */

        if (pWorker->Ctl.enmType != CR_SERVER_RPW_CTL_TYPE_UNDEFINED)
        {
            enmCtlType = pWorker->Ctl.enmType;
            pCtlEntry = pWorker->Ctl.pEntry;
            pWorker->Ctl.enmType = CR_SERVER_RPW_CTL_TYPE_UNDEFINED;
            pWorker->Ctl.pEntry = NULL;
        }

        crServerRpwWorkerGetWorkLocked(pWorker, &WorkList);

        RTCritSectLeave(&pWorker->CritSect);

        if (enmCtlType != CR_SERVER_RPW_CTL_TYPE_UNDEFINED)
        {
            switch (enmCtlType)
            {
                case CR_SERVER_RPW_CTL_TYPE_WAIT_COMPLETE:
                    break;
                case CR_SERVER_RPW_CTL_TYPE_TERM:
                    fExit = true;
                    break;
                default:
                    crWarning("unexpected CtlType %d", enmCtlType);
                    break;
            }
            enmCtlType = CR_SERVER_RPW_CTL_TYPE_UNDEFINED;
            pCtlEntry = NULL;
            fNotifyCmdCompleted = true;
        }

        bool fNewItems = !RTListIsEmpty(&WorkList);
        bool fCompleted = false;

        if (fNewItems)
        {
            crServerRpwWorkerGpuSubmit(&WorkList);
        }

        if (!RTListIsEmpty(&GpuSubmittedList))
        {
            if (fForceComplete || fNewItems)
            {
                crServerRpwWorkerGpuComplete(&GpuSubmittedList);
                fForceComplete = false;
                fCompleted = true;
            }
        }

        rc = RTCritSectEnter(&pWorker->CritSect);
        if (!RT_SUCCESS(rc))
        {
            crWarning("RTCritSectEnter failed, rc %d", rc);
            break;
        }

        /* fNewGpuItems means new entries arrived. WorkList contains new GPU submitted data
         * fCompleted means completion was performed, GpuSubmittedList contains old GPU submitted data,
         * which is now completed and should be released */
        if (fNewItems || fCompleted)
        {
            crServerRpwWorkerGpuMarkGpuCompletedSubmitedLocked(&GpuSubmittedList, &WorkList);
        }

        if (fExit || !fNewItems)
        {
            RTCritSectLeave(&pWorker->CritSect);

            if (fNotifyCmdCompleted)
            {
                rc = RTSemEventSignal(pWorker->Ctl.hCompleteEvent);
                if (!RT_SUCCESS(rc))
                {
                    crWarning("RTSemEventSignal failed rc %d", rc);
                    break;
                }
                fNotifyCmdCompleted = false;
            }

            if (fExit)
                break;

            if (!RTListIsEmpty(&GpuSubmittedList))
                cWaitMillis = 17; /* ~60Hz */
            else
                cWaitMillis = RT_INDEFINITE_WAIT;

            rc = RTSemEventWait(pWorker->hSubmitEvent, cWaitMillis);
            if (!RT_SUCCESS(rc) && rc != VERR_TIMEOUT)
            {
                crWarning("RTSemEventWait failed, rc %d", rc);
                break;
            }

            if (rc == VERR_TIMEOUT)
            {
                Assert(!RTListIsEmpty(&GpuSubmittedList));
                fForceComplete = true;
            }

            rc = RTCritSectEnter(&pWorker->CritSect);
            if (!RT_SUCCESS(rc))
            {
                crWarning("RTCritSectEnter failed, rc %d", rc);
                break;
            }
        }
    }

end:
    cr_server.head_spu->dispatch_table.MakeCurrent(0, 0, 0);

    return rc;
}

static int crServerRpwCtlNotify(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry)
{
    int rc = RTSemEventSignal(pWorker->hSubmitEvent);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventWait(pWorker->Ctl.hCompleteEvent, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc))
        {
            rc = pWorker->Ctl.rc;
            if (!RT_SUCCESS(rc))
            {
                crWarning("WdCtl command failed rc %d", rc);
            }
        }
        else
        {
            crWarning("RTSemEventWait failed rc %d", rc);
        }
    }
    else
    {
        int tmpRc;
        crWarning("RTSemEventSignal failed rc %d", rc);
        tmpRc = RTCritSectEnter(&pWorker->CritSect);
        if (RT_SUCCESS(tmpRc))
        {
            pWorker->Ctl.enmType = CR_SERVER_RPW_CTL_TYPE_UNDEFINED;
            pWorker->Ctl.pEntry = NULL;
            RTCritSectLeave(&pWorker->CritSect);
        }
        else
        {
            crWarning("RTSemEventSignal failed tmpRc %d", tmpRc);
        }
    }

    return rc;
}

static int crServerRpwCtl(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_CTL_TYPE enmType, CR_SERVER_RPW_ENTRY *pEntry)
{
    int rc = RTCritSectEnter(&pWorker->CritSect);
    if (RT_SUCCESS(rc))
    {
        pWorker->Ctl.enmType = enmType;
        pWorker->Ctl.pEntry = pEntry;
        RTCritSectLeave(&pWorker->CritSect);
    }
    else
    {
        crWarning("RTCritSectEnter failed rc %d", rc);
        return rc;
    }

    rc = crServerRpwCtlNotify(pWorker, pEntry);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwCtlNotify failed rc %d", rc);
        return rc;
    }
    return VINF_SUCCESS;
}

int crServerRpwInit(CR_SERVER_RPW *pWorker)
{
    int rc;

    memset(pWorker, 0, sizeof (*pWorker));

    RTListInit(&pWorker->WorkList);

    rc = RTCritSectInit(&pWorker->CritSect);
    if (RT_SUCCESS(rc))
    {
        rc =  RTSemEventCreate(&pWorker->hSubmitEvent);
        if (RT_SUCCESS(rc))
        {
            rc =  RTSemEventCreate(&pWorker->Ctl.hCompleteEvent);
            if (RT_SUCCESS(rc))
            {
                CRASSERT(cr_server.MainContextInfo.CreateInfo.realVisualBits);
                CRASSERT(cr_server.MainContextInfo.SpuContext);

                pWorker->ctxId = cr_server.head_spu->dispatch_table.CreateContext("", cr_server.MainContextInfo.CreateInfo.realVisualBits, cr_server.MainContextInfo.SpuContext);
                if (pWorker->ctxId)
                {
                    CRMuralInfo *pDummyMural;
                    pWorker->ctxVisBits = cr_server.MainContextInfo.CreateInfo.realVisualBits;
                    pDummyMural = crServerGetDummyMural(pWorker->ctxVisBits);
                    if (pDummyMural)
                    {
                        /* since CreateContext does not actually create it on some platforms, e.g. on win,
                         * we need to do MakeCurrent to ensure it is created.
                         * There is some black magic in doing that to work around ogl driver bugs
                         * (i.e. we need to switch offscreen rendering off before doing make current) */
                        CR_SERVER_CTX_SWITCH CtxSwitch;

                        crServerCtxSwitchPrepare(&CtxSwitch, NULL);

                        cr_server.head_spu->dispatch_table.Flush();

                        cr_server.head_spu->dispatch_table.MakeCurrent(pDummyMural->spuWindow, 0, pWorker->ctxId);

                        if (cr_server.currentCtxInfo)
                        {
                            CRASSERT(cr_server.currentMural);
                            cr_server.head_spu->dispatch_table.MakeCurrent(cr_server.currentMural->spuWindow, 0,
                                    cr_server.currentCtxInfo->SpuContext > 0 ? cr_server.currentCtxInfo->SpuContext : cr_server.MainContextInfo.SpuContext);
                        }
                        else
                            cr_server.head_spu->dispatch_table.MakeCurrent(CR_RENDER_DEFAULT_WINDOW_ID, 0, CR_RENDER_DEFAULT_CONTEXT_ID);

                        crServerCtxSwitchPostprocess(&CtxSwitch);

                        rc = RTThreadCreate(&pWorker->hThread, crServerRpwWorkerThread, pWorker, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "CrServerDw");
                        if (RT_SUCCESS(rc))
                        {
                            rc = RTSemEventWait(pWorker->Ctl.hCompleteEvent, RT_INDEFINITE_WAIT);
                            if (RT_SUCCESS(rc))
                            {
                                return VINF_SUCCESS;
                            }
                            else
                            {
                                crWarning("RTSemEventWait failed rc %d", rc);
                            }
                        }
                        else
                        {
                            crWarning("RTThreadCreate failed rc %d", rc);
                        }
                    }
                    else
                    {
                        crWarning("Failed to get dummy mural");
                        rc = VERR_GENERAL_FAILURE;
                    }
                    cr_server.head_spu->dispatch_table.DestroyContext(pWorker->ctxId);
                }
                else
                {
                    crWarning("CreateContext failed rc %d", rc);
                }

                RTSemEventDestroy(pWorker->Ctl.hCompleteEvent);
            }
            else
            {
                crWarning("RTSemEventCreate failed rc %d", rc);
            }
            RTSemEventDestroy(pWorker->hSubmitEvent);
        }
        else
        {
            crWarning("RTSemEventCreate failed rc %d", rc);
        }

        RTCritSectDelete(&pWorker->CritSect);
    }
    else
    {
        crWarning("RTCritSectInit failed rc %d", rc);
    }

    return rc;
}

int crServerRpwEntryResizeCleaned(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry, uint32_t width, uint32_t height)
{
    CRContext *pContext;
    if (!width || !height)
    {
        return VINF_SUCCESS;
    }

    if (!cr_server.currentCtxInfo)
    {
        CRMuralInfo *pDummy = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        if (!pDummy)
        {
            crWarning("crServerGetDummyMural failed");
            return VERR_GENERAL_FAILURE;
        }


        crServerPerformMakeCurrent(pDummy, &cr_server.MainContextInfo);
    }

    Assert(width);
    Assert(height);

    pContext = cr_server.currentCtxInfo->pContext;

    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }

    for (int i = 0; i < 4; ++i)
    {
        cr_server.head_spu->dispatch_table.GenTextures(1, &pEntry->aidWorkerTexs[i]);

        cr_server.head_spu->dispatch_table.BindTexture(GL_TEXTURE_2D, pEntry->aidWorkerTexs[i]);
        cr_server.head_spu->dispatch_table.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        cr_server.head_spu->dispatch_table.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        cr_server.head_spu->dispatch_table.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        cr_server.head_spu->dispatch_table.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        cr_server.head_spu->dispatch_table.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
                       0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }

    pEntry->iTexDraw = -pEntry->iTexDraw;

    if (crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pContext->bufferobject.unpackBuffer->hwid);
    }

    if (cr_server.bUsePBOForReadback)
    {
        for (int i = 0; i < 2; ++i)
        {
            cr_server.head_spu->dispatch_table.GenBuffersARB(1, &pEntry->aidPBOs[i]);
            cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pEntry->aidPBOs[i]);
            cr_server.head_spu->dispatch_table.BufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, width*height*4, 0, GL_STREAM_READ_ARB);
        }

        if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
        {
            cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pContext->bufferobject.packBuffer->hwid);
        }
        else
        {
            cr_server.head_spu->dispatch_table.BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
        }
        pEntry->iCurPBO = 0;
    }


    GLuint uid = pContext->texture.unit[pContext->texture.curTextureUnit].currentTexture2D->hwid;
    cr_server.head_spu->dispatch_table.BindTexture(GL_TEXTURE_2D, uid);


    pEntry->Size.cx = width;
    pEntry->Size.cy = height;

    crServerRpwEntryDbgVerify(pEntry);

    return VINF_SUCCESS;
}

int crServerRpwEntryCleanup(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry)
{
    if (!pEntry->Size.cx)
        return VINF_SUCCESS;

    int rc = crServerRpwEntryCancel(pWorker, pEntry);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwEntryCancel failed rc %d", rc);
        return rc;
    }

    if (!cr_server.currentCtxInfo)
    {
        CRMuralInfo *pDummy = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        if (!pDummy)
        {
            crWarning("crServerGetDummyMural failed");
            return VERR_GENERAL_FAILURE;
        }


        crServerPerformMakeCurrent(pDummy, &cr_server.MainContextInfo);
    }

    cr_server.head_spu->dispatch_table.DeleteTextures(4, pEntry->aidWorkerTexs);

    if (CR_SERVER_RPW_ENTRY_PBO_IS_ACTIVE(pEntry))
    {
        cr_server.head_spu->dispatch_table.DeleteBuffersARB(2, pEntry->aidPBOs);
        memset(pEntry->aidPBOs, 0, sizeof (pEntry->aidPBOs));
        pEntry->iCurPBO = -1;
    }

    memset(pEntry->aidWorkerTexs, 0, sizeof (pEntry->aidWorkerTexs));
    CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Submitted);
    CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Worker);
    CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Gpu);
    pEntry->iTexDraw = -1;
    pEntry->iTexSubmitted = -2;
    pEntry->iTexWorker = -3;
    pEntry->iTexGpu = -4;
    pEntry->Size.cx = 0;
    pEntry->Size.cy = 0;
    return VINF_SUCCESS;
}

int crServerRpwEntryResize(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry, uint32_t width, uint32_t height)
{
    if (!width || !height)
    {
        width = 0;
        height = 0;
    }

    if (width == pEntry->Size.cx && width == pEntry->Size.cy)
        return VINF_SUCCESS;

    int rc = crServerRpwEntryCleanup(pWorker, pEntry);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwEntryCleanup failed rc %d", rc);
        return rc;
    }

    rc = crServerRpwEntryResizeCleaned(pWorker, pEntry, width, height);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwEntryResizeCleaned failed rc %d", rc);
    }
    return rc;
}

int crServerRpwEntryInit(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry, uint32_t width, uint32_t height, PFNCR_SERVER_RPW_DATA pfnData)
{
    memset(pEntry, 0, sizeof (*pEntry));

    pEntry->iTexDraw = -1;
    pEntry->iTexSubmitted = -2;
    pEntry->iTexWorker = -3;
    pEntry->iTexGpu = -4;
    pEntry->iCurPBO = -1;
    pEntry->pfnData = pfnData;
    int rc = crServerRpwEntryResizeCleaned(pWorker, pEntry, width, height);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwEntryResizeCleaned failed rc %d", rc);
        return rc;
    }
    return VINF_SUCCESS;
}

int crServerRpwEntrySubmit(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry)
{
    if (!CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Draw))
    {
        crWarning("submitting empty entry, ignoting");
        Assert(!pEntry->Size.cx);
        Assert(!pEntry->Size.cy);
        return VERR_INVALID_PARAMETER;
    }

    Assert(pEntry->Size.cx);
    Assert(pEntry->Size.cy);

    int rc = RTCritSectEnter(&pWorker->CritSect);
    if (RT_SUCCESS(rc))
    {
        Assert(pWorker->Ctl.enmType == CR_SERVER_RPW_CTL_TYPE_UNDEFINED);
        if (!CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Submitted))
        {
            CR_SERVER_RPW_ENTRY_TEX_PROMOTE_KEEPVALID(pEntry, Draw, Submitted);
            RTListAppend(&pWorker->WorkList, &pEntry->WorkEntry);
        }
        else
        {
            CR_SERVER_RPW_ENTRY_TEX_XCHG_VALID(pEntry, Draw, Submitted);
        }
        RTCritSectLeave(&pWorker->CritSect);

        RTSemEventSignal(pWorker->hSubmitEvent);
    }
    else
    {
        crWarning("RTCritSectEnter failed rc %d", rc);
        return rc;
    }

    return rc;
}

static int crServerRpwEntryCancelCtl(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry, CR_SERVER_RPW_CTL_TYPE enmType)
{
    if (CR_SERVER_RPW_CTL_TYPE_TERM == enmType && pEntry)
    {
        crWarning("Entry should be null for term request");
        pEntry = NULL;
    }

    int rc = RTCritSectEnter(&pWorker->CritSect);
    if (RT_SUCCESS(rc))
    {
        if (pEntry)
        {
            if (CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Submitted))
            {
                CR_SERVER_RPW_ENTRY_TEX_INVALIDATE(pEntry, Submitted);
                RTListNodeRemove(&pEntry->WorkEntry);
            }

            if (!CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Worker) && !CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pEntry, Gpu))
            {
                /* can cancel it wight away */
                RTCritSectLeave(&pWorker->CritSect);
                return VINF_SUCCESS;
            }
        }
        else
        {
            CR_SERVER_RPW_ENTRY *pCurEntry, *pNextEntry;
            RTListForEachSafe(&pWorker->WorkList, pCurEntry, pNextEntry, CR_SERVER_RPW_ENTRY, WorkEntry)
            {
                CR_SERVER_RPW_ENTRY_TEX_IS_VALID(pCurEntry, Submitted);
                CR_SERVER_RPW_ENTRY_TEX_INVALIDATE(pEntry, Submitted);
                RTListNodeRemove(&pCurEntry->WorkEntry);
            }
        }
        pWorker->Ctl.enmType = enmType;
        pWorker->Ctl.pEntry = pEntry;
        RTCritSectLeave(&pWorker->CritSect);
    }
    else
    {
        crWarning("RTCritSectEnter failed rc %d", rc);
        return rc;
    }

    rc = crServerRpwCtlNotify(pWorker, pEntry);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwCtlNotify failed rc %d", rc);
    }
    return VINF_SUCCESS;
}

int crServerRpwEntryWaitComplete(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry)
{
    int rc = crServerRpwCtl(pWorker, CR_SERVER_RPW_CTL_TYPE_WAIT_COMPLETE, pEntry);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwCtl failed rc %d", rc);
    }
    return rc;
}

int crServerRpwEntryCancel(CR_SERVER_RPW *pWorker, CR_SERVER_RPW_ENTRY *pEntry)
{
    return crServerRpwEntryCancelCtl(pWorker, pEntry, CR_SERVER_RPW_CTL_TYPE_WAIT_COMPLETE);
}

static int crServerRpwCtlTerm(CR_SERVER_RPW *pWorker)
{
    int rc = crServerRpwEntryCancelCtl(pWorker, NULL, CR_SERVER_RPW_CTL_TYPE_TERM);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwCtl failed rc %d", rc);
    }
    return rc;
}

int crServerRpwTerm(CR_SERVER_RPW *pWorker)
{
    int rc = crServerRpwCtlTerm(pWorker);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crServerRpwCtlTerm failed rc %d", rc);
        return rc;
    }

    rc = RTThreadWait(pWorker->hThread, RT_INDEFINITE_WAIT, NULL);
    if (!RT_SUCCESS(rc))
    {
        crWarning("RTThreadWait failed rc %d", rc);
        return rc;
    }

    RTSemEventDestroy(pWorker->Ctl.hCompleteEvent);
    RTSemEventDestroy(pWorker->hSubmitEvent);
    RTCritSectDelete(&pWorker->CritSect);

    return VINF_SUCCESS;
}
