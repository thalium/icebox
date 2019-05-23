/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_spu.h"
#include "cr_error.h"
#include "cr_string.h"
#include "cr_url.h"
#include "cr_environment.h"
#include "renderspu.h"
#include <stdio.h>

#ifdef RT_OS_DARWIN
# include <iprt/semaphore.h>
#endif /* RT_OS_DARWIN */

static SPUNamedFunctionTable _cr_render_table[1000];

SPUFunctions render_functions = {
    NULL, /* CHILD COPY */
    NULL, /* DATA */
    _cr_render_table /* THE ACTUAL FUNCTIONS */
};

RenderSPU render_spu;
uint64_t render_spu_parent_window_id = 0;

#ifdef CHROMIUM_THREADSAFE
CRtsd _RenderTSD;
#endif

static void swapsyncConnect(void)
{
    char hostname[4096], protocol[4096];
    unsigned short port;

    crNetInit(NULL, NULL);

    if (!crParseURL( render_spu.swap_master_url, protocol, hostname,
                    &port, 9876))
        crError( "Bad URL: %s", render_spu.swap_master_url );

    if (render_spu.is_swap_master)
    {
        int a;

        render_spu.swap_conns = (CRConnection **)crAlloc(
                        render_spu.num_swap_clients*sizeof(CRConnection *));
        for (a=0; a<render_spu.num_swap_clients; a++)
        {
            render_spu.swap_conns[a] = crNetAcceptClient( protocol, hostname, port,
                                                        render_spu.swap_mtu, 1);
        }
    }
    else
    {
        render_spu.swap_conns = (CRConnection **)crAlloc(sizeof(CRConnection *));

        render_spu.swap_conns[0] = crNetConnectToServer(render_spu.swap_master_url,
                                    port, render_spu.swap_mtu, 1);
        if (!render_spu.swap_conns[0])
            crError("Failed connection");
    }
}

#ifdef RT_OS_WINDOWS
static DWORD WINAPI renderSPUWindowThreadProc(void* unused)
{
    MSG msg;
    BOOL bRet;

    (void) unused;

    /* Force system to create the message queue.
     * Else, there's a chance that render spu will issue PostThreadMessage
     * before this thread calls GetMessage for first time.
     */
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    crDebug("RenderSPU: Window thread started (%x)", crThreadID());
    SetEvent(render_spu.hWinThreadReadyEvent);

    while( (bRet = GetMessage( &msg, 0, 0, 0 )) != 0)
    {
        if (bRet == -1)
        {
            crError("RenderSPU: Window thread GetMessage failed (%x)", GetLastError());
            break;
        }
        else
        {
            if (msg.message == WM_VBOX_RENDERSPU_CREATE_WINDOW)
            {
                LPCREATESTRUCT pCS = (LPCREATESTRUCT) msg.lParam;
                HWND hWnd;
                WindowInfo *pWindow = (WindowInfo *)pCS->lpCreateParams;

                CRASSERT(msg.lParam && !msg.wParam && pCS->lpCreateParams);

                hWnd = CreateWindowEx(pCS->dwExStyle, pCS->lpszName, pCS->lpszClass, pCS->style,
                                        pCS->x, pCS->y, pCS->cx, pCS->cy,
                                        pCS->hwndParent, pCS->hMenu, pCS->hInstance, &render_spu);

                pWindow->hWnd = hWnd;

                SetEvent(render_spu.hWinThreadReadyEvent);
            }
            else if (msg.message == WM_VBOX_RENDERSPU_DESTROY_WINDOW)
            {
                CRASSERT(msg.lParam && !msg.wParam);

                DestroyWindow(((VBOX_RENDERSPU_DESTROY_WINDOW*) msg.lParam)->hWnd);

                SetEvent(render_spu.hWinThreadReadyEvent);
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    render_spu.dwWinThreadId = 0;

    crDebug("RenderSPU: Window thread stopped (%x)", crThreadID());
    SetEvent(render_spu.hWinThreadReadyEvent);

    return 0;
}
#endif

int renderspuDefaultCtxInit()
{
    GLint defaultWin, defaultCtx;
    WindowInfo *windowInfo;

    /*
     * Create the default window and context.  Their indexes are zero and
     * a client can use them without calling CreateContext or WindowCreate.
     */
    crDebug("Render SPU: Creating default window (visBits=0x%x, id=0)",
            render_spu.default_visual);
    defaultWin = renderspuWindowCreateEx( NULL, render_spu.default_visual, CR_RENDER_DEFAULT_WINDOW_ID );
    if (defaultWin != CR_RENDER_DEFAULT_WINDOW_ID) {
        crError("Render SPU: Couldn't get a double-buffered, RGB visual with Z!");
        return VERR_GENERAL_FAILURE;
    }
    crDebug( "Render SPU: WindowCreate returned %d (0=normal)", defaultWin );

    crDebug("Render SPU: Creating default context, visBits=0x%x",
            render_spu.default_visual );
    defaultCtx = renderspuCreateContextEx( NULL, render_spu.default_visual, CR_RENDER_DEFAULT_CONTEXT_ID, 0 );
    if (defaultCtx != CR_RENDER_DEFAULT_CONTEXT_ID) {
        crError("Render SPU: failed to create default context!");
        return VERR_GENERAL_FAILURE;
    }

    renderspuMakeCurrent( defaultWin, 0, defaultCtx );

    /* Get windowInfo for the default window */
    windowInfo = (WindowInfo *) crHashtableSearch(render_spu.windowTable, CR_RENDER_DEFAULT_WINDOW_ID);
    CRASSERT(windowInfo);
    windowInfo->mapPending = GL_TRUE;

    return VINF_SUCCESS;
}

static SPUFunctions *
renderSPUInit( int id, SPU *child, SPU *self,
               unsigned int context_id, unsigned int num_contexts )
{
    int numFuncs, numSpecial;

    const char * pcpwSetting;
    int rc;

    (void) child;
    (void) context_id;
    (void) num_contexts;

    self->privatePtr = (void *) &render_spu;

#ifdef CHROMIUM_THREADSAFE
    crDebug("Render SPU: thread-safe");
    crInitTSD(&_RenderTSD);
#endif

    crMemZero(&render_spu, sizeof(render_spu));

    render_spu.id = id;
    renderspuSetVBoxConfiguration(&render_spu);

    if (render_spu.swap_master_url)
        swapsyncConnect();


    /* Get our special functions. */
    numSpecial = renderspuCreateFunctions( _cr_render_table );

#ifdef RT_OS_WINDOWS
    /* Start thread to create windows and process window messages */
    crDebug("RenderSPU: Starting windows serving thread");
    render_spu.hWinThreadReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!render_spu.hWinThreadReadyEvent)
    {
        crError("RenderSPU: Failed to create WinThreadReadyEvent! (%x)", GetLastError());
        return NULL;
    }

    if (!CreateThread(NULL, 0, renderSPUWindowThreadProc, 0, 0, &render_spu.dwWinThreadId))
    {
        crError("RenderSPU: Failed to start windows thread! (%x)", GetLastError());
        return NULL;
    }
    WaitForSingleObject(render_spu.hWinThreadReadyEvent, INFINITE);
#endif

    /* Get the OpenGL functions. */
    numFuncs = crLoadOpenGL( &render_spu.ws, _cr_render_table + numSpecial );
    if (numFuncs == 0) {
        crError("The render SPU was unable to load the native OpenGL library");
        return NULL;
    }

    numFuncs += numSpecial;

    render_spu.contextTable = crAllocHashtableEx(1, INT32_MAX);
    render_spu.windowTable = crAllocHashtableEx(1, INT32_MAX);

    render_spu.dummyWindowTable = crAllocHashtable();

    pcpwSetting = crGetenv("CR_RENDER_ENABLE_SINGLE_PRESENT_CONTEXT");
    if (pcpwSetting)
    {
        if (pcpwSetting[0] == '0')
            pcpwSetting = NULL;
    }

    if (pcpwSetting)
    {
        /* TODO: need proper blitter synchronization, do not use so far!
         * the problem is that rendering can be done in multiple thread: the main command (hgcm) thread and the redraw thread
         * we currently use per-window synchronization, while we'll need a per-blitter synchronization if one blitter is used for multiple windows
         * this is not done currently */
        crWarning("TODO: need proper blitter synchronization, do not use so far!");
        render_spu.blitterTable = crAllocHashtable();
        CRASSERT(render_spu.blitterTable);
    }
    else
        render_spu.blitterTable = NULL;

    CRASSERT(render_spu.default_visual & CR_RGB_BIT);
    
    rc = renderspu_SystemInit();
    if (!RT_SUCCESS(rc))
    {
        crError("renderspu_SystemInit failed rc %d", rc);
        return NULL;
    }
#ifdef USE_OSMESA
    if (render_spu.use_osmesa) {
        if (!crLoadOSMesa(&render_spu.OSMesaCreateContext,
                          &render_spu.OSMesaMakeCurrent,
                          &render_spu.OSMesaDestroyContext)) {
            crError("Unable to load OSMesa library");
        }
    }
#endif

#ifdef DARWIN
# ifdef VBOX_WITH_COCOA_QT
# else /* VBOX_WITH_COCOA_QT */
    render_spu.hRootVisibleRegion = 0;
    render_spu.currentBufferName = 1;
    render_spu.uiDockUpdateTS = 0;
    /* Create a mutex for synchronizing events from the main Qt thread & this
       thread */
    RTSemFastMutexCreate(&render_spu.syncMutex);
    /* Create our window groups */
    CreateWindowGroup(kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrSharedActivation | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrFixedLevel, &render_spu.pMasterGroup);
    CreateWindowGroup(kWindowGroupAttrMoveTogether | kWindowGroupAttrLayerTogether | kWindowGroupAttrSharedActivation | kWindowGroupAttrHideOnCollapse | kWindowGroupAttrFixedLevel, &render_spu.pParentGroup);
    /* Make the correct z-layering */
    SendWindowGroupBehind (render_spu.pParentGroup, render_spu.pMasterGroup);
    /* and set the gParentGroup as parent for gMasterGroup. */
    SetWindowGroupParent (render_spu.pMasterGroup, render_spu.pParentGroup);
    /* Install the event handlers */
    EventTypeSpec eventList[] =
    {
        {kEventClassVBox, kEventVBoxUpdateContext}, /* Update the context after show/size/move events */
        {kEventClassVBox, kEventVBoxBoundsChanged}  /* Clip/Pos the OpenGL windows when the main window is changed in pos/size */
    };
    /* We need to process events from our main window */
    render_spu.hParentEventHandler = NewEventHandlerUPP(windowEvtHndlr);
    InstallApplicationEventHandler (render_spu.hParentEventHandler,
                                    GetEventTypeCount(eventList), eventList,
                                    NULL, NULL);
    render_spu.fInit = true;
# endif /* VBOX_WITH_COCOA_QT */
#endif /* DARWIN */

    rc = renderspuDefaultCtxInit();
    if (!RT_SUCCESS(rc))
    {
        WARN(("renderspuDefaultCtxInit failed %d", rc));
        return NULL;
    }

    /*
     * Get the OpenGL extension functions.
     * SIGH -- we have to wait until the very bitter end to load the
     * extensions, because the context has to be bound before
     * wglGetProcAddress will work correctly.  No such issue with GLX though.
     */
    numFuncs += crLoadOpenGLExtensions( &render_spu.ws, _cr_render_table + numFuncs );
    CRASSERT(numFuncs < 1000);

#ifdef WINDOWS
    /*
     * Same problem as above, these are extensions so we need to
     * load them after a context has been bound. As they're WGL
     * extensions too, we can't simply tag them into the spu_loader.
     * So we do them here for now.
     * Grrr, NVIDIA driver uses EXT for GetExtensionsStringEXT,
     * but ARB for others. Need further testing here....
     */
    render_spu.ws.wglGetExtensionsStringEXT =
        (wglGetExtensionsStringEXTFunc_t)
        render_spu.ws.wglGetProcAddress( "wglGetExtensionsStringEXT" );
    render_spu.ws.wglChoosePixelFormatEXT =
        (wglChoosePixelFormatEXTFunc_t)
        render_spu.ws.wglGetProcAddress( "wglChoosePixelFormatARB" );
    render_spu.ws.wglGetPixelFormatAttribivEXT =
        (wglGetPixelFormatAttribivEXTFunc_t)
        render_spu.ws.wglGetProcAddress( "wglGetPixelFormatAttribivARB" );
    render_spu.ws.wglGetPixelFormatAttribfvEXT =
        (wglGetPixelFormatAttribfvEXTFunc_t)
        render_spu.ws.wglGetProcAddress( "wglGetPixelFormatAttribfvARB" );

    if (render_spu.ws.wglGetProcAddress("glCopyTexSubImage3D"))
    {
        _cr_render_table[numFuncs].name = crStrdup("CopyTexSubImage3D");
        _cr_render_table[numFuncs].fn = (SPUGenericFunction) render_spu.ws.wglGetProcAddress("glCopyTexSubImage3D");
        ++numFuncs;
        crDebug("Render SPU: Found glCopyTexSubImage3D function");
    }

    if (render_spu.ws.wglGetProcAddress("glDrawRangeElements"))
    {
        _cr_render_table[numFuncs].name = crStrdup("DrawRangeElements");
        _cr_render_table[numFuncs].fn = (SPUGenericFunction) render_spu.ws.wglGetProcAddress("glDrawRangeElements");
        ++numFuncs;
        crDebug("Render SPU: Found glDrawRangeElements function");
    }

    if (render_spu.ws.wglGetProcAddress("glTexSubImage3D"))
    {
        _cr_render_table[numFuncs].name = crStrdup("TexSubImage3D");
        _cr_render_table[numFuncs].fn = (SPUGenericFunction) render_spu.ws.wglGetProcAddress("glTexSubImage3D");
        ++numFuncs;
        crDebug("Render SPU: Found glTexSubImage3D function");
    }

    if (render_spu.ws.wglGetProcAddress("glTexImage3D"))
    {
        _cr_render_table[numFuncs].name = crStrdup("TexImage3D");
        _cr_render_table[numFuncs].fn = (SPUGenericFunction) render_spu.ws.wglGetProcAddress("glTexImage3D");
        ++numFuncs;
        crDebug("Render SPU: Found glTexImage3D function");
    }

    if (render_spu.ws.wglGetExtensionsStringEXT) {
        crDebug("WGL - found wglGetExtensionsStringEXT\n");
    }
    if (render_spu.ws.wglChoosePixelFormatEXT) {
        crDebug("WGL - found wglChoosePixelFormatEXT\n");
    }
#endif

    render_spu.barrierHash = crAllocHashtable();

    render_spu.cursorX = 0;
    render_spu.cursorY = 0;
    render_spu.use_L2 = 0;

    render_spu.gather_conns = NULL;

    numFuncs = renderspu_SystemPostprocessFunctions(_cr_render_table, numFuncs, RT_ELEMENTS(_cr_render_table));

    crDebug("Render SPU: ---------- End of Init -------------");

    return &render_functions;
}

static void renderSPUSelfDispatch(SPUDispatchTable *self)
{
    crSPUInitDispatchTable( &(render_spu.self) );
    crSPUCopyDispatchTable( &(render_spu.self), self );

    crSPUInitDispatchTable( &(render_spu.blitterDispatch) );
    crSPUCopyDispatchTable( &(render_spu.blitterDispatch), self );

    render_spu.server = (CRServer *)(self->server);

    {
        GLfloat version;
        version = crStrToFloat((const char *) render_spu.ws.glGetString(GL_VERSION));

        if (version>=2.f || crStrstr((const char*)render_spu.ws.glGetString(GL_EXTENSIONS), "GL_ARB_vertex_shader"))
        {
            GLint mu=0;
            render_spu.self.GetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &mu);
            crInfo("Render SPU: GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB=%i", mu);
        }
    }
}


static void DeleteContextCallback( void *data )
{
    ContextInfo *context = (ContextInfo *) data;
    renderspuContextMarkDeletedAndRelease(context);
}

static void DeleteWindowCallback( void *data )
{
    WindowInfo *window = (WindowInfo *) data;
    renderspuWinTermOnShutdown(window);
    renderspuWinRelease(window);
}

static void DeleteBlitterCallback( void *data )
{
    PCR_BLITTER pBlitter = (PCR_BLITTER) data;
    CrBltTerm(pBlitter);
    crFree(pBlitter);
}

static void renderspuBlitterCleanupCB(unsigned long key, void *data1, void *data2)
{
    WindowInfo *window = (WindowInfo *) data1;
    CRASSERT(window);

    renderspuVBoxPresentBlitterCleanup( window );
}


static void renderspuDeleteBlitterCB(unsigned long key, void *data1, void *data2)
{
    CRHashTable *pTbl = (CRHashTable*)data2;

    crHashtableDelete( pTbl, key, NULL );

    DeleteBlitterCallback(data1);
}


static void renderspuDeleteWindowCB(unsigned long key, void *data1, void *data2)
{
    CRHashTable *pTbl = (CRHashTable*)data2;

    crHashtableDelete( pTbl, key, NULL );

    DeleteWindowCallback(data1);
}

static void renderspuDeleteBarierCB(unsigned long key, void *data1, void *data2)
{
    CRHashTable *pTbl = (CRHashTable*)data2;

    crHashtableDelete( pTbl, key, NULL );

    crFree(data1);
}


static void renderspuDeleteContextCB(unsigned long key, void *data1, void *data2)
{
    CRHashTable *pTbl = (CRHashTable*)data2;

    crHashtableDelete( pTbl, key, NULL );

    DeleteContextCallback(data1);
}

void renderspuCleanupBase(bool fDeleteTables)
{
    renderspuVBoxCompositorClearAll();

    if (render_spu.blitterTable)
    {
        if (fDeleteTables)
        {
            crFreeHashtable(render_spu.blitterTable, DeleteBlitterCallback);
            render_spu.blitterTable = NULL;
        }
        else
        {
            crHashtableWalk(render_spu.blitterTable, renderspuDeleteBlitterCB, render_spu.contextTable);
        }
    }
    else
    {
        crHashtableWalk(render_spu.windowTable, renderspuBlitterCleanupCB, NULL);

        crHashtableWalk(render_spu.dummyWindowTable, renderspuBlitterCleanupCB, NULL);
    }

    renderspuSetDefaultSharedContext(NULL);

    if (fDeleteTables)
    {
        crFreeHashtable(render_spu.contextTable, DeleteContextCallback);
        render_spu.contextTable = NULL;
        crFreeHashtable(render_spu.windowTable, DeleteWindowCallback);
        render_spu.windowTable = NULL;
        crFreeHashtable(render_spu.dummyWindowTable, DeleteWindowCallback);
        render_spu.dummyWindowTable = NULL;
        crFreeHashtable(render_spu.barrierHash, crFree);
        render_spu.barrierHash = NULL;
    }
    else
    {
        crHashtableWalk(render_spu.contextTable, renderspuDeleteContextCB, render_spu.contextTable);
        crHashtableWalk(render_spu.windowTable, renderspuDeleteWindowCB, render_spu.windowTable);
        crHashtableWalk(render_spu.dummyWindowTable, renderspuDeleteWindowCB, render_spu.dummyWindowTable);
        crHashtableWalk(render_spu.barrierHash, renderspuDeleteBarierCB, render_spu.barrierHash);
    }
}

static int renderSPUCleanup(void)
{
    renderspuCleanupBase(true);

#ifdef RT_OS_DARWIN
# ifndef VBOX_WITH_COCOA_QT
    render_spu.fInit = false;
    DisposeEventHandlerUPP(render_spu.hParentEventHandler);
    ReleaseWindowGroup(render_spu.pMasterGroup);
    ReleaseWindowGroup(render_spu.pParentGroup);
    if (render_spu.hRootVisibleRegion)
    {
        DisposeRgn(render_spu.hRootVisibleRegion);
        render_spu.hRootVisibleRegion = 0;
    }
    render_spu.currentBufferName = 1;
    render_spu.uiDockUpdateTS = 0;
    RTSemFastMutexDestroy(render_spu.syncMutex);
# else /* VBOX_WITH_COCOA_QT */
# endif /* VBOX_WITH_COCOA_QT */
#endif /* RT_OS_DARWIN */

#ifdef RT_OS_WINDOWS
    if (render_spu.dwWinThreadId)
    {
        HANDLE hNative;

        hNative = OpenThread(SYNCHRONIZE|THREAD_QUERY_INFORMATION|THREAD_TERMINATE,
                             false, render_spu.dwWinThreadId);
        if (!hNative)
        {
            crWarning("Failed to get handle for window thread(%#x)", GetLastError());
        }

        if (PostThreadMessage(render_spu.dwWinThreadId, WM_QUIT, 0, 0))
        {
            WaitForSingleObject(render_spu.hWinThreadReadyEvent, INFINITE);

            /*wait for os thread to actually finish*/
            if (hNative && WaitForSingleObject(hNative, 3000)==WAIT_TIMEOUT)
            {
                crDebug("Wait failed, terminating");
                if (!TerminateThread(hNative, 1))
                {
                    crWarning("TerminateThread failed");
                }
            }
        }

        if (hNative)
        {
            CloseHandle(hNative);
        }
    }
    CloseHandle(render_spu.hWinThreadReadyEvent);
    render_spu.hWinThreadReadyEvent = NULL;
#endif

    crUnloadOpenGL();

#ifdef CHROMIUM_THREADSAFE
    crFreeTSD(&_RenderTSD);
#endif

    return 1;
}


extern SPUOptions renderSPUOptions[];

int SPULoad( char **name, char **super, SPUInitFuncPtr *init,
         SPUSelfDispatchFuncPtr *self, SPUCleanupFuncPtr *cleanup,
         SPUOptionsPtr *options, int *flags )
{
    *name = "render";
    *super = NULL;
    *init = renderSPUInit;
    *self = renderSPUSelfDispatch;
    *cleanup = renderSPUCleanup;
    *options = renderSPUOptions;
    *flags = (SPU_NO_PACKER|SPU_IS_TERMINAL|SPU_MAX_SERVERS_ZERO);

    return 1;
}

DECLEXPORT(void) renderspuSetWindowId(uint64_t winId)
{
    render_spu_parent_window_id = winId;
    crDebug("Set new parent window %p (no actual reparent performed)", winId);
}
