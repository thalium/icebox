/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "cr_net.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_net.h"
#include "cr_environment.h"
#include "cr_process.h"
#include "cr_rand.h"
#include "cr_netserver.h"
#include "stub.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <iprt/initterm.h>
#include <iprt/thread.h>
#include <iprt/err.h>
#include <iprt/asm.h>
#ifndef WINDOWS
# include <sys/types.h>
# include <unistd.h>
#endif

#ifdef VBOX_WITH_WDDM
#include <d3d9types.h>
#include <D3dumddi.h>
#endif

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
# include <VBoxCrHgsmi.h>
#endif

/**
 * If you change this, see the comments in tilesortspu_context.c
 */
#define MAGIC_CONTEXT_BASE 500

#define CONFIG_LOOKUP_FILE ".crconfigs"

#ifdef WINDOWS
#define PYTHON_EXE "python.exe"
#else
#define PYTHON_EXE "python"
#endif

static bool stub_initialized = 0;
#ifdef WINDOWS
static CRmutex stub_init_mutex;
#define STUB_INIT_LOCK() do { crLockMutex(&stub_init_mutex); } while (0)
#define STUB_INIT_UNLOCK() do { crUnlockMutex(&stub_init_mutex); } while (0)
#else
#define STUB_INIT_LOCK() do { } while (0)
#define STUB_INIT_UNLOCK() do { } while (0)
#endif

/* NOTE: 'SPUDispatchTable glim' is declared in NULLfuncs.py now */
/* NOTE: 'SPUDispatchTable stubThreadsafeDispatch' is declared in tsfuncs.c */
Stub stub;
#ifdef CHROMIUM_THREADSAFE
static bool g_stubIsCurrentContextTSDInited;
CRtsd g_stubCurrentContextTSD;
#endif


#ifndef VBOX_NO_NATIVEGL
static void stubInitNativeDispatch( void )
{
# define MAX_FUNCS 1000
    SPUNamedFunctionTable gl_funcs[MAX_FUNCS];
    int numFuncs;

    numFuncs = crLoadOpenGL( &stub.wsInterface, gl_funcs );

    stub.haveNativeOpenGL = (numFuncs > 0);

    /* XXX call this after context binding */
    numFuncs += crLoadOpenGLExtensions( &stub.wsInterface, gl_funcs + numFuncs );

    CRASSERT(numFuncs < MAX_FUNCS);

    crSPUInitDispatchTable( &stub.nativeDispatch );
    crSPUInitDispatch( &stub.nativeDispatch, gl_funcs );
    crSPUInitDispatchNops( &stub.nativeDispatch );
# undef MAX_FUNCS
}
#endif /* !VBOX_NO_NATIVEGL */


/** Pointer to the SPU's real glClear and glViewport functions */
static ClearFunc_t origClear;
static ViewportFunc_t origViewport;
static SwapBuffersFunc_t origSwapBuffers;
static DrawBufferFunc_t origDrawBuffer;
static ScissorFunc_t origScissor;

static void stubCheckWindowState(WindowInfo *window, GLboolean bFlushOnChange)
{
    bool bForceUpdate = false;
    bool bChanged = false;

#ifdef WINDOWS
    /* @todo install hook and track for WM_DISPLAYCHANGE */
    {
        DEVMODE devMode;

        devMode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);

        if (devMode.dmPelsWidth!=window->dmPelsWidth || devMode.dmPelsHeight!=window->dmPelsHeight)
        {
            crDebug("Resolution changed(%d,%d), forcing window Pos/Size update", devMode.dmPelsWidth, devMode.dmPelsHeight);
            window->dmPelsWidth = devMode.dmPelsWidth;
            window->dmPelsHeight = devMode.dmPelsHeight;
            bForceUpdate = true;
        }
    }
#endif

    bChanged = stubUpdateWindowGeometry(window, bForceUpdate) || bForceUpdate;

#if defined(GLX) || defined (WINDOWS)
    if (stub.trackWindowVisibleRgn)
    {
        bChanged = stubUpdateWindowVisibileRegions(window) || bChanged;
    }
#endif

    if (stub.trackWindowVisibility && window->type == CHROMIUM && window->drawable) {
        const int mapped = stubIsWindowVisible(window);
        if (mapped != window->mapped) {
            crDebug("Dispatched: WindowShow(%i, %i)", window->spuWindow, mapped);
            stub.spu->dispatch_table.WindowShow(window->spuWindow, mapped);
            window->mapped = mapped;
            bChanged = true;
        }
    }

    if (bFlushOnChange && bChanged)
    {
        stub.spu->dispatch_table.Flush();
    }
}

static bool stubSystemWindowExist(WindowInfo *pWindow)
{
#ifdef WINDOWS
    if (pWindow->hWnd!=WindowFromDC(pWindow->drawable))
    {
        return false;
    }
#else
    Window root;
    int x, y;
    unsigned int border, depth, w, h;
    Display *dpy;

    dpy = stubGetWindowDisplay(pWindow);

    XLOCK(dpy);
    if (!XGetGeometry(dpy, pWindow->drawable, &root, &x, &y, &w, &h, &border, &depth))
    {
        XUNLOCK(dpy);
        return false;
    }
    XUNLOCK(dpy);
#endif

    return true;
}

static void stubCheckWindowsCB(unsigned long key, void *data1, void *data2)
{
    WindowInfo *pWindow = (WindowInfo *) data1;
    ContextInfo *pCtx = (ContextInfo *) data2;
    (void)key;

    if (pWindow == pCtx->currentDrawable
        || pWindow->type!=CHROMIUM
        || pWindow->pOwner!=pCtx)
    {
        return;
    }

    if (!stubSystemWindowExist(pWindow))
    {
#ifdef WINDOWS
        stubDestroyWindow(CR_CTX_CON(pCtx), (GLint)pWindow->hWnd);
#else
        stubDestroyWindow(CR_CTX_CON(pCtx), (GLint)pWindow->drawable);
#endif
        return;
    }

    stubCheckWindowState(pWindow, GL_FALSE);
}

static void stubCheckWindowsState(void)
{
    ContextInfo *context = stubGetCurrentContext();

    CRASSERT(stub.trackWindowSize || stub.trackWindowPos);

    if (!context)
        return;

#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    if (stub.bRunningUnderWDDM)
        return;
#endif

    /* Try to keep a consistent locking order. */
    crHashtableLock(stub.windowTable);
#if defined(CR_NEWWINTRACK) && !defined(WINDOWS)
    crLockMutex(&stub.mutex);
#endif

    stubCheckWindowState(context->currentDrawable, GL_TRUE);
    crHashtableWalkUnlocked(stub.windowTable, stubCheckWindowsCB, context);

#if defined(CR_NEWWINTRACK) && !defined(WINDOWS)
    crUnlockMutex(&stub.mutex);
#endif
    crHashtableUnlock(stub.windowTable);
}


/**
 * Override the head SPU's glClear function.
 * We're basically trapping this function so that we can poll the
 * application window size at a regular interval.
 */
static void SPU_APIENTRY trapClear(GLbitfield mask)
{
    stubCheckWindowsState();
    /* call the original SPU glClear function */
    origClear(mask);
}

/**
 * As above, but for glViewport.  Most apps call glViewport before
 * glClear when a window is resized.
 */
static void SPU_APIENTRY trapViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    stubCheckWindowsState();
    /* call the original SPU glViewport function */
    origViewport(x, y, w, h);
}

/*static void SPU_APIENTRY trapSwapBuffers(GLint window, GLint flags)
{
    stubCheckWindowsState();
    origSwapBuffers(window, flags);
}

static void SPU_APIENTRY trapDrawBuffer(GLenum buf)
{
    stubCheckWindowsState();
    origDrawBuffer(buf);
}*/

#if 0 /* unused */
static void SPU_APIENTRY trapScissor(GLint x, GLint y, GLsizei w, GLsizei h)
{
    int winX, winY;
    unsigned int winW, winH;
    WindowInfo *pWindow;
    ContextInfo *context = stubGetCurrentContext();
    (void)x; (void)y; (void)w; (void)h;

    pWindow = context->currentDrawable;
    stubGetWindowGeometry(pWindow, &winX, &winY, &winW, &winH);
    origScissor(0, 0, winW, winH);
}
#endif /* unused */

/**
 * Use the GL function pointers in \<spu\> to initialize the static glim
 * dispatch table.
 */
static void stubInitSPUDispatch(SPU *spu)
{
    crSPUInitDispatchTable( &stub.spuDispatch );
    crSPUCopyDispatchTable( &stub.spuDispatch, &(spu->dispatch_table) );

    if (stub.trackWindowSize || stub.trackWindowPos || stub.trackWindowVisibleRgn) {
        /* patch-in special glClear/Viewport function to track window sizing */
        origClear = stub.spuDispatch.Clear;
        origViewport = stub.spuDispatch.Viewport;
        origSwapBuffers = stub.spuDispatch.SwapBuffers;
        origDrawBuffer = stub.spuDispatch.DrawBuffer;
        origScissor = stub.spuDispatch.Scissor;
        stub.spuDispatch.Clear = trapClear;
        stub.spuDispatch.Viewport = trapViewport;

        /*stub.spuDispatch.SwapBuffers = trapSwapBuffers;
        stub.spuDispatch.DrawBuffer = trapDrawBuffer;*/
    }

    crSPUCopyDispatchTable( &glim, &stub.spuDispatch );
}

#if 0 /** @todo stubSPUTearDown & stubSPUTearDownLocked are not referenced */

// Callback function, used to destroy all created contexts
static void hsWalkStubDestroyContexts(unsigned long key, void *data1, void *data2)
{
    (void)data1; (void)data2;
    stubDestroyContext(key);
}

/**
 * This is called when we exit.
 * We call all the SPU's cleanup functions.
 */
static void stubSPUTearDownLocked(void)
{
    crDebug("stubSPUTearDownLocked");

#ifdef WINDOWS
# ifndef CR_NEWWINTRACK
    stubUninstallWindowMessageHook();
# endif
#endif

#ifdef CR_NEWWINTRACK
    ASMAtomicWriteBool(&stub.bShutdownSyncThread, true);
#endif

    //delete all created contexts
    stubMakeCurrent( NULL, NULL);

    /* the lock order is windowTable->contextTable (see wglMakeCurrent_prox, glXMakeCurrent)
     * this is why we need to take a windowTable lock since we will later do stub.windowTable access & locking */
    crHashtableLock(stub.windowTable);
    crHashtableWalk(stub.contextTable, hsWalkStubDestroyContexts, NULL);
    crHashtableUnlock(stub.windowTable);

    /* shutdown, now trap any calls to a NULL dispatcher */
    crSPUCopyDispatchTable(&glim, &stubNULLDispatch);

    crSPUUnloadChain(stub.spu);
    stub.spu = NULL;

#ifndef Linux
    crUnloadOpenGL();
#endif

#ifndef WINDOWS
    crNetTearDown();
#endif

#ifdef GLX
    if (stub.xshmSI.shmid>=0)
    {
        shmctl(stub.xshmSI.shmid, IPC_RMID, 0);
        shmdt(stub.xshmSI.shmaddr);
    }
    crFreeHashtable(stub.pGLXPixmapsHash, crFree);
#endif

    crFreeHashtable(stub.windowTable, crFree);
    crFreeHashtable(stub.contextTable, NULL);

    crMemset(&stub, 0, sizeof(stub));

}

/**
 * This is called when we exit.
 * We call all the SPU's cleanup functions.
 */
static void stubSPUTearDown(void)
{
    STUB_INIT_LOCK();
    if (stub_initialized)
    {
        stubSPUTearDownLocked();
        stub_initialized = 0;
    }
    STUB_INIT_UNLOCK();
}

#endif /** @todo stubSPUTearDown & stubSPUTearDownLocked are not referenced */

static void stubSPUSafeTearDown(void)
{
#ifdef CHROMIUM_THREADSAFE
    CRmutex *mutex;
#endif

    if (!stub_initialized) return;
    stub_initialized = 0;

#ifdef CHROMIUM_THREADSAFE
    mutex = &stub.mutex;
    crLockMutex(mutex);
#endif
    crDebug("stubSPUSafeTearDown");

#ifdef WINDOWS
# ifndef CR_NEWWINTRACK
    stubUninstallWindowMessageHook();
# endif
#endif

#if defined(CR_NEWWINTRACK)
    crUnlockMutex(mutex);
# if defined(WINDOWS)
    if (stub.hSyncThread && RTThreadGetState(stub.hSyncThread)!=RTTHREADSTATE_TERMINATED)
    {
        HANDLE hNative;
        DWORD ec=0;

        hNative = OpenThread(SYNCHRONIZE|THREAD_QUERY_INFORMATION|THREAD_TERMINATE,
                             false, RTThreadGetNative(stub.hSyncThread));
        if (!hNative)
        {
            crWarning("Failed to get handle for sync thread(%#x)", GetLastError());
        }
        else
        {
            crDebug("Got handle %p for thread %#x", hNative, RTThreadGetNative(stub.hSyncThread));
        }

        ASMAtomicWriteBool(&stub.bShutdownSyncThread, true);

        if (PostThreadMessage(RTThreadGetNative(stub.hSyncThread), WM_QUIT, 0, 0))
        {
            RTThreadWait(stub.hSyncThread, 1000, NULL);

            /*Same issue as on linux, RTThreadWait exits before system thread is terminated, which leads
             * to issues as our dll goes to be unloaded.
             *@todo
             *We usually call this function from DllMain which seems to be holding some lock and thus we have to
             * kill thread via TerminateThread.
             */
            if (WaitForSingleObject(hNative, 100)==WAIT_TIMEOUT)
            {
                crDebug("Wait failed, terminating");
                if (!TerminateThread(hNative, 1))
                {
                    crDebug("TerminateThread failed");
                }
            }
            if (GetExitCodeThread(hNative, &ec))
            {
                crDebug("Thread %p exited with ec=%i", hNative, ec);
            }
            else
            {
                crDebug("GetExitCodeThread failed(%#x)", GetLastError());
            }
        }
        else
        {
            crDebug("Sync thread killed before DLL_PROCESS_DETACH");
        }

        if (hNative)
        {
            CloseHandle(hNative);
        }
    }
#else
    if (stub.hSyncThread!=NIL_RTTHREAD)
    {
        ASMAtomicWriteBool(&stub.bShutdownSyncThread, true);
        {
            int rc = RTThreadWait(stub.hSyncThread, RT_INDEFINITE_WAIT, NULL);
            if (RT_FAILURE(rc))
            {
                WARN(("RTThreadWait_join failed %i", rc));
            }
        }
    }
#endif
    crLockMutex(mutex);
#endif

#ifndef WINDOWS
    crNetTearDown();
#endif

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(mutex);
    crFreeMutex(mutex);
#endif
    crMemset(&stub, 0, sizeof(stub));
}


static void stubExitHandler(void)
{
    stubSPUSafeTearDown();
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
}

/**
 * Called when we receive a SIGTERM signal.
 */
static void stubSignalHandler(int signo)
{
    (void)signo;
    stubSPUSafeTearDown();
    exit(0);  /* this causes stubExitHandler() to be called */
}

#ifndef RT_OS_WINDOWS
# ifdef CHROMIUM_THREADSAFE
static void stubThreadTlsDtor(void *pvValue)
{
    ContextInfo *pCtx = (ContextInfo*)pvValue;
    VBoxTlsRefRelease(pCtx);
}
# endif
#endif


/**
 * Init variables in the stub structure, install signal handler.
 */
static void stubInitVars(void)
{
    WindowInfo *defaultWin;

#ifdef CHROMIUM_THREADSAFE
    crInitMutex(&stub.mutex);
#endif

    /* At the very least we want CR_RGB_BIT. */
    stub.haveNativeOpenGL = GL_FALSE;
    stub.spu = NULL;
    stub.appDrawCursor = 0;
    stub.minChromiumWindowWidth = 0;
    stub.minChromiumWindowHeight = 0;
    stub.maxChromiumWindowWidth = 0;
    stub.maxChromiumWindowHeight = 0;
    stub.matchChromiumWindowCount = 0;
    stub.matchChromiumWindowID = NULL;
    stub.matchWindowTitle = NULL;
    stub.ignoreFreeglutMenus = 0;
    stub.threadSafe = GL_FALSE;
    stub.trackWindowSize = 0;
    stub.trackWindowPos = 0;
    stub.trackWindowVisibility = 0;
    stub.trackWindowVisibleRgn = 0;
    stub.mothershipPID = 0;
    stub.spu_dir = NULL;

    stub.freeContextNumber = MAGIC_CONTEXT_BASE;
    stub.contextTable = crAllocHashtable();
#ifndef RT_OS_WINDOWS
# ifdef CHROMIUM_THREADSAFE
    if (!g_stubIsCurrentContextTSDInited)
    {
        crInitTSDF(&g_stubCurrentContextTSD, stubThreadTlsDtor);
        g_stubIsCurrentContextTSDInited = true;
    }
# endif
#endif
    stubSetCurrentContext(NULL);

    stub.windowTable = crAllocHashtable();

#ifdef CR_NEWWINTRACK
    stub.bShutdownSyncThread = false;
    stub.hSyncThread = NIL_RTTHREAD;
#endif

    defaultWin = (WindowInfo *) crCalloc(sizeof(WindowInfo));
    defaultWin->type = CHROMIUM;
    defaultWin->spuWindow = 0;  /* window 0 always exists */
#ifdef WINDOWS
    defaultWin->hVisibleRegion = INVALID_HANDLE_VALUE;
#elif defined(GLX)
    defaultWin->pVisibleRegions = NULL;
    defaultWin->cVisibleRegions = 0;
#endif
    crHashtableAdd(stub.windowTable, 0, defaultWin);

#if 1
    atexit(stubExitHandler);
    signal(SIGTERM, stubSignalHandler);
    signal(SIGINT, stubSignalHandler);
#ifndef WINDOWS
    signal(SIGPIPE, SIG_IGN); /* the networking code should catch this */
#endif
#else
    (void) stubExitHandler;
    (void) stubSignalHandler;
#endif
}


#if 0 /* unused */

/**
 * Return a free port number for the mothership to use, or -1 if we
 * can't find one.
 */
static int
GenerateMothershipPort(void)
{
    const int MAX_PORT = 10100;
    unsigned short port;

    /* generate initial port number randomly */
    crRandAutoSeed();
    port = (unsigned short) crRandInt(10001, MAX_PORT);

#ifdef WINDOWS
    /* XXX should implement a free port check here */
    return port;
#else
    /*
     * See if this port number really is free, try another if needed.
     */
    {
        struct sockaddr_in servaddr;
        int so_reuseaddr = 1;
        int sock, k;

        /* create socket */
        sock = socket(AF_INET, SOCK_STREAM, 0);
        CRASSERT(sock > 2);

        /* deallocate socket/port when we exit */
        k = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                                     (char *) &so_reuseaddr, sizeof(so_reuseaddr));
        CRASSERT(k == 0);

        /* initialize the servaddr struct */
        crMemset(&servaddr, 0, sizeof(servaddr) );
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        while (port < MAX_PORT) {
            /* Bind to the given port number, return -1 if we fail */
            servaddr.sin_port = htons((unsigned short) port);
            k = bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));
            if (k) {
                /* failed to create port. try next one. */
                port++;
            }
            else {
                /* free the socket/port now so mothership can make it */
                close(sock);
                return port;
            }
        }
    }
#endif /* WINDOWS */
    return -1;
}


/**
 * Try to determine which mothership configuration to use for this program.
 */
static char **
LookupMothershipConfig(const char *procName)
{
    const int procNameLen = crStrlen(procName);
    FILE *f;
    const char *home;
    char configPath[1000];

    /* first, check if the CR_CONFIG env var is set */
    {
        const char *conf = crGetenv("CR_CONFIG");
        if (conf && crStrlen(conf) > 0)
            return crStrSplit(conf, " ");
    }

    /* second, look up config name from config file */
    home = crGetenv("HOME");
    if (home)
        sprintf(configPath, "%s/%s", home, CONFIG_LOOKUP_FILE);
    else
        crStrcpy(configPath, CONFIG_LOOKUP_FILE); /* from current dir */
    /* Check if the CR_CONFIG_PATH env var is set. */
    {
        const char *conf = crGetenv("CR_CONFIG_PATH");
        if (conf)
            crStrcpy(configPath, conf); /* from env var */
    }

    f = fopen(configPath, "r");
    if (!f) {
        return NULL;
    }

    while (!feof(f)) {
        char line[1000];
        char **args;
        fgets(line, 999, f);
        line[crStrlen(line) - 1] = 0; /* remove trailing newline */
        if (crStrncmp(line, procName, procNameLen) == 0 &&
            (line[procNameLen] == ' ' || line[procNameLen] == '\t'))
        {
            crWarning("Using Chromium configuration for %s from %s",
                                procName, configPath);
            args = crStrSplit(line + procNameLen + 1, " ");
            return args;
        }
    }
    fclose(f);
    return NULL;
}


static int Mothership_Awake = 0;


/**
 * Signal handler to determine when mothership is ready.
 */
static void
MothershipPhoneHome(int signo)
{
    crDebug("Got signal %d: mothership is awake!", signo);
    Mothership_Awake = 1;
}

#endif /* 0 */

static void stubSetDefaultConfigurationOptions(void)
{
    unsigned char key[16]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    stub.appDrawCursor = 0;
    stub.minChromiumWindowWidth = 0;
    stub.minChromiumWindowHeight = 0;
    stub.maxChromiumWindowWidth = 0;
    stub.maxChromiumWindowHeight = 0;
    stub.matchChromiumWindowID = NULL;
    stub.numIgnoreWindowID = 0;
    stub.matchWindowTitle = NULL;
    stub.ignoreFreeglutMenus = 0;
    stub.trackWindowSize = 1;
    stub.trackWindowPos = 1;
    stub.trackWindowVisibility = 1;
    stub.trackWindowVisibleRgn = 1;
    stub.matchChromiumWindowCount = 0;
    stub.spu_dir = NULL;
    crNetSetRank(0);
    crNetSetContextRange(32, 35);
    crNetSetNodeRange("iam0", "iamvis20");
    crNetSetKey(key,sizeof(key));
    stub.force_pbuffers = 0;

#ifdef WINDOWS
# ifdef VBOX_WITH_WDDM
    stub.bRunningUnderWDDM = false;
# endif
#endif
}

#ifdef CR_NEWWINTRACK
# ifdef VBOX_WITH_WDDM
static void stubDispatchVisibleRegions(WindowInfo *pWindow)
{
    DWORD dwCount;
    LPRGNDATA lpRgnData;

    dwCount = GetRegionData(pWindow->hVisibleRegion, 0, NULL);
    lpRgnData = crAlloc(dwCount);

    if (lpRgnData)
    {
        GetRegionData(pWindow->hVisibleRegion, dwCount, lpRgnData);
        crDebug("Dispatched WindowVisibleRegion (%i, cRects=%i)", pWindow->spuWindow, lpRgnData->rdh.nCount);
        stub.spuDispatch.WindowVisibleRegion(pWindow->spuWindow, lpRgnData->rdh.nCount, (GLint*) lpRgnData->Buffer);
        crFree(lpRgnData);
    }
    else crWarning("GetRegionData failed, VisibleRegions update failed");
}

# endif /* VBOX_WITH_WDDM */

static void stubSyncTrCheckWindowsCB(unsigned long key, void *data1, void *data2)
{
    WindowInfo *pWindow = (WindowInfo *) data1;
    (void)key; (void) data2;

    if (pWindow->type!=CHROMIUM || pWindow->spuWindow==0)
    {
        return;
    }

    stub.spu->dispatch_table.VBoxPackSetInjectID(pWindow->u32ClientID);

    if (!stubSystemWindowExist(pWindow))
    {
#ifdef WINDOWS
        stubDestroyWindow(0, (GLint)pWindow->hWnd);
#else
        stubDestroyWindow(0, (GLint)pWindow->drawable);
#endif
        /*No need to flush here as crWindowDestroy does it*/
        return;
    }

#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    if (stub.bRunningUnderWDDM)
        return;
#endif
    stubCheckWindowState(pWindow, GL_TRUE);
}

static DECLCALLBACK(int) stubSyncThreadProc(RTTHREAD ThreadSelf, void *pvUser)
{
#ifdef WINDOWS
    MSG msg;
# ifdef VBOX_WITH_WDDM
    HMODULE hVBoxD3D = NULL;
    GLint spuConnection = 0;
# endif
#endif

    (void) pvUser;

    crDebug("Sync thread started");
#ifdef WINDOWS
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
# ifdef VBOX_WITH_WDDM
    hVBoxD3D = NULL;
    if (!GetModuleHandleEx(0, VBOX_MODNAME_DISPD3D, &hVBoxD3D))
    {
        crDebug("GetModuleHandleEx failed err %d", GetLastError());
        hVBoxD3D = NULL;
    }

    if (hVBoxD3D)
    {
                    crDebug("running with " VBOX_MODNAME_DISPD3D);
                    stub.trackWindowVisibleRgn = 0;
                    stub.bRunningUnderWDDM = true;
    }
# endif /* VBOX_WITH_WDDM */
#endif /* WINDOWS */

    crLockMutex(&stub.mutex);
#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    spuConnection =
#endif
            stub.spu->dispatch_table.VBoxPackSetInjectThread(NULL);
#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    if (stub.bRunningUnderWDDM && !spuConnection)
    {
        crError("VBoxPackSetInjectThread failed!");
    }
#endif
    crUnlockMutex(&stub.mutex);

    RTThreadUserSignal(ThreadSelf);

    while(!stub.bShutdownSyncThread)
    {
#ifdef WINDOWS
        if (!PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
# ifdef VBOX_WITH_WDDM
            if (stub.bRunningUnderWDDM)
            {

            }
            else
# endif
            {
                crHashtableWalk(stub.windowTable, stubSyncTrCheckWindowsCB, NULL);
                RTThreadSleep(50);
            }
        }
        else
        {
            if (WM_QUIT==msg.message)
            {
                crDebug("Sync thread got WM_QUIT");
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
#else
        /* Try to keep a consistent locking order. */
        crHashtableLock(stub.windowTable);
        crLockMutex(&stub.mutex);
        crHashtableWalkUnlocked(stub.windowTable, stubSyncTrCheckWindowsCB, NULL);
        crUnlockMutex(&stub.mutex);
        crHashtableUnlock(stub.windowTable);
        RTThreadSleep(50);
#endif
    }

#ifdef VBOX_WITH_WDDM
    if (spuConnection)
    {
        stub.spu->dispatch_table.VBoxConDestroy(spuConnection);
    }
    if (hVBoxD3D)
    {
        FreeLibrary(hVBoxD3D);
    }
#endif
    crDebug("Sync thread stopped");
    return 0;
}
#endif /* CR_NEWWINTRACK */

/**
 * Do one-time initializations for the faker.
 * Returns TRUE on success, FALSE otherwise.
 */
static bool
stubInitLocked(void)
{
    /* Here is where we contact the mothership to find out what we're supposed
     * to  be doing.  Networking code in a DLL initializer.  I sure hope this
     * works :)
     *
     * HOW can I pass the mothership address to this if I already know it?
     */

    char response[1024];
    char **spuchain;
    int num_spus;
    int *spu_ids;
    char **spu_names;
    const char *app_id;
    int i;
    int disable_sync = 0;
#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    HMODULE hVBoxD3D = NULL;
#endif

    stubInitVars();

    crGetProcName(response, 1024);
    crDebug("Stub launched for %s", response);

#if defined(CR_NEWWINTRACK) && !defined(WINDOWS)
    /*@todo when vm boots with compiz turned on, new code causes hang in xcb_wait_for_reply in the sync thread
     * as at the start compiz runs our code under XGrabServer.
     */
    if (!crStrcmp(response, "compiz") || !crStrcmp(response, "compiz_real") || !crStrcmp(response, "compiz.real")
	|| !crStrcmp(response, "compiz-bin"))
    {
        disable_sync = 1;
    }
#endif

    /* @todo check if it'd be of any use on other than guests, no use for windows */
    app_id = crGetenv( "CR_APPLICATION_ID_NUMBER" );

    crNetInit( NULL, NULL );

#ifndef WINDOWS
    {
        CRNetServer ns;

        ns.name = "vboxhgcm://host:0";
        ns.buffer_size = 1024;
        crNetServerConnect(&ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , NULL
#endif
                );
        if (!ns.conn)
        {
            crWarning("Failed to connect to host. Make sure 3D acceleration is enabled for this VM.");
# ifdef VBOXOGL_FAKEDRI
            return false;
# else
            exit(1);
# endif
        }
        else
        {
            crNetFreeConnection(ns.conn);
        }
    }
#endif

    strcpy(response, "2 0 feedback 1 pack");
    spuchain = crStrSplit( response, " " );
    num_spus = crStrToInt( spuchain[0] );
    spu_ids = (int *) crAlloc( num_spus * sizeof( *spu_ids ) );
    spu_names = (char **) crAlloc( num_spus * sizeof( *spu_names ) );
    for (i = 0 ; i < num_spus ; i++)
    {
        spu_ids[i] = crStrToInt( spuchain[2*i+1] );
        spu_names[i] = crStrdup( spuchain[2*i+2] );
        crDebug( "SPU %d/%d: (%d) \"%s\"", i+1, num_spus, spu_ids[i], spu_names[i] );
    }

    stubSetDefaultConfigurationOptions();

#if defined(WINDOWS) && defined(VBOX_WITH_WDDM)
    hVBoxD3D = NULL;
    if (!GetModuleHandleEx(0, VBOX_MODNAME_DISPD3D, &hVBoxD3D))
    {
        crDebug("GetModuleHandleEx failed err %d", GetLastError());
        hVBoxD3D = NULL;
    }

    if (hVBoxD3D)
    {
        disable_sync = 1;
        crDebug("running with %s", VBOX_MODNAME_DISPD3D);
        stub.trackWindowVisibleRgn = 0;
        /* @todo: should we enable that? */
        stub.trackWindowSize = 0;
        stub.trackWindowPos = 0;
        stub.trackWindowVisibility = 0;
        stub.bRunningUnderWDDM = true;
    }
#endif

    stub.spu = crSPULoadChain( num_spus, spu_ids, spu_names, stub.spu_dir, NULL );

    crFree( spuchain );
    crFree( spu_ids );
    for (i = 0; i < num_spus; ++i)
        crFree(spu_names[i]);
    crFree( spu_names );

    // spu chain load failed somewhere
    if (!stub.spu) {
        return false;
    }

    crSPUInitDispatchTable( &glim );

    /* This is unlikely to change -- We still want to initialize our dispatch
     * table with the functions of the first SPU in the chain. */
    stubInitSPUDispatch( stub.spu );

    /* we need to plug one special stub function into the dispatch table */
    glim.GetChromiumParametervCR = stub_GetChromiumParametervCR;

#if !defined(VBOX_NO_NATIVEGL)
    /* Load pointers to native OpenGL functions into stub.nativeDispatch */
    stubInitNativeDispatch();
#endif

/*crDebug("stub init");
raise(SIGINT);*/

#ifdef WINDOWS
# ifndef CR_NEWWINTRACK
    stubInstallWindowMessageHook();
# endif
#endif

#ifdef CR_NEWWINTRACK
    {
        int rc;

        RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);

        if (!disable_sync)
        {
            crDebug("Starting sync thread");

            rc = RTThreadCreate(&stub.hSyncThread, stubSyncThreadProc, NULL, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "Sync");
            if (RT_FAILURE(rc))
            {
                crError("Failed to start sync thread! (%x)", rc);
            }
            RTThreadUserWait(stub.hSyncThread, 60 * 1000);
            RTThreadUserReset(stub.hSyncThread);

            crDebug("Going on");
        }
    }
#endif

#ifdef GLX
    stub.xshmSI.shmid = -1;
    stub.bShmInitFailed = GL_FALSE;
    stub.pGLXPixmapsHash = crAllocHashtable();

    stub.bXExtensionsChecked = GL_FALSE;
    stub.bHaveXComposite = GL_FALSE;
    stub.bHaveXFixes = GL_FALSE;
#endif

    return true;
}

/**
 * Do one-time initializations for the faker.
 * Returns TRUE on success, FALSE otherwise.
 */
bool
stubInit(void)
{
    bool bRc = true;
    /* we need to serialize the initialization, otherwise racing is possible
     * for XPDM-based d3d when a d3d switcher is testing the gl lib in two or more threads
     * NOTE: the STUB_INIT_LOCK/UNLOCK is a NOP for non-win currently */
    STUB_INIT_LOCK();
    if (!stub_initialized)
        bRc = stub_initialized = stubInitLocked();
    STUB_INIT_UNLOCK();
    return bRc;
}

/* Sigh -- we can't do initialization at load time, since Windows forbids
 * the loading of other libraries from DLLMain. */

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if 1//def DEBUG_misha
 /* debugging: this is to be able to catch first-chance notifications
  * for exceptions other than EXCEPTION_BREAKPOINT in kernel debugger */
# define VDBG_VEHANDLER
#endif

#ifdef VDBG_VEHANDLER
# include <dbghelp.h>
# include <cr_string.h>
static PVOID g_VBoxVehHandler = NULL;
static DWORD g_VBoxVehEnable = 0;

/* generate a crash dump on exception */
#define VBOXVEH_F_DUMP 0x00000001
/* generate a debugger breakpoint exception */
#define VBOXVEH_F_BREAK 0x00000002
/* exit on exception */
#define VBOXVEH_F_EXIT  0x00000004

static DWORD g_VBoxVehFlags = 0;

typedef BOOL WINAPI FNVBOXDBG_MINIDUMPWRITEDUMP(HANDLE hProcess,
            DWORD ProcessId,
            HANDLE hFile,
            MINIDUMP_TYPE DumpType,
            PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
            PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
            PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
typedef FNVBOXDBG_MINIDUMPWRITEDUMP *PFNVBOXDBG_MINIDUMPWRITEDUMP;

static HMODULE g_hVBoxMdDbgHelp = NULL;
static PFNVBOXDBG_MINIDUMPWRITEDUMP g_pfnVBoxMdMiniDumpWriteDump = NULL;
static size_t g_cVBoxMdFilePrefixLen = 0;
static WCHAR g_aszwVBoxMdFilePrefix[MAX_PATH];
static WCHAR g_aszwVBoxMdDumpCount = 0;
static MINIDUMP_TYPE g_enmVBoxMdDumpType = MiniDumpNormal
        | MiniDumpWithDataSegs
        | MiniDumpWithFullMemory
        | MiniDumpWithHandleData
////        | MiniDumpFilterMemory
////        | MiniDumpScanMemory
//        | MiniDumpWithUnloadedModules
////        | MiniDumpWithIndirectlyReferencedMemory
////        | MiniDumpFilterModulePaths
//        | MiniDumpWithProcessThreadData
//        | MiniDumpWithPrivateReadWriteMemory
////        | MiniDumpWithoutOptionalData
//        | MiniDumpWithFullMemoryInfo
//        | MiniDumpWithThreadInfo
//        | MiniDumpWithCodeSegs
//        | MiniDumpWithFullAuxiliaryState
//        | MiniDumpWithPrivateWriteCopyMemory
//        | MiniDumpIgnoreInaccessibleMemory
//        | MiniDumpWithTokenInformation
////        | MiniDumpWithModuleHeaders
////        | MiniDumpFilterTriage
        ;



#define VBOXMD_DUMP_DIR_DEFAULT "C:\\dumps"
#define VBOXMD_DUMP_NAME_PREFIX_W L"VBoxDmp_"

static HMODULE loadSystemDll(const char *pszName)
{
#ifndef DEBUG
    char   szPath[MAX_PATH];
    UINT   cchPath = GetSystemDirectoryA(szPath, sizeof(szPath));
    size_t cbName  = strlen(pszName) + 1;
    if (cchPath + 1 + cbName > sizeof(szPath))
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return NULL;
    }
    szPath[cchPath] = '\\';
    memcpy(&szPath[cchPath + 1], pszName, cbName);
    return LoadLibraryA(szPath);
#else
    return LoadLibraryA(pszName);
#endif
}

static DWORD vboxMdMinidumpCreate(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    WCHAR aszwMdFileName[MAX_PATH];
    HANDLE hProcess = GetCurrentProcess();
    DWORD ProcessId = GetCurrentProcessId();
    MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
    HANDLE hFile;
    DWORD winErr = ERROR_SUCCESS;

    if (!g_pfnVBoxMdMiniDumpWriteDump)
    {
        if (!g_hVBoxMdDbgHelp)
        {
            g_hVBoxMdDbgHelp = loadSystemDll("DbgHelp.dll");
            if (!g_hVBoxMdDbgHelp)
                return GetLastError();
        }

        g_pfnVBoxMdMiniDumpWriteDump = (PFNVBOXDBG_MINIDUMPWRITEDUMP)GetProcAddress(g_hVBoxMdDbgHelp, "MiniDumpWriteDump");
        if (!g_pfnVBoxMdMiniDumpWriteDump)
            return GetLastError();
    }

    ++g_aszwVBoxMdDumpCount;

    memcpy(aszwMdFileName, g_aszwVBoxMdFilePrefix, g_cVBoxMdFilePrefixLen * sizeof (g_aszwVBoxMdFilePrefix[0]));
    swprintf(aszwMdFileName + g_cVBoxMdFilePrefixLen, RT_ELEMENTS(aszwMdFileName) - g_cVBoxMdFilePrefixLen, L"%d_%d.dmp", ProcessId, g_aszwVBoxMdDumpCount);

    hFile = CreateFileW(aszwMdFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    ExceptionInfo.ThreadId = GetCurrentThreadId();
    ExceptionInfo.ExceptionPointers = pExceptionInfo;
    ExceptionInfo.ClientPointers = FALSE;

    if (!g_pfnVBoxMdMiniDumpWriteDump(hProcess, ProcessId, hFile, g_enmVBoxMdDumpType, &ExceptionInfo, NULL, NULL))
        winErr = GetLastError();

    CloseHandle(hFile);
    return winErr;
}

LONG WINAPI vboxVDbgVectoredHandler(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
    PCONTEXT pContextRecord = pExceptionInfo->ContextRecord;
    switch (pExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_INVALID_OPERATION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            if (g_VBoxVehFlags & VBOXVEH_F_BREAK)
            {
                BOOL fBreak = TRUE;
#ifndef DEBUG_misha
                if (pExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT)
                {
                    HANDLE hProcess = GetCurrentProcess();
                    BOOL fDebuggerPresent = FALSE;
                    /* we do not want to generate breakpoint exceptions recursively, so do it only when running under debugger */
                    if (CheckRemoteDebuggerPresent(hProcess, &fDebuggerPresent))
                        fBreak = !!fDebuggerPresent;
                    else
                        fBreak = FALSE; /* <- the function has failed, don't break for sanity */
                }
#endif

                if (fBreak)
                {
                    RT_BREAKPOINT();
                }
            }

            if (g_VBoxVehFlags & VBOXVEH_F_DUMP)
                vboxMdMinidumpCreate(pExceptionInfo);

            if (g_VBoxVehFlags & VBOXVEH_F_EXIT)
                exit(1);
            break;
        default:
            break;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void vboxVDbgVEHandlerRegister()
{
    CRASSERT(!g_VBoxVehHandler);
    g_VBoxVehHandler = AddVectoredExceptionHandler(1,vboxVDbgVectoredHandler);
    CRASSERT(g_VBoxVehHandler);
}

void vboxVDbgVEHandlerUnregister()
{
    ULONG uResult;
    if (g_VBoxVehHandler)
    {
        uResult = RemoveVectoredExceptionHandler(g_VBoxVehHandler);
        CRASSERT(uResult);
        g_VBoxVehHandler = NULL;
    }
}
#endif

/* Windows crap */
BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    (void) lpvReserved;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        CRNetServer ns;
        const char * env;
#if defined(DEBUG_misha)
        HMODULE hCrUtil;
        char aName[MAX_PATH];

        GetModuleFileNameA(hDLLInst, aName, RT_ELEMENTS(aName));
        crDbgCmdSymLoadPrint(aName, hDLLInst);

        hCrUtil = GetModuleHandleA("VBoxOGLcrutil.dll");
        Assert(hCrUtil);
        crDbgCmdSymLoadPrint("VBoxOGLcrutil.dll", hCrUtil);
#endif
#ifdef CHROMIUM_THREADSAFE
        crInitTSD(&g_stubCurrentContextTSD);
#endif

        crInitMutex(&stub_init_mutex);

#ifdef VDBG_VEHANDLER
        env = crGetenv("CR_DBG_VEH_ENABLE");
        g_VBoxVehEnable = crStrParseI32(env,
# ifdef DEBUG_misha
                1
# else
                0
# endif
                );

        if (g_VBoxVehEnable)
        {
            char procName[1024];
            size_t cProcName;
            size_t cChars;

            env = crGetenv("CR_DBG_VEH_FLAGS");
            g_VBoxVehFlags = crStrParseI32(env,
                    0
# ifdef DEBUG_misha
                    | VBOXVEH_F_BREAK
# else
                    | VBOXVEH_F_DUMP
# endif
                    );

            env = crGetenv("CR_DBG_VEH_DUMP_DIR");
            if (!env)
                env = VBOXMD_DUMP_DIR_DEFAULT;

            g_cVBoxMdFilePrefixLen = strlen(env);

            if (RT_ELEMENTS(g_aszwVBoxMdFilePrefix) <= g_cVBoxMdFilePrefixLen + 26 + (sizeof (VBOXMD_DUMP_NAME_PREFIX_W) - sizeof (WCHAR)) / sizeof (WCHAR))
            {
                g_cVBoxMdFilePrefixLen = 0;
                env = "";
            }

            mbstowcs_s(&cChars, g_aszwVBoxMdFilePrefix, g_cVBoxMdFilePrefixLen + 1, env, _TRUNCATE);

            Assert(cChars == g_cVBoxMdFilePrefixLen + 1);

            g_cVBoxMdFilePrefixLen = cChars - 1;

            if (g_cVBoxMdFilePrefixLen && g_aszwVBoxMdFilePrefix[g_cVBoxMdFilePrefixLen - 1] != L'\\')
                g_aszwVBoxMdFilePrefix[g_cVBoxMdFilePrefixLen++] = L'\\';

            memcpy(g_aszwVBoxMdFilePrefix + g_cVBoxMdFilePrefixLen, VBOXMD_DUMP_NAME_PREFIX_W, sizeof (VBOXMD_DUMP_NAME_PREFIX_W) - sizeof (WCHAR));
            g_cVBoxMdFilePrefixLen += (sizeof (VBOXMD_DUMP_NAME_PREFIX_W) - sizeof (WCHAR)) / sizeof (WCHAR);

            crGetProcName(procName, RT_ELEMENTS(procName));
            cProcName = strlen(procName);

            if (RT_ELEMENTS(g_aszwVBoxMdFilePrefix) > g_cVBoxMdFilePrefixLen + cProcName + 1 + 26)
            {
                mbstowcs_s(&cChars, g_aszwVBoxMdFilePrefix + g_cVBoxMdFilePrefixLen, cProcName + 1, procName, _TRUNCATE);
                Assert(cChars == cProcName + 1);
                g_cVBoxMdFilePrefixLen += cChars - 1;
                g_aszwVBoxMdFilePrefix[g_cVBoxMdFilePrefixLen++] = L'_';
            }

            /* sanity */
            g_aszwVBoxMdFilePrefix[g_cVBoxMdFilePrefixLen] = L'\0';

            env = crGetenv("CR_DBG_VEH_DUMP_TYPE");

            g_enmVBoxMdDumpType = crStrParseI32(env,
                    MiniDumpNormal
                    | MiniDumpWithDataSegs
                    | MiniDumpWithFullMemory
                    | MiniDumpWithHandleData
            ////        | MiniDumpFilterMemory
            ////        | MiniDumpScanMemory
            //        | MiniDumpWithUnloadedModules
            ////        | MiniDumpWithIndirectlyReferencedMemory
            ////        | MiniDumpFilterModulePaths
            //        | MiniDumpWithProcessThreadData
            //        | MiniDumpWithPrivateReadWriteMemory
            ////        | MiniDumpWithoutOptionalData
            //        | MiniDumpWithFullMemoryInfo
            //        | MiniDumpWithThreadInfo
            //        | MiniDumpWithCodeSegs
            //        | MiniDumpWithFullAuxiliaryState
            //        | MiniDumpWithPrivateWriteCopyMemory
            //        | MiniDumpIgnoreInaccessibleMemory
            //        | MiniDumpWithTokenInformation
            ////        | MiniDumpWithModuleHeaders
            ////        | MiniDumpFilterTriage
                    );

            vboxVDbgVEHandlerRegister();
        }
#endif

        crNetInit(NULL, NULL);
        ns.name = "vboxhgcm://host:0";
        ns.buffer_size = 1024;
        crNetServerConnect(&ns
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
                , NULL
#endif
);
        if (!ns.conn)
        {
            crDebug("Failed to connect to host (is guest 3d acceleration enabled?), aborting ICD load.");
#ifdef VDBG_VEHANDLER
            if (g_VBoxVehEnable)
                vboxVDbgVEHandlerUnregister();
#endif
            return FALSE;
        }
        else
        {
            crNetFreeConnection(ns.conn);
        }

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        VBoxCrHgsmiInit();
#endif
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        /* do exactly the same thing as for DLL_THREAD_DETACH since
         * DLL_THREAD_DETACH is not called for the thread doing DLL_PROCESS_DETACH according to msdn docs */
        stubSetCurrentContext(NULL);
        if (stub_initialized)
        {
            CRASSERT(stub.spu);
            stub.spu->dispatch_table.VBoxDetachThread();
        }

        
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        VBoxCrHgsmiTerm();
#endif

        stubSPUSafeTearDown();

#ifdef CHROMIUM_THREADSAFE
        crFreeTSD(&g_stubCurrentContextTSD);
#endif

#ifdef VDBG_VEHANDLER
        if (g_VBoxVehEnable)
            vboxVDbgVEHandlerUnregister();
#endif
        break;
    }

    case DLL_THREAD_ATTACH:
    {
        if (stub_initialized)
        {
            CRASSERT(stub.spu);
            stub.spu->dispatch_table.VBoxAttachThread();
        }
        break;
    }

    case DLL_THREAD_DETACH:
    {
        stubSetCurrentContext(NULL);
        if (stub_initialized)
        {
            CRASSERT(stub.spu);
            stub.spu->dispatch_table.VBoxDetachThread();
        }
        break;
    }

    default:
        break;
    }

    return TRUE;
}
#endif
