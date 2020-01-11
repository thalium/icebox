/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/**
 * \mainpage OpenGL_stub
 *
 * \section OpenGL_stubIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The OpenGL_stub module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */

/**
 * This file manages OpenGL rendering contexts in the faker library.
 * The big issue is switching between Chromium and native GL context
 * management.  This is where we support multiple client OpenGL
 * windows.  Typically, one window is handled by Chromium while any
 * other windows are handled by the native OpenGL library.
 */

#include "chromium.h"
#include "cr_error.h"
#include "cr_spu.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "stub.h"

/**
 * This function should be called from MakeCurrent().  It'll detect if
 * we're in a multi-thread situation, and do the right thing for dispatch.
 */
static void stubCheckMultithread( void )
{
    static unsigned long knownID;
    static GLboolean firstCall = GL_TRUE;

    if (stub.threadSafe)
    return;  /* nothing new, nothing to do */

    if (firstCall) {
    knownID = crThreadID();
    firstCall = GL_FALSE;
    }
    else if (knownID != crThreadID()) {
    /* going thread-safe now! */
    stub.threadSafe = GL_TRUE;
    crSPUCopyDispatchTable(&glim, &stubThreadsafeDispatch);
    }
}


/**
 * Install the given dispatch table as the table used for all gl* calls.
 */
    static void
stubSetDispatch( SPUDispatchTable *table )
{
    CRASSERT(table);

    /* always set the per-thread dispatch pointer */
    crSetTSD(&stub.dispatchTSD, (void *) table);
    if (stub.threadSafe) {
    /* Do nothing - the thread-safe dispatch functions will call GetTSD()
     * to get a pointer to the dispatch table, and jump through it.
     */
    }
    else
    {
    /* Single thread mode - just install the caller's dispatch table */
    /* This conditional is an optimization to try to avoid unnecessary
     * copying.  It seems to work with atlantis, multiwin, etc. but
     * _could_ be a problem. (Brian)
     */
    if (glim.copy_of != table->copy_of)
        crSPUCopyDispatchTable(&glim, table);
    }
}

void stubForcedFlush(GLint con)
{
#if 0
    GLint buffer;
    stub.spu->dispatch_table.GetIntegerv(GL_DRAW_BUFFER, &buffer);
    stub.spu->dispatch_table.DrawBuffer(GL_FRONT);
    stub.spu->dispatch_table.Flush();
    stub.spu->dispatch_table.DrawBuffer(buffer);
#else
    if (con)
    {
        stub.spu->dispatch_table.VBoxConFlush(con);
    }
    else
    {
        stub.spu->dispatch_table.Flush();
    }
#endif
}

void stubConChromiumParameteriCR(GLint con, GLenum param, GLint value)
{
//    if (con)
        stub.spu->dispatch_table.VBoxConChromiumParameteriCR(con, param, value);
//    else
//        crError("VBoxConChromiumParameteriCR called with null connection");
}

void stubConChromiumParametervCR(GLint con, GLenum target, GLenum type, GLsizei count, const GLvoid *values)
{
//    if (con)
        stub.spu->dispatch_table.VBoxConChromiumParametervCR(con, target, type, count, values);
//    else
//        crError("VBoxConChromiumParameteriCR called with null connection");
}

void stubConFlush(GLint con)
{
    if (con)
        stub.spu->dispatch_table.VBoxConFlush(con);
    else
        crError("stubConFlush called with null connection");
}

static void stubWindowCleanupForContextsCB(unsigned long key, void *data1, void *data2)
{
    ContextInfo *context = (ContextInfo *) data1;
    RT_NOREF(key);

    CRASSERT(context);

    if (context->currentDrawable == data2)
        context->currentDrawable = NULL;
}

void stubDestroyWindow( GLint con, GLint window )
{
    WindowInfo *winInfo = (WindowInfo *)
        crHashtableSearch(stub.windowTable, (unsigned int) window);
    if (winInfo && winInfo->type == CHROMIUM && stub.spu)
    {
        crHashtableLock(stub.windowTable);

        stub.spu->dispatch_table.VBoxWindowDestroy(con, winInfo->spuWindow );

#ifdef WINDOWS
        if (winInfo->hVisibleRegion != INVALID_HANDLE_VALUE)
        {
            DeleteObject(winInfo->hVisibleRegion);
        }
#elif defined(GLX)
        if (winInfo->pVisibleRegions)
        {
            XFree(winInfo->pVisibleRegions);
        }
# ifdef CR_NEWWINTRACK
        if (winInfo->syncDpy)
        {
            XCloseDisplay(winInfo->syncDpy);
        }
# endif
#endif

        stubForcedFlush(con);

        crHashtableWalk(stub.contextTable, stubWindowCleanupForContextsCB, winInfo);

        crHashtableDelete(stub.windowTable, window, crFree);

        crHashtableUnlock(stub.windowTable);
    }
}

/**
 * Create a new _Chromium_ window, not GLX, WGL or CGL.
 * Called by crWindowCreate() only.
 */
    GLint
stubNewWindow( const char *dpyName, GLint visBits )
{
    WindowInfo *winInfo;
    GLint spuWin, size[2];

    spuWin = stub.spu->dispatch_table.WindowCreate( dpyName, visBits );
    if (spuWin < 0) {
    return -1;
    }

    winInfo = (WindowInfo *) crCalloc(sizeof(WindowInfo));
    if (!winInfo) {
    stub.spu->dispatch_table.WindowDestroy(spuWin);
    return -1;
    }

    winInfo->type = CHROMIUM;

    /* Ask the head SPU for the initial window size */
    size[0] = size[1] = 0;
    stub.spu->dispatch_table.GetChromiumParametervCR(GL_WINDOW_SIZE_CR, 0, GL_INT, 2, size);
    if (size[0] == 0 && size[1] == 0) {
    /* use some reasonable defaults */
    size[0] = size[1] = 512;
    }
    winInfo->width = size[0];
    winInfo->height = size[1];
#ifdef VBOX_WITH_WDDM
    if (stub.bRunningUnderWDDM)
    {
        crError("Should not be here: WindowCreate/Destroy & VBoxPackGetInjectID require connection id!");
        winInfo->mapped = 0;
    }
    else
#endif
    {
        winInfo->mapped = 1;
    }

    if (!dpyName)
    dpyName = "";

    crStrncpy(winInfo->dpyName, dpyName, MAX_DPY_NAME);
    winInfo->dpyName[MAX_DPY_NAME-1] = 0;

    /* Use spuWin as the hash table index and GLX/WGL handle */
#ifdef WINDOWS
    winInfo->drawable = (HDC) spuWin;
    winInfo->hVisibleRegion = INVALID_HANDLE_VALUE;
#elif defined(Darwin)
    winInfo->drawable = (CGSWindowID) spuWin;
#elif defined(GLX)
    winInfo->drawable = (GLXDrawable) spuWin;
    winInfo->pVisibleRegions = NULL;
    winInfo->cVisibleRegions = 0;
#endif
#ifdef CR_NEWWINTRACK
    winInfo->u32ClientID = stub.spu->dispatch_table.VBoxPackGetInjectID(0);
#endif
    winInfo->spuWindow = spuWin;

    crHashtableAdd(stub.windowTable, (unsigned int) spuWin, winInfo);

    return spuWin;
}

#ifdef GLX
# if 0 /* unused */
static XErrorHandler oldErrorHandler;
static unsigned char lastXError = Success;

static int
errorHandler (Display *dpy, XErrorEvent *e)
{
    RT_NOREF(dpy);

    lastXError = e->error_code;
    return 0;
}
# endif /* unused */
#endif

GLboolean
stubIsWindowVisible(WindowInfo *win)
{
#if defined(WINDOWS)
# ifdef VBOX_WITH_WDDM
    if (stub.bRunningUnderWDDM)
        return win->mapped;
# endif
    return GL_TRUE;
#elif defined(Darwin)
    return GL_TRUE;
#elif defined(GLX)
    Display *dpy = stubGetWindowDisplay(win);
    if (dpy)
    {
        XWindowAttributes attr;
        XLOCK(dpy);
        XGetWindowAttributes(dpy, win->drawable, &attr);
        XUNLOCK(dpy);

        if (attr.map_state == IsUnmapped)
        {
            return GL_FALSE;
        }
# if 1
        return GL_TRUE;
# else
        if (attr.override_redirect)
        {
            return GL_TRUE;
        }

        if (!stub.bXExtensionsChecked)
        {
            stubCheckXExtensions(win);
        }

        if (!stub.bHaveXComposite)
        {
            return GL_TRUE;
        }
        else
        {
            Pixmap p;

            crLockMutex(&stub.mutex);

            XLOCK(dpy);
            XSync(dpy, false);
            oldErrorHandler = XSetErrorHandler(errorHandler);
            /** @todo this will create new pixmap for window every call*/
            p = XCompositeNameWindowPixmap(dpy, win->drawable);
            XSync(dpy, false);
            XSetErrorHandler(oldErrorHandler);
            XUNLOCK(dpy);

            switch (lastXError)
            {
                case Success:
                    XFreePixmap(dpy, p);
                    crUnlockMutex(&stub.mutex);
                    return GL_FALSE;
                    break;
                case BadMatch:
                    /*Window isn't redirected*/
                    lastXError = Success;
                    break;
                default:
                    crWarning("Unexpected XError %i", (int)lastXError);
                    lastXError = Success;
            }

            crUnlockMutex(&stub.mutex);

            return GL_TRUE;
        }
# endif
    }
    else {
        /* probably created by crWindowCreate() */
        return win->mapped;
    }
#endif
}


/**
 * Given a Windows HDC or GLX Drawable, return the corresponding
 * WindowInfo structure.  Create a new one if needed.
 */
WindowInfo *
#ifdef WINDOWS
    stubGetWindowInfo( HDC drawable )
#elif defined(Darwin)
    stubGetWindowInfo( CGSWindowID drawable )
#elif defined(GLX)
stubGetWindowInfo( Display *dpy, GLXDrawable drawable )
#endif
{
#ifndef WINDOWS
    WindowInfo *winInfo = (WindowInfo *) crHashtableSearch(stub.windowTable, (unsigned int) drawable);
#else
    WindowInfo *winInfo;
    HWND hwnd;
    hwnd = WindowFromDC(drawable);

    if (!hwnd)
    {
        return NULL;
    }

    winInfo = (WindowInfo *) crHashtableSearch(stub.windowTable, (unsigned int) hwnd);
#endif
    if (!winInfo) {
    winInfo = (WindowInfo *) crCalloc(sizeof(WindowInfo));
    if (!winInfo)
        return NULL;
#ifdef GLX
    crStrncpy(winInfo->dpyName, DisplayString(dpy), MAX_DPY_NAME);
    winInfo->dpyName[MAX_DPY_NAME-1] = 0;
    winInfo->dpy = dpy;
    winInfo->pVisibleRegions = NULL;
#elif defined(Darwin)
    winInfo->connection = _CGSDefaultConnection(); // store our connection as default
#elif defined(WINDOWS)
    winInfo->hVisibleRegion = INVALID_HANDLE_VALUE;
    winInfo->hWnd = hwnd;
#endif
    winInfo->drawable = drawable;
    winInfo->type = UNDECIDED;
    winInfo->spuWindow = -1;
#ifdef VBOX_WITH_WDDM
    if (stub.bRunningUnderWDDM)
        winInfo->mapped = 0;
    else
#endif
    {
        winInfo->mapped = -1; /* don't know */
    }
    winInfo->pOwner = NULL;
#ifdef CR_NEWWINTRACK
    winInfo->u32ClientID = -1;
#endif
#ifndef WINDOWS
    crHashtableAdd(stub.windowTable, (unsigned int) drawable, winInfo);
#else
    crHashtableAdd(stub.windowTable, (unsigned int) hwnd, winInfo);
#endif
    }
#ifdef WINDOWS
    else
    {
        winInfo->drawable = drawable;
    }
#endif
    return winInfo;
}

static void stubWindowCheckOwnerCB(unsigned long key, void *data1, void *data2);

static void
stubContextFree( ContextInfo *context )
{
    crMemZero(context, sizeof(ContextInfo));  /* just to be safe */
    crFree(context);
}

static void
stubDestroyContextLocked( ContextInfo *context )
{
    unsigned long contextId = context->id;
    if (context->type == NATIVE) {
#ifdef WINDOWS
        stub.wsInterface.wglDeleteContext( context->hglrc );
#elif defined(Darwin)
        stub.wsInterface.CGLDestroyContext( context->cglc );
#elif defined(GLX)
        stub.wsInterface.glXDestroyContext( context->dpy, context->glxContext );
#endif
    }
    else if (context->type == CHROMIUM) {
        /* Have pack SPU or tilesort SPU, etc. destroy the context */
        CRASSERT(context->spuContext >= 0);
        stub.spu->dispatch_table.DestroyContext( context->spuContext );
        crHashtableWalk(stub.windowTable, stubWindowCheckOwnerCB, context);
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        if (context->spuConnection)
        {
            stub.spu->dispatch_table.VBoxConDestroy(context->spuConnection);
            context->spuConnection = 0;
        }
#endif
    }

#ifdef GLX
    crFreeHashtable(context->pGLXPixmapsHash, crFree);
#endif

    crHashtableDelete(stub.contextTable, contextId, NULL);
}

static DECLCALLBACK(void) stubContextDtor(void*pvContext)
{
    stubContextFree((ContextInfo*)pvContext);
}

/**
 * Allocate a new ContextInfo object, initialize it, put it into the
 * context hash table.  If type==CHROMIUM, call the head SPU's
 * CreateContext() function too.
 */
    ContextInfo *
stubNewContext(char *dpyName, GLint visBits, ContextType type, unsigned long shareCtx
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , struct VBOXUHGSMI *pHgsmi
#endif
    )
{
    GLint spuContext = -1, spuShareCtx = 0, spuConnection = 0;
    ContextInfo *context;

    if (shareCtx > 0) {
    /* translate shareCtx to a SPU context ID */
    context = (ContextInfo *)
        crHashtableSearch(stub.contextTable, shareCtx);
        if (context)
            spuShareCtx = context->spuContext;
    }

    if (type == CHROMIUM) {
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        if (pHgsmi)
        {
            spuConnection = stub.spu->dispatch_table.VBoxConCreate(pHgsmi);
            if (!spuConnection)
            {
                crWarning("VBoxConCreate failed");
                return NULL;
            }
        }
#endif
        spuContext
            = stub.spu->dispatch_table.VBoxCreateContext(spuConnection, dpyName, visBits, spuShareCtx);
        if (spuContext < 0)
        {
            crWarning("VBoxCreateContext failed");
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
            if (spuConnection)
                stub.spu->dispatch_table.VBoxConDestroy(spuConnection);
#endif
            return NULL;
        }
    }

    context = crCalloc(sizeof(ContextInfo));
    if (!context) {
        stub.spu->dispatch_table.DestroyContext(spuContext);
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        if (spuConnection)
            stub.spu->dispatch_table.VBoxConDestroy(spuConnection);
#endif
        return NULL;
    }

    if (!dpyName)
        dpyName = "";

    context->id = stub.freeContextNumber++;
    context->type = type;
    context->spuContext = spuContext;
    context->visBits = visBits;
    context->currentDrawable = NULL;
    crStrncpy(context->dpyName, dpyName, MAX_DPY_NAME);
    context->dpyName[MAX_DPY_NAME-1] = 0;

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    context->spuConnection = spuConnection;
    context->pHgsmi = pHgsmi;
#endif

    VBoxTlsRefInit(context, stubContextDtor);

#if defined(GLX) || defined(DARWIN)
    context->share = (ContextInfo *)
        crHashtableSearch(stub.contextTable, (unsigned long) shareCtx);
#endif

#ifdef GLX
    context->pGLXPixmapsHash = crAllocHashtable();
    context->damageQueryFailed = GL_FALSE;
    context->damageEventsBase = 0;
#endif

    crHashtableAdd(stub.contextTable, context->id, (void *) context);

    return context;
}


#ifdef Darwin

#define SET_ATTR(l,i,a)     ( (l)[(i)++] = (a) )
#define SET_ATTR_V(l,i,a,v) ( SET_ATTR(l,i,a), SET_ATTR(l,i,v) )

void stubSetPFA( ContextInfo *ctx, CGLPixelFormatAttribute *attribs, int size, GLint *num ) {
    GLuint visual = ctx->visBits;
    int i = 0;

    CRASSERT(visual & CR_RGB_BIT);

    SET_ATTR_V(attribs, i, kCGLPFAColorSize, 8);

    if( visual & CR_DEPTH_BIT )
        SET_ATTR_V(attribs, i, kCGLPFADepthSize, 16);

    if( visual & CR_ACCUM_BIT )
        SET_ATTR_V(attribs, i, kCGLPFAAccumSize, 1);

    if( visual & CR_STENCIL_BIT )
        SET_ATTR_V(attribs, i, kCGLPFAStencilSize, 1);

    if( visual & CR_ALPHA_BIT )
        SET_ATTR_V(attribs, i, kCGLPFAAlphaSize, 1);

    if( visual & CR_DOUBLE_BIT )
        SET_ATTR(attribs, i, kCGLPFADoubleBuffer);

    if( visual & CR_STEREO_BIT )
        SET_ATTR(attribs, i, kCGLPFAStereo);

/*  SET_ATTR_V(attribs, i, kCGLPFASampleBuffers, 1);
    SET_ATTR_V(attribs, i, kCGLPFASamples, 0);
    SET_ATTR_V(attribs, i, kCGLPFADisplayMask, 0);  */
    SET_ATTR(attribs, i, kCGLPFABackingStore);
    //SET_ATTR(attribs, i, kCGLPFAWindow); // kCGLPFAWindow deprecated starting from OSX 10.7
    SET_ATTR_V(attribs, i, kCGLPFADisplayMask, ctx->disp_mask);

    SET_ATTR(attribs, i, 0);

    *num = i;
}

#endif

#ifndef GLX
/**
 * This creates a native GLX/WGL context.
 */
static GLboolean
InstantiateNativeContext( WindowInfo *window, ContextInfo *context )
{
#ifdef WINDOWS
    context->hglrc = stub.wsInterface.wglCreateContext( window->drawable );
    return context->hglrc ? GL_TRUE : GL_FALSE;
#elif defined(Darwin)
    CGLContextObj shareCtx = NULL;
    CGLPixelFormatObj pix;
    long npix;

    CGLPixelFormatAttribute attribs[16];
    GLint ind = 0;

    if( context->share ) {
        if( context->cglc != context->share->cglc ) {
            crWarning("CGLCreateContext() is trying to share a non-existant "
                      "CGL context.  Setting share context to zero.");
            shareCtx = 0;
        }
        else
            shareCtx = context->cglc;
    }

    stubSetPFA( context, attribs, 16, &ind );

    stub.wsInterface.CGLChoosePixelFormat( attribs, &pix, &npix );
    stub.wsInterface.CGLCreateContext( pix, shareCtx, &context->cglc );
    if( !context->cglc )
        crError("InstantiateNativeContext: Couldn't Create the context!");

    stub.wsInterface.CGLDestroyPixelFormat( pix );

    if( context->parambits ) {
        /* Set the delayed parameters */
        if( context->parambits & VISBIT_SWAP_RECT )
            stub.wsInterface.CGLSetParameter( context->cglc, kCGLCPSwapRectangle, context->swap_rect );

        if( context->parambits & VISBIT_SWAP_INTERVAL )
            stub.wsInterface.CGLSetParameter( context->cglc, kCGLCPSwapInterval, &(context->swap_interval) );

        if( context->parambits & VISBIT_CLIENT_STORAGE )
            stub.wsInterface.CGLSetParameter( context->cglc, kCGLCPClientStorage, (long*)&(context->client_storage) );

        context->parambits = 0;
    }

    return context->cglc ? GL_TRUE : GL_FALSE;
#elif defined(GLX)
    GLXContext shareCtx = 0;

    /* sort out context sharing here */
    if (context->share) {
            if (context->glxContext != context->share->glxContext) {
                    crWarning("glXCreateContext() is trying to share a non-existant "
                                        "GLX context.  Setting share context to zero.");
                    shareCtx = 0;
            }
            else {
                    shareCtx = context->glxContext;
            }
    }

    context->glxContext = stub.wsInterface.glXCreateContext( window->dpy,
                 context->visual, shareCtx, context->direct );

    return context->glxContext ? GL_TRUE : GL_FALSE;
#endif
}
#endif /* !GLX */


/**
 * Utility functions to get window size and titlebar text.
 */
#ifdef WINDOWS

void
stubGetWindowGeometry(WindowInfo *window, int *x, int *y,
                      unsigned int *w, unsigned int *h )
{
    RECT rect;

    if (!window->drawable || !window->hWnd) {
        *w = *h = 0;
        return;
    }

    if (window->hWnd!=WindowFromDC(window->drawable))
    {
        crWarning("Window(%i) DC is no longer valid", window->spuWindow);
        return;
    }

    if (!GetClientRect(window->hWnd, &rect))
    {
        crWarning("GetClientRect failed for %p", window->hWnd);
        *w = *h = 0;
        return;
    }
    *w = rect.right - rect.left;
    *h = rect.bottom - rect.top;

    if (!ClientToScreen( window->hWnd, (LPPOINT) &rect ))
    {
        crWarning("ClientToScreen failed for %p", window->hWnd);
        *w = *h = 0;
        return;
    }
    *x = rect.left;
    *y = rect.top;
}

static void
GetWindowTitle( const WindowInfo *window, char *title )
{
    /* XXX - we don't handle recurseUp */
    if (window->hWnd)
        GetWindowText(window->hWnd, title, 100);
    else
        title[0] = 0;
}

static void
GetCursorPosition(WindowInfo *window, int pos[2])
{
    RECT rect;
    POINT point;
    GLint size[2], x, y;
    unsigned int NativeHeight, NativeWidth, ChromiumHeight, ChromiumWidth;
    float WidthRatio, HeightRatio;
    static int DebugFlag = 0;

    // apparently the "window" parameter passed to this
    // function contains the native window information
    HWND NATIVEhwnd = window->hWnd;

    if (NATIVEhwnd!=WindowFromDC(window->drawable))
    {
        crWarning("Window(%i) DC is no longer valid", window->spuWindow);
        return;
    }

    // get the native window's height and width
    stubGetWindowGeometry(window, &x, &y, &NativeWidth, &NativeHeight);

    // get the spu window's height and width
    stub.spu->dispatch_table.GetChromiumParametervCR(GL_WINDOW_SIZE_CR, window->spuWindow, GL_INT, 2, size);
    ChromiumWidth = size[0];
    ChromiumHeight = size[1];

    // get the ratio of the size of the native window to the cr window
    WidthRatio = (float)ChromiumWidth / (float)NativeWidth;
    HeightRatio = (float)ChromiumHeight / (float)NativeHeight;

    // output some debug information at the beginning
    if(DebugFlag)
    {
        DebugFlag = 0;
        crDebug("Native Window Handle = %d", NATIVEhwnd);
        crDebug("Native Width = %i", NativeWidth);
        crDebug("Native Height = %i", NativeHeight);
        crDebug("Chromium Width = %i", ChromiumWidth);
        crDebug("Chromium Height = %i", ChromiumHeight);
    }

    if (NATIVEhwnd)
    {
        GetClientRect( NATIVEhwnd, &rect );
        GetCursorPos (&point);

        // make sure these coordinates are relative to the native window,
        // not the whole desktop
        ScreenToClient(NATIVEhwnd, &point);

        // calculate the new position of the virtual cursor
        pos[0] = (int)(point.x * WidthRatio);
        pos[1] = (int)((NativeHeight - point.y) * HeightRatio);
    }
    else
    {
        pos[0] = 0;
        pos[1] = 0;
    }
}

#elif defined(Darwin)

extern OSStatus CGSGetScreenRectForWindow( CGSConnectionID cid, CGSWindowID wid, float *outRect );
extern OSStatus CGSGetWindowBounds( CGSConnectionID cid, CGSWindowID wid, float *bounds );

void
stubGetWindowGeometry( const WindowInfo *window, int *x, int *y, unsigned int *w, unsigned int *h )
{
    float rect[4];

    if( !window ||
        !window->connection ||
        !window->drawable ||
        CGSGetWindowBounds( window->connection, window->drawable, rect ) != noErr )
    {
        *x = *y = 0;
        *w = *h = 0;
    } else {
        *x = (int) rect[0];
        *y = (int) rect[1];
        *w = (int) rect[2];
        *h = (int) rect[3];
    }
}


static void
GetWindowTitle( const WindowInfo *window, char *title )
{
    /* XXX \todo Darwin window Title */
    title[0] = '\0';
}


static void
GetCursorPosition( const WindowInfo *window, int pos[2] )
{
    Point mouse_pos;
    float window_rect[4];

    GetMouse( &mouse_pos );
    CGSGetScreenRectForWindow( window->connection, window->drawable, window_rect );

    pos[0] = mouse_pos.h - (int) window_rect[0];
    pos[1] = (int) window_rect[3] - (mouse_pos.v - (int) window_rect[1]);

    /*crDebug( "%i %i", pos[0], pos[1] );*/
}

#elif defined(GLX)

void
stubGetWindowGeometry(WindowInfo *window, int *x, int *y, unsigned int *w, unsigned int *h)
{
    Window root, child;
    unsigned int border, depth;
    Display *dpy;

    dpy = stubGetWindowDisplay(window);

    /// @todo Performing those checks is expensive operation, especially for simple apps with high FPS.
    //       Disabling those triples glxgears fps, thus using xevents instead of per frame polling is much more preferred.
    /// @todo Check similar on windows guests, though doubtful as there're no XSync like calls on windows.
    if (window && dpy)
    {
        XLOCK(dpy);
    }

    if (!window
        || !dpy
        || !window->drawable
        || !XGetGeometry(dpy, window->drawable, &root, x, y, w, h, &border, &depth)
        || !XTranslateCoordinates(dpy, window->drawable, root, 0, 0, x, y, &child))
    {
        crWarning("Failed to get windows geometry for %p, try xwininfo", window);
        *x = *y = 0;
        *w = *h = 0;
    }

    if (window && dpy)
    {
        XUNLOCK(dpy);
    }
}

static char *
GetWindowTitleHelper( Display *dpy, Window window, GLboolean recurseUp )
{
    while (1) {
        char *name;
        if (!XFetchName(dpy, window, &name))
            return NULL;
        if (name[0]) {
            return name;
        }
        else if (recurseUp) {
            /* This window has no name, try the parent */
            Status stat;
            Window root, parent, *children;
            unsigned int numChildren;
            stat = XQueryTree( dpy, window, &root, &parent,
                                                 &children, &numChildren );
            if (!stat || window == root)
                return NULL;
            if (children)
                XFree(children);
            window = parent;
        }
        else {
            XFree(name);
            return NULL;
        }
    }
}

static void
GetWindowTitle( const WindowInfo *window, char *title )
{
    char *t = GetWindowTitleHelper(window->dpy, window->drawable, GL_TRUE);
    if (t) {
        crStrcpy(title, t);
        XFree(t);
    }
    else {
        title[0] = 0;
    }
}


/**
 *Return current cursor position in local window coords.
 */
static void
GetCursorPosition(WindowInfo *window, int pos[2] )
{
    int rootX, rootY;
    Window root, child;
    unsigned int mask;
    int x, y;

    XLOCK(window->dpy);

    Bool q = XQueryPointer(window->dpy, window->drawable, &root, &child,
                                                 &rootX, &rootY, &pos[0], &pos[1], &mask);
    if (q) {
        unsigned int w, h;
        stubGetWindowGeometry( window, &x, &y, &w, &h );
        /* invert Y */
        pos[1] = (int) h - pos[1] - 1;
    }
    else {
        pos[0] = pos[1] = 0;
    }

    XUNLOCK(window->dpy);
}

#endif


/**
 * This function is called by MakeCurrent() and determines whether or
 * not a new rendering context should be bound to Chromium or the native
 * OpenGL.
 * \return  GL_FALSE if native OpenGL should be used, or GL_TRUE if Chromium
 *          should be used.
 */
static GLboolean
stubCheckUseChromium( WindowInfo *window )
{
    int x, y;
    unsigned int w, h;

    /* If the provided window is CHROMIUM, we're clearly intended
     * to create a CHROMIUM context.
     */
    if (window->type == CHROMIUM)
        return GL_TRUE;

    if (stub.ignoreFreeglutMenus) {
        const char *glutMenuTitle = "freeglut menu";
        char title[1000];
        GetWindowTitle(window, title);
        if (crStrcmp(title, glutMenuTitle) == 0) {
            crDebug("GL faker: Ignoring freeglut menu window");
            return GL_FALSE;
        }
    }

    /*  If the user's specified a window count for Chromium, see if
        *  this window satisfies that criterium.
        */
    stub.matchChromiumWindowCounter++;
    if (stub.matchChromiumWindowCount > 0) {
        if (stub.matchChromiumWindowCounter != stub.matchChromiumWindowCount) {
            crDebug("Using native GL, app window doesn't meet match_window_count");
            return GL_FALSE;
        }
    }

    /* If the user's specified a window list to ignore, see if this
     * window satisfies that criterium.
     */
    if (stub.matchChromiumWindowID) {
        GLuint i;

        for (i = 0; i <= stub.numIgnoreWindowID; i++) {
            if (stub.matchChromiumWindowID[i] == stub.matchChromiumWindowCounter) {
                crDebug("Ignore window ID %d, using native GL", stub.matchChromiumWindowID[i]);
                return GL_FALSE;
            }
        }
    }

    /* If the user's specified a minimum window size for Chromium, see if
     * this window satisfies that criterium.
     */
    if (stub.minChromiumWindowWidth > 0 &&
        stub.minChromiumWindowHeight > 0) {
        stubGetWindowGeometry( window, &x, &y, &w, &h );
        if (w >= stub.minChromiumWindowWidth &&
            h >= stub.minChromiumWindowHeight) {

            /* Check for maximum sized window now too */
            if (stub.maxChromiumWindowWidth &&
                stub.maxChromiumWindowHeight) {
                if (w < stub.maxChromiumWindowWidth &&
                    h < stub.maxChromiumWindowHeight)
                    return GL_TRUE;
                else
                    return GL_FALSE;
            }

            return GL_TRUE;
        }
        crDebug("Using native GL, app window doesn't meet minimum_window_size");
        return GL_FALSE;
    }
    else if (stub.matchWindowTitle) {
        /* If the user's specified a window title for Chromium, see if this
         * window satisfies that criterium.
         */
        GLboolean wildcard = GL_FALSE;
        char title[1000];
        char *titlePattern;
        int len;
        /* check for leading '*' wildcard */
        if (stub.matchWindowTitle[0] == '*') {
            titlePattern = crStrdup( stub.matchWindowTitle + 1 );
            wildcard = GL_TRUE;
        }
        else {
            titlePattern = crStrdup( stub.matchWindowTitle );
        }
        /* check for trailing '*' wildcard */
        len = crStrlen(titlePattern);
        if (len > 0 && titlePattern[len - 1] == '*') {
            titlePattern[len - 1] = '\0'; /* terminate here */
            wildcard = GL_TRUE;
        }

        GetWindowTitle( window, title );
        if (title[0]) {
            if (wildcard) {
                if (crStrstr(title, titlePattern)) {
                    crFree(titlePattern);
                    return GL_TRUE;
                }
            }
            else if (crStrcmp(title, titlePattern) == 0) {
                crFree(titlePattern);
                return GL_TRUE;
            }
        }
        crFree(titlePattern);
        crDebug("Using native GL, app window title doesn't match match_window_title string (\"%s\" != \"%s\")", title, stub.matchWindowTitle);
        return GL_FALSE;
    }

    /* Window title and size don't matter */
    CRASSERT(stub.minChromiumWindowWidth == 0);
    CRASSERT(stub.minChromiumWindowHeight == 0);
    CRASSERT(stub.matchWindowTitle == NULL);

    /* User hasn't specified a width/height or window title.
     * We'll use chromium for this window (and context) if no other is.
     */

    return GL_TRUE;  /* use Chromium! */
}

static void stubWindowCheckOwnerCB(unsigned long key, void *data1, void *data2)
{
    WindowInfo *pWindow = (WindowInfo *) data1;
    ContextInfo *pCtx = (ContextInfo *) data2;

    RT_NOREF(key);


    if (pWindow->pOwner == pCtx)
    {
#ifdef WINDOWS
            /* Note: can't use WindowFromDC(context->pOwnWindow->drawable) here
               because GL context is already released from DC and actual guest window
               could be destroyed.
             */
            stubDestroyWindow(CR_CTX_CON(pCtx), (GLint)pWindow->hWnd);
#else
            stubDestroyWindow(CR_CTX_CON(pCtx), (GLint)pWindow->drawable);
#endif
    }
}

GLboolean stubCtxCreate(ContextInfo *context)
{
    /*
     * Create a Chromium context.
     */
#if defined(GLX) || defined(DARWIN)
    GLint spuShareCtx = context->share ? context->share->spuContext : 0;
#else
    GLint spuShareCtx = 0;
#endif
    GLint spuConnection = 0;
    CRASSERT(stub.spu);
    CRASSERT(stub.spu->dispatch_table.CreateContext);
    context->type = CHROMIUM;

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    if (context->pHgsmi)
    {
        spuConnection = stub.spu->dispatch_table.VBoxConCreate(context->pHgsmi);
        if (!spuConnection)
        {
            crError("VBoxConCreate failed");
            return GL_FALSE;
        }
        context->spuConnection = spuConnection;
    }
#endif

    context->spuContext
        = stub.spu->dispatch_table.VBoxCreateContext(spuConnection, context->dpyName,
                                            context->visBits,
                                            spuShareCtx);

    return GL_TRUE;
}

GLboolean stubCtxCheckCreate(ContextInfo *context)
{
    if (context->type == UNDECIDED)
        return stubCtxCreate(context);
    return CHROMIUM == context->type;
}


GLboolean
stubMakeCurrent( WindowInfo *window, ContextInfo *context )
{
    GLboolean retVal = GL_FALSE;

    /*
     * Get WindowInfo and ContextInfo pointers.
     */

    if (!context || !window) {
        ContextInfo * currentContext = stubGetCurrentContext();
        if (currentContext)
            currentContext->currentDrawable = NULL;
        if (context)
            context->currentDrawable = NULL;
        stubSetCurrentContext(NULL);
        return GL_TRUE;  /* OK */
    }

    stubCheckMultithread();

    if (context->type == UNDECIDED) {
        /* Here's where we really create contexts */
        crLockMutex(&stub.mutex);

        if (stubCheckUseChromium(window)) {
            GLint spuConnection = 0;

            if (!stubCtxCreate(context))
            {
                crWarning("stubCtxCreate failed");
                return GL_FALSE;
            }

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
            spuConnection = context->spuConnection;
#endif

            if (window->spuWindow == -1)
            {
                /*crDebug("(1)stubMakeCurrent ctx=%p(%i) window=%p(%i)", context, context->spuContext, window, window->spuWindow);*/
                window->spuWindow = stub.spu->dispatch_table.VBoxWindowCreate(spuConnection, window->dpyName, context->visBits );
#ifdef CR_NEWWINTRACK
                window->u32ClientID = stub.spu->dispatch_table.VBoxPackGetInjectID(spuConnection);
#endif
            }
        }
#ifndef GLX
        else {
            /*
             * Create a native OpenGL context.
             */
            if (!InstantiateNativeContext(window, context))
            {
                crUnlockMutex(&stub.mutex);
                return 0; /* false */
            }
            context->type = NATIVE;
        }
#endif /* !GLX */

        crUnlockMutex(&stub.mutex);
    }


    if (context->type == NATIVE) {
        /*
         * Native OpenGL MakeCurrent().
         */
#ifdef WINDOWS
        retVal = (GLboolean) stub.wsInterface.wglMakeCurrent( window->drawable, context->hglrc );
#elif defined(Darwin)
        // XXX \todo We need to differentiate between these two..
        retVal = ( stub.wsInterface.CGLSetSurface(context->cglc, window->connection, window->drawable, window->surface) == noErr );
        retVal = ( stub.wsInterface.CGLSetCurrentContext(context->cglc) == noErr );
#elif defined(GLX)
        retVal = (GLboolean) stub.wsInterface.glXMakeCurrent( window->dpy, window->drawable, context->glxContext );
#endif
    }
    else {
        /*
         * SPU chain MakeCurrent().
         */
        CRASSERT(context->type == CHROMIUM);
        CRASSERT(context->spuContext >= 0);

        /*if (context->currentDrawable && context->currentDrawable != window)
            crDebug("Rebinding context %p to a different window", context);*/

        if (window->type == NATIVE) {
            crWarning("Can't rebind a chromium context to a native window\n");
            retVal = 0;
        }
        else {
            if (window->spuWindow == -1)
            {
                /*crDebug("(2)stubMakeCurrent ctx=%p(%i) window=%p(%i)", context, context->spuContext, window, window->spuWindow);*/
                window->spuWindow = stub.spu->dispatch_table.VBoxWindowCreate(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                        context->spuConnection,
#else
                        0,
#endif
                        window->dpyName, context->visBits );
#ifdef CR_NEWWINTRACK
                window->u32ClientID = stub.spu->dispatch_table.VBoxPackGetInjectID(
# if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                        context->spuConnection
# else
                        0
# endif
                        );
#endif
                if (context->currentDrawable && context->currentDrawable->type==CHROMIUM
                    && context->currentDrawable->pOwner==context)
                {
#ifdef WINDOWS
                        if (context->currentDrawable->hWnd!=WindowFromDC(context->currentDrawable->drawable))
                        {
                            stubDestroyWindow(CR_CTX_CON(context), (GLint)context->currentDrawable->hWnd);
                        }
#else
                        Window root;
                        int x, y;
                        unsigned int border, depth, w, h;

                        XLOCK(context->currentDrawable->dpy);
                        if (!XGetGeometry(context->currentDrawable->dpy, context->currentDrawable->drawable, &root, &x, &y, &w, &h, &border, &depth))
                        {
                            stubDestroyWindow(CR_CTX_CON(context), (GLint)context->currentDrawable->drawable);
                        }
                        XUNLOCK(context->currentDrawable->dpy);
#endif

                }
            }

            if (window->spuWindow != (GLint)window->drawable)
                 stub.spu->dispatch_table.MakeCurrent( window->spuWindow, (GLint) window->drawable, context->spuContext );
            else
                 stub.spu->dispatch_table.MakeCurrent( window->spuWindow, 0, /* native window handle */ context->spuContext );

            retVal = 1;
        }
    }

    window->type = context->type;
    window->pOwner = context;
    context->currentDrawable = window;
    stubSetCurrentContext(context);

    if (retVal) {
        /* Now, if we've transitions from Chromium to native rendering, or
         * vice versa, we have to change all the OpenGL entrypoint pointers.
         */
        if (context->type == NATIVE) {
            /* Switch to native API */
            /*printf("  Switching to native API\n");*/
            stubSetDispatch(&stub.nativeDispatch);
        }
        else if (context->type == CHROMIUM) {
            /* Switch to stub (SPU) API */
            /*printf("  Switching to spu API\n");*/
            stubSetDispatch(&stub.spuDispatch);
        }
        else {
            /* no API switch needed */
        }
    }

    if (!window->width && window->type == CHROMIUM) {
        /* One time window setup */
        int x, y;
        unsigned int winW, winH;

        stubGetWindowGeometry( window, &x, &y, &winW, &winH );

        /* If we're not using GLX/WGL (no app window) we'll always get
         * a width and height of zero here.  In that case, skip the viewport
         * call since we're probably using a tilesort SPU with fake_window_dims
         * which the tilesort SPU will use for the viewport.
         */
        window->width = winW;
        window->height = winH;
#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
        if (stubIsWindowVisible(window))
#endif
        {
            if (stub.trackWindowSize)
                stub.spuDispatch.WindowSize( window->spuWindow, winW, winH );
            if (stub.trackWindowPos)
                stub.spuDispatch.WindowPosition(window->spuWindow, x, y);
            if (winW > 0 && winH > 0)
                stub.spu->dispatch_table.Viewport( 0, 0, winW, winH );
        }
#ifdef VBOX_WITH_WDDM
        if (stub.trackWindowVisibleRgn)
            stub.spu->dispatch_table.WindowVisibleRegion(window->spuWindow, 0, NULL);
#endif
    }

    /* Update window mapping state.
     * Basically, this lets us hide render SPU windows which correspond
     * to unmapped application windows.  Without this, "pertly" (for example)
     * opens *lots* of temporary windows which otherwise clutter the screen.
     */
    if (stub.trackWindowVisibility && window->type == CHROMIUM && window->drawable) {
        const int mapped = stubIsWindowVisible(window);
        if (mapped != window->mapped) {
            crDebug("Dispatched: WindowShow(%i, %i)", window->spuWindow, mapped);
            stub.spu->dispatch_table.WindowShow(window->spuWindow, mapped);
            window->mapped = mapped;
        }
    }

    return retVal;
}

void
stubDestroyContext( unsigned long contextId )
{
    ContextInfo *context;

    if (!stub.contextTable) {
        return;
    }

    /* the lock order is windowTable->contextTable (see wglMakeCurrent_prox, glXMakeCurrent)
     * this is why we need to take a windowTable lock since we will later do stub.windowTable access & locking */
    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, contextId);
    if (context)
        stubDestroyContextLocked(context);
    else
        crError("No context.");

    if (stubGetCurrentContext() == context) {
        stubSetCurrentContext(NULL);
    }

    VBoxTlsRefMarkDestroy(context);
    VBoxTlsRefRelease(context);
    crHashtableUnlock(stub.contextTable);
    crHashtableUnlock(stub.windowTable);
}

void
stubSwapBuffers(WindowInfo *window, GLint flags)
{
    if (!window)
        return;

    /* Determine if this window is being rendered natively or through
     * Chromium.
     */

    if (window->type == NATIVE) {
        /*printf("*** Swapping native window %d\n", (int) drawable);*/
#ifdef WINDOWS
        (void) stub.wsInterface.wglSwapBuffers( window->drawable );
#elif defined(Darwin)
        /* ...is this ok? */
/*      stub.wsInterface.CGLFlushDrawable( context->cglc ); */
        crDebug("stubSwapBuffers: unable to swap (no context!)");
#elif defined(GLX)
        stub.wsInterface.glXSwapBuffers( window->dpy, window->drawable );
#endif
    }
    else if (window->type == CHROMIUM) {
        /* Let the SPU do the buffer swap */
        /*printf("*** Swapping chromium window %d\n", (int) drawable);*/
        if (stub.appDrawCursor) {
            int pos[2];
            GetCursorPosition(window, pos);
            stub.spu->dispatch_table.ChromiumParametervCR(GL_CURSOR_POSITION_CR, GL_INT, 2, pos);
        }
        stub.spu->dispatch_table.SwapBuffers( window->spuWindow, flags );
    }
    else {
        crDebug("Calling SwapBuffers on a window we haven't seen before (no-op).");
    }
}
