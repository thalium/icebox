/* $Id: renderspu_cocoa.c $ */
/** @file
 * VirtualBox OpenGL Cocoa Window System implementation
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <OpenGL/OpenGL.h>

#include "renderspu.h"
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/path.h>

#include <cr_string.h>
#include <cr_mem.h>

GLboolean renderspu_SystemInitVisual(VisualInfo *pVisInfo)
{
    CRASSERT(pVisInfo);

/*    cocoaGLVisualCreate(&pCtxInfo->context);*/

    return GL_TRUE;
}

GLboolean renderspu_SystemCreateContext(VisualInfo *pVisInfo, ContextInfo *pCtxInfo, ContextInfo *pSharedCtxInfo)
{
    CRASSERT(pVisInfo);
    CRASSERT(pCtxInfo);

    pCtxInfo->currentWindow = NULL;

    cocoaGLCtxCreate(&pCtxInfo->context, pVisInfo->visAttribs, pSharedCtxInfo ? pSharedCtxInfo->context : NULL);

    return GL_TRUE;
}

void renderspu_SystemDestroyContext(ContextInfo *pCtxInfo)
{
    if(!pCtxInfo)
        return;

    if(pCtxInfo->context)
    {
        cocoaGLCtxDestroy(pCtxInfo->context);
        pCtxInfo->context = NULL;
    }
}

void renderspuFullscreen(WindowInfo *pWinInfo, GLboolean fFullscreen)
{
    /* Real fullscreen isn't supported by VirtualBox */
}

GLboolean renderspu_SystemVBoxCreateWindow(VisualInfo *pVisInfo, GLboolean fShowIt, WindowInfo *pWinInfo)
{
    CRASSERT(pVisInfo);
    CRASSERT(pWinInfo);

    /* VirtualBox is the only frontend which support 3D right now. */
    char pszName[256];
    if (RTProcGetExecutablePath(pszName, sizeof(pszName)))
        /* Check for VirtualBox and VirtualBoxVM */
        if (RTStrNICmp(RTPathFilename(pszName), "VirtualBox", 10) != 0)
            return GL_FALSE;

    pWinInfo->visual = pVisInfo;
    pWinInfo->window = NULL;
    pWinInfo->nativeWindow = NULL;
    pWinInfo->currentCtx = NULL;

    NativeNSViewRef pParentWin = (NativeNSViewRef)(uintptr_t)render_spu_parent_window_id;

    cocoaViewCreate(&pWinInfo->window, pWinInfo, pParentWin, pVisInfo->visAttribs);

    if (fShowIt)
        renderspu_SystemShowWindow(pWinInfo, fShowIt);

    return GL_TRUE;
}

void renderspu_SystemReparentWindow(WindowInfo *pWinInfo)
{
    NativeNSViewRef pParentWin = (NativeNSViewRef)(uintptr_t)render_spu_parent_window_id;
    cocoaViewReparent(pWinInfo->window, pParentWin);
}

void renderspu_SystemDestroyWindow(WindowInfo *pWinInfo)
{
    CRASSERT(pWinInfo);

    cocoaViewDestroy(pWinInfo->window);
}

void renderspu_SystemWindowPosition(WindowInfo *pWinInfo, GLint x, GLint y)
{
    CRASSERT(pWinInfo);
    NativeNSViewRef pParentWin = (NativeNSViewRef)(uintptr_t)render_spu_parent_window_id;

    /*pParentWin is unused in the call, otherwise it might hold incorrect value if for ex. last reparent call was for
      a different screen*/
    cocoaViewSetPosition(pWinInfo->window, pParentWin, x, y);
}

void renderspu_SystemWindowSize(WindowInfo *pWinInfo, GLint w, GLint h)
{
    CRASSERT(pWinInfo);

    cocoaViewSetSize(pWinInfo->window, w, h);
}

void renderspu_SystemGetWindowGeometry(WindowInfo *pWinInfo, GLint *pX, GLint *pY, GLint *pW, GLint *pH)
{
    CRASSERT(pWinInfo);

    cocoaViewGetGeometry(pWinInfo->window, pX, pY, pW, pH);
}

void renderspu_SystemGetMaxWindowSize(WindowInfo *pWinInfo, GLint *pW, GLint *pH)
{
    CRASSERT(pWinInfo);

    *pW = 10000;
    *pH = 10000;
}

void renderspu_SystemShowWindow(WindowInfo *pWinInfo, GLboolean fShowIt)
{
    CRASSERT(pWinInfo);

    cocoaViewShow(pWinInfo->window, fShowIt);
}

void renderspu_SystemVBoxPresentComposition( WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry )
{
    cocoaViewPresentComposition(window->window, pChangedEntry);
}

void renderspu_SystemMakeCurrent(WindowInfo *pWinInfo, GLint nativeWindow, ContextInfo *pCtxInfo)
{
/*    if(pWinInfo->visual != pCtxInfo->visual)*/
/*        printf ("visual mismatch .....................\n");*/

    nativeWindow = 0;

    if (pWinInfo && pCtxInfo)
        cocoaViewMakeCurrentContext(pWinInfo->window, pCtxInfo->context);
    else
        cocoaViewMakeCurrentContext(NULL, NULL);
}

void renderspu_SystemSwapBuffers(WindowInfo *pWinInfo, GLint flags)
{
    CRASSERT(pWinInfo);

    cocoaViewDisplay(pWinInfo->window);
}

GLboolean renderspu_SystemWindowNeedEmptyPresent(WindowInfo *pWinInfo)
{
    return cocoaViewNeedsEmptyPresent(pWinInfo->window);
}

void renderspu_SystemWindowVisibleRegion(WindowInfo *pWinInfo, GLint cRects, const GLint* paRects)
{
    CRASSERT(pWinInfo);

    cocoaViewSetVisibleRegion(pWinInfo->window, cRects, paRects);
}

void renderspu_SystemWindowApplyVisibleRegion(WindowInfo *pWinInfo)
{
}

int renderspu_SystemInit()
{
    return VINF_SUCCESS;
}

int renderspu_SystemTerm()
{
    CrGlslTerm(&render_spu.GlobalShaders);
    return VINF_SUCCESS;
}

static SPUNamedFunctionTable * renderspuFindEntry(SPUNamedFunctionTable *aFunctions, const char *pcszName)
{
    SPUNamedFunctionTable *pCur;

    for (pCur = aFunctions ; pCur->name != NULL ; pCur++)
    {
        if (!crStrcmp( pcszName, pCur->name ) )
        {
            return pCur;
        }
    }

    AssertFailed();

    return NULL;
}

typedef struct CR_RENDER_CTX_INFO
{
    ContextInfo * pContext;
    WindowInfo * pWindow;
} CR_RENDER_CTX_INFO;

void renderspuCtxInfoInitCurrent(CR_RENDER_CTX_INFO *pInfo)
{
    GET_CONTEXT(pCurCtx);
    pInfo->pContext = pCurCtx;
    pInfo->pWindow = pCurCtx ? pCurCtx->currentWindow : NULL;
}

void renderspuCtxInfoRestoreCurrent(CR_RENDER_CTX_INFO *pInfo)
{
    GET_CONTEXT(pCurCtx);
    if (pCurCtx == pInfo->pContext && (!pCurCtx || pCurCtx->currentWindow == pInfo->pWindow))
        return;
    renderspuPerformMakeCurrent(pInfo->pWindow, 0, pInfo->pContext);
}

GLboolean renderspuCtxSetCurrentWithAnyWindow(ContextInfo * pContext, CR_RENDER_CTX_INFO *pInfo)
{
    WindowInfo * window;
    renderspuCtxInfoInitCurrent(pInfo);

    if (pInfo->pContext == pContext)
        return GL_TRUE;

    window = pContext->currentWindow;
    if (!window)
    {
        window = renderspuGetDummyWindow(pContext->BltInfo.Base.visualBits);
        if (!window)
        {
            WARN(("renderspuGetDummyWindow failed"));
            return GL_FALSE;
        }
    }

    Assert(window);

    renderspuPerformMakeCurrent(window, 0, pContext);
    return GL_TRUE;
}

void renderspu_SystemDefaultSharedContextChanged(ContextInfo *fromContext, ContextInfo *toContext)
{
    CRASSERT(fromContext != toContext);

    if (!CrGlslIsInited(&render_spu.GlobalShaders))
    {
        CrGlslInit(&render_spu.GlobalShaders, &render_spu.blitterDispatch);
    }

    if (fromContext)
    {
        if (CrGlslNeedsCleanup(&render_spu.GlobalShaders))
        {
            CR_RENDER_CTX_INFO Info;
            if (renderspuCtxSetCurrentWithAnyWindow(fromContext, &Info))
            {
                CrGlslCleanup(&render_spu.GlobalShaders);
                renderspuCtxInfoRestoreCurrent(&Info);
            }
            else
                WARN(("renderspuCtxSetCurrentWithAnyWindow failed!"));
        }
    }
    else
    {
        CRASSERT(!CrGlslNeedsCleanup(&render_spu.GlobalShaders));
    }

    CRASSERT(!CrGlslNeedsCleanup(&render_spu.GlobalShaders));

    if (toContext)
    {
        CR_RENDER_CTX_INFO Info;
        if (renderspuCtxSetCurrentWithAnyWindow(toContext, &Info))
        {
            int rc = CrGlslProgGenAllNoAlpha(&render_spu.GlobalShaders);
            if (!RT_SUCCESS(rc))
                WARN(("CrGlslProgGenAllNoAlpha failed, rc %d", rc));

            renderspuCtxInfoRestoreCurrent(&Info);
        }
        else
            crWarning("renderspuCtxSetCurrentWithAnyWindow failed!");
    }
}

AssertCompile(sizeof (GLhandleARB) == sizeof (void*));

static VBoxGLhandleARB crHndlSearchVBox(GLhandleARB hNative)
{
    CRASSERT(!(((uintptr_t)hNative) >> 32));
    return (VBoxGLhandleARB)((uintptr_t)hNative);
}

static GLhandleARB crHndlSearchNative(VBoxGLhandleARB hVBox)
{
    return (GLhandleARB)((uintptr_t)hVBox);
}

static VBoxGLhandleARB crHndlAcquireVBox(GLhandleARB hNative)
{
    CRASSERT(!(((uintptr_t)hNative) >> 32));
    return (VBoxGLhandleARB)((uintptr_t)hNative);
}

static GLhandleARB crHndlReleaseVBox(VBoxGLhandleARB hVBox)
{
    return (GLhandleARB)((uintptr_t)hVBox);
}

static void SPU_APIENTRY renderspu_SystemDeleteObjectARB(VBoxGLhandleARB obj)
{
    GLhandleARB hNative = crHndlReleaseVBox(obj);
    if (!hNative)
    {
        crWarning("no native for %d", obj);
        return;
    }

    render_spu.pfnDeleteObject(hNative);
}

static void SPU_APIENTRY renderspu_SystemGetAttachedObjectsARB( VBoxGLhandleARB containerObj, GLsizei maxCount, GLsizei * pCount, VBoxGLhandleARB * obj )
{
    GLhandleARB *paAttachments;
    GLhandleARB hNative = crHndlSearchNative(containerObj);
    GLsizei count, i;

    if (pCount)
        *pCount = 0;

    if (!hNative)
    {
        crWarning("no native for %d", obj);
        return;
    }

    paAttachments = crCalloc(maxCount * sizeof (*paAttachments));
    if (!paAttachments)
    {
        crWarning("crCalloc failed");
        return;
    }

    render_spu.pfnGetAttachedObjects(hNative, maxCount, &count, paAttachments);
    if (pCount)
        *pCount = count;
    if (count > maxCount)
    {
        crWarning("count too big");
        count = maxCount;
    }

    for (i = 0; i < count; ++i)
    {
        obj[i] = crHndlSearchVBox(paAttachments[i]);
        CRASSERT(obj[i]);
    }

    crFree(paAttachments);
}

static VBoxGLhandleARB SPU_APIENTRY renderspu_SystemGetHandleARB(GLenum pname)
{
    GLhandleARB hNative = render_spu.pfnGetHandle(pname);
    VBoxGLhandleARB hVBox;
    if (!hNative)
    {
        crWarning("pfnGetHandle failed");
        return 0;
    }
    hVBox = crHndlAcquireVBox(hNative);
    CRASSERT(hVBox);
    return hVBox;
}

static void SPU_APIENTRY renderspu_SystemGetInfoLogARB( VBoxGLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog )
{
    GLhandleARB hNative = crHndlSearchNative(obj);
    if (!hNative)
    {
        crWarning("invalid handle!");
        return;
    }

    render_spu.pfnGetInfoLog(hNative, maxLength, length, infoLog);
}

static void SPU_APIENTRY renderspu_SystemGetObjectParameterfvARB( VBoxGLhandleARB obj, GLenum pname, GLfloat * params )
{
    GLhandleARB hNative = crHndlSearchNative(obj);
    if (!hNative)
    {
        crWarning("invalid handle!");
        return;
    }

    render_spu.pfnGetObjectParameterfv(hNative, pname, params);
}

static void SPU_APIENTRY renderspu_SystemGetObjectParameterivARB( VBoxGLhandleARB obj, GLenum pname, GLint * params )
{
    GLhandleARB hNative = crHndlSearchNative(obj);
    if (!hNative)
    {
        crWarning("invalid handle!");
        return;
    }

    render_spu.pfnGetObjectParameteriv(hNative, pname, params);
}

uint32_t renderspu_SystemPostprocessFunctions(SPUNamedFunctionTable *aFunctions, uint32_t cFunctions, uint32_t cTable)
{
    SPUNamedFunctionTable * pEntry;

    pEntry = renderspuFindEntry(aFunctions, "DeleteObjectARB");
    if (pEntry)
    {
        render_spu.pfnDeleteObject = (PFNDELETE_OBJECT)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemDeleteObjectARB;
    }

    pEntry = renderspuFindEntry(aFunctions, "GetAttachedObjectsARB");
    if (pEntry)
    {
        render_spu.pfnGetAttachedObjects = (PFNGET_ATTACHED_OBJECTS)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemGetAttachedObjectsARB;
    }

    pEntry = renderspuFindEntry(aFunctions, "GetHandleARB");
    if (pEntry)
    {
        render_spu.pfnGetHandle = (PFNGET_HANDLE)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemGetHandleARB;
    }

    pEntry = renderspuFindEntry(aFunctions, "GetInfoLogARB");
    if (pEntry)
    {
        render_spu.pfnGetInfoLog = (PFNGET_INFO_LOG)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemGetInfoLogARB;
    }

    pEntry = renderspuFindEntry(aFunctions, "GetObjectParameterfvARB");
    if (pEntry)
    {
        render_spu.pfnGetObjectParameterfv = (PFNGET_OBJECT_PARAMETERFV)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemGetObjectParameterfvARB;
    }

    pEntry = renderspuFindEntry(aFunctions, "GetObjectParameterivARB");
    if (pEntry)
    {
        render_spu.pfnGetObjectParameteriv = (PFNGET_OBJECT_PARAMETERIV)pEntry->fn;
        pEntry->fn = (SPUGenericFunction)renderspu_SystemGetObjectParameterivARB;
    }

    return cFunctions;
}
