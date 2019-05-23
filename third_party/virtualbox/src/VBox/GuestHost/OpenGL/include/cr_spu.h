/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */
#ifndef CR_SPU_H
#define CR_SPU_H

#ifdef WINDOWS
#define SPULOAD_APIENTRY __stdcall
#else
#define SPULOAD_APIENTRY
#endif

#include "cr_dll.h"
#include "spu_dispatch_table.h"
#include "cr_net.h"

#include <iprt/types.h>

#ifdef DARWIN
# include <OpenGL/OpenGL.h>
# ifdef VBOX_WITH_COCOA_QT
# else
#  include <AGL/agl.h>
# endif
#endif

#define SPU_ENTRY_POINT_NAME "SPULoad"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREADS               32      /**< max threads per spu */

typedef struct _SPUSTRUCT SPU;

typedef void (*SPUGenericFunction)(void);

/**
 * SPU Named function descriptor
 */
typedef struct {
    char *name;
    SPUGenericFunction fn;
} SPUNamedFunctionTable;

/**
 * SPU function table descriptor
 */
typedef struct {
    SPUDispatchTable *childCopy;
    void *data;
    SPUNamedFunctionTable *table;
} SPUFunctions;

/**
 * SPU Option callback
 * \param spu
 * \param response
 */
typedef void (*SPUOptionCB)( void *spu, const char *response );

typedef enum { CR_BOOL, CR_INT, CR_FLOAT, CR_STRING, CR_ENUM } cr_type;

/**
 * SPU Options table
 */
typedef struct {
    const char *option; /**< Name of the option */
    cr_type type;       /**< Type of option */
    int numValues;  /**< usually 1 */
    const char *deflt;  /**< comma-separated string of [numValues] defaults */
    const char *min;    /**< comma-separated string of [numValues] minimums */
    const char *max;    /**< comma-separated string of [numValues] maximums */
    const char *description; /**< Textual description of the option */
    SPUOptionCB cb;     /**< Callback function */
} SPUOptions, *SPUOptionsPtr;


/** Init spu */
typedef SPUFunctions *(*SPUInitFuncPtr)(int id, SPU *child,
        SPU *super, unsigned int, unsigned int );
typedef void (*SPUSelfDispatchFuncPtr)(SPUDispatchTable *);
/** Cleanup spu */
typedef int (*SPUCleanupFuncPtr)(void);
/** Load spu */
typedef int (*SPULoadFunction)(char **, char **, void *, void *, void *,
                   SPUOptionsPtr *, int *);


/**
 * masks for spu_flags
 */
#define SPU_PACKER_MASK           0x1
#define SPU_NO_PACKER             0x0
#define SPU_HAS_PACKER            0x1
#define SPU_TERMINAL_MASK         0x2
#define SPU_NOT_TERMINAL          0x0
#define SPU_IS_TERMINAL           0x2
#define SPU_MAX_SERVERS_MASK      0xc
#define SPU_MAX_SERVERS_ZERO      0x0
#define SPU_MAX_SERVERS_ONE       0x4
#define SPU_MAX_SERVERS_UNLIMITED 0x8


/**
 * SPU descriptor
 */
struct _SPUSTRUCT {
    char *name;         /**< Name of the spu */
    char *super_name;       /**< Name of the super class of the spu */
    int id;             /**< Id num of the spu */
        int spu_flags;          /**< options fags for the SPU */
    struct _SPUSTRUCT *superSPU;    /**< Pointer to the descriptor for the super class */
    CRDLL *dll;         /**< pointer to shared lib for spu */
    SPULoadFunction entry_point;    /**< SPU's entry point (SPULoad()) */
    SPUInitFuncPtr init;        /**< SPU init function */
    SPUSelfDispatchFuncPtr self;    /**< */
    SPUCleanupFuncPtr cleanup;  /**< SPU cleanup func */
    SPUFunctions *function_table;   /**< Function table for spu */
    SPUOptions *options;        /**< Options table */
    SPUDispatchTable dispatch_table;
    void *privatePtr;       /**< pointer to SPU-private data */
};


/**
 * These are the OpenGL / window system interface functions
 */
#if defined(WINDOWS)
/**
 * Windows/WGL
 */
/*@{*/
typedef HGLRC (WGL_APIENTRY *wglCreateContextFunc_t)(HDC);
typedef void (WGL_APIENTRY *wglDeleteContextFunc_t)(HGLRC);
typedef BOOL (WGL_APIENTRY *wglShareListsFunc_t)(HGLRC,HGLRC);
typedef BOOL (WGL_APIENTRY *wglMakeCurrentFunc_t)(HDC,HGLRC);
typedef BOOL (WGL_APIENTRY *wglSwapBuffersFunc_t)(HDC);
typedef int (WGL_APIENTRY *wglChoosePixelFormatFunc_t)(HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL (WGL_APIENTRY *wglChoosePixelFormatEXTFunc_t)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef int (WGL_APIENTRY *wglDescribePixelFormatFunc_t)(HDC, int, UINT, CONST PIXELFORMATDESCRIPTOR *);
typedef int (WGL_APIENTRY *wglSetPixelFormatFunc_t)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
typedef HGLRC (WGL_APIENTRY *wglGetCurrentContextFunc_t)();
typedef PROC (WGL_APIENTRY *wglGetProcAddressFunc_t)();
typedef BOOL (WGL_APIENTRY *wglChoosePixelFormatEXTFunc_t)(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
typedef BOOL (WGL_APIENTRY *wglGetPixelFormatAttribivEXTFunc_t)(HDC, int, int, UINT, int *, int *);
typedef BOOL (WGL_APIENTRY *wglGetPixelFormatAttribfvEXTFunc_t)(HDC, int, int, UINT, int *, float *);
typedef const GLubyte *(WGL_APIENTRY *glGetStringFunc_t)( GLenum );
typedef const GLubyte *(WGL_APIENTRY *wglGetExtensionsStringEXTFunc_t)();
typedef const GLubyte *(WGL_APIENTRY *wglGetExtensionsStringARBFunc_t)(HDC);
/*@}*/
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
/**
 * Apple/AGL
 */
/*@{*/
typedef AGLContext (*aglCreateContextFunc_t)( AGLPixelFormat, AGLContext );
typedef GLboolean (*aglDestroyContextFunc_t)( AGLContext );
typedef GLboolean (*aglSetCurrentContextFunc_t)( AGLContext );
typedef void (*aglSwapBuffersFunc_t)( AGLContext );
typedef AGLPixelFormat (*aglChoosePixelFormatFunc_t) (const AGLDevice *, GLint, const GLint *);
typedef GLboolean (*aglDescribePixelFormatFunc_t)( AGLPixelFormat, GLint, GLint * );
/* <--set pixel format */
typedef AGLContext (*aglGetCurrentContextFunc_t)();
/* <--get proc address -- none exists */
typedef void* (*aglGetProcAddressFunc_t)( const GLubyte *name );

/* These are here just in case */
typedef GLboolean (*aglDescribeRendererFunc_t)( AGLRendererInfo, GLint, GLint * );
typedef void (*aglDestroyPixelFormatFunc_t)( AGLPixelFormat );
typedef void (*aglDestroyRendererInfoFunc_t)( AGLRendererInfo );
typedef AGLDevice* (*aglDevicesOfPixelFormatFunc_t)( AGLPixelFormat, GLint );
typedef GLboolean (*aglDisableFunc_t)( AGLContext, GLenum );
typedef GLboolean (*aglEnableFunc_t)( AGLContext, GLenum );
typedef const GLubyte* (*aglErrorStringFunc_t)( GLenum );
typedef AGLDrawable (*aglGetDrawableFunc_t)( AGLContext );
typedef GLenum (*aglGetErrorFunc_t)();
typedef GLboolean (*aglGetIntegerFunc_t)( AGLContext, GLenum, GLint* );
typedef void (*aglGetVersionFunc_t)( GLint *, GLint * );
typedef GLint (*aglGetVirtualScreenFunc_t)( AGLContext );
typedef GLboolean (*aglIsEnabledFunc_t)( AGLContext, GLenum );
typedef AGLPixelFormat (*aglNextPixelFormatFunc_t)( AGLPixelFormat );
typedef AGLRendererInfo (*aglNextRendererInfoFunc_t)( AGLRendererInfo );
typedef AGLRendererInfo (*aglQueryRendererInfoFunc_t)( const AGLDevice *, GLint );
typedef void (*aglReserLibraryFunc_t)();
typedef GLboolean (*aglSetDrawableFunc_t)( AGLContext, AGLDrawable );
typedef GLboolean (*aglSetFullScreenFunc_t)( AGLContext, GLsizei, GLsizei, GLsizei, GLint );
typedef GLboolean (*aglSetIntegerFunc_t)( AGLContext, GLenum, const GLint * );
typedef GLboolean (*aglSetOffScreenFunc_t)( AGLContext, GLsizei, GLsizei, GLsizei, void * );
typedef GLboolean (*aglSetVirtualScreenFunc_t)( AGLContext, GLint );
typedef GLboolean (*aglUpdateContextFunc_t)( AGLContext );
typedef GLboolean (*aglUseFontFunc_t)( AGLContext, GLint, Style, GLint, GLint, GLint, GLint );
# endif

typedef const GLubyte *(*glGetStringFunc_t)( GLenum );
/*@}*/

/**
 * Apple/CGL
 */
/*@{*/
typedef CGLError (*CGLSetCurrentContextFunc_t)( CGLContextObj );
typedef CGLContextObj (*CGLGetCurrentContextFunc_t)();

typedef CGLError (*CGLChoosePixelFormatFunc_t)( const CGLPixelFormatAttribute *, CGLPixelFormatObj *, long * );
typedef CGLError (*CGLDestroyPixelFormatFunc_t)( CGLPixelFormatObj );
typedef CGLError (*CGLDescribePixelFormatFunc_t)( CGLPixelFormatObj , long , CGLPixelFormatAttribute , long * );

typedef CGLError (*CGLQueryRendererInfoFunc_t)( unsigned long, CGLRendererInfoObj *, long * );
typedef CGLError (*CGLDestroyRendererInfoFunc_t)( CGLRendererInfoObj );
typedef CGLError (*CGLDescribeRendererFunc_t)( CGLRendererInfoObj, long, CGLRendererProperty, long * );

typedef CGLError (*CGLCreateContextFunc_t)( CGLPixelFormatObj, CGLContextObj, CGLContextObj * );
typedef CGLError (*CGLDestroyContextFunc_t)( CGLContextObj );
typedef CGLError (*CGLCopyContextFunc_t)( CGLContextObj src, CGLContextObj, unsigned long );

typedef CGLError (*CGLCreatePBufferFunc_t)( long, long, unsigned long, unsigned long, long, CGLPBufferObj * ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;
typedef CGLError (*CGLDestroyPBufferFunc_t)( CGLPBufferObj ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;
typedef CGLError (*CGLDescribePBufferFunc_t)( CGLPBufferObj, long *, long *, unsigned long *, unsigned long *, long * ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;
typedef CGLError (*CGLTexImagePBufferFunc_t)( CGLContextObj, CGLPBufferObj, unsigned long ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;

typedef CGLError (*CGLSetOffScreenFunc_t)( CGLContextObj, long, long, long, void * );
typedef CGLError (*CGLGetOffScreenFunc_t)( CGLContextObj, long *, long *, long *, void ** );
typedef CGLError (*CGLSetFullScreenFunc_t)( CGLContextObj );

typedef CGLError (*CGLSetPBufferFunc_t)( CGLContextObj, CGLPBufferObj, unsigned long, long, long ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;
typedef CGLError (*CGLGetPBufferFunc_t)( CGLContextObj, CGLPBufferObj *, unsigned long *, long *, long * ) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;

typedef CGLError (*CGLClearDrawableFunc_t)( CGLContextObj );
typedef CGLError (*CGLFlushDrawableFunc_t)( CGLContextObj ); /* <-- swap buffers */

typedef CGLError (*CGLEnableFunc_t)( CGLContextObj, CGLContextEnable );
typedef CGLError (*CGLDisableFunc_t)( CGLContextObj, CGLContextEnable );
typedef CGLError (*CGLIsEnabledFunc_t)( CGLContextObj, CGLContextEnable, long * );

typedef CGLError (*CGLSetParameterFunc_t)( CGLContextObj, CGLContextParameter, const long * );
typedef CGLError (*CGLGetParameterFunc_t)( CGLContextObj, CGLContextParameter, long * );

typedef CGLError (*CGLSetVirtualScreenFunc_t)( CGLContextObj, long );
typedef CGLError (*CGLGetVirtualScreenFunc_t)( CGLContextObj, long *);

typedef CGLError (*CGLSetOptionFunc_t)( CGLGlobalOption, long );
typedef CGLError (*CGLGetOptionFunc_t)( CGLGlobalOption, long * );
typedef void (*CGLGetVersionFunc_t)( long *, long * );

typedef const char * (*CGLErrorStringFunc_t)( CGLError );

/** XXX \todo Undocumented CGL functions. Are these all correct? */
typedef void *CGSConnectionID;
typedef int CGSWindowID;
typedef int CGSSurfaceID;

typedef CGLError (*CGLSetSurfaceFunc_t)( CGLContextObj, CGSConnectionID, CGSWindowID, CGSSurfaceID );
typedef CGLError (*CGLGetSurfaceFunc_t)( CGLContextObj, CGSConnectionID, CGSWindowID, CGSSurfaceID* );
typedef CGLError (*CGLUpdateContextFunc_t)( CGLContextObj );
/*@}*/
#else
/**
 * X11/GLX
 */
/*@{*/
typedef int (*glXGetConfigFunc_t)( Display *, XVisualInfo *, int, int * );
typedef Bool (*glXQueryExtensionFunc_t) (Display *, int *, int * );
typedef const char *(*glXQueryExtensionsStringFunc_t) (Display *, int );
typedef Bool (*glXQueryVersionFunc_t)( Display *dpy, int *maj, int *min );
typedef XVisualInfo *(*glXChooseVisualFunc_t)( Display *, int, int * );
typedef GLXContext (*glXCreateContextFunc_t)( Display *, XVisualInfo *, GLXContext, Bool );
typedef void (*glXUseXFontFunc_t)(Font font, int first, int count, int listBase);
typedef void (*glXDestroyContextFunc_t)( Display *, GLXContext );
typedef Bool (*glXIsDirectFunc_t)( Display *, GLXContext );
typedef Bool (*glXMakeCurrentFunc_t)( Display *, GLXDrawable, GLXContext );
typedef void (*glXSwapBuffersFunc_t)( Display *, GLXDrawable );
typedef CR_GLXFuncPtr (*glXGetProcAddressARBFunc_t)( const GLubyte *name );
typedef Display *(*glXGetCurrentDisplayFunc_t)( void );
typedef GLXContext (*glXGetCurrentContextFunc_t)( void );
typedef GLXDrawable (*glXGetCurrentDrawableFunc_t)( void );
typedef char * (*glXGetClientStringFunc_t)( Display *dpy, int name );
typedef void (*glXWaitGLFunc_t)(void);
typedef void (*glXWaitXFunc_t)(void);
typedef void (*glXCopyContextFunc_t)(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask );
typedef const GLubyte *(*glGetStringFunc_t)( GLenum );
typedef Bool (*glXJoinSwapGroupNVFunc_t)(Display *dpy, GLXDrawable drawable, GLuint group);
typedef Bool (*glXBindSwapBarrierNVFunc_t)(Display *dpy, GLuint group, GLuint barrier);
typedef Bool (*glXQuerySwapGroupNVFunc_t)(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier);
typedef Bool (*glXQueryMaxSwapGroupsNVFunc_t)(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers);
typedef Bool (*glXQueryFrameCountNVFunc_t)(Display *dpy, int screen, GLuint *count);
typedef Bool (*glXResetFrameCountNVFunc_t)(Display *dpy, int screen);
#ifdef GLX_VERSION_1_3
typedef GLXContext (*glXCreateNewContextFunc_t)( Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct );
typedef GLXWindow (*glXCreateWindowFunc_t)(Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
typedef Bool (*glXMakeContextCurrentFunc_t)( Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx );
typedef GLXFBConfig *(*glXChooseFBConfigFunc_t)( Display *dpy, int screen, const int *attribList, int *nitems );
typedef GLXFBConfig *(*glXGetFBConfigsFunc_t)(Display *dpy, int screen, int *nelements);
typedef int (*glXGetFBConfigAttribFunc_t)(Display *dpy, GLXFBConfig config, int attribute, int *value);
typedef XVisualInfo *(*glXGetVisualFromFBConfigFunc_t)(Display *dpy, GLXFBConfig config);
typedef GLXPbuffer (*glXCreatePbufferFunc_t)( Display *dpy, GLXFBConfig config, const int *attribList );
typedef void (*glXDestroyPbufferFunc_t)( Display *dpy, GLXPbuffer pbuf );
typedef int (*glXQueryContextFunc_t)(Display *dpy, GLXContext ctx, int attribute, int *value);
typedef void (*glXQueryDrawableFunc_t)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
#endif /* GLX_VERSION_1_3 */
/*@}*/
#endif


/**
 * Package up the WGL/AGL/CGL/GLX function pointers into a struct.  We use
 * this in a few different places.
 */
typedef struct {
#if defined(WINDOWS)
    wglGetProcAddressFunc_t wglGetProcAddress;
    wglCreateContextFunc_t wglCreateContext;
    wglDeleteContextFunc_t wglDeleteContext;
    wglShareListsFunc_t wglShareLists;
    wglMakeCurrentFunc_t wglMakeCurrent;
    wglSwapBuffersFunc_t wglSwapBuffers;
    wglGetCurrentContextFunc_t wglGetCurrentContext;
    wglChoosePixelFormatFunc_t wglChoosePixelFormat;
    wglDescribePixelFormatFunc_t wglDescribePixelFormat;
    wglSetPixelFormatFunc_t wglSetPixelFormat;
    wglChoosePixelFormatEXTFunc_t wglChoosePixelFormatEXT;
    wglGetPixelFormatAttribivEXTFunc_t wglGetPixelFormatAttribivEXT;
    wglGetPixelFormatAttribfvEXTFunc_t wglGetPixelFormatAttribfvEXT;
    wglGetExtensionsStringEXTFunc_t wglGetExtensionsStringEXT;
#elif defined(DARWIN)
# ifndef VBOX_WITH_COCOA_QT
    aglCreateContextFunc_t          aglCreateContext;
    aglDestroyContextFunc_t         aglDestroyContext;
    aglSetCurrentContextFunc_t      aglSetCurrentContext;
    aglSwapBuffersFunc_t            aglSwapBuffers;
    aglChoosePixelFormatFunc_t      aglChoosePixelFormat;
    aglDestroyPixelFormatFunc_t     aglDestroyPixelFormat;
    aglDescribePixelFormatFunc_t    aglDescribePixelFormat;
    aglGetCurrentContextFunc_t      aglGetCurrentContext;
    aglSetDrawableFunc_t            aglSetDrawable;
    aglGetDrawableFunc_t            aglGetDrawable;
    aglSetFullScreenFunc_t          aglSetFullScreen;
    aglGetProcAddressFunc_t         aglGetProcAddress;
    aglUpdateContextFunc_t          aglUpdateContext;
    aglUseFontFunc_t                aglUseFont;
    aglSetIntegerFunc_t             aglSetInteger;
    aglGetErrorFunc_t               aglGetError;
    aglGetIntegerFunc_t             aglGetInteger;
    aglEnableFunc_t                 aglEnable;
    aglDisableFunc_t                aglDisable;
# endif

    CGLChoosePixelFormatFunc_t      CGLChoosePixelFormat;
    CGLDestroyPixelFormatFunc_t     CGLDestroyPixelFormat;
    CGLDescribePixelFormatFunc_t    CGLDescribePixelFormat;
    CGLQueryRendererInfoFunc_t      CGLQueryRendererInfo;
    CGLDestroyRendererInfoFunc_t    CGLDestroyRendererInfo;
    CGLDescribeRendererFunc_t       CGLDescribeRenderer;
    CGLCreateContextFunc_t          CGLCreateContext;
    CGLDestroyContextFunc_t         CGLDestroyContext;
    CGLCopyContextFunc_t            CGLCopyContext;
    CGLSetCurrentContextFunc_t      CGLSetCurrentContext;
    CGLGetCurrentContextFunc_t      CGLGetCurrentContext;
    CGLCreatePBufferFunc_t          CGLCreatePBuffer;
    CGLDestroyPBufferFunc_t         CGLDestroyPBuffer;
    CGLDescribePBufferFunc_t        CGLDescribePBuffer;
    CGLTexImagePBufferFunc_t        CGLTexImagePBuffer;
    CGLSetOffScreenFunc_t           CGLSetOffScreen;
    CGLGetOffScreenFunc_t           CGLGetOffScreen;
    CGLSetFullScreenFunc_t          CGLSetFullScreen;
    CGLSetPBufferFunc_t             CGLSetPBuffer;
    CGLGetPBufferFunc_t             CGLGetPBuffer;
    CGLClearDrawableFunc_t          CGLClearDrawable;
    CGLFlushDrawableFunc_t          CGLFlushDrawable;
    CGLEnableFunc_t                 CGLEnable;
    CGLDisableFunc_t                CGLDisable;
    CGLIsEnabledFunc_t              CGLIsEnabled;
    CGLSetParameterFunc_t           CGLSetParameter;
    CGLGetParameterFunc_t           CGLGetParameter;
    CGLSetVirtualScreenFunc_t       CGLSetVirtualScreen;
    CGLGetVirtualScreenFunc_t       CGLGetVirtualScreen;
    CGLSetOptionFunc_t              CGLSetOption;
    CGLGetOptionFunc_t              CGLGetOption;
    CGLGetVersionFunc_t             CGLGetVersion;
    CGLErrorStringFunc_t            CGLErrorString;

    CGLSetSurfaceFunc_t             CGLSetSurface;
    CGLGetSurfaceFunc_t             CGLGetSurface;
    CGLUpdateContextFunc_t          CGLUpdateContext;
#else
    glXGetConfigFunc_t  glXGetConfig;
    glXQueryExtensionFunc_t glXQueryExtension;
    glXQueryVersionFunc_t glXQueryVersion;
    glXQueryExtensionsStringFunc_t glXQueryExtensionsString;
    glXChooseVisualFunc_t glXChooseVisual;
    glXCreateContextFunc_t glXCreateContext;
    glXDestroyContextFunc_t glXDestroyContext;
    glXUseXFontFunc_t glXUseXFont;
    glXIsDirectFunc_t glXIsDirect;
    glXMakeCurrentFunc_t glXMakeCurrent;
    glXSwapBuffersFunc_t glXSwapBuffers;
    glXGetProcAddressARBFunc_t glXGetProcAddressARB;
    glXGetCurrentDisplayFunc_t glXGetCurrentDisplay;
    glXGetCurrentContextFunc_t glXGetCurrentContext;
    glXGetCurrentDrawableFunc_t glXGetCurrentDrawable;
    glXGetClientStringFunc_t glXGetClientString;
    glXWaitGLFunc_t glXWaitGL;
    glXWaitXFunc_t glXWaitX;
    glXCopyContextFunc_t glXCopyContext;
    /* GLX_NV_swap_group */
    glXJoinSwapGroupNVFunc_t glXJoinSwapGroupNV;
    glXBindSwapBarrierNVFunc_t glXBindSwapBarrierNV;
    glXQuerySwapGroupNVFunc_t glXQuerySwapGroupNV;
    glXQueryMaxSwapGroupsNVFunc_t glXQueryMaxSwapGroupsNV;
    glXQueryFrameCountNVFunc_t glXQueryFrameCountNV;
    glXResetFrameCountNVFunc_t glXResetFrameCountNV;
#ifdef GLX_VERSION_1_3
    glXCreateNewContextFunc_t glXCreateNewContext;
    glXCreateWindowFunc_t glXCreateWindow;
    glXMakeContextCurrentFunc_t glXMakeContextCurrent;
    glXChooseFBConfigFunc_t glXChooseFBConfig;
    glXGetFBConfigsFunc_t glXGetFBConfigs;
    glXGetFBConfigAttribFunc_t glXGetFBConfigAttrib;
    glXGetVisualFromFBConfigFunc_t glXGetVisualFromFBConfig;
    glXCreatePbufferFunc_t glXCreatePbuffer;
    glXDestroyPbufferFunc_t glXDestroyPbuffer;
    glXQueryContextFunc_t glXQueryContext;
    glXQueryDrawableFunc_t glXQueryDrawable;
#endif
#endif
    glGetStringFunc_t glGetString;
} crOpenGLInterface;


/** This is the one required function in _all_ SPUs */
DECLEXPORT(int) SPULoad( char **name, char **super, SPUInitFuncPtr *init,
                    SPUSelfDispatchFuncPtr *self, SPUCleanupFuncPtr *cleanup,
                    SPUOptionsPtr *options, int *flags );

DECLEXPORT(SPU *) crSPULoad( SPU *child, int id, char *name, char *dir, void *server);
DECLEXPORT(SPU *) crSPULoadChain( int count, int *ids, char **names, char *dir, void *server );
DECLEXPORT(void) crSPUUnloadChain(SPU *headSPU);

DECLEXPORT(void) crSPUInitDispatchTable( SPUDispatchTable *table );
DECLEXPORT(void) crSPUCopyDispatchTable( SPUDispatchTable *dst, SPUDispatchTable *src );
DECLEXPORT(void) crSPUChangeInterface( SPUDispatchTable *table, void *origFunc, void *newFunc );


DECLEXPORT(void) crSPUSetDefaultParams( void *spu, SPUOptions *options );
DECLEXPORT(int) crSPUGetEnumIndex( const SPUOptions *option, const char *optName, const char *value );


DECLEXPORT(SPUGenericFunction) crSPUFindFunction( const SPUNamedFunctionTable *table, const char *fname );
DECLEXPORT(void) crSPUInitDispatch( SPUDispatchTable *dispatch, const SPUNamedFunctionTable *table );
DECLEXPORT(void) crSPUInitDispatchNops(SPUDispatchTable *table);

DECLEXPORT(int) crLoadOpenGL( crOpenGLInterface *crInterface, SPUNamedFunctionTable table[] );
DECLEXPORT(void) crUnloadOpenGL( void );
DECLEXPORT(int) crLoadOpenGLExtensions( const crOpenGLInterface *crInterface, SPUNamedFunctionTable table[] );
DECLEXPORT(void) crSPUChangeDispatch(SPUDispatchTable *dispatch, const SPUNamedFunctionTable *newtable);

#if defined(GLX)
DECLEXPORT(XVisualInfo *)
crChooseVisual(const crOpenGLInterface *ws, Display *dpy, int screen,
                             GLboolean directColor, int visBits);
#else
DECLEXPORT(int)
crChooseVisual(const crOpenGLInterface *ws, int visBits);
#endif


#ifdef USE_OSMESA
DECLEXPORT(int)
crLoadOSMesa( OSMesaContext (**createContext)( GLenum format, OSMesaContext sharelist ),
              GLboolean (**makeCurrent)( OSMesaContext ctx, GLubyte *buffer,
                         GLenum type, GLsizei width, GLsizei height ),
              void (**destroyContext)( OSMesaContext ctx ));
#endif

#ifdef __cplusplus
}
#endif

#endif /* CR_SPU_H */
