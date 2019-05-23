/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "stub.h"
#include <iprt/thread.h>

#ifdef GLX
Display* stubGetWindowDisplay(WindowInfo *pWindow)
{
#if defined(CR_NEWWINTRACK)
    if ((NIL_RTTHREAD!=stub.hSyncThread) && (RTThreadNativeSelf()==RTThreadGetNative(stub.hSyncThread)))
    {
        if (pWindow && pWindow->dpy && !pWindow->syncDpy)
        {
            crDebug("going to XOpenDisplay(%s)", pWindow->dpyName);
            pWindow->syncDpy = XOpenDisplay(pWindow->dpyName);
            if (!pWindow->syncDpy)
            {
                crWarning("Failed to open display %s", pWindow->dpyName);
            }
            return pWindow->syncDpy;
        }
        else
        {
            return pWindow ? pWindow->syncDpy:NULL;
        }
    }
    else
#endif
    {
        return pWindow ? pWindow->dpy:NULL;
    }
}
#endif

/**
 * Returns -1 on error
 */
GLint APIENTRY crCreateContext(char *dpyName, GLint visBits)
{
    ContextInfo *context;
    stubInit();
    /* XXX in Chromium 1.5 and earlier, the last parameter was UNDECIDED.
     * That didn't seem right so it was changed to CHROMIUM. (Brian)
     */
    context = stubNewContext(dpyName, visBits, CHROMIUM, 0
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , NULL
#endif
            );
    return context ? (int) context->id : -1;
}

void APIENTRY crDestroyContext( GLint context )
{
    stubDestroyContext(context);
}

void APIENTRY crMakeCurrent( GLint window, GLint context )
{
    WindowInfo *winInfo = (WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    ContextInfo *contextInfo = (ContextInfo *)
        crHashtableSearch(stub.contextTable, context);
    if (contextInfo && contextInfo->type == NATIVE) {
        crWarning("Can't call crMakeCurrent with native GL context");
        return;
    }

    stubMakeCurrent(winInfo, contextInfo);
}

GLint APIENTRY crGetCurrentContext( void )
{
    ContextInfo *context;
    stubInit();
    context = stubGetCurrentContext();
    if (context)
      return (GLint) context->id;
    else
      return 0;
}

GLint APIENTRY crGetCurrentWindow( void )
{
    ContextInfo *context;
    stubInit();
    context = stubGetCurrentContext();
    if (context && context->currentDrawable)
      return context->currentDrawable->spuWindow;
    else
      return -1;
}

void APIENTRY crSwapBuffers( GLint window, GLint flags )
{
    WindowInfo *winInfo = (WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo)
        stubSwapBuffers(winInfo, flags);
}

/**
 * Returns -1 on error
 */
GLint APIENTRY crWindowCreate( const char *dpyName, GLint visBits )
{
    stubInit();
    return stubNewWindow( dpyName, visBits );
}

void APIENTRY crWindowDestroy( GLint window )
{
    stubDestroyWindow( 0, window );
}

void APIENTRY crWindowSize( GLint window, GLint w, GLint h )
{
    const WindowInfo *winInfo = (const WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo && winInfo->type == CHROMIUM)
    {
        crDebug("Dispatched crWindowSize (%i)", window);
        stub.spu->dispatch_table.WindowSize( window, w, h );
    }
}

void APIENTRY crWindowPosition( GLint window, GLint x, GLint y )
{
    const WindowInfo *winInfo = (const WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo && winInfo->type == CHROMIUM)
    {
        crDebug("Dispatched crWindowPosition (%i)", window);
        stub.spu->dispatch_table.WindowPosition( window, x, y );
    }
}

void APIENTRY crWindowVisibleRegion( GLint window, GLint cRects, const void *pRects )
{
    const WindowInfo *winInfo = (const WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo && winInfo->type == CHROMIUM)
    {
        crDebug("Dispatched crWindowVisibleRegion (%i, cRects=%i)", window, cRects);
        stub.spu->dispatch_table.WindowVisibleRegion( window, cRects, pRects );
    }
}

void APIENTRY crVBoxTexPresent(GLuint texture, GLuint cfg, GLint xPos, GLint yPos, GLint cRects, const GLint *pRects)
{
    RT_NOREF(texture, cfg, xPos, yPos, cRects, pRects);
    crError("not expected!");
}

void APIENTRY crWindowShow( GLint window, GLint flag )
{
    WindowInfo *winInfo = (WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo && winInfo->type == CHROMIUM)
        stub.spu->dispatch_table.WindowShow( window, flag );
    winInfo->mapped = flag ? GL_TRUE : GL_FALSE;
}

void APIENTRY stub_GetChromiumParametervCR( GLenum target, GLuint index, GLenum type, GLsizei count, GLvoid *values )
{
    char **ret;
    switch( target )
    {
        case GL_HEAD_SPU_NAME_CR:
            ret = (char **) values;
            *ret = stub.spu->name;
            return;
        default:
            stub.spu->dispatch_table.GetChromiumParametervCR( target, index, type, count, values );
            break;
    }
}

/*
 *  Updates geometry info for given spu window.
 *  Returns GL_TRUE if it changed since last call, GL_FALSE otherwise.
 *  bForceUpdate - forces dispatching of geometry info even if it's unchanged
 */
GLboolean stubUpdateWindowGeometry(WindowInfo *pWindow, GLboolean bForceUpdate)
{
    int winX, winY;
    unsigned int winW, winH;
    GLboolean res = GL_FALSE;

    CRASSERT(pWindow);

    stubGetWindowGeometry(pWindow, &winX, &winY, &winW, &winH);

    /** @todo remove "if (winW && winH)"?*/
    if (winW && winH) {
        if (stub.trackWindowSize) {
            if (bForceUpdate || winW != pWindow->width || winH != pWindow->height) {
                crDebug("Dispatched WindowSize (%i)", pWindow->spuWindow);
#ifdef VBOX_WITH_WDDM
                if (!stub.bRunningUnderWDDM || pWindow->mapped)
#endif
                {
                    stub.spuDispatch.WindowSize(pWindow->spuWindow, winW, winH);
                }
                pWindow->width = winW;
                pWindow->height = winH;
                res = GL_TRUE;
            }
        }
        if (stub.trackWindowPos) {
            if (bForceUpdate || winX != pWindow->x || winY != pWindow->y) {
                crDebug("Dispatched WindowPosition (%i)", pWindow->spuWindow);
#ifdef VBOX_WITH_WDDM
                if (!stub.bRunningUnderWDDM || pWindow->mapped)
#endif
                {
                    stub.spuDispatch.WindowPosition(pWindow->spuWindow, winX, winY);
                }
                pWindow->x = winX;
                pWindow->y = winY;
                res = GL_TRUE;
            }
        }
    }

    return res;
}

#ifdef WINDOWS
/*
 *  Updates visible regions for given spu window.
 *  Returns GL_TRUE if regions changed since last call, GL_FALSE otherwise.
 */
GLboolean stubUpdateWindowVisibileRegions(WindowInfo *pWindow)
{
    HRGN hVisRgn;
    HWND hwnd;
    DWORD dwCount;
    LPRGNDATA lpRgnData;
    POINT pt;
    int iret;

    if (!pWindow) return GL_FALSE;
    hwnd = pWindow->hWnd;
    if (!hwnd) return GL_FALSE;

    if (hwnd!=WindowFromDC(pWindow->drawable))
    {
        crWarning("Window(%i) DC is no longer valid", pWindow->spuWindow);
        return GL_FALSE;
    }

    hVisRgn = CreateRectRgn(0,0,0,0);
    iret = GetRandomRgn(pWindow->drawable, hVisRgn, SYSRGN);

    if (iret==1)
    {
        /** @todo check win95/win98 here, as rects should be already in client space there*/
        /* Convert screen related rectangles to client related rectangles */
        pt.x = 0;
        pt.y = 0;
        ScreenToClient(hwnd, &pt);
        OffsetRgn(hVisRgn, pt.x, pt.y);

        /*
        dwCount = GetRegionData(hVisRgn, 0, NULL);
        lpRgnData = crAlloc(dwCount);
        crDebug("GetRandomRgn returned 1, dwCount=%d", dwCount);
        GetRegionData(hVisRgn, dwCount, lpRgnData);
        crDebug("Region consists of %d rects", lpRgnData->rdh.nCount);

        pRects = (RECT*) lpRgnData->Buffer;
        for (i=0; i<lpRgnData->rdh.nCount; ++i)
        {
            crDebug("Rgn[%d] = (%d, %d, %d, %d)", i, pRects[i].left, pRects[i].top, pRects[i].right, pRects[i].bottom);
        }
        crFree(lpRgnData);
        */

        if (pWindow->hVisibleRegion==INVALID_HANDLE_VALUE
            || !EqualRgn(pWindow->hVisibleRegion, hVisRgn))
        {
            DeleteObject(pWindow->hVisibleRegion);
            pWindow->hVisibleRegion = hVisRgn;

            dwCount = GetRegionData(hVisRgn, 0, NULL);
            lpRgnData = crAlloc(dwCount);

            if (lpRgnData)
            {
                GetRegionData(hVisRgn, dwCount, lpRgnData);
                crDebug("Dispatched WindowVisibleRegion (%i, cRects=%i)", pWindow->spuWindow, lpRgnData->rdh.nCount);
                stub.spuDispatch.WindowVisibleRegion(pWindow->spuWindow, lpRgnData->rdh.nCount, (GLint*) lpRgnData->Buffer);
                crFree(lpRgnData);
                return GL_TRUE;
            }
            else crWarning("GetRegionData failed, VisibleRegions update failed");
        }
        else
        {
            DeleteObject(hVisRgn);
        }
    }
    else
    {
        crWarning("GetRandomRgn returned (%d) instead of (1), VisibleRegions update failed", iret);
        DeleteObject(hVisRgn);
    }

    return GL_FALSE;
}

# ifndef CR_NEWWINTRACK
static void stubCBCheckWindowsInfo(unsigned long key, void *data1, void *data2)
{
    WindowInfo *winInfo = (WindowInfo *) data1;
    CWPRETSTRUCT *pMsgInfo = (PCWPRETSTRUCT) data2;

    (void) key;

    if (winInfo && pMsgInfo && winInfo->type == CHROMIUM)
    {
        switch (pMsgInfo->message)
        {
            case WM_MOVING:
            case WM_SIZING:
            case WM_MOVE:
            case WM_CREATE:
            case WM_SIZE:
            {
                GLboolean changed = stub.trackWindowVisibleRgn && stubUpdateWindowVisibileRegions(winInfo);

                if (stubUpdateWindowGeometry(winInfo, GL_FALSE) || changed)
                {
                    stubForcedFlush(0);
                }
                break;
            }

            case WM_SHOWWINDOW:
            case WM_ACTIVATEAPP:
            case WM_PAINT:
            case WM_NCPAINT:
            case WM_NCACTIVATE:
            case WM_ERASEBKGND:
            {
                if (stub.trackWindowVisibleRgn && stubUpdateWindowVisibileRegions(winInfo))
                {
                    stubForcedFlush(0);
                }
                break;
            }

            default:
            {
                if (stub.trackWindowVisibleRgn && stubUpdateWindowVisibileRegions(winInfo))
                {
                    crDebug("Visibility info updated due to unknown hooked message (%d)", pMsgInfo->message);
                    stubForcedFlush(0);
                }
                break;
            }
        }
    }
}

LRESULT CALLBACK stubCBWindowMessageHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    CWPRETSTRUCT *pMsgInfo = (PCWPRETSTRUCT) lParam;

    if (nCode>=0 && pMsgInfo)
    {
        switch (pMsgInfo->message)
        {
            case WM_MOVING:
            case WM_SIZING:
            case WM_MOVE:
            case WM_ACTIVATEAPP:
            case WM_NCPAINT:
            case WM_NCACTIVATE:
            case WM_ERASEBKGND:
            case WM_CREATE:
            case WM_SIZE:
            case WM_SHOWWINDOW:
            {
                crHashtableWalk(stub.windowTable, stubCBCheckWindowsInfo, (void *) lParam);
                break;
            }

            /** @todo remove it*/
            default:
            {
                /*crDebug("hook: unknown message (%d)", pMsgInfo->message);*/
                crHashtableWalk(stub.windowTable, stubCBCheckWindowsInfo, (void *) lParam);
                break;
            }
        }
    }

    return CallNextHookEx(stub.hMessageHook, nCode, wParam, lParam);
}

void stubInstallWindowMessageHook()
{
    stub.hMessageHook = SetWindowsHookEx(WH_CALLWNDPROCRET, stubCBWindowMessageHookProc, 0, crThreadID());

    if (!stub.hMessageHook)
        crWarning("Window message hook install failed! (not fatal)");
}

void stubUninstallWindowMessageHook()
{
    if (stub.hMessageHook)
        UnhookWindowsHookEx(stub.hMessageHook);
}
# endif /*# ifndef CR_NEWWINTRACK*/

#elif defined(GLX) //#ifdef WINDOWS
void stubCheckXExtensions(WindowInfo *pWindow)
{
    int evb, erb, vmi=0, vma=0;
    Display *dpy = stubGetWindowDisplay(pWindow);

    stub.bXExtensionsChecked = GL_TRUE;
    stub.trackWindowVisibleRgn = 0;

    XLOCK(dpy);
    if (XCompositeQueryExtension(dpy, &evb, &erb)
        && XCompositeQueryVersion(dpy, &vma, &vmi)
        && (vma>0 || vmi>=4))
    {
        stub.bHaveXComposite = GL_TRUE;
        crDebug("XComposite %i.%i", vma, vmi);
        vma=0;
        vmi=0;
        if (XFixesQueryExtension(dpy, &evb, &erb)
            && XFixesQueryVersion(dpy, &vma, &vmi)
            && vma>=2)
        {
            crDebug("XFixes %i.%i", vma, vmi);
            stub.bHaveXFixes = GL_TRUE;
            stub.trackWindowVisibleRgn = 1;
            XUNLOCK(dpy);
            return;
        }
        else
        {
            crWarning("XFixes not found or old version (%i.%i), no VisibilityTracking", vma, vmi);
        }
    }
    else
    {
        crWarning("XComposite not found or old version (%i.%i), no VisibilityTracking", vma, vmi);
    }
    XUNLOCK(dpy);
    return;
}

/*
 *  Updates visible regions for given spu window.
 *  Returns GL_TRUE if regions changed since last call, GL_FALSE otherwise.
 */
GLboolean stubUpdateWindowVisibileRegions(WindowInfo *pWindow)
{
    XserverRegion xreg;
    int cRects, i;
    XRectangle *pXRects;
    GLint* pGLRects;
    Display *dpy;
    bool bNoUpdate = false;

    if (!stub.bXExtensionsChecked)
    {
        stubCheckXExtensions(pWindow);
        if (!stub.trackWindowVisibleRgn)
        {
            return GL_FALSE;
        }
    }

    dpy = stubGetWindowDisplay(pWindow);

    /** @todo see comment regarding size/position updates and XSync, same applies to those functions but
    * it seems there's no way to get even based updates for this. Or I've failed to find the appropriate extension.
    */
    XLOCK(dpy);
    xreg = XCompositeCreateRegionFromBorderClip(dpy, pWindow->drawable);
    pXRects = XFixesFetchRegion(dpy, xreg, &cRects);
    XFixesDestroyRegion(dpy, xreg);
    XUNLOCK(dpy);

    /* Check for compiz main window */
    if (!pWindow->pVisibleRegions && !cRects)
    {
        bNoUpdate = true;
    }

    if (!bNoUpdate
        && (!pWindow->pVisibleRegions
            || pWindow->cVisibleRegions!=cRects
            || (pWindow->pVisibleRegions && crMemcmp(pWindow->pVisibleRegions, pXRects, cRects * sizeof(XRectangle)))))
    {
        if (pWindow->pVisibleRegions)
        {
            XFree(pWindow->pVisibleRegions);
        }

        pWindow->pVisibleRegions = pXRects;
        pWindow->cVisibleRegions = cRects;

        pGLRects = crAlloc(cRects ? 4*cRects*sizeof(GLint) : 4*sizeof(GLint));
        if (!pGLRects)
        {
            crWarning("stubUpdateWindowVisibileRegions: failed to allocate %lu bytes",
                    (unsigned long)(4*cRects*sizeof(GLint)));
            return GL_FALSE;
        }

        //crDebug("Got %i rects.", cRects);
        for (i=0; i<cRects; ++i)
        {
            pGLRects[4*i+0] = pXRects[i].x;
            pGLRects[4*i+1] = pXRects[i].y;
            pGLRects[4*i+2] = pXRects[i].x+pXRects[i].width;
            pGLRects[4*i+3] = pXRects[i].y+pXRects[i].height;
            //crDebug("Rect[%i]=(%i,%i,%i,%i)", i, pGLRects[4*i+0], pGLRects[4*i+1], pGLRects[4*i+2], pGLRects[4*i+3]);
        }

        crDebug("Dispatched WindowVisibleRegion (%i, cRects=%i)", pWindow->spuWindow, cRects);
        stub.spuDispatch.WindowVisibleRegion(pWindow->spuWindow, cRects, pGLRects);
        crFree(pGLRects);
        return GL_TRUE;
    }
    else
    {
        XFree(pXRects);
    }

    return GL_FALSE;
}
#endif //#ifdef WINDOWS
