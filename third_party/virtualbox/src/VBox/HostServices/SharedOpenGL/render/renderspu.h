/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_RENDERSPU_H
#define CR_RENDERSPU_H

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <iprt/win/windows.h>
#define RENDER_APIENTRY __stdcall
#define snprintf _snprintf
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
#  include <AGL/AGL.h>
# else
#  include "renderspu_cocoa_helper.h"
# endif
#define RENDER_APIENTRY
#else
#include <GL/glx.h>
#define RENDER_APIENTRY
#endif
#include "cr_threads.h"
#include "cr_spu.h"
#include "cr_hash.h"
#include "cr_server.h"
#include "cr_blitter.h"
#include "cr_compositor.h"

#include <iprt/cdefs.h>
#include <iprt/critsect.h>
#if defined(GLX) /* @todo: unify windows and glx thread creation code */
#include <iprt/thread.h>
#include <iprt/semaphore.h>

/* special window id used for representing the command window CRWindowInfo */
#define CR_RENDER_WINCMD_ID (INT32_MAX-2)
AssertCompile(CR_RENDER_WINCMD_ID != CR_RENDER_DEFAULT_WINDOW_ID);
/* CRHashTable is using unsigned long keys, we use it to trore X Window -> CRWindowInfo association */
AssertCompile(sizeof (Window) == sizeof (unsigned long));
#endif


#define MAX_VISUALS 32

#ifdef RT_OS_DARWIN
# ifndef VBOX_WITH_COCOA_QT
enum
{
    /* Event classes */
    kEventClassVBox         = 'vbox',
    /* Event kinds */
    kEventVBoxShowWindow    = 'swin',
    kEventVBoxHideWindow    = 'hwin',
    kEventVBoxMoveWindow    = 'mwin',
    kEventVBoxResizeWindow  = 'rwin',
    kEventVBoxDisposeWindow = 'dwin',
    kEventVBoxUpdateDock    = 'udck',
    kEventVBoxUpdateContext = 'uctx',
    kEventVBoxBoundsChanged = 'bchg'
};
pascal OSStatus windowEvtHndlr(EventHandlerCallRef myHandler, EventRef event, void* userData);
# endif
#endif /* RT_OS_DARWIN */

/**
 * Visual info
 */
typedef struct {
    GLbitfield visAttribs;
    const char *displayName;
#if defined(WINDOWS)
//    HDC device_context;
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
    WindowRef window;
# endif
#elif defined(GLX)
    Display *dpy;
    XVisualInfo *visual;
#ifdef GLX_VERSION_1_3
    GLXFBConfig fbconfig;
#endif /* GLX_VERSION_1_3 */
#endif
} VisualInfo;

/**
 * Window info
 */
typedef struct WindowInfo {
    int x, y;
//    int width, height;
//    int id; /**< integer window ID */
    CR_BLITTER_WINDOW BltInfo;

    VisualInfo *visual;

    volatile uint32_t cRefs;

    GLboolean mapPending;
    GLboolean visible;
    GLboolean everCurrent; /**< has this window ever been bound? */
    char *title;

    const VBOXVR_SCR_COMPOSITOR *pCompositor;
    /* the composotor lock is used to synchronize the current compositor access,
     * i.e. the compositor can be accessed by a gui refraw thread,
     * while chromium thread might try to set a new compositor
     * note that the compositor internally has its own lock to be used for accessing its data
     * see CrVrScrCompositorLock/Unlock; renderspu and crserverlib would use it for compositor data access */
    RTCRITSECT CompositorLock;
    PCR_BLITTER pBlitter;
#if defined(WINDOWS)
    HDC nativeWindow; /**< for render_to_app_window */
    HWND hWnd;
    HDC device_context;
    HDC redraw_device_context;
    HRGN hRgn;
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
    WindowRef window;
    WindowRef nativeWindow; /**< for render_to_app_window */
    WindowRef appWindow;
    EventHandlerUPP event_handler;
    GLint bufferName;
    AGLContext dummyContext;
    RgnHandle hVisibleRegion;
    /* unsigned long context_ptr; */
# else
    NativeNSViewRef window;
    NativeNSViewRef nativeWindow; /**< for render_to_app_window */
    NativeNSOpenGLContextRef *currentCtx;
# endif
#elif defined(GLX)
    Window window;
    Window nativeWindow;  /**< for render_to_app_window */
    Window appWindow;     /**< Same as nativeWindow but for garbage collections purposes */
#endif
    int nvSwapGroup;

#ifdef USE_OSMESA
    GLubyte *buffer;    /**< for rendering to off screen buffer.  */
    int in_buffer_width;
    int in_buffer_height;
#endif

} WindowInfo;

/**
 * Context Info
 */
typedef struct _ContextInfo {
//    int id; /**< integer context ID */
    CR_BLITTER_CONTEXT BltInfo;
    VisualInfo *visual;
    GLboolean everCurrent;
    GLboolean haveWindowPosARB;
    WindowInfo *currentWindow;
#if defined(WINDOWS)
    HGLRC hRC;
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
    AGLContext context;
# else
    NativeNSOpenGLContextRef context;
# endif
#elif defined(GLX)
    GLXContext context;
#endif
    struct _ContextInfo *shared;
    char *extensionString;
    volatile uint32_t cRefs;
} ContextInfo;

/**
 * Barrier info
 */
typedef struct {
    CRbarrier barrier;
    GLuint count;
} Barrier;

#ifdef GLX
typedef enum
{
	CR_RENDER_WINCMD_TYPE_UNDEFINED = 0,
	/* create the window (not used for now) */
	CR_RENDER_WINCMD_TYPE_WIN_CREATE,
	/* destroy the window (not used for now) */
	CR_RENDER_WINCMD_TYPE_WIN_DESTROY,
        /* notify the WinCmd thread about window creation */
        CR_RENDER_WINCMD_TYPE_WIN_ON_CREATE,
        /* notify the WinCmd thread about window destroy */
        CR_RENDER_WINCMD_TYPE_WIN_ON_DESTROY,
        /* nop used to synchronize with the WinCmd thread */
        CR_RENDER_WINCMD_TYPE_NOP,
	/* exit Win Cmd thread */
	CR_RENDER_WINCMD_TYPE_EXIT,
} CR_RENDER_WINCMD_TYPE;

typedef struct CR_RENDER_WINCMD
{
    /* command type */
	CR_RENDER_WINCMD_TYPE enmCmd;
	/* command result */
	int rc;
	/* valid for WIN_CREATE & WIN_DESTROY only */
	WindowInfo *pWindow;
} CR_RENDER_WINCMD, *PCR_RENDER_WINCMD;
#endif

#ifdef RT_OS_DARWIN
typedef void (*PFNDELETE_OBJECT)(GLhandleARB obj);
typedef void (*PFNGET_ATTACHED_OBJECTS)( GLhandleARB containerObj, GLsizei maxCount, GLsizei * count, GLhandleARB * obj );
typedef GLhandleARB (*PFNGET_HANDLE)(GLenum pname);
typedef void (*PFNGET_INFO_LOG)( GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog );
typedef void (*PFNGET_OBJECT_PARAMETERFV)( GLhandleARB obj, GLenum pname, GLfloat * params );
typedef void (*PFNGET_OBJECT_PARAMETERIV)( GLhandleARB obj, GLenum pname, GLint * params );
#endif

typedef DECLCALLBACKPTR(void, PFNVCRSERVER_CLIENT_CALLOUT_CB)(void *pvCb);
typedef DECLCALLBACKPTR(void, PFNVCRSERVER_CLIENT_CALLOUT)(PFNVCRSERVER_CLIENT_CALLOUT_CB pfnCb, void*pvCb);


/**
 * Renderspu state info
 */
typedef struct {
    SPUDispatchTable self;
    int id;

    /** config options */
    /*@{*/
    char *window_title;
    int defaultX, defaultY;
    unsigned int defaultWidth, defaultHeight;
    int default_visual;
    int use_L2;
    int fullscreen, ontop;
    char display_string[100];
#if defined(GLX)
    int try_direct;
    int force_direct;
    int sync;
#endif
    int force_present_main_thread;
    int render_to_app_window;
    int render_to_crut_window;
    int crut_drawable;
    int resizable;
    int use_lut8, lut8[3][256];
    int borderless;
    int nvSwapGroup;
    int ignore_papi;
    int ignore_window_moves;
    int pbufferWidth, pbufferHeight;
    int use_glxchoosevisual;
    int draw_bbox;
    /*@}*/

    CRServer *server;
    int gather_port;
    int gather_userbuf_size;
    CRConnection **gather_conns;

    GLint drawCursor;
    GLint cursorX, cursorY;

    int numVisuals;
    VisualInfo visuals[MAX_VISUALS];

    CRHashTable *windowTable;
    CRHashTable *contextTable;

    CRHashTable *dummyWindowTable;

    ContextInfo *defaultSharedContext;

#ifndef CHROMIUM_THREADSAFE
    ContextInfo *currentContext;
#endif

    crOpenGLInterface ws;  /**< Window System interface */

    CRHashTable *barrierHash;

    int is_swap_master, num_swap_clients;
    int swap_mtu;
    char *swap_master_url;
    CRConnection **swap_conns;

    SPUDispatchTable blitterDispatch;
    CRHashTable *blitterTable;

    PFNVCRSERVER_CLIENT_CALLOUT pfnClientCallout;

#ifdef USE_OSMESA
    /** Off screen rendering hooks.  */
    int use_osmesa;

    OSMesaContext (*OSMesaCreateContext)( GLenum format, OSMesaContext sharelist );
    GLboolean (* OSMesaMakeCurrent)( OSMesaContext ctx,
                     GLubyte *buffer,
                     GLenum type,
                     GLsizei width,
                     GLsizei height );
    void (*OSMesaDestroyContext)( OSMesaContext ctx );
#endif

#if defined(GLX)
    RTTHREAD hWinCmdThread;
    VisualInfo WinCmdVisual;
    WindowInfo WinCmdWindow;
    RTSEMEVENT hWinCmdCompleteEvent;
    /* display connection used to send data to the WinCmd thread */
    Display *pCommunicationDisplay;
    Atom WinCmdAtom;
    /* X Window -> CRWindowInfo table */
    CRHashTable *pWinToInfoTable;
#endif

#ifdef RT_OS_WINDOWS
    DWORD dwWinThreadId;
    HANDLE hWinThreadReadyEvent;
#endif

#ifdef RT_OS_DARWIN
# ifdef VBOX_WITH_COCOA_QT
    PFNDELETE_OBJECT pfnDeleteObject;
    PFNGET_ATTACHED_OBJECTS pfnGetAttachedObjects;
    PFNGET_HANDLE pfnGetHandle;
    PFNGET_INFO_LOG pfnGetInfoLog;
    PFNGET_OBJECT_PARAMETERFV pfnGetObjectParameterfv;
    PFNGET_OBJECT_PARAMETERIV pfnGetObjectParameteriv;

    CR_GLSL_CACHE GlobalShaders;
# else
    RgnHandle hRootVisibleRegion;
    RTSEMFASTMUTEX syncMutex;
    EventHandlerUPP hParentEventHandler;
    WindowGroupRef pParentGroup;
    WindowGroupRef pMasterGroup;
    GLint currentBufferName;
    uint64_t uiDockUpdateTS;
    bool fInit;
# endif
#endif /* RT_OS_DARWIN */
    /* If TRUE, render should tell window server to prevent artificial content
     * up-scaling when displayed on HiDPI monitor. */
    bool fUnscaledHiDPI;
} RenderSPU;

#ifdef RT_OS_WINDOWS

/* Asks window thread to create new window.
   msg.lParam - holds pointer to CREATESTRUCT structure
                note that lpCreateParams is used to specify address to store handle of created window
   msg.wParam - unused, should be NULL
*/
#define WM_VBOX_RENDERSPU_CREATE_WINDOW (WM_APP+1)

typedef struct _VBOX_RENDERSPU_DESTROY_WINDOW {
    HWND hWnd; /* handle to window to destroy */
} VBOX_RENDERSPU_DESTROY_WINDOW;

/* Asks window thread to destroy previously created window.
   msg.lParam - holds pointer to RENDERSPU_VBOX_WINDOW_DESTROY structure
   msg.wParam - unused, should be NULL
*/
#define WM_VBOX_RENDERSPU_DESTROY_WINDOW (WM_APP+2)

#endif

extern RenderSPU render_spu;

/* @todo remove this hack */
extern uint64_t render_spu_parent_window_id;

#ifdef CHROMIUM_THREADSAFE
extern CRtsd _RenderTSD;
#define GET_CONTEXT_VAL() ((ContextInfo *) crGetTSD(&_RenderTSD))
#define SET_CONTEXT_VAL(_v) do { \
        crSetTSD(&_RenderTSD, (_v)); \
    } while (0)
#else
#define GET_CONTEXT_VAL() (render_spu.currentContext)
#define SET_CONTEXT_VAL(_v) do { \
        render_spu.currentContext = (_v); \
    } while (0)

#endif

#define GET_CONTEXT(T)  ContextInfo *T = GET_CONTEXT_VAL()


extern void renderspuSetDefaultSharedContext(ContextInfo *pCtx);
extern void renderspuSetVBoxConfiguration( RenderSPU *spu );
extern void renderspuMakeVisString( GLbitfield visAttribs, char *s );
extern VisualInfo *renderspuFindVisual(const char *displayName, GLbitfield visAttribs );
extern GLboolean renderspu_SystemInitVisual( VisualInfo *visual );
extern GLboolean renderspu_SystemCreateContext( VisualInfo *visual, ContextInfo *context, ContextInfo *sharedContext );
extern void renderspu_SystemDestroyContext( ContextInfo *context );
extern GLboolean renderspu_SystemCreateWindow( VisualInfo *visual, GLboolean showIt, WindowInfo *window );
extern GLboolean renderspu_SystemVBoxCreateWindow( VisualInfo *visual, GLboolean showIt, WindowInfo *window );
extern void renderspu_SystemDestroyWindow( WindowInfo *window );
extern void renderspu_SystemWindowSize( WindowInfo *window, GLint w, GLint h );
extern void renderspu_SystemGetWindowGeometry( WindowInfo *window, GLint *x, GLint *y, GLint *w, GLint *h );
extern void renderspu_SystemGetMaxWindowSize( WindowInfo *window, GLint *w, GLint *h );
extern void renderspu_SystemWindowPosition( WindowInfo *window, GLint x, GLint y );
extern void renderspu_SystemWindowVisibleRegion(WindowInfo *window, GLint cRects, const GLint* pRects);
extern GLboolean renderspu_SystemWindowNeedEmptyPresent(WindowInfo *window);
extern int renderspu_SystemInit();
extern int renderspu_SystemTerm();
extern void renderspu_SystemDefaultSharedContextChanged(ContextInfo *fromContext, ContextInfo *toContext);
extern void renderspu_SystemShowWindow( WindowInfo *window, GLboolean showIt );
extern void renderspu_SystemMakeCurrent( WindowInfo *window, GLint windowInfor, ContextInfo *context );
extern void renderspu_SystemSwapBuffers( WindowInfo *window, GLint flags );
extern void renderspu_SystemReparentWindow(WindowInfo *window);
extern void renderspu_SystemVBoxPresentComposition( WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry );
uint32_t renderspu_SystemPostprocessFunctions(SPUNamedFunctionTable *aFunctions, uint32_t cFunctions, uint32_t cTable);
extern void renderspu_GCWindow(void);
extern int renderspuCreateFunctions( SPUNamedFunctionTable table[] );
extern GLboolean renderspuVBoxCompositorSet( WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR * pCompositor);
extern void renderspuVBoxCompositorClearAll();
extern int renderspuVBoxCompositorLock(WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR **ppCompositor);
extern int renderspuVBoxCompositorUnlock(WindowInfo *window);
extern const struct VBOXVR_SCR_COMPOSITOR * renderspuVBoxCompositorAcquire( WindowInfo *window);
extern int renderspuVBoxCompositorTryAcquire(WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR **ppCompositor);
extern void renderspuVBoxCompositorRelease( WindowInfo *window);
extern void renderspuVBoxPresentCompositionGeneric( WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR * pCompositor,
        const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry, int32_t i32MakeCurrentUserData,
        bool fRedraw);
extern PCR_BLITTER renderspuVBoxPresentBlitterGet( WindowInfo *window );
void renderspuVBoxPresentBlitterCleanup( WindowInfo *window );
extern int renderspuVBoxPresentBlitterEnter( PCR_BLITTER pBlitter, int32_t i32MakeCurrentUserData );
extern PCR_BLITTER renderspuVBoxPresentBlitterGetAndEnter( WindowInfo *window, int32_t i32MakeCurrentUserData, bool fRedraw );
extern PCR_BLITTER renderspuVBoxPresentBlitterEnsureCreated( WindowInfo *window, int32_t i32MakeCurrentUserData );
WindowInfo* renderspuWinCreate(GLint visBits, GLint id);
void renderspuWinTermOnShutdown(WindowInfo *window);
void renderspuWinTerm( WindowInfo *window );
void renderspuWinCleanup(WindowInfo *window);
void renderspuWinDestroy(WindowInfo *window);
GLboolean renderspuWinInitWithVisual( WindowInfo *window, VisualInfo *visual, GLboolean showIt, GLint id );
GLboolean renderspuWinInit(WindowInfo *pWindow, const char *dpyName, GLint visBits, GLint id);

DECLINLINE(void) renderspuWinRetain(WindowInfo *window)
{
    ASMAtomicIncU32(&window->cRefs);
}

DECLINLINE(bool) renderspuWinIsTermed(WindowInfo *window)
{
    return window->BltInfo.Base.id < 0;
}

DECLINLINE(void) renderspuWinRelease(WindowInfo *window)
{
    uint32_t cRefs = ASMAtomicDecU32(&window->cRefs);
    if (!cRefs)
    {
        renderspuWinDestroy(window);
    }
}

extern WindowInfo* renderspuGetDummyWindow(GLint visBits);
extern void renderspuPerformMakeCurrent(WindowInfo *window, GLint nativeWindow, ContextInfo *context);
extern GLboolean renderspuInitVisual(VisualInfo *pVisInfo, const char *displayName, GLbitfield visAttribs);
extern void renderspuVBoxCompositorBlit ( const struct VBOXVR_SCR_COMPOSITOR * pCompositor, PCR_BLITTER pBlitter);
extern void renderspuVBoxCompositorBlitStretched ( const struct VBOXVR_SCR_COMPOSITOR * pCompositor, PCR_BLITTER pBlitter, GLfloat scaleX, GLfloat scaleY);
extern GLint renderspuCreateContextEx(const char *dpyName, GLint visBits, GLint id, GLint shareCtx);
extern GLint renderspuWindowCreateEx( const char *dpyName, GLint visBits, GLint id );

extern GLint RENDER_APIENTRY renderspuWindowCreate( const char *dpyName, GLint visBits );
void RENDER_APIENTRY renderspuWindowDestroy( GLint win );
extern GLint RENDER_APIENTRY renderspuCreateContext( const char *dpyname, GLint visBits, GLint shareCtx );
extern void RENDER_APIENTRY renderspuMakeCurrent(GLint crWindow, GLint nativeWindow, GLint ctx);
extern void RENDER_APIENTRY renderspuSwapBuffers( GLint window, GLint flags );

extern uint32_t renderspuContextMarkDeletedAndRelease( ContextInfo *context );

int renderspuDefaultCtxInit();
void renderspuCleanupBase(bool fDeleteTables);

ContextInfo * renderspuDefaultSharedContextAcquire();
void renderspuDefaultSharedContextRelease(ContextInfo * pCtx);
uint32_t renderspuContextRelease(ContextInfo *context);
uint32_t renderspuContextRetain(ContextInfo *context);

bool renderspuCalloutAvailable();
bool renderspuCalloutClient(PFNVCRSERVER_CLIENT_CALLOUT_CB pfnCb, void *pvCb);


#ifdef __cplusplus
extern "C" {
#endif
DECLEXPORT(void) renderspuSetWindowId(uint64_t winId);
DECLEXPORT(void) renderspuReparentWindow(GLint window);
DECLEXPORT(void) renderspuSetUnscaledHiDPI(bool fEnable);
#ifdef __cplusplus
}
#endif

#endif /* CR_RENDERSPU_H */
