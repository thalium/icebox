/* $Id: renderspu_cocoa_helper.m $ */
/** @file
 * VirtualBox OpenGL Cocoa Window System Helper Implementation.
 *
 * This source file is shared between the SharedOpenGL HGCM service and the
 * SVGA3d emulation.
 */

/*
 * Copyright (C) 2009-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @page pg_opengl_cocoa  OpenGL - Cocoa Window System Helper
 *
 * How this works:
 * In general it is not so easy like on the other platforms, cause Cocoa
 * doesn't support any clipping of already painted stuff. In Mac OS X there is
 * the concept of translucent canvas's e.g. windows and there it is just
 * painted what should be visible to the user. Unfortunately this isn't the
 * concept of chromium. Therefor I reroute all OpenGL operation from the guest
 * to a frame buffer object (FBO). This is a OpenGL extension, which is
 * supported by all OS X versions we support (AFAIC tell). Of course the guest
 * doesn't know that and we have to make sure that the OpenGL state always is
 * in the right state to paint into the FBO and not to the front/back buffer.
 * Several functions below (like cocoaBindFramebufferEXT, cocoaGetIntegerv,
 * ...) doing this. When a swap or finish is triggered by the guest, the
 * content (which is already bound to an texture) is painted on the screen
 * within a separate OpenGL context. This allows the usage of the same
 * resources (texture ids, buffers ...) but at the same time having an
 * different internal OpenGL state. Another advantage is that we can paint a
 * thumbnail of the current output in a much more smaller (GPU accelerated
 * scale) version on a third context and use glReadPixels to get the actual
 * data. glReadPixels is a very slow operation, but as we just use a much more
 * smaller image, we can handle it (anyway this is only done 5 times per
 * second).
 *
 * Other things to know:
 * - If the guest request double buffering, we have to make sure there are two
 *   buffers. We use the same FBO with 2 color attachments. Also glDrawBuffer
 *   and glReadBuffer is intercepted to make sure it is painted/read to/from
 *   the correct buffers. On swap our buffers are swapped and not the
 *   front/back buffer.
 * - If the guest request a depth/stencil buffer, a combined render buffer for
 *   this is created.
 * - If the size of the guest OpenGL window changes, all FBO's, textures, ...
 *   need to be recreated.
 * - We need to track any changes to the parent window
 *   (create/destroy/move/resize). The various classes like OverlayHelperView,
 *   OverlayWindow, ... are there for.
 * - The HGCM service runs on a other thread than the Main GUI. Keeps this
 *   always in mind (see e.g. performSelectorOnMainThread in renderFBOToView)
 * - We make heavy use of late binding. We can not be sure that the GUI (or any
 *   other third party GUI), overwrite our NSOpenGLContext. So we always ask if
 *   this is our own one, before use. Really neat concept of Objective-C/Cocoa
 *   ;)
 * 
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifdef IN_VMSVGA3D
# define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#endif
#include "renderspu_cocoa_helper.h"

#import <Cocoa/Cocoa.h>
#undef PVM /* sys/param.h (included via Cocoa.h) pollutes the namespace with this define. */

#ifndef IN_VMSVGA3D
# include "chromium.h" /* For the visual bits of chromium */
#endif

#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/thread.h>

#include <VBox/VBoxOGL.h>
#include <VBox/log.h>

#ifdef IN_VMSVGA3D
# include "DevVGA-SVGA3d-cocoa.h"
# include <OpenGL/OpenGL.h>
# include <OpenGL/gl3.h>
# include <OpenGL/gl3ext.h>
# include <OpenGL/glext.h>
#else
# include <cr_vreg.h>
# include <cr_error.h>
# include <cr_blitter.h>
# ifdef VBOX_WITH_CRDUMPER_THUMBNAIL
#  include <cr_pixeldata.h>
# endif
# include "renderspu.h"
#endif



/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/* Debug macros */
/** @def FBO 
 * Disable this to see how the output is without the FBO in the middle of the processing chain. */
#define FBO 1
/** @def CR_RENDER_FORCE_PRESENT_MAIN_THREAD
 * Force present schedule to main thread. */ 
/** @def SHOW_WINDOW_BACKGROUND
 * Define this to see the window background even if the window is clipped. */ 
/** @def DEBUG_VERBOSE 
 * Define this to get some debug info about the messages flow. */
#if 0 || defined(DOXYGEN_RUNNING)
# define CR_RENDER_FORCE_PRESENT_MAIN_THREAD
# define SHOW_WINDOW_BACKGROUND 1
# define DEBUG_VERBOSE
#endif

#ifdef DEBUG_VERBOSE
# error "should be disabled!"
# ifdef IN_VMSVGA3D
#  define DEBUG_INFO(text) do { LogRel(text); AssertFailed(); } while (0)
#  define DEBUG_WARN(text) do { LogRel(text); AssertFailed(); } while (0)
# else
#  define DEBUG_INFO(text) do { LogRel(text); AssertFailed(); } while (0)
#  define DEBUG_WARN(text) do { LogRel(text); AssertFailed(); } while (0)
# endif

# define DEBUG_MSG(text)   do { LogRel(text); } while (0)
# define DEBUG_MSG_1(text) do { LogRel(text); } while (0)

#else

# ifdef IN_VMSVGA3D
#  define DEBUG_INFO(text) do { LogRel(text); } while (0)
#  define DEBUG_WARN(text) do { LogRel(text); } while (0)
# else
#  define DEBUG_INFO(text) do { crInfo text; } while (0)
#  define DEBUG_WARN(text) do { crWarning text; } while (0)
#endif
# define DEBUG_MSG(text)   do {} while (0)
# define DEBUG_MSG_1(text) do {} while (0)

#endif
#ifdef IN_VMSVGA3D
# define DEBUG_MSG_NOT_VMSVGA3D(a_TextArgs)     do {} while (0)
# define COCOA_LOG_FLOW(a_TextArgs)             LogFlow(a_TextArgs)
#else
# define DEBUG_MSG_NOT_VMSVGA3D(a_TextArgs)     DEBUG_MSG(a_TextArgs)
# define COCOA_LOG_FLOW(a_TextArgs)             DEBUG_MSG(a_TextArgs)
#endif


#define DEBUG_FUNC_ENTER() DEBUG_MSG(("==>%s\n", __PRETTY_FUNCTION__))
#define DEBUG_FUNC_LEAVE() DEBUG_MSG(("<==%s\n", __PRETTY_FUNCTION__))

#define DEBUG_GL_SAVE_STATE() \
    do { \
        glPushAttrib(GL_ALL_ATTRIB_BITS); \
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS); \
        glMatrixMode(GL_PROJECTION); \
        glPushMatrix(); \
        glMatrixMode(GL_TEXTURE); \
        glPushMatrix(); \
        glMatrixMode(GL_COLOR); \
        glPushMatrix(); \
        glMatrixMode(GL_MODELVIEW); \
        glPushMatrix(); \
    } while (0)

#define DEBUG_GL_RESTORE_STATE() \
    do { \
        glMatrixMode(GL_MODELVIEW); \
        glPopMatrix(); \
        glMatrixMode(GL_COLOR); \
        glPopMatrix(); \
        glMatrixMode(GL_TEXTURE); \
        glPopMatrix(); \
        glMatrixMode(GL_PROJECTION); \
        glPopMatrix(); \
        glPopClientAttrib(); \
        glPopAttrib(); \
    } while (0)

#ifdef VBOX_STRICT
# define DEBUG_CLEAR_GL_ERRORS() \
    do { \
        while (glGetError() != GL_NO_ERROR) \
        { /* nothing */ } \
    } while (0)
# define DEBUG_CHECK_GL_ERROR(a_szOp) \
    do { \
        GLenum iGlCheckErr = glGetError(); \
        if (RT_UNLIKELY(iGlCheckErr != GL_NO_ERROR)) \
            AssertMsgFailed((a_szOp ": iGlCheckErr=%#x\n", iGlCheckErr)); \
    } while (0) 
#else
# define DEBUG_CLEAR_GL_ERRORS()        do {} while (0)
# define DEBUG_CHECK_GL_ERROR(a_szOp)   do {} while (0)
#endif

/* Whether we control NSView automatic content zooming on Retina/HiDPI displays. */
#define VBOX_WITH_CONFIGURABLE_HIDPI_SCALING    1


#ifdef IN_VMSVGA3D

/*
 * VMSVGA3D compatibility glue.
 */
typedef struct WindowInfo WindowInfo;

# define CR_RGB_BIT             RT_BIT_32(0)

# define CR_ALPHA_BIT           RT_BIT_32(1)
# define CR_DEPTH_BIT           RT_BIT_32(2)
# define CR_STENCIL_BIT         RT_BIT_32(3)
# define CR_ACCUM_BIT           RT_BIT_32(4)
# define CR_DOUBLE_BIT          RT_BIT_32(5)
# define CR_STEREO_BIT          RT_BIT_32(6)
# define CR_MULTISAMPLE_BIT     RT_BIT_32(7)

# define CR_OVERLAY_BIT         RT_BIT_32(8)
# define CR_PBUFFER_BIT         RT_BIT_32(9)
# define VMSVGA3D_NON_DEFAULT_PROFILE_BIT RT_BIT_32(31)
# define CR_ALL_BITS            UINT32_C(0x800003ff)

#endif /* IN_VMSVGA3D */


/**
 * Works functions that creates a NSOpenGLPixelFormat instance. 
 *  
 * @returns Instance.
 * @param   fVisParams          Context flags.
 */
static NSOpenGLPixelFormat *vboxCreatePixelFormat(GLbitfield fVisParams)
{
    NSOpenGLPixelFormatAttribute attribs[24] =
    {
#ifdef IN_VMSVGA3D
        NSOpenGLPFAOpenGLProfile, (NSOpenGLPixelFormatAttribute)0,
#endif
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAColorSize, (NSOpenGLPixelFormatAttribute)24
    };
#ifndef IN_VMSVGA3D
    int i = 3;
#else
    int i = 3+2;
    if (fVisParams & VMSVGA3D_NON_DEFAULT_PROFILE_BIT)
        attribs[1] = VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE >= 3.2 ? NSOpenGLProfileVersionLegacy :  NSOpenGLProfileVersion3_2Core;
    else
        attribs[1] = VBOX_VMSVGA3D_DEFAULT_OGL_PROFILE >= 3.2 ? NSOpenGLProfileVersion3_2Core : NSOpenGLProfileVersionLegacy;
#endif

    if (fVisParams & CR_ALPHA_BIT)
    {
        COCOA_LOG_FLOW(("  CR_ALPHA_BIT requested\n"));
        attribs[i++] = NSOpenGLPFAAlphaSize;
        attribs[i++] = 8;
    }
    if (fVisParams & CR_DEPTH_BIT)
    {
        COCOA_LOG_FLOW(("  CR_DEPTH_BIT requested\n"));
        attribs[i++] = NSOpenGLPFADepthSize;
        attribs[i++] = 24;
    }
    if (fVisParams & CR_STENCIL_BIT)
    {
        COCOA_LOG_FLOW(("  CR_STENCIL_BIT requested\n"));
        attribs[i++] = NSOpenGLPFAStencilSize;
        attribs[i++] = 8;
    }
    if (fVisParams & CR_ACCUM_BIT)
    {
        COCOA_LOG_FLOW(("  CR_ACCUM_BIT requested\n"));
        attribs[i++] = NSOpenGLPFAAccumSize;
        if (fVisParams & CR_ALPHA_BIT)
            attribs[i++] = 32;
        else
            attribs[i++] = 24;
    }
    if (fVisParams & CR_MULTISAMPLE_BIT)
    {
        COCOA_LOG_FLOW(("  CR_MULTISAMPLE_BIT requested\n"));
        attribs[i++] = NSOpenGLPFASampleBuffers;
        attribs[i++] = 1;
        attribs[i++] = NSOpenGLPFASamples;
        attribs[i++] = 4;
    }
    if (fVisParams & CR_DOUBLE_BIT)
    {
        COCOA_LOG_FLOW(("  CR_DOUBLE_BIT requested\n"));
        attribs[i++] = NSOpenGLPFADoubleBuffer;
    }
    if (fVisParams & CR_STEREO_BIT)
    {
        /* We don't support that.
        COCOA_LOG_FLOW(("  CR_STEREO_BIT requested\n"));
        attribs[i++] = NSOpenGLPFAStereo;
        */
    }

    if (VBoxOglIsOfflineRenderingAppropriate())
    {
        COCOA_LOG_FLOW(("  Offline rendering is enabled\n"));
        attribs[i++] = NSOpenGLPFAAllowOfflineRenderers;
    }

    /* Mark the end */
    attribs[i++] = 0;

    /* Instantiate the pixel format object. */
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
}


static NSOpenGLContext *vboxCtxGetCurrent(void)
{
#ifdef IN_VMSVGA3D
    return [NSOpenGLContext currentContext];
#else
    GET_CONTEXT(pCtxInfo);
    if (pCtxInfo)
    {
        Assert(pCtxInfo->context);
        return pCtxInfo->context;
    }
    return nil;
#endif
}

static bool vboxCtxSyncCurrentInfo(void)
{
#ifdef IN_VMSVGA3D
    return false;
#else
    bool fAdjusted = false;
    GET_CONTEXT(pCtxInfo);
    NSOpenGLContext *pCtx = [NSOpenGLContext currentContext];
    NSView *pView = pCtx ? [pCtx view] : nil;
    if (pCtxInfo)
    {
        WindowInfo *pWinInfo = pCtxInfo->currentWindow;
        Assert(pWinInfo);
        if (   pCtxInfo->context != pCtx
            || pWinInfo->window  != pView)
        {
            renderspu_SystemMakeCurrent(pWinInfo, 0, pCtxInfo);
            fAdjusted = true;
        }
    }
    else if (pCtx)
    {
        [NSOpenGLContext clearCurrentContext];
        fAdjusted = true;
    }
    
    return fAdjusted;
#endif
}


/** 
 * State carrying structure for use with vboxCtxEnter and vboxCtxLeave 
 */ 
typedef struct VBOX_CR_RENDER_CTX_INFO
{
    bool                fIsValid;
    NSOpenGLContext    *pCtx;
    NSView             *pView;
} VBOX_CR_RENDER_CTX_INFO;
/** Pointer to render context info for use with vboxCtxEnter/Leave. */
typedef VBOX_CR_RENDER_CTX_INFO *PVBOX_CR_RENDER_CTX_INFO;

static void vboxCtxEnter(NSOpenGLContext *pNewCtx, PVBOX_CR_RENDER_CTX_INFO pCtxInfo)
{
    NSOpenGLContext *pOldCtx  = vboxCtxGetCurrent(); 
    NSView          *pOldView = pOldCtx ? [pOldCtx view] : nil;
    NSView          *pNewView = [pNewCtx view];

    Assert(pNewCtx);

    if (   pOldCtx  != pNewCtx 
        || pOldView != pNewView)
    {
        if (pOldCtx != nil)
            glFlush();
        
        DEBUG_CLEAR_GL_ERRORS();
        [pNewCtx makeCurrentContext];
        DEBUG_CHECK_GL_ERROR("makeCurrentContext");
        
        pCtxInfo->fIsValid = true;
        pCtxInfo->pCtx     = pOldCtx;
        /** @todo r=bird: Why do we save the NEW VIEW here? vboxCtxLeave calls it 'pOldView'. Bug? */
        pCtxInfo->pView    = pNewView; 
    }
    else
    {
        /* No context switch necessary. */
        pCtxInfo->fIsValid = false;
    }
}    
    
static void vboxCtxLeave(PVBOX_CR_RENDER_CTX_INFO pCtxInfo)
{
    if (pCtxInfo->fIsValid)
    {
        NSOpenGLContext *pOldCtx  = pCtxInfo->pCtx;
        NSView          *pOldView = pCtxInfo->pView;
    
        glFlush();
        if (pOldCtx != nil)
        {
            /* vboxCtxEnter saves the new view, not the old. So, what we actually
               do here is switching the view of the old context to that of the new
               one (wrt vboxCtxEnter) before making it current. */
            /** @todo r=bird: Figure out what we really want to do here, and either rename
             *        pOldView or fix the code. */
            if ([pOldCtx view] != pOldView) 
            {
                DEBUG_CLEAR_GL_ERRORS();
                [pOldCtx setView: pOldView];
                DEBUG_CHECK_GL_ERROR("setView");
            }
        
            DEBUG_CLEAR_GL_ERRORS();
            [pOldCtx makeCurrentContext];
            DEBUG_CHECK_GL_ERROR("makeCurrentContext");
            
#ifdef VBOX_STRICT
            {
                NSOpenGLContext *pTstOldCtx  = [NSOpenGLContext currentContext];
                NSView          *pTstOldView = pTstOldCtx ? [pTstOldCtx view] : nil;
                Assert(pTstOldCtx == pOldCtx);
                Assert(pTstOldView == pOldView);
            }
#endif
        }
        else
        {
            [NSOpenGLContext clearCurrentContext];
        }
    }
}


/** 
 * Custom OpenGL context class.
 *
 * This implementation doesn't allow to set a view to the context, but save the
 * view for later use.  Also it saves a copy of the pixel format used to create 
 * that context for later use. 
 */ 
@interface OverlayOpenGLContext: NSOpenGLContext
{
@private
    NSOpenGLPixelFormat *m_pPixelFormat;
    NSView              *m_pView;
}
- (NSOpenGLPixelFormat *)openGLPixelFormat;
@end

/**
 * Abstrack task class.
 */
@interface VBoxTask : NSObject
{
}
- (void)run;
@end

@implementation VBoxTask
/** Run method that the child classes must reimplement.
 *  This will abort the process. */
- (void)run
{
    AssertReleaseFailed();
}
@end


/**
 * Generic task class for executing a given method select.
 */
@interface VBoxTaskPerformSelector : VBoxTask
{
@private
    id  m_Object;
    SEL m_Selector;
    id  m_Arg;
}
- (id)initWithObject:(id)aObject selector:(SEL)aSelector arg:(id)aArg;
- (void)run;
- (void)dealloc;
@end

@implementation VBoxTaskPerformSelector
    
/**
 * Initializes a VBoxTaskPerformSelector.
 *  
 * @param   aObject             The object (reference not consumed).
 * @param   aSelector           The method selector.
 * @param   aArg                The method argument (reference not consumed).
 */
- (id)initWithObject:(id)aObject selector:(SEL)aSelector arg:(id)aArg
{
    self = [super init];
    if (self)
    {
        [aObject retain];
        m_Object = aObject;
        m_Selector = aSelector;
        if (aArg != nil)
            [aArg retain];
        m_Arg = aArg;
    }

    return self;
}

- (void)run
{
    [m_Object performSelector:m_Selector withObject:m_Arg];
}

- (void)dealloc
{
    [m_Object release];
    if (m_Arg != nil)
        [m_Arg release];

    [super dealloc];
}
@end


/**
 * 
 */
@interface VBoxTaskComposite : VBoxTask
{
@private
    NSUInteger      m_CurIndex;
    RTCRITSECT      m_Lock;
    NSMutableArray *m_pArray;
}
- (id)init;
- (void)add:(VBoxTask *)pTask;
- (void)run;
- (void)dealloc;
@end

@implementation VBoxTaskComposite
- (id)init
{
    self = [super init];

    if (self)
    {
        int rc = RTCritSectInit(&m_Lock);
        if (!RT_SUCCESS(rc))
        {
            DEBUG_WARN(("RTCritSectInit failed %d\n", rc));
            return nil;
        }

        m_CurIndex = 0;

        m_pArray = [[NSMutableArray alloc] init];
    }

    return self;
}

/**
 * Adds a task to the composite task object.
 *  
 * @param   pTask               Task to add.  Reference is NOT consumed.
 */
- (void)add:(VBoxTask *)pTask
{
    [pTask retain];
    int rc = RTCritSectEnter(&m_Lock);
    if (RT_SUCCESS(rc))
    {
        [m_pArray addObject:pTask];
        RTCritSectLeave(&m_Lock);
    }
    else
    {
        DEBUG_WARN(("RTCritSectEnter failed %d\n", rc));
        [pTask release];
    }
}

- (void)run
{
    for (;;)
    {
        /*
         * Dequeue a task.
         */
        int rc = RTCritSectEnter(&m_Lock);
        if (RT_FAILURE(rc))
        {
            DEBUG_WARN(("RTCritSectEnter failed %d\n", rc));
            break;
        }
        
        NSUInteger count = [m_pArray count];
        Assert(m_CurIndex <= count);
        if (m_CurIndex == count)
        {
            [m_pArray removeAllObjects];
            m_CurIndex = 0;
            RTCritSectLeave(&m_Lock);
            break;
        }

        VBoxTask *pTask = (VBoxTask *)[m_pArray objectAtIndex:m_CurIndex];
        Assert(pTask != nil);
        
        ++m_CurIndex;
        
        /*
         * Remove the first 1025 empty entires.
         */
        if (m_CurIndex > 1024)
        {
            NSRange range;
            range.location = 0;
            range.length = m_CurIndex;
            [m_pArray removeObjectsInRange:range];
            m_CurIndex = 0;
        }
        RTCritSectLeave(&m_Lock);
        
        /*
         * Run the task and release it.
         */
        [pTask run];
        [pTask release];
    }
}

- (void)dealloc
{
    NSUInteger count = [m_pArray count];
    for (;m_CurIndex < count; ++m_CurIndex)
    {
        VBoxTask *pTask = (VBoxTask*)[m_pArray objectAtIndex:m_CurIndex];
        DEBUG_WARN(("dealloc with non-empty tasks! %p\n", pTask));
        [pTask release];
    }
    
    [m_pArray release];
    RTCritSectDelete(&m_Lock);

    [super dealloc];
}
@end


/**
 * 
 * 
 */
@interface VBoxMainThreadTaskRunner : NSObject
{
@private
    VBoxTaskComposite *m_pTasks;
}
- (id)init;
- (void)add:(VBoxTask *)pTask;
- (void)addObj:(id)aObject selector:(SEL)aSelector arg:(id)aArg;
- (void)runTasks;
- (bool)runTasksSyncIfPossible;
- (void)dealloc;
+ (VBoxMainThreadTaskRunner *) globalInstance;
@end

@implementation VBoxMainThreadTaskRunner
- (id)init
{
    self = [super init];
    if (self)
    {
        m_pTasks = [[VBoxTaskComposite alloc] init];
    }
    
    return self;
}

+ (VBoxMainThreadTaskRunner *) globalInstance
{
    static dispatch_once_t s_DispatchOnce;
    static VBoxMainThreadTaskRunner *s_pRunner = nil;
    dispatch_once(&s_DispatchOnce, ^{
        s_pRunner = [[VBoxMainThreadTaskRunner alloc] init];
    });
    return s_pRunner;
}

- (void)add:(VBoxTask *)pTask
{
    DEBUG_FUNC_ENTER();
    [m_pTasks add:pTask];
    /** @todo r=bird: Unbalanced [self retain]. */
    [self retain];

    if (![self runTasksSyncIfPossible])
    {
        DEBUG_MSG(("task will be processed async\n"));
        [self performSelectorOnMainThread:@selector(runTasks) withObject:nil waitUntilDone:NO];
    }
    
    DEBUG_FUNC_LEAVE();
}

/**
 * Adds a task calling an object method (selector).
 *  
 * @param   aObject             The object (reference not consumed)..
 * @param   aSelector           The method selector.
 * @param   aArg                The method argument (reference not consumed).
 */
- (void)addObj:(id)aObject selector:(SEL)aSelector arg:(id)aArg
{
    VBoxTaskPerformSelector *pSelTask = [[VBoxTaskPerformSelector alloc] initWithObject:aObject selector:aSelector arg:aArg];
    [self add:pSelTask];
    [pSelTask release];
}

    
/**
 * Internal method for running the pending tasks.
 */
- (void)runTasks
{
    if ([NSThread isMainThread])
    {
        [m_pTasks run];
        /** @todo r=bird: This release and the retain in the add method aren't
         *        necessarily balanced if there are more than one call to add().
         *  
         *  This could probably end up deleting the singleton prematurely and leave
         *  globalInstance() returning pointers to a stale object in freed memory,
         *  quite possibly causing crashes or/and heap corruption. */
        [self release];
    }
    else
    {
        DEBUG_WARN(("run tasks called not on main thread!\n"));
#ifndef DEBUG_VERBOSE
        AssertFailed();
#endif
        [self performSelectorOnMainThread:@selector(runTasks) withObject:nil waitUntilDone:YES];
    }
}

/** 
 * Callback for calling runTasks via renderspuCalloutClient. 
 * @param   pvUser      The VBoxMainThreadTaskRunner singleton. 
 */ 
static DECLCALLBACK(void) VBoxMainThreadTaskRunner_RcdRunCallback(void *pvUser)
{
    DEBUG_FUNC_ENTER();
    VBoxMainThreadTaskRunner *pRunner = (VBoxMainThreadTaskRunner *)pvUser;
    Assert(pRunner == [VBoxMainThreadTaskRunner globalInstance]);
    [pRunner runTasks];
    DEBUG_FUNC_LEAVE();
}

/** 
 * Runs pending tasks synchronously, if possible in the current context. 
 *  
 * @returns true if executed tasks, false if not possible.
 */
- (bool)runTasksSyncIfPossible
{
#ifndef IN_VMSVGA3D
    /* 
     * Call on main thread (?) via renderspuCalloutClient (whatever that is).
     */
    if (renderspuCalloutAvailable())
    {
        Assert(![NSThread isMainThread]);
        renderspuCalloutClient(VBoxMainThreadTaskRunner_RcdRunCallback, self);
        return true;
    }
#endif
    
    /*
     * Run directly if on main thread.
     */
    if ([NSThread isMainThread])
    {
        [self runTasks];
        return true;
    }
    
    /* Not possible. */
    return false;
}

- (void)dealloc
{
    /** @todo r=bird: WTF is the point of the deallocator. The object is a singelton
     *        stored in an inaccessible static variable! */
    [m_pTasks release];
    m_pTasks = nil;

    [super dealloc];
}

@end

#ifndef IN_VMSVGA3D
@class DockOverlayView;
#endif

/** 
 * The custom view class. 
 *  
 * This is the main class of the cocoa OpenGL implementation.  It manages a 
 * frame buffer object for the rendering of the guest applications.  The guest 
 * applications render in this frame buffer which is bound to an OpenGL texture.
 * To display the guest content, a secondary shared OpenGL context of the main 
 * OpenGL context is created.  The secondary context is marked as non-opaque and
 * the texture is displayed on an object which is composed out of the several 
 * visible region rectangles. 
 */ 
@interface OverlayView : NSView
{
@private
    NSView             *m_pParentView;
    NSWindow           *m_pOverlayWin;
                       
    NSOpenGLContext    *m_pGLCtx;
    NSOpenGLContext    *m_pSharedGLCtx;
    RTTHREAD            m_Thread;
                       
    GLuint              m_FBOId;

#ifndef IN_VMSVGA3D
    /** The corresponding dock tile view of this OpenGL view & all helper
     * members. */
    DockOverlayView    *m_DockTileView;

    GLfloat             m_FBOThumbScaleX;
    GLfloat             m_FBOThumbScaleY;
    uint64_t            m_msDockUpdateTS;
#endif

    /** @name For clipping
     * @remarks appears to be unused and a complete waste of time + heap.
     * @{ */
    GLint               m_cClipRects;
    GLint              *m_paClipRects;
    /** @} */

    /** @name Position/Size tracking
     * @{ */ 
    NSPoint             m_Pos;
    NSSize              m_Size;
    /** @} */

    /** This is necessary for clipping on the root window */
    NSRect              m_RootRect;
    float               m_yInvRootOffset;
    
#ifndef IN_VMSVGA3D
    CR_BLITTER         *m_pBlitter;
    WindowInfo         *m_pWinInfo;
#endif
    bool                m_fNeedViewportUpdate;
    bool                m_fNeedCtxUpdate;
    bool                m_fDataVisible;
    bool                m_fCleanupNeeded;
    bool                m_fEverSized;
}
- (id)initWithFrame:(NSRect)frame thread:(RTTHREAD)aThread parentView:(NSView *)pParentView winInfo:(WindowInfo *)pWinInfo 
         fVisParams:(GLbitfield) fVisParams;
- (void)setGLCtx:(NSOpenGLContext*)pCtx;
- (NSOpenGLContext *)glCtx;

- (void)setParentView: (NSView *)view;
- (NSView *)parentView;
- (void)setOverlayWin: (NSWindow *)win;
- (NSWindow *)overlayWin;

- (void)vboxSetPos:(NSPoint)pos;
- (void)vboxSetPosUI:(NSPoint)pos;
- (void)vboxSetPosUIObj:(NSValue *)pPos;
- (NSPoint)pos;
- (bool)isEverSized;
- (void)vboxDestroy;
- (void)vboxSetSizeUI:(NSSize)size;
- (void)vboxSetSizeUIObj:(NSValue *)pSize;
- (void)vboxSetSize:(NSSize)size;
- (NSSize)size;
- (void)updateViewportCS;
#ifdef VBOX_WITH_CONFIGURABLE_HIDPI_SCALING
- (NSRect)safeConvertRectToBacking:(NSRect *)pRect;
- (CGFloat)safeGetBackingScaleFactor;
#endif
- (NSRect)safeConvertToScreen:(NSRect *)pRect;
- (void)vboxReshapePerform;
- (void)vboxReshapeOnResizePerform;
- (void)vboxReshapeOnReparentPerform;

#ifndef IN_VMSVGA3D
- (void)createDockTile;
- (void)deleteDockTile;
#endif

- (void)makeCurrentFBO;
- (void)swapFBO;
- (void)vboxSetVisible:(GLboolean)fVisible;
- (void)vboxSetVisibleUIObj:(NSNumber *)pVisible;
- (void)vboxSetVisibleUI:(GLboolean)fVisible;
- (void)vboxTryDraw;
- (void)vboxTryDrawUI;
- (void)vboxReparent:(NSView *)pParentView;
- (void)vboxReparentUI:(NSView *)pParentView;
- (void)vboxPresent:(const VBOXVR_SCR_COMPOSITOR *)pCompositor;
- (void)vboxPresentCS:(const VBOXVR_SCR_COMPOSITOR *)pCompositor;
#ifndef IN_VMSVGA3D
- (void)vboxPresentToDockTileCS:(const VBOXVR_SCR_COMPOSITOR *)pCompositor;
#endif
- (void)vboxPresentToViewCS:(const VBOXVR_SCR_COMPOSITOR *)pCompositor;
- (void)presentComposition:(const VBOXVR_SCR_COMPOSITOR_ENTRY *)pChangedEntry;
#ifndef IN_VMSVGA3D
- (void)vboxBlitterSyncWindow;
#endif

- (void)clearVisibleRegions;
- (void)setVisibleRegions:(GLint)cRects paRects:(const GLint *)paRects;
- (GLboolean)vboxNeedsEmptyPresent;

#ifndef IN_VMSVGA3D
- (NSView *)dockTileScreen;
- (void)reshapeDockTile;
#endif
- (void)cleanupData;
@end

/** 
 * Helper view.
 *
 * This view is added as a sub view of the parent view to track
 * main window changes. Whenever the main window is changed
 * (which happens on fullscreen/seamless entry/exit) the overlay
 * window is informed & can add them self as a child window
 * again. 
 */ 
@class OverlayWindow;
@interface OverlayHelperView: NSView
{
@private
    OverlayWindow *m_pOverlayWindow;
}
-(id)initWithOverlayWindow:(NSRect)frame overlayWindow:(OverlayWindow *)pOverlayWindow;
@end

/** 
 * Custom window class.
 *
 * This is the overlay window which contains our custom NSView.
 * Its a direct child of the Qt Main window. It marks its background
 * transparent & non opaque to make clipping possible. It also disable mouse
 * events and handle frame change events of the parent view. 
 */ 
@interface OverlayWindow : NSWindow
{
@private
    NSView             *m_pParentView;
    OverlayView        *m_pOverlayView;
    OverlayHelperView  *m_pOverlayHelperView;
    NSThread           *m_Thread;
}
- (id)initWithParentView:(NSView *)pParentView overlayView:(OverlayView *)pOverlayView;
- (void)parentWindowFrameChanged:(NSNotification *)note;
- (void)parentWindowChanged:(NSWindow *)pWindow;
@end


#ifndef IN_VMSVGA3D
/**
 * Dock overlay view class.
 */
@interface DockOverlayView: NSView
{
    NSBitmapImageRep   *m_ThumbBitmap;
    NSImage            *m_ThumbImage;
    NSLock             *m_Lock;
}
- (void)dealloc;
- (void)cleanup;
- (void)lock;
- (void)unlock;
- (void)setFrame:(NSRect)frame;
- (void)drawRect:(NSRect)aRect;
- (NSBitmapImageRep *)thumbBitmap;
- (NSImage *)thumbImage;
@end

@implementation DockOverlayView
- (id)init
{
    DEBUG_FUNC_ENTER();
    self = [super init];
    if (self)
    {
        /* 
         * We need a lock cause the thumb image could be accessed from the main
         * thread when someone is calling display on the dock tile & from the
         * OpenGL thread when the thumbnail is updated. 
         */ 
        m_Lock = [[NSLock alloc] init];
    }

    DEBUG_FUNC_LEAVE();
    
    return self;
}

- (void)dealloc
{
    DEBUG_FUNC_ENTER();
    
    [self cleanup];
    [m_Lock release];

    [super dealloc];
    
    DEBUG_FUNC_LEAVE();
}

- (void)cleanup
{
    DEBUG_FUNC_ENTER();
    
    if (m_ThumbImage != nil)
    {
        [m_ThumbImage release];
        m_ThumbImage = nil;
    }

    if (m_ThumbBitmap != nil)
    {
        [m_ThumbBitmap release];
        m_ThumbBitmap = nil;
    }
    
    DEBUG_FUNC_LEAVE();
}

- (void)lock
{
    DEBUG_FUNC_ENTER();
    [m_Lock lock];
    DEBUG_FUNC_LEAVE();
}

- (void)unlock
{
    DEBUG_FUNC_ENTER();
    [m_Lock unlock];
    DEBUG_FUNC_LEAVE();
}

- (void)setFrame:(NSRect)frame
{
    DEBUG_FUNC_ENTER();
    [super setFrame:frame];

    [self lock];
    [self cleanup];

    if (   frame.size.width > 0
        && frame.size.height > 0)
    {
        /* Create a buffer for our thumbnail image. Its in the size of this view. */
        m_ThumbBitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
            pixelsWide:frame.size.width
            pixelsHigh:frame.size.height
            bitsPerSample:8
            samplesPerPixel:4
            hasAlpha:YES
            isPlanar:NO
            colorSpaceName:NSDeviceRGBColorSpace
            bitmapFormat:NSAlphaFirstBitmapFormat
            bytesPerRow:frame.size.width * 4
            bitsPerPixel:8 * 4
        ];
        m_ThumbImage = [[NSImage alloc] initWithSize:[m_ThumbBitmap size]];
        [m_ThumbImage addRepresentation:m_ThumbBitmap];
    }

    [self unlock];
    DEBUG_FUNC_LEAVE();
}

- (BOOL)isFlipped
{
    DEBUG_FUNC_ENTER();
    DEBUG_FUNC_LEAVE();
    return YES;
}

- (void)drawRect:(NSRect)aRect
{
    NSRect frame;
    DEBUG_FUNC_ENTER();
    [self lock];

#ifdef SHOW_WINDOW_BACKGROUND
    [[NSColor colorWithCalibratedRed:1.0 green:0.0 blue:0.0 alpha:0.7] set];
    frame = [self frame];
    [NSBezierPath fillRect:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
#endif /* SHOW_WINDOW_BACKGROUND */
    if (m_ThumbImage != nil)
        [m_ThumbImage drawAtPoint:NSMakePoint(0, 0) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];

    [self unlock];
    DEBUG_FUNC_LEAVE();
}

- (NSBitmapImageRep *)thumbBitmap
{
    DEBUG_FUNC_ENTER();
    DEBUG_FUNC_LEAVE();
    return m_ThumbBitmap;
}

- (NSImage *)thumbImage
{
    DEBUG_FUNC_ENTER();
    DEBUG_FUNC_LEAVE();
    return m_ThumbImage;
}
@end
#endif /* !IN_VMSVGA3D */


/********************************************************************************
*
* OverlayOpenGLContext class implementation
*
********************************************************************************/
@implementation OverlayOpenGLContext

-(id)initWithFormat:(NSOpenGLPixelFormat *)format shareContext:(NSOpenGLContext *)share
{
    DEBUG_FUNC_ENTER();

    m_pPixelFormat = NULL;
    m_pView = NULL;

    self = [super initWithFormat:format shareContext:share];
    Assert(self != nil);
    if (self)
        m_pPixelFormat = format;

    DEBUG_MSG(("OCTX(%p): init OverlayOpenGLContext\n", (void *)self));
    DEBUG_FUNC_LEAVE();
    return self;
}

- (void)dealloc
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OCTX(%p): dealloc OverlayOpenGLContext\n", (void *)self));

    [m_pPixelFormat release];

    [super dealloc];
    
    DEBUG_FUNC_LEAVE();
}

-(bool)isDoubleBuffer
{
    DEBUG_FUNC_ENTER();

    GLint val;
    [m_pPixelFormat getValues:&val forAttribute:NSOpenGLPFADoubleBuffer forVirtualScreen:0];
    
    DEBUG_FUNC_LEAVE();
    return val == GL_TRUE ? YES : NO;
}

-(void)setView:(NSView *)view
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OCTX(%p): setView: new view: %p\n", (void *)self, (void *)view));

#if 1 /* def FBO */
    m_pView = view;;
#else
    [super setView: view];
#endif

    DEBUG_FUNC_LEAVE();
}

-(NSView *)view
{
    DEBUG_FUNC_ENTER();
    DEBUG_FUNC_LEAVE();
#if 1 /* def FBO */
    return m_pView;
#else
    return [super view];
#endif
}

-(void)clearDrawable
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OCTX(%p): clearDrawable\n", (void *)self));

    m_pView = NULL;;
    [super clearDrawable];
    
    DEBUG_FUNC_LEAVE();
}

-(NSOpenGLPixelFormat *)openGLPixelFormat
{
    DEBUG_FUNC_ENTER();
    DEBUG_FUNC_LEAVE();

    return m_pPixelFormat;
}

@end  /* @implementation OverlayOpenGLContext */


/********************************************************************************
*
* OverlayHelperView class implementation
*
********************************************************************************/
@implementation OverlayHelperView

-(id)initWithOverlayWindow:(NSRect)frame overlayWindow:(OverlayWindow *)pOverlayWindow
{
    DEBUG_FUNC_ENTER();

    self = [super initWithFrame:frame];
#ifdef IN_VMSVGA3D
    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
#endif

    m_pOverlayWindow = pOverlayWindow;

    DEBUG_MSG(("OHVW(%p): init OverlayHelperView\n", (void *)self));
    DEBUG_FUNC_LEAVE();
    return self;
}

-(void)viewDidMoveToWindow
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OHVW(%p): viewDidMoveToWindow: new win: %p\n", (void *)self, (void *)[self window]));

    [m_pOverlayWindow parentWindowChanged:[self window]];
    
    DEBUG_FUNC_LEAVE();
}

@end


/********************************************************************************
*
* OverlayWindow class implementation
*
********************************************************************************/
@implementation OverlayWindow

- (id)initWithParentView:(NSView *)pParentView overlayView:(OverlayView *)pOverlayView
{
    DEBUG_FUNC_ENTER();
    NSWindow *pParentWin = nil;

    self = [super initWithContentRect:NSZeroRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    if (self)
    {
        m_pParentView = pParentView;
        m_pOverlayView = pOverlayView;
        m_Thread = [NSThread currentThread];

        [m_pOverlayView setOverlayWin: self];

#ifdef IN_VMSVGA3D                                                                           
        NSRect frame = [pParentView frame];
        frame.origin.x = frame.origin.x = 0;
        m_pOverlayHelperView = [[OverlayHelperView alloc] initWithOverlayWindow:frame
                                                                  overlayWindow:self];
#else
        m_pOverlayHelperView = [[OverlayHelperView alloc] initWithOverlayWindow:NSZeroRect
                                                                  overlayWindow:self];
#endif

        /* Add the helper view as a child of the parent view to get notifications */
        [pParentView addSubview:m_pOverlayHelperView];

        /* Make sure this window is transparent */
#ifdef SHOW_WINDOW_BACKGROUND
        /* For debugging */
        [self setBackgroundColor:[NSColor colorWithCalibratedRed:1.0 green:0.0 blue:0.0 alpha:0.7]];
#else
        [self setBackgroundColor:[NSColor clearColor]];
#endif
        [self setOpaque:NO];
        [self setAlphaValue:.999];

        /* Disable mouse events for this window */
        [self setIgnoresMouseEvents:YES];

        pParentWin = [m_pParentView window];

        /* Initial set the position to the parents view top/left (Compiz fix). */
        [self setFrameOrigin:
            [pParentWin convertBaseToScreen:
                [m_pParentView convertPoint:NSZeroPoint toView:nil]]];

        /* Set the overlay view as our content view */
        [self setContentView:m_pOverlayView];

        /* Add ourself as a child to the parent views window. Note: this has to
         * be done last so that everything else is setup in
         * parentWindowChanged. */
        [pParentWin addChildWindow:self ordered:NSWindowAbove];
    }

    DEBUG_MSG(("OWIN(%p): init OverlayWindow\n", (void *)self));
    DEBUG_FUNC_LEAVE();
    return self;
}

- (void)dealloc
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OWIN(%p): dealloc OverlayWindow\n", (void *)self));

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [m_pOverlayHelperView removeFromSuperview];
    [m_pOverlayHelperView release];

    [super dealloc];
    
    DEBUG_FUNC_LEAVE();
}

- (void)parentWindowFrameChanged:(NSNotification *)pNote
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OWIN(%p): parentWindowFrameChanged\n", (void *)self));

    /* 
     * Reposition this window with the help of the OverlayView. Perform the
     * call in the OpenGL thread. 
     */ 
    /*
    [m_pOverlayView performSelector:@selector(vboxReshapePerform) onThread:m_Thread withObject:nil waitUntilDone:YES];
    */

    if ([m_pOverlayView isEverSized])
    {    
        if ([NSThread isMainThread])
            [m_pOverlayView vboxReshapePerform];
        else
            [self performSelectorOnMainThread:@selector(vboxReshapePerform) withObject:nil waitUntilDone:NO];
    }
    
    DEBUG_FUNC_LEAVE();
}

- (void)parentWindowChanged:(NSWindow *)pWindow
{
    DEBUG_FUNC_ENTER();
    DEBUG_MSG(("OWIN(%p): parentWindowChanged\n", (void *)self));

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    if (pWindow != nil)
    {
        /* Ask to get notifications when our parent window frame changes. */
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(parentWindowFrameChanged:)
            name:NSWindowDidResizeNotification
            object:pWindow];

        /* Add us self as child window */
        [pWindow addChildWindow:self ordered:NSWindowAbove];

        /* 
         * Reshape the overlay view after a short waiting time to let the main
         * window resize itself properly.
         */
        /*
        [m_pOverlayView performSelector:@selector(vboxReshapePerform) withObject:nil afterDelay:0.2];
        [NSTimer scheduledTimerWithTimeInterval:0.2 target:m_pOverlayView selector:@selector(vboxReshapePerform) userInfo:nil repeats:NO];
        */

        if ([m_pOverlayView isEverSized])
        {    
            if ([NSThread isMainThread])
                [m_pOverlayView vboxReshapePerform];
            else
                [self performSelectorOnMainThread:@selector(vboxReshapePerform) withObject:nil waitUntilDone:NO];
        }        
    }
    
    DEBUG_FUNC_LEAVE();
}

@end /* @implementation OverlayWindow */



/********************************************************************************
*
* OverlayView class implementation
*
********************************************************************************/
@implementation OverlayView

- (id)initWithFrame:(NSRect)frame thread:(RTTHREAD)aThread parentView:(NSView *)pParentView winInfo:(WindowInfo *)pWinInfo 
         fVisParams:(GLbitfield) fVisParams
{
    COCOA_LOG_FLOW(("%s: self=%p aThread=%p pParentView=%p pWinInfo=%p fVisParams=%#x\n", __PRETTY_FUNCTION__, (void *)self, 
                    (void *)aThread, (void *)pParentView, (void *)pWinInfo, fVisParams));

    m_pParentView             = pParentView;
    /* Make some reasonable defaults */
    m_pGLCtx                  = nil;
    m_pSharedGLCtx            = nil;
    m_Thread                  = aThread;
    m_FBOId                   = 0;
    m_cClipRects              = 0;
    m_paClipRects             = NULL;
    m_Pos                     = NSZeroPoint;
    m_Size                    = NSMakeSize(1, 1);
    m_RootRect                = NSMakeRect(0, 0, m_Size.width, m_Size.height);
    m_yInvRootOffset          = 0;
#ifndef IN_VMSVGA3D
    m_pBlitter                = nil;
    m_pWinInfo                = pWinInfo;
#endif
    m_fNeedViewportUpdate     = true;
    m_fNeedCtxUpdate          = true;
    m_fDataVisible            = false;
    m_fCleanupNeeded          = false;
    m_fEverSized              = false;

    self = [super initWithFrame:frame];
#if defined(VBOX_WITH_CONFIGURABLE_HIDPI_SCALING) && !defined(IN_VMSVGA3D)
    /* Always allocate HiDPI-ready backing store for NSView, so we will be able change HiDPI scaling option in runtime. */
    crDebug("HiDPI: Allocate big backing store for NSView. Up-scaling is currently %s.", render_spu.fUnscaledHiDPI ? "OFF" : "ON");
    [self performSelector:@selector(setWantsBestResolutionOpenGLSurface:) withObject: (id)YES];
#endif

    COCOA_LOG_FLOW(("%s: returns self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    return self;
}

- (void)cleanupData
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

#ifndef IN_VMSVGA3D
    [self deleteDockTile];
#endif

    [self setGLCtx:nil];
    
    if (m_pSharedGLCtx)
    {
        if ([m_pSharedGLCtx view] == self)
            [m_pSharedGLCtx clearDrawable];

        [m_pSharedGLCtx release];
        m_pSharedGLCtx = nil;
        

#ifndef IN_VMSVGA3D
        CrBltTerm(m_pBlitter);
        RTMemFree(m_pBlitter);
        m_pBlitter = nil;
#endif
    }

    [self clearVisibleRegions];
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)dealloc
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

    [self cleanupData];
    [super dealloc];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)drawRect:(NSRect)aRect
{
    COCOA_LOG_FLOW(("%s: self=%p aRect=%d,%d %d,%d\n", __PRETTY_FUNCTION__, (void *)self, (int)aRect.origin.x, (int)aRect.origin.y, 
                    (int)aRect.size.width, (int)aRect.size.height));

    [self vboxTryDrawUI];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)setGLCtx:(NSOpenGLContext *)pCtx
{
    COCOA_LOG_FLOW(("%s: self=%p pCtx=%p (old=%p)\n", __PRETTY_FUNCTION__, (void *)self, (void *)pCtx, m_pGLCtx));

    /*
     * Only do something if the context changes.
     */
    if (m_pGLCtx != pCtx)
    {
        /* Ensure the context drawable is cleared to avoid holding a reference to inexistent view. */
        if (m_pGLCtx)
        {
#ifdef IN_VMSVGA3D
            Assert(!pCtx);
#endif
            [m_pGLCtx clearDrawable];
            [m_pGLCtx release];
            /*[m_pGLCtx performSelectorOnMainThread:@selector(release) withObject:nil waitUntilDone:NO];*/
        }

        m_pGLCtx = pCtx;
        if (pCtx)
            [pCtx retain];
    }
        
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (NSOpenGLContext *)glCtx
{
    COCOA_LOG_FLOW(("%s: self=%p returns %p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_pGLCtx));
    return m_pGLCtx;
}

- (NSView *)parentView
{
    COCOA_LOG_FLOW(("%s: self=%p returns %p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_pParentView));
    return m_pParentView;
}

- (void)setParentView:(NSView *)pView
{
    COCOA_LOG_FLOW(("%s: self=%p pView=%p (old=%p)\n", __PRETTY_FUNCTION__, (void *)self, (void *)pView, m_pParentView));

    m_pParentView = pView;
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)setOverlayWin:(NSWindow *)pWin
{
    COCOA_LOG_FLOW(("%s: self=%p pWin=%p (old=%p)\n", __PRETTY_FUNCTION__, (void *)self, (void *)pWin, m_pOverlayWin));

    m_pOverlayWin = pWin;
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (NSWindow *)overlayWin
{
    COCOA_LOG_FLOW(("%s: self=%p returns %p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_pOverlayWin));
    return m_pOverlayWin;
}

- (void)vboxSetPosUI:(NSPoint)pos
{
    COCOA_LOG_FLOW(("%s: self=%p pos=%d,%d (old pos=%d,%d)\n", __PRETTY_FUNCTION__, (void *)self, (int)pos.x, (int)pos.y, 
                    (int)m_Pos.x, (int)m_Pos.y));

    DEBUG_MSG(("vboxSetPosUI: [%d, %d].\n", (int)pos.x, (int)pos.y));

    m_Pos = pos;

    if (m_fEverSized)
        [self vboxReshapePerform];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetPosUIObj:(NSValue *)pPos
{
    COCOA_LOG_FLOW(("%s: self=%p pPos=%p (%d,%d) (old pos=%d,%d)\n", __PRETTY_FUNCTION__, (void *)self, pPos,
                    (int)[pPos pointValue].x, (int)[pPos pointValue].y, (int)m_Pos.x, (int)m_Pos.y));

    NSPoint pos = [pPos pointValue];
    [self vboxSetPosUI:pos];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetPos:(NSPoint)pos
{
    COCOA_LOG_FLOW(("%s: self=%p pos=%d,%d (old pos=%d,%d)\n", __PRETTY_FUNCTION__, (void *)self, (int)pos.x, (int)pos.y, 
                    (int)m_Pos.x, (int)m_Pos.y));

    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    NSValue *pPos =  [NSValue valueWithPoint:pos];
    [pRunner addObj:self selector:@selector(vboxSetPosUIObj:) arg:pPos];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (NSPoint)pos
{
    COCOA_LOG_FLOW(("%s: self=%p returns %d,%d\n", __PRETTY_FUNCTION__, (void *)self, (int)m_Pos.x, (int)m_Pos.y));
    return m_Pos;
}

- (bool)isEverSized
{
    COCOA_LOG_FLOW(("%s: self=%p returns %d\n", __PRETTY_FUNCTION__, (void *)self, m_fEverSized));
    return m_fEverSized;
}

- (void)vboxDestroy
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    BOOL        fIsMain = [NSThread isMainThread];
    NSWindow   *pWin    = nil;
    
    Assert(fIsMain);

    /* Hide the view early. */
    [self setHidden: YES];

    pWin = [self window];
    [[NSNotificationCenter defaultCenter] removeObserver:pWin];
    [pWin setContentView: nil];
    [[pWin parentWindow] removeChildWindow: pWin];
    
    if (fIsMain)
        [pWin release];
    else
    {
        /* We can NOT run synchronously with the main thread since this may lead to a deadlock,
           caused by main thread waiting xpcom thread, xpcom thread waiting to main hgcm thread,
           and main hgcm thread waiting for us, this is why use waitUntilDone:NO, 
           which should cause no harm. */ 
        [pWin performSelectorOnMainThread:@selector(release) withObject:nil waitUntilDone:NO];
    }

    [self cleanupData];

    if (fIsMain)
        [self release];
    else
    {
        /* We can NOT run synchronously with the main thread since this may lead to a deadlock,
           caused by main thread waiting xpcom thread, xpcom thread waiting to main hgcm thread,
           and main hgcm thread waiting for us, this is why use waitUntilDone:NO. 
           We need to avoid concurrency though, so we cleanup some data right away via a cleanupData call. */
        [self performSelectorOnMainThread:@selector(release) withObject:nil waitUntilDone:NO];
    }
    
#ifndef IN_VMSVGA3D
    renderspuWinRelease(m_pWinInfo);
#endif
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetSizeUIObj:(NSValue *)pSize
{
    COCOA_LOG_FLOW(("%s: self=%p pSize=%p (%d,%d)\n", __PRETTY_FUNCTION__, (void *)self, (void *)pSize,
                    (int)[pSize sizeValue].width, (int)[pSize sizeValue].height));

    NSSize size = [pSize sizeValue];
    [self vboxSetSizeUI:size];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetSizeUI:(NSSize)size
{
    COCOA_LOG_FLOW(("%s: self=%p size=%d,%d\n", __PRETTY_FUNCTION__, (void *)self, (int)size.width, (int)size.height));

    m_Size = size;
    m_fEverSized = true;

    DEBUG_MSG(("OVIW(%p): vboxSetSize: new size: %dx%d\n", (void *)self, (int)m_Size.width, (int)m_Size.height));
    [self vboxReshapeOnResizePerform];

    /* ensure window contents is updated after that */
    [self vboxTryDrawUI];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetSize:(NSSize)size
{
    COCOA_LOG_FLOW(("%s: self=%p size=%d,%d\n", __PRETTY_FUNCTION__, (void *)self, (int)size.width, (int)size.height));
    
    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    NSValue *pSize = [NSValue valueWithSize:size];
    [pRunner addObj:self selector:@selector(vboxSetSizeUIObj:) arg:pSize];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (NSSize)size
{
    COCOA_LOG_FLOW(("%s: self=%p returns %d,%d\n", __PRETTY_FUNCTION__, (void *)self, (int)m_Size.width, (int)m_Size.height));
    return m_Size;
}

- (void)updateViewportCS
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

    /* Update the viewport for our OpenGL view. */
    [m_pSharedGLCtx update];

#ifndef IN_VMSVGA3D
    [self vboxBlitterSyncWindow];
#endif
        
    /* Clear background to transparent. */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxReshapeOnResizePerform
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

    [self vboxReshapePerform];
#ifndef IN_VMSVGA3D
    [self createDockTile];
#endif

    /* have to rebind GL_TEXTURE_RECTANGLE_ARB as m_FBOTexId could be changed in updateFBO call */
    m_fNeedViewportUpdate = true;
#if 0
    pCurCtx = [NSOpenGLContext currentContext];
    if (pCurCtx && pCurCtx == m_pGLCtx && (pCurView = [pCurCtx view]) == self)
    {
        [m_pGLCtx update];
        m_fNeedCtxUpdate = false;
    }
    else
    {
        /* do it in a lazy way */
        m_fNeedCtxUpdate = true;
    }
#endif

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxReshapeOnReparentPerform
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    [self vboxReshapePerform];
#ifndef IN_VMSVGA3D
    [self createDockTile];
#endif
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

#ifdef VBOX_WITH_CONFIGURABLE_HIDPI_SCALING
- (NSRect)safeConvertRectToBacking:(NSRect *)pRect
{
    NSRect resultingRect = NSZeroRect;

    NSWindow *pWindow = [m_pParentView window];
    if (pWindow)
    {
        if ([pWindow respondsToSelector:@selector(convertRectToBacking:)])
        {
            NSMethodSignature *pSignature = [pWindow methodSignatureForSelector:@selector(convertRectToBacking:)];
            if (pSignature)
            {
                NSInvocation *pInvocation = [NSInvocation invocationWithMethodSignature:pSignature];
                if (pInvocation)
                {
                    [pInvocation setSelector:@selector(convertRectToBacking:)];
                    [pInvocation setTarget:pWindow];
                    [pInvocation setArgument:pRect atIndex:2];
                    [pInvocation invoke];
                    [pInvocation getReturnValue:&resultingRect];

                    DEBUG_MSG(("safeConvertRectToBacking: convert [X, Y, WxH]: [%d, %d, %dx%d] -> [%d, %d, %dx%d]\n",
                        (int)pRect       ->origin.x, (int)pRect       ->origin.y, (int)pRect       ->size.width, (int)pRect       ->size.width,
                        (int)resultingRect.origin.x, (int)resultingRect.origin.y, (int)resultingRect.size.width, (int)resultingRect.size.width));

                    return resultingRect;
                }
            }
        }
    }
    else
        /* Should never happen. */
        DEBUG_WARN(("safeConvertRectToBacking: parent widget has no window.\n"));

    resultingRect = *pRect;

    DEBUG_MSG(("safeConvertRectToBacking (reurn as is): convert [X, Y, WxH]: [%d, %d, %dx%d] -> [%d, %d, %dx%d]\n",
        (int)pRect       ->origin.x, (int)pRect       ->origin.y, (int)pRect       ->size.width, (int)pRect       ->size.width,
        (int)resultingRect.origin.x, (int)resultingRect.origin.y, (int)resultingRect.size.width, (int)resultingRect.size.width));

    return resultingRect;
}


- (CGFloat)safeGetBackingScaleFactor
{
    /* Assume its default value. */
    CGFloat backingScaleFactor = 1.;

    NSWindow *pWindow = [m_pParentView window];
    if (pWindow)
    {
        NSScreen *pScreen = [pWindow screen];
        if (pScreen)
        {
            if ([pScreen respondsToSelector:@selector(backingScaleFactor)])
            {
                NSMethodSignature *pSignature = [pScreen methodSignatureForSelector:@selector(backingScaleFactor)];
                if (pSignature)
                {
                    NSInvocation *pInvocation = [NSInvocation invocationWithMethodSignature:pSignature];
                    if (pInvocation)
                    {
                        [pInvocation setSelector:@selector(backingScaleFactor)];
                        [pInvocation setTarget:pScreen];
                        [pInvocation invoke];
                        [pInvocation getReturnValue:&backingScaleFactor];

                        DEBUG_MSG(("safeGetBackingScaleFactor: %d\n", (int)backingScaleFactor));

                        return backingScaleFactor;
                    }
                    else
                        DEBUG_WARN(("safeGetBackingScaleFactor: unable to create invocation for backingScaleFactor method signature.\n"));
                }
                else
                    DEBUG_WARN(("safeGetBackingScaleFactor: unable to create method signature for backingScaleFactor selector.\n"));
            }
            else
                DEBUG_WARN(("safeGetBackingScaleFactor: NSScreen does not respond to backingScaleFactor selector.\n"));
        }
        else
            /* Should never happen. */
            DEBUG_WARN(("safeGetBackingScaleFactor: parent window has no screen.\n"));
    }
    else
        /* Should never happen. */
        DEBUG_WARN(("safeGetBackingScaleFactor: parent widget has no window.\n"));

    return backingScaleFactor;
}

#endif


- (NSRect)safeConvertToScreen:(NSRect *)pRect
{
    NSRect resultingRect = NSZeroRect;

    NSWindow *pWindow = [m_pParentView window];
    if (pWindow)
    {
        if ([pWindow respondsToSelector:@selector(convertRectToScreen:)])
        {
            NSMethodSignature *pSignature = [pWindow methodSignatureForSelector:@selector(convertRectToScreen:)];
            if (pSignature)
            {
                NSInvocation *pInvocation = [NSInvocation invocationWithMethodSignature:pSignature];
                if (pInvocation)
                {
                    [pInvocation setSelector:@selector(convertRectToScreen:)];
                    [pInvocation setTarget:pWindow];
                    [pInvocation setArgument:pRect atIndex:2];
                    [pInvocation invoke];
                    [pInvocation getReturnValue:&resultingRect];

                    DEBUG_MSG(("safeConvertToScreen: convert [X, Y, WxH]: [%d, %d, %dx%d] -> [%d, %d, %dx%d]\n",
                        (int)pRect       ->origin.x, (int)pRect       ->origin.y, (int)pRect       ->size.width, (int)pRect       ->size.width,
                        (int)resultingRect.origin.x, (int)resultingRect.origin.y, (int)resultingRect.size.width, (int)resultingRect.size.width));

                    return resultingRect;
                }
            }
        }

        /* If we failed, let's use deprecated @selector(convertBaseToScreen:). It is a bit hacky,
         * but what to do if we stick to SDK 10.6. */
        resultingRect.origin = [[m_pParentView window] convertBaseToScreen:pRect->origin];
        resultingRect.size   = pRect->size;
    }
    else
        /* Should never happen. */
        DEBUG_WARN(("safeConvertToScreen: parent widget has no window.\n"));

    DEBUG_MSG(("safeConvertToScreen (deprecated method): convert [X, Y, WxH]: [%d, %d, %dx%d] -> [%d, %d, %dx%d]\n",
        (int)pRect       ->origin.x, (int)pRect       ->origin.y, (int)pRect       ->size.width, (int)pRect       ->size.width,
        (int)resultingRect.origin.x, (int)resultingRect.origin.y, (int)resultingRect.size.width, (int)resultingRect.size.width));

    return resultingRect;
}

- (void)vboxReshapePerform
{
#ifndef IN_VMSVGA3D
    COCOA_LOG_FLOW(("%s: self=%p - m_DockTileView=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_DockTileView));
#else
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
#endif

    /* NOTE: Please consider the next naming convention for variables.
     *
     * Rectangle variables:
     *
     *      <object to represent><coordinate system>:
     *          <object to represent>:
     *              parentFrame - a frame of the parent container (NSView) object
     *              childFrame  - a frame required to display guest content
     *              windowFrame - resulting window frame constructed as an intersection of parentFrame and childFrame
     *          <coordinate system>:
     *              VCS - View Coordinate System
     *              WCS - Window Coordinate System
     *              SCS - Screen Coordinate System
     *
     * The same convention applied to offset variables naming as well which are of format:
     *
     *      <object to represent><coordinate><coordinate system>.
     *
     * https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CocoaDrawingGuide/Transforms/Transforms.html
     */

    NSRect parentFrameVCS, parentFrameWCS, parentFrameSCS;
    NSRect childFrameWCS, childFrameSCS;
    NSRect windowFrameSCS;

    CGFloat childFrameXWCS, childFrameYWCS;

    /* We need to construct a new window frame (windowFrameSCS) for entire NSWindow object in
     * screen coordinates. In order to make 3D overlay window to do not overlap Cocoa and Qt GUI elements (titlebar,
     * Qt statusbar, scroll bars etc) let's do the next. Get parent view visible area (parentFrameSCS) in (NS)Screen
     * coordinates. Then get the area required to diaplay guest content (childFrameSCS) in (NS)Screen coordinates as well.
     * The intersection of these two areas in screen coordinates will be a new frame for entire NSWindow object. */

    parentFrameVCS = [m_pParentView frame];
    parentFrameWCS = [m_pParentView convertRect:parentFrameVCS toView:nil];
    parentFrameSCS = [self safeConvertToScreen:&parentFrameWCS];

    /* Choose childFrame origin in a bit special way. Its pop-left corner should stick to its parent top-left corner. */
    childFrameXWCS = parentFrameWCS.origin.x + m_Pos.x;
    childFrameYWCS = parentFrameWCS.origin.y - m_Pos.y - (m_Size.height - parentFrameWCS.size.height);
    childFrameWCS  = NSMakeRect(childFrameXWCS, childFrameYWCS, m_Size.width, m_Size.height);
    childFrameSCS  = [self safeConvertToScreen:&childFrameWCS];

    windowFrameSCS = NSIntersectionRect(parentFrameSCS, childFrameSCS);

    DEBUG_MSG(("vboxReshapePerform: a new overlay frame [%d, %d, %dx%d] has been constructed from intersection of window frame "
               "[%d, %d, %dx%d] and guest content rectangle [%d, %d, %dx%d]; m_Pos=[%d, %d], m_Size=%dx%d.\n",
               (int)windowFrameSCS.origin.x, (int)windowFrameSCS.origin.y, (int)windowFrameSCS.size.width, (int)windowFrameSCS.size.width,
               (int)parentFrameSCS.origin.x, (int)parentFrameSCS.origin.y, (int)parentFrameSCS.size.width, (int)parentFrameSCS.size.width,
               (int)childFrameSCS .origin.x, (int)childFrameSCS .origin.y, (int)childFrameSCS .size.width, (int)childFrameSCS .size.width,
               (int)m_Pos.x, (int)m_Pos.y, (int)m_Size.width, (int)m_Size.height));

    /* @todo galitsyn: drop this!
     * Later we have to correct the texture position in the case the window is
     * out of the parents window frame. So save the shift values for later use. */ 
    m_RootRect.origin.x = windowFrameSCS.origin.x - childFrameSCS.origin.x;
    m_RootRect.origin.y =  childFrameSCS.size.height + childFrameSCS.origin.y - (windowFrameSCS.size.height + windowFrameSCS.origin.y);
    m_RootRect.size = windowFrameSCS.size;
    m_yInvRootOffset = windowFrameSCS.origin.y - childFrameSCS.origin.y;

    DEBUG_MSG(("vboxReshapePerform: [%#p]: m_RootRect pos[%d : %d] size[%d : %d]\n",
               (void *)self, (int)m_RootRect.origin.x, (int)m_RootRect.origin.y, (int)m_RootRect.size.width, (int)m_RootRect.size.height));

    /* Set the new frame. */
    [[self window] setFrame:windowFrameSCS display:YES];

#ifndef IN_VMSVGA3D
    /* Inform the dock tile view as well. */
    [self reshapeDockTile];
#endif

    /* Make sure the context is updated accordingly. */
    /* [self updateViewport]; */
    if (m_pSharedGLCtx)
    {
        VBOX_CR_RENDER_CTX_INFO CtxInfo; 
        vboxCtxEnter(m_pSharedGLCtx, &CtxInfo);

        [self updateViewportCS];

        vboxCtxLeave(&CtxInfo);
    }

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

#ifndef IN_VMSVGA3D

- (void)createDockTile
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    NSView *pDockScreen = nil;

    [self deleteDockTile];

    /* Is there a dock tile preview enabled in the GUI? If so setup a
     * additional thumbnail view for the dock tile. */
    pDockScreen = [self dockTileScreen];
    if (pDockScreen)
    {
        m_DockTileView = [[DockOverlayView alloc] init];
        [self reshapeDockTile];
        [pDockScreen addSubview:m_DockTileView];
    }

    COCOA_LOG_FLOW(("%s: returns - m_DockTileView\n", __PRETTY_FUNCTION__, (void *)m_DockTileView));
}

- (void)deleteDockTile
{
    COCOA_LOG_FLOW(("%s: self=%p - m_DockTileView=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_DockTileView));

    if (m_DockTileView != nil)
    {
        [m_DockTileView removeFromSuperview];
        [m_DockTileView release];
        m_DockTileView = nil;
    }

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

#endif /* !IN_VMSVGA3D */

- (void)makeCurrentFBO
{
    COCOA_LOG_FLOW(("%s: self=%p - m_pGLCtx=%p m_fNeedCtxUpdate=%d\n", __PRETTY_FUNCTION__, (void *)self, 
                    (void *)m_pGLCtx, m_fNeedCtxUpdate));

    if (m_pGLCtx)
    {
        NSOpenGLContext *pPrevCtx = [NSOpenGLContext currentContext];

#ifdef IN_VMSVGA3D
        /* Always flush before flush. glXMakeCurrent and wglMakeCurrent does this
           implicitly, seemingly NSOpenGLContext::makeCurrentContext doesn't. */
        if (pPrevCtx != nil)
        {
            DEBUG_CLEAR_GL_ERRORS();
            glFlush();
            DEBUG_CHECK_GL_ERROR("glFlush");
        }
#endif

        if ([m_pGLCtx view] != self)
        {
#ifndef IN_VMSVGA3D
            /* We change the active view, so flush first */
            if (pPrevCtx != nil)
                glFlush();
#endif
            DEBUG_CLEAR_GL_ERRORS();
            [m_pGLCtx setView: self];
            DEBUG_CHECK_GL_ERROR("setView");
        }

#if 0
        if (pPrevCtx != m_pGLCtx)
#endif
        {
            DEBUG_CLEAR_GL_ERRORS();
            [m_pGLCtx makeCurrentContext];
            DEBUG_CHECK_GL_ERROR("makeCurrentContext");
            Assert([NSOpenGLContext currentContext] == m_pGLCtx);
            Assert([m_pGLCtx view] == self);
        }

        if (m_fNeedCtxUpdate == true)
        {
            [m_pGLCtx update];
            m_fNeedCtxUpdate = false;
        }
        
        if (!m_FBOId)
        {
            glGenFramebuffersEXT(1, &m_FBOId);
            Assert(m_FBOId);
        }
    }

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (bool)vboxSharedCtxCreate
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

    if (m_pSharedGLCtx)
    {
        COCOA_LOG_FLOW(("%s: returns true (m_pSharedGLCtx=%p)\n", __PRETTY_FUNCTION__, (void *)m_pSharedGLCtx));
        return true;
    }
        
#ifndef IN_VMSVGA3D            
    Assert(!m_pBlitter);
    m_pBlitter = RTMemAlloc(sizeof(*m_pBlitter));
    if (RT_UNLIKELY(!m_pBlitter))
    {
        DEBUG_WARN(("m_pBlitter allocation failed"));
        COCOA_LOG_FLOW(("%s: returns false - m_pBlitter allocation failed\n", __PRETTY_FUNCTION__));
        return false;
    }
        
    int rc = CrBltInit(m_pBlitter, NULL, false /*fCreateNewCtx*/, false /*fForceDrawBlt*/, 
                       &render_spu.GlobalShaders, &render_spu.blitterDispatch);
    if (RT_FAILURE(rc))
    {
        DEBUG_WARN(("CrBltInit failed, rc %d", rc));
        RTMemFree(m_pBlitter);
        m_pBlitter = NULL;

        COCOA_LOG_FLOW(("%s: returns false - CrBltInit failed with rc=%Rrc\n", __PRETTY_FUNCTION__, rc));
        return false;
    }        

    COCOA_LOG_FLOW(("%s: blitter (%p) created successfully for view 0x%p\n", (void *)m_pBlitter, (void *)self));
#endif /* !IN_VMSVGA3D */

    /* Create a shared context out of the main context. Use the same pixel format. */
    NSOpenGLPixelFormat *pPixelFormat = [(OverlayOpenGLContext *)m_pGLCtx openGLPixelFormat];
    NSOpenGLContext     *pSharedGLCtx = [[NSOpenGLContext alloc] initWithFormat:pPixelFormat shareContext:m_pGLCtx];
        
    /* Set the new context as non opaque */
    GLint opaque = 0;
    [pSharedGLCtx setValues:&opaque forParameter:NSOpenGLCPSurfaceOpacity];

    /* Set this view as the drawable for the new context */
    [pSharedGLCtx setView:self];
    m_fNeedViewportUpdate = true;

    m_pSharedGLCtx = pSharedGLCtx;
    
    COCOA_LOG_FLOW(("%s: returns true - new m_pSharedGLCtx=%p\n", __PRETTY_FUNCTION__, (void *)m_pSharedGLCtx));
    return true;
}

- (void)vboxTryDraw
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));

    glFlush();
                  
    /* Issue to the gui thread. */
    [self performSelectorOnMainThread:@selector(vboxTryDrawUI) withObject:nil waitUntilDone:NO];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetVisible:(GLboolean)fVisible
{
    COCOA_LOG_FLOW(("%s: self=%p fVisible=%d\n", __PRETTY_FUNCTION__, (void *)self, fVisible));
    
    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    NSNumber *pVisObj = [NSNumber numberWithBool:fVisible];
    [pRunner addObj:self selector:@selector(vboxSetVisibleUIObj:) arg:pVisObj];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetVisibleUI:(GLboolean)fVisible
{
    COCOA_LOG_FLOW(("%s: self=%p fVisible=%d\n", __PRETTY_FUNCTION__, (void *)self, fVisible));

    [self setHidden: !fVisible];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxSetVisibleUIObj:(NSNumber *)pVisibleObj
{
    COCOA_LOG_FLOW(("%s: self=%p pVisibleObj=%p(%d)\n", __PRETTY_FUNCTION__, 
                    (void *)self, (void *)pVisibleObj, [pVisibleObj boolValue]));

    BOOL fVisible = [pVisibleObj boolValue];
    [self vboxSetVisibleUI:fVisible];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxReparent:(NSView *)pParentView
{
    COCOA_LOG_FLOW(("%s: self=%p pParentView=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pParentView));

    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    [pRunner addObj:self selector:@selector(vboxReparentUI:) arg:pParentView];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxReparentUI:(NSView *)pParentView
{
    COCOA_LOG_FLOW(("%s: self=%p pParentView=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pParentView));

    /* Make sure the window is removed from any previous parent window. */
    if ([[self overlayWin] parentWindow] != nil)
    {
        [[[self overlayWin] parentWindow] removeChildWindow:[self overlayWin]];
    }

    /* Set the new parent view */
    [self setParentView: pParentView];

    /* Add the overlay window as a child to the new parent window */
    if (pParentView != nil)
    {
        [[pParentView window] addChildWindow:[self overlayWin] ordered:NSWindowAbove];
        if ([self isEverSized])
            [self vboxReshapeOnReparentPerform];
    }
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxTryDrawUI
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    
    if ([self isHidden])
    {
        COCOA_LOG_FLOW(("%s: returns - request to draw on a hidden view\n", __PRETTY_FUNCTION__));
        return;
    }

    if ([[self overlayWin] parentWindow] == nil)
    {
        COCOA_LOG_FLOW(("%s: returns - request to draw a view w/o a parent\n", __PRETTY_FUNCTION__));
        return;
    }

#ifdef IN_VMSVGA3D
    if (!m_pSharedGLCtx)
    {
        Assert(!m_fDataVisible);
        Assert(!m_fCleanupNeeded);
        if (![self vboxSharedCtxCreate])
        {
            COCOA_LOG_FLOW(("%s: returns - vboxSharedCtxCreate failed\n", __PRETTY_FUNCTION__));
            return;
        }
        Assert(m_pSharedGLCtx);
    }
#endif
    
    const VBOXVR_SCR_COMPOSITOR *pCompositor = NULL;
#ifndef IN_VMSVGA3D
    int rc = renderspuVBoxCompositorLock(m_pWinInfo, &pCompositor);
    if (RT_FAILURE(rc))
    {
        COCOA_LOG_FLOW(("%s: returns - renderspuVBoxCompositorLock failed (%Rrc)\n", __PRETTY_FUNCTION__, rc));
        return;
    }

    if (!pCompositor && !m_fCleanupNeeded)
    {
        renderspuVBoxCompositorUnlock(m_pWinInfo);
        COCOA_LOG_FLOW(("%s: returns - noCompositorUI\n", __PRETTY_FUNCTION__));
        return;
    }

    VBOXVR_SCR_COMPOSITOR TmpCompositor;
    if (pCompositor)
    {
        if (!m_pSharedGLCtx)
        {
            Assert(!m_fDataVisible);
            Assert(!m_fCleanupNeeded);
            renderspuVBoxCompositorRelease(m_pWinInfo);
            if (![self vboxSharedCtxCreate])
            {
                COCOA_LOG_FLOW(("%s: returns - vboxSharedCtxCreate failed\n", __PRETTY_FUNCTION__));
                return;
            }
            
            Assert(m_pSharedGLCtx);
            
            pCompositor = renderspuVBoxCompositorAcquire(m_pWinInfo);
            Assert(!m_fDataVisible);
            Assert(!m_fCleanupNeeded);
            if (!pCompositor)
            {
                COCOA_LOG_FLOW(("%s: returns - Failed to reacquire compositor\n", __PRETTY_FUNCTION__));
                return;
            }
        }
    }
    else
    {
        DEBUG_MSG(("%s: NeedCleanup\n", __PRETTY_FUNCTION__));
        Assert(m_fCleanupNeeded);
        CrVrScrCompositorInit(&TmpCompositor, NULL);
        pCompositor = &TmpCompositor;
    }
#endif /* !IN_VMSVGA3D */

    
    if ([self lockFocusIfCanDraw])
    {
        COCOA_LOG_FLOW(("%s: Calling vboxPresent\n", __PRETTY_FUNCTION__));
        [self vboxPresent:pCompositor];            
        [self unlockFocus];
    }
#ifndef IN_VMSVGA3D /** @todo VMSVGA3 */
    else if (!m_pWinInfo->visible)
    {
        COCOA_LOG_FLOW(("%s: NotVisible\n", __PRETTY_FUNCTION__));
        m_fCleanupNeeded = false;
    }
#endif    
    else
    {
        COCOA_LOG_FLOW(("%s: Reschedule\n", __PRETTY_FUNCTION__));
        [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(vboxTryDrawUI) userInfo:nil repeats:NO];
    }
    
#ifndef IN_VMSVGA3D
    renderspuVBoxCompositorUnlock(m_pWinInfo);
#endif
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)swapFBO
{
    COCOA_LOG_FLOW(("%s: self=%p - m_pGLCtx=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)m_pGLCtx));
    [m_pGLCtx flushBuffer];
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxPresent:(PCVBOXVR_SCR_COMPOSITOR)pCompositor
{
    COCOA_LOG_FLOW(("%s: self=%p pCompositor=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pCompositor));
    /*DEBUG_MSG(("OVIW(%p): renderFBOToView\n", (void *)self));*/
#ifndef IN_VMSVGA3D
    AssertPtr(pCompositor);
#endif

    VBOX_CR_RENDER_CTX_INFO CtxInfo;    
    vboxCtxEnter(m_pSharedGLCtx, &CtxInfo);
    
    [self vboxPresentCS:pCompositor];
    
    vboxCtxLeave(&CtxInfo);
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)vboxPresentCS:(PCVBOXVR_SCR_COMPOSITOR)pCompositor
{
    COCOA_LOG_FLOW(("%s: self=%p pCompositor=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pCompositor));
    if ([m_pSharedGLCtx view] != self)
    {
        COCOA_LOG_FLOW(("%s: Not current view of shared ctx! Switching... (self=%p, view=%p, m_pSharedGLCtx)\n", 
                        __PRETTY_FUNCTION__, (void *)self, (void *)[m_pSharedGLCtx view], (void *)m_pSharedGLCtx));
        [m_pSharedGLCtx setView: self];
        m_fNeedViewportUpdate = true;
    }
    
    if (m_fNeedViewportUpdate)
    {
        [self updateViewportCS];
        m_fNeedViewportUpdate = false;
    }
    
    m_fCleanupNeeded = false;
    
#ifndef IN_VMSVGA3D
    /* Render FBO content to the dock tile when necessary. */
    [self vboxPresentToDockTileCS:pCompositor];
#endif

    /* change to #if 0 to see thumbnail image */            
#if 1
    [self vboxPresentToViewCS:pCompositor];
#else
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    [m_pSharedGLCtx flushBuffer];
#endif

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

DECLINLINE(void) vboxNSRectToRect(const NSRect *pR, RTRECT *pRect)
{
    pRect->xLeft   = (int)pR->origin.x;
    pRect->yTop    = (int)pR->origin.y;
    pRect->xRight  = (int)(pR->origin.x + pR->size.width);
    pRect->yBottom = (int)(pR->origin.y + pR->size.height);
}

DECLINLINE(void) vboxNSRectToRectUnstretched(const NSRect *pR, RTRECT *pRect, float xStretch, float yStretch)
{
    pRect->xLeft   = (int)(pR->origin.x / xStretch);
    pRect->yTop    = (int)(pR->origin.y / yStretch);
    pRect->xRight  = (int)((pR->origin.x + pR->size.width) / xStretch);
    pRect->yBottom = (int)((pR->origin.y + pR->size.height) / yStretch);
}

DECLINLINE(void) vboxNSRectToRectStretched(const NSRect *pR, RTRECT *pRect, float xStretch, float yStretch)
{
    pRect->xLeft   = (int)(pR->origin.x * xStretch);
    pRect->yTop    = (int)(pR->origin.y * yStretch);
    pRect->xRight  = (int)((pR->origin.x + pR->size.width) * xStretch);
    pRect->yBottom = (int)((pR->origin.y + pR->size.height) * yStretch);
}

- (void)vboxPresentToViewCS:(PCVBOXVR_SCR_COMPOSITOR)pCompositor
{
    NSRect r = [self frame];
    COCOA_LOG_FLOW(("%s: self=%p - r={%d,%d %d,%d}\n", __PRETTY_FUNCTION__, (void *)self, 
                    (int)r.origin.x, (int)r.origin.y, (int)r.size.width, (int)r.size.height));

#if 1 /* Set to 0 to see the docktile instead of the real output */
    float backingStretchFactor = 1.;
#  if defined(VBOX_WITH_CONFIGURABLE_HIDPI_SCALING) && !defined(IN_VMSVGA3D)
    /* Adjust viewport according to current NSView's backing store parameters. */
    if (render_spu.fUnscaledHiDPI)
    {
        /* Update stretch factor in order to satisfy current NSView's backing store parameters. */
        backingStretchFactor = [self safeGetBackingScaleFactor];
    }

    NSRect regularBounds = [self bounds];
    NSRect backingBounds = [self safeConvertRectToBacking:&regularBounds];
    glViewport(0, 0, backingBounds.size.width, backingBounds.size.height);

    //crDebug("HiDPI: vboxPresentToViewCS: up-scaling is %s (backingStretchFactor=%d).",
    //    render_spu.fUnscaledHiDPI ? "OFF" : "ON", (int)backingStretchFactor);
#  endif

    glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
    glDrawBuffer(GL_BACK);

    /* Clear background to transparent */
    glClear(GL_COLOR_BUFFER_BIT);
    
    m_fDataVisible = false;
    
# ifndef IN_VMSVGA3D
    float xStretch;
    float yStretch;
    CrVrScrCompositorGetStretching(pCompositor, &xStretch, &yStretch);
        
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR CIter;
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;
    CrVrScrCompositorConstIterInit(pCompositor, &CIter);

    while ((pEntry = CrVrScrCompositorConstIterNext(&CIter)) != NULL)
    {
        uint32_t cRegions;
        const RTRECT *paSrcRegions, *paDstRegions;
        int rc = CrVrScrCompositorEntryRegionsGet(pCompositor, pEntry, &cRegions, &paSrcRegions, &paDstRegions, NULL);
        uint32_t fFlags = CrVrScrCompositorEntryFlagsCombinedGet(pCompositor, pEntry);
        if (RT_SUCCESS(rc))
        {
            rc = CrBltEnter(m_pBlitter);
            if (RT_SUCCESS(rc))
            {                   
                uint32_t i;
                for (i = 0; i < cRegions; ++i)
                {
                    const CR_TEXDATA *pTexData;
                    PCRTRECT pSrcRect = &paSrcRegions[i];
                    PCRTRECT pDstRect = &paDstRegions[i];
                    RTRECT   DstRect, RestrictDstRect;
                    RTRECT   SrcRect, RestrictSrcRect;

                    vboxNSRectToRect(&m_RootRect, &RestrictDstRect);
                    VBoxRectIntersected(&RestrictDstRect, pDstRect, &DstRect);
                    
                    if (VBoxRectIsZero(&DstRect))
                        continue;

                    VBoxRectTranslate(&DstRect, -RestrictDstRect.xLeft, -RestrictDstRect.yTop);

                    vboxNSRectToRectUnstretched(&m_RootRect, &RestrictSrcRect, xStretch / backingStretchFactor, yStretch / backingStretchFactor);
                    VBoxRectTranslate(&RestrictSrcRect, 
                                      -CrVrScrCompositorEntryRectGet(pEntry)->xLeft, 
                                      -CrVrScrCompositorEntryRectGet(pEntry)->yTop);
                    VBoxRectIntersected(&RestrictSrcRect, pSrcRect, &SrcRect);
                    
                    if (VBoxRectIsZero(&SrcRect))
                        continue;

                    pSrcRect = &SrcRect;
                    pDstRect = &DstRect;
                    
                    pTexData = CrVrScrCompositorEntryTexGet(pEntry);
                    
                    CrBltBlitTexMural(m_pBlitter, true, CrTdTexGet(pTexData), pSrcRect, pDstRect, 1, fFlags | CRBLT_F_NOALPHA);
                    
                    m_fDataVisible = true;
                }
                CrBltLeave(m_pBlitter);
            }
            else
            {
                DEBUG_WARN(("CrBltEnter failed rc %d", rc));
#  ifndef DEBUG_VERBOSE
                AssertMsgFailed(("CrBltEnter failed rc %Rrc", rc));
#  endif
            }
        }
        else
        {
            AssertMsgFailed(("BlitStretched: CrVrScrCompositorEntryRegionsGet failed rc %Rrc\n", rc));
            DEBUG_MSG_1(("BlitStretched: CrVrScrCompositorEntryRegionsGet failed rc %d\n", rc));
        }
    }
# endif /* !IN_VMSVGA3D */
#endif

    /*
    glFinish();
    */
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    [m_pSharedGLCtx flushBuffer];

    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

- (void)presentComposition:(PCVBOXVR_SCR_COMPOSITOR_ENTRY)pChangedEntry
{
    COCOA_LOG_FLOW(("%s: self=%p pChangedEntry=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pChangedEntry));
    [self vboxTryDraw];
}

#ifndef IN_VMSVGA3D
- (void)vboxBlitterSyncWindow
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    CR_BLITTER_WINDOW   WinInfo;
    NSRect              r;
    
    if (!m_pBlitter)
        return;
        
    RT_ZERO(WinInfo);
    
    r = [self frame];
    WinInfo.width = r.size.width;
    WinInfo.height = r.size.height;
    
    Assert(WinInfo.width == m_RootRect.size.width);
    Assert(WinInfo.height == m_RootRect.size.height);

    /*CrBltMuralSetCurrentInfo(m_pBlitter, NULL);*/
    
    CrBltMuralSetCurrentInfo(m_pBlitter, &WinInfo);
    CrBltCheckUpdateViewport(m_pBlitter);
}
#endif /* !IN_VMSVGA3D */

#ifndef IN_VMSVGA3D
# ifdef VBOX_WITH_CRDUMPER_THUMBNAIL
static int g_cVBoxTgaCtr = 0;
# endif
- (void)vboxPresentToDockTileCS:(PCVBOXVR_SCR_COMPOSITOR)pCompositor
{
    COCOA_LOG_FLOW(("%s: self=%p pCompositor=%p\n", __PRETTY_FUNCTION__, (void *)self, (void *)pCompositor));
    NSRect      r   = [self frame];
    NSRect      rr  = NSZeroRect;
    NSDockTile *pDT = nil;
    float       xStretch;
    float       yStretch;

    if ([m_DockTileView thumbBitmap] != nil)
    {
        /* 
         * Only update after at least 200 ms, cause glReadPixels is
         * heavy performance wise. 
         */ 
        uint64_t                                msTS = RTTimeSystemMilliTS();
        VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR    CIter;
        const VBOXVR_SCR_COMPOSITOR_ENTRY      *pEntry;
        
        if (msTS - m_msDockUpdateTS > 200)
        {
            m_msDockUpdateTS = msTS;
# if 0
            /* todo: check this for optimization */
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, myTextureName);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_STORAGE_HINT_APPLE,
                            GL_STORAGE_SHARED_APPLE);
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
            glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA,
                         sizex, sizey, 0, GL_BGRA,
                         GL_UNSIGNED_INT_8_8_8_8_REV, myImagePtr);
            glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,
                                0, 0, 0, 0, 0, image_width, image_height);
            glFlush();
            /* Do other work processing here, using a double or triple buffer */
            glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_BGRA,
                          GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
# endif
            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
            glDrawBuffer(GL_BACK);

            /* Clear background to transparent */
            glClear(GL_COLOR_BUFFER_BIT);

            rr = [m_DockTileView frame];
            
            CrVrScrCompositorGetStretching(pCompositor, &xStretch, &yStretch);
            
            CrVrScrCompositorConstIterInit(pCompositor, &CIter);
            while ((pEntry = CrVrScrCompositorConstIterNext(&CIter)) != NULL)
            {
                uint32_t    cRegions;
                PCRTRECT    paSrcRegions;
                PCRTRECT    paDstRegions;
                int rc = CrVrScrCompositorEntryRegionsGet(pCompositor, pEntry, &cRegions, &paSrcRegions, &paDstRegions, NULL);
                uint32_t    fFlags = CrVrScrCompositorEntryFlagsCombinedGet(pCompositor, pEntry);
                if (RT_SUCCESS(rc))
                {
                    rc = CrBltEnter(m_pBlitter);
                    if (RT_SUCCESS(rc))
                    {                   
                        uint32_t i;
                        for (i = 0; i < cRegions; ++i)
                        {
                            const CR_TEXDATA *pTexData;
                            PCRTRECT pSrcRect = &paSrcRegions[i];
                            PCRTRECT pDstRect = &paDstRegions[i];
                            RTRECT   DstRect, RestrictDstRect;
                            RTRECT   SrcRect, RestrictSrcRect;
                    
                            vboxNSRectToRect(&m_RootRect, &RestrictDstRect);
                            VBoxRectIntersected(&RestrictDstRect, pDstRect, &DstRect);
                            
                            VBoxRectTranslate(&DstRect, -RestrictDstRect.xLeft, -RestrictDstRect.yTop);
                            
                            VBoxRectScale(&DstRect, m_FBOThumbScaleX, m_FBOThumbScaleY);
                    
                            if (VBoxRectIsZero(&DstRect))
                                continue;
                        
                            vboxNSRectToRectUnstretched(&m_RootRect, &RestrictSrcRect, xStretch, yStretch);
                            VBoxRectTranslate(&RestrictSrcRect, 
                                              -CrVrScrCompositorEntryRectGet(pEntry)->xLeft, 
                                              -CrVrScrCompositorEntryRectGet(pEntry)->yTop);
                            VBoxRectIntersected(&RestrictSrcRect, pSrcRect, &SrcRect);
                    
                            if (VBoxRectIsZero(&SrcRect))
                                continue;

                            pSrcRect = &SrcRect;
                            pDstRect = &DstRect;
                            
                            pTexData = CrVrScrCompositorEntryTexGet(pEntry);
                            
                            CrBltBlitTexMural(m_pBlitter, true, CrTdTexGet(pTexData), pSrcRect, pDstRect, 1, fFlags);
                        }
                        CrBltLeave(m_pBlitter);
                    }
                    else
                    {
                        DEBUG_WARN(("CrBltEnter failed rc %d", rc));
# ifndef DEBUG_VERBOSE
                        AssertMsgFailed(("CrBltEnter failed rc %Rrc", rc));
# endif
                    }
                }
                else
                {
                    DEBUG_MSG_1(("BlitStretched: CrVrScrCompositorEntryRegionsGet failed rc %d\n", rc));
                    AssertMsgFailed(("BlitStretched: CrVrScrCompositorEntryRegionsGet failed rc %Rrc\n", rc));
                }
            }
            
            glFinish();

            glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
            glReadBuffer(GL_BACK);

            /* Here the magic of reading the FBO content in our own buffer
             * happens. We have to lock this access, in the case the dock
             * is updated currently. */
            [m_DockTileView lock];
            glReadPixels(0, m_RootRect.size.height - rr.size.height, rr.size.width, rr.size.height,
                         GL_BGRA,
                         GL_UNSIGNED_INT_8_8_8_8,
                         [[m_DockTileView thumbBitmap] bitmapData]);
            [m_DockTileView unlock];
            
# ifdef VBOX_WITH_CRDUMPER_THUMBNAIL
            ++g_cVBoxTgaCtr;
            crDumpNamedTGAF((GLint)rr.size.width, (GLint)rr.size.height, 
                [[m_DockTileView thumbBitmap] bitmapData], "/Users/leo/vboxdumps/dump%d.tga", g_cVBoxTgaCtr);
# endif                

            pDT = [[NSApplication sharedApplication] dockTile];

            /* Send a display message to the dock tile in the main thread */
            [[[NSApplication sharedApplication] dockTile] performSelectorOnMainThread:@selector(display) withObject:nil 
                                                                        waitUntilDone:NO];
        }
    }
}
#endif /* !IN_VMSVGA3D */

- (void)clearVisibleRegions
{
    if (m_paClipRects)
    {
        RTMemFree(m_paClipRects);
        m_paClipRects = NULL;
    }
    m_cClipRects = 0;
}

- (GLboolean)vboxNeedsEmptyPresent
{
    if (m_fDataVisible)
    {
        m_fCleanupNeeded = true;
        return GL_TRUE;
    }
    
    return GL_FALSE;
}

- (void)setVisibleRegions:(GLint)cRects paRects:(const GLint *)paRects
{
    COCOA_LOG_FLOW(("%s: self=%p cRects=%d paRects=%p\n", __PRETTY_FUNCTION__, (void *)self, cRects, (void *)paRects));
    GLint cOldRects = m_cClipRects;

    [self clearVisibleRegions];

    if (cRects > 0)
    {
#ifdef DEBUG_poetzsch
        int i = 0;
        for (i = 0; i < cRects; ++i)
            DEBUG_MSG_1(("OVIW(%p): setVisibleRegions: %d - %d %d %d %d\n", (void *)self, i, paRects[i * 4], paRects[i * 4 + 1], paRects[i * 4 + 2], paRects[i * 4 + 3]));
#endif

        m_paClipRects = (GLint *)RTMemDup(paRects, sizeof(GLint) * 4 * cRects);
        m_cClipRects  = cRects;
    }
    
    COCOA_LOG_FLOW(("%s: returns\n", __PRETTY_FUNCTION__));
}

#ifndef IN_VMSVGA3D

- (NSView *)dockTileScreen
{
    COCOA_LOG_FLOW(("%s: self=%p\n", __PRETTY_FUNCTION__, (void *)self));
    NSView *pContentView = [[[NSApplication sharedApplication] dockTile] contentView];
    NSView *pScreenContent = nil;

    /* 
     * First try the new variant which checks if this window is within the
     * screen which is previewed in the dock. 
     */ 
    if ([pContentView respondsToSelector:@selector(screenContentWithParentView:)])
        pScreenContent = [pContentView performSelector:@selector(screenContentWithParentView:) withObject:(id)m_pParentView];
    /* 
     * If it fails, fall back to the old variant (VBox...).
     */
    else if ([pContentView respondsToSelector:@selector(screenContent)])
        pScreenContent = [pContentView performSelector:@selector(screenContent)];
    
    COCOA_LOG_FLOW(("%s: returns %p (pContentView=%p)\n", __PRETTY_FUNCTION__, (void *)pScreenContent, (void *)pContentView));
    return pScreenContent;
}

- (void)reshapeDockTile
{
    COCOA_LOG_FLOW(("%s:\n", __PRETTY_FUNCTION__));
    NSRect newFrame = NSZeroRect;
    NSView *pView = [self dockTileScreen];
    if (pView != nil)
    {
        NSRect dockFrame = [pView frame];
        /** @todo This is not correct, we should use framebuffer size here, while
         *   parent view frame size may differ in case of scrolling. */
        NSRect parentFrame = [m_pParentView frame];

        m_FBOThumbScaleX = (float)dockFrame.size.width / parentFrame.size.width;
        m_FBOThumbScaleY = (float)dockFrame.size.height / parentFrame.size.height;
        newFrame = NSMakeRect((int)(m_Pos.x * m_FBOThumbScaleX), 
                              (int)(dockFrame.size.height - (m_Pos.y + m_Size.height - m_yInvRootOffset) * m_FBOThumbScaleY), 
                              (int)(m_Size.width * m_FBOThumbScaleX), 
                              (int)(m_Size.height * m_FBOThumbScaleY));
        /*
        NSRect newFrame = NSMakeRect ((int)roundf(m_Pos.x * m_FBOThumbScaleX), (int)roundf(dockFrame.size.height - (m_Pos.y + m_Size.height) * m_FBOThumbScaleY), (int)roundf(m_Size.width * m_FBOThumbScaleX), (int)roundf(m_Size.height * m_FBOThumbScaleY));
        NSRect newFrame = NSMakeRect ((m_Pos.x * m_FBOThumbScaleX), (dockFrame.size.height - (m_Pos.y + m_Size.height) * m_FBOThumbScaleY), (m_Size.width * m_FBOThumbScaleX), (m_Size.height * m_FBOThumbScaleY));
        printf ("%f %f %f %f - %f %f\n", newFrame.origin.x, newFrame.origin.y, newFrame.size.width, newFrame.size.height, m_Size.height, m_FBOThumbScaleY);
        */
        [m_DockTileView setFrame: newFrame];
    }
    COCOA_LOG_FLOW(("%s: returns - newFrame={%d,%d %d,%d} pView=%d\n", __PRETTY_FUNCTION__,  (int)newFrame.origin.x, 
                    (int)newFrame.origin.y, (int)newFrame.size.width, (int)newFrame.size.height, (void *)pView));
}

#endif /* !IN_VMSVGA3D */

@end /* @implementation OverlayView */


/********************************************************************************
*
* OpenGL context management
*
********************************************************************************/
void cocoaGLCtxCreate(NativeNSOpenGLContextRef *ppCtx, GLbitfield fVisParams, NativeNSOpenGLContextRef pSharedCtx)
{
    COCOA_LOG_FLOW(("cocoaGLCtxCreate: ppCtx=%p fVisParams=%#x pSharedCtx=%p\n", (void *)ppCtx, fVisParams, (void *)pSharedCtx));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    NSOpenGLPixelFormat *pFmt = vboxCreatePixelFormat(fVisParams);
    if (pFmt)
    {
        *ppCtx = [[OverlayOpenGLContext alloc] initWithFormat:pFmt shareContext:pSharedCtx];
        Assert(*ppCtx);

        /* Enable multi threaded OpenGL engine */
        /*
        CGLContextObj cglCtx = [*ppCtx CGLContextObj];
        CGLError err = CGLEnable(cglCtx, kCGLCEMPEngine);
        if (err != kCGLNoError)
            printf ("Couldn't enable MT OpenGL engine!\n");
        */
    }
    else
    {
        AssertFailed();
        *ppCtx = NULL;
    }

    [pPool release];
    COCOA_LOG_FLOW(("cocoaGLCtxCreate: returns *ppCtx=%p\n", (void *)*ppCtx));
}

void cocoaGLCtxDestroy(NativeNSOpenGLContextRef pCtx)
{
    COCOA_LOG_FLOW(("cocoaGLCtxDestroy: pCtx=%p\n", (void *)pCtx));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    [pCtx release];
    /*[pCtx performSelectorOnMainThread:@selector(release) withObject:nil waitUntilDone:NO];*/

    [pPool release];
    COCOA_LOG_FLOW(("cocoaGLCtxDestroy: returns\n"));
}

/********************************************************************************
*
* View management
*
********************************************************************************/
static OverlayView *vboxViewCreate(WindowInfo *pWinInfo, NativeNSViewRef pParentView, GLbitfield fVisParams)
{
    COCOA_LOG_FLOW(("vboxViewCreate: pWinInfo=%p pParentView=%p fVisParams=%#x\n", pWinInfo, (void *)pParentView, fVisParams));

    /* Create our worker view. */
    OverlayView *pView = [[OverlayView alloc] initWithFrame:NSZeroRect 
                                                     thread:RTThreadSelf() 
                                                 parentView:pParentView 
                                                    winInfo:pWinInfo
                                                 fVisParams:fVisParams];

    if (pView)
    {
        /* We need a real window as container for the view */
        [[OverlayWindow alloc] initWithParentView:pParentView overlayView:pView];
        /* Return the freshly created overlay view */
        COCOA_LOG_FLOW(("vboxViewCreate: returns %p\n", (void *)pView));
        return pView;
    }
    
    COCOA_LOG_FLOW(("vboxViewCreate: returns NULL\n"));
    return NULL;
}

#ifndef IN_VMSVGA3D

typedef struct CR_RCD_CREATEVIEW
{
    WindowInfo *pWinInfo;
    NSView *pParentView;
    GLbitfield fVisParams;
    /* out */
    OverlayView *pView;
} CR_RCD_CREATEVIEW;

static DECLCALLBACK(void) vboxRcdCreateView(void *pvCb)
{
    CR_RCD_CREATEVIEW *pCreateView = (CR_RCD_CREATEVIEW *)pvCb;
    pCreateView->pView = vboxViewCreate(pCreateView->pWinInfo, pCreateView->pParentView, pCreateView->fVisParams);
    COCOA_LOG_FLOW(("vboxRcdCreateView: returns pView=%p\n", (void *)pCreateView->pView));
}

#endif /* !IN_VMSVGA3D */

void cocoaViewCreate(NativeNSViewRef *ppView, WindowInfo *pWinInfo, NativeNSViewRef pParentView, GLbitfield fVisParams)
{
    COCOA_LOG_FLOW(("cocoaViewCreate: ppView=%p pWinInfo=%p pParentView=%p fVisParams=%#x\n", 
                    (void *)ppView, (void *)pWinInfo, (void *)pParentView, fVisParams));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    /* make sure all tasks are run, to preserve the order */
    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    [pRunner runTasksSyncIfPossible];
    
#ifndef IN_VMSVGA3D
    renderspuWinRetain(pWinInfo);

    if (renderspuCalloutAvailable())
    {
        CR_RCD_CREATEVIEW CreateView;
        CreateView.pWinInfo = pWinInfo;
        CreateView.pParentView = pParentView;
        CreateView.fVisParams = fVisParams;
        CreateView.pView = NULL;
        renderspuCalloutClient(vboxRcdCreateView, &CreateView);
        *ppView = CreateView.pView;
    }
    else
#endif
    {
        DEBUG_MSG_NOT_VMSVGA3D(("no callout available on createWindow\n"));
#if 0
        dispatch_sync(dispatch_get_main_queue(), ^{
#endif
            *ppView = vboxViewCreate(pWinInfo, pParentView, fVisParams);
#if 0
        });
#endif
    }
    
#ifndef IN_VMSVGA3D
    if (!*ppView)
        renderspuWinRelease(pWinInfo);
#endif
    
    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewCreate: returns *ppView=%p\n", (void *)*ppView));
}

#ifndef IN_VMSVGA3D
void cocoaViewReparent(NativeNSViewRef pView, NativeNSViewRef pParentView)
{
    COCOA_LOG_FLOW(("cocoaViewReparent: pView=%p pParentView=%p\n", (void *)pView, (void *)pParentView));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    OverlayView *pOView = (OverlayView *)pView;
    if (pOView)
        [pOView vboxReparent:pParentView];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewReparent: returns\n"));
}
#endif /* !IN_VMSVGA3D */

void cocoaViewDestroy(NativeNSViewRef pView)
{
    COCOA_LOG_FLOW(("cocoaViewDestroy: pView=%p\n", (void *)pView));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    [pRunner addObj:pView selector:@selector(vboxDestroy) arg:nil];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewDestroy: returns\n"));
}

#ifndef IN_VMSVGA3D
void cocoaViewShow(NativeNSViewRef pView, GLboolean fShowIt)
{
    COCOA_LOG_FLOW(("cocoaViewShow: pView=%p fShowIt=%d\n", (void *)pView, fShowIt));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    [(OverlayView *)pView vboxSetVisible:fShowIt];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewShow: returns\n"));
}
#endif /* IN_VMSVGA3D */

void cocoaViewDisplay(NativeNSViewRef pView)
{
    COCOA_LOG_FLOW(("cocoaViewDisplay: pView=%p\n", (void *)pView));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

#ifndef IN_VMSVGA3D
    DEBUG_WARN(("cocoaViewDisplay should never happen!\n"));
    DEBUG_MSG_1(("cocoaViewDisplay %p\n", (void *)pView));
#endif
    [(OverlayView *)pView swapFBO];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewDisplay: returns\n"));
}

void cocoaViewSetPosition(NativeNSViewRef pView, NativeNSViewRef pParentView, int x, int y)
{
    COCOA_LOG_FLOW(("cocoaViewSetPosition: pView=%p pParentView=%p x=%d y=%d\n", (void *)pView, (void *)pParentView, x, y));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    [(OverlayView *)pView vboxSetPos:NSMakePoint(x, y)];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewSetPosition: returns\n"));
}

void cocoaViewSetSize(NativeNSViewRef pView, int cx, int cy)
{
    COCOA_LOG_FLOW(("cocoaViewSetSize: pView=%p cx=%d cy=%d\n", (void *)pView, cx, cy));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    OverlayView *pOverlayView = (OverlayView *)pView;

    [pOverlayView vboxSetSize:NSMakeSize(cx, cy)];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewSetSize: returns\n"));
}

#ifndef IN_VMSVGA3D

typedef struct CR_RCD_GETGEOMETRY
{
    OverlayView    *pView;
    NSRect          rect;
} CR_RCD_GETGEOMETRY;

static DECLCALLBACK(void) vboxRcdGetGeomerty(void *pvUser)
{
    CR_RCD_GETGEOMETRY *pGetGeometry = (CR_RCD_GETGEOMETRY *)pvUser;
    pGetGeometry->rect = [[pGetGeometry->pView window] frame];
    COCOA_LOG_FLOW(("vboxRcdGetGeomerty: (x,y)=(%d,%d) (cx,cy)=(%d,%d)\n", pGetGeometry->rect.origin.x, pGetGeometry->rect.origin.y, 
                    pGetGeometry->rect.size.width, pGetGeometry->rect.size.height));
}

void cocoaViewGetGeometry(NativeNSViewRef pView, int *px, int *py, int *pcx, int *pcy)
{
    COCOA_LOG_FLOW(("cocoaViewGetGeometry: pView=%p px=%p py=%p pcx=%p pcy=%p\n", 
                    (void *)pView, (void *)px, (void *)py, (void *)pcx, (void *)pcy));
    NSAutoreleasePool *pPool;
    pPool = [[NSAutoreleasePool alloc] init];

    /* make sure all tasks are run, to preserve the order */
    VBoxMainThreadTaskRunner *pRunner = [VBoxMainThreadTaskRunner globalInstance];
    [pRunner runTasksSyncIfPossible];
    
    NSRect frame;
#ifndef IN_VMSVGA3D
    if (renderspuCalloutAvailable())
    {
        CR_RCD_GETGEOMETRY GetGeometry;
        GetGeometry.pView = (OverlayView *)pView;
        renderspuCalloutClient(vboxRcdGetGeomerty, &GetGeometry);
        frame = GetGeometry.rect;
    }
    else
#endif
    {
        DEBUG_MSG_NOT_VMSVGA3D(("no callout available on getGeometry\n"));
        frame = [[pView window] frame];
    }
    
    *px  = frame.origin.x;
    *py  = frame.origin.y;
    *pcx = frame.size.width;
    *pcy = frame.size.height;

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewGetGeometry: returns *px=%d, *py=%d, *pcx=%d, *pcy=%d\n", *px, *py, *pcx, *pcy));
}

void cocoaViewPresentComposition(NativeNSViewRef pView, PCVBOXVR_SCR_COMPOSITOR_ENTRY pChangedEntry)
{
    COCOA_LOG_FLOW(("cocoaViewPresentComposition: pView=%p pChangedEntry=%p\n", (void *)pView, (void *)pChangedEntry));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    NSOpenGLContext *pCtx;

# ifdef IN_VMSVGA3D
    Assert([(OverlayView *)pView glCtx]);

# else
    /* The view may not necesserily have a GL context set. */
    pCtx = [(OverlayView *)pView glCtx];
    if (!pCtx)
    {
        ContextInfo *pCtxInfo = renderspuDefaultSharedContextAcquire();
        if (!pCtxInfo)
        {
            DEBUG_WARN(("renderspuDefaultSharedContextAcquire returned NULL"));
            
            [pPool release];
            DEBUG_FUNC_LEAVE();
            return;
        }
        
        pCtx = pCtxInfo->context;
        
        [(OverlayView *)pView setGLCtx:pCtx];
    }
# endif
    
    [(OverlayView *)pView presentComposition:pChangedEntry];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewPresentComposition: returns\n"));
}

#endif /* !IN_VMSVGA3D */

void cocoaViewMakeCurrentContext(NativeNSViewRef pView, NativeNSOpenGLContextRef pCtx)
{
    COCOA_LOG_FLOW(("cocoaViewMakeCurrentContext: pView=%p pCtx=%p\n", (void *)pView, (void *)pCtx));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    if (pView)
    {
        [(OverlayView *)pView setGLCtx:pCtx];
        [(OverlayView *)pView makeCurrentFBO];
    }
    else
    {
#ifdef IN_VMSVGA3D
        /* Always flush before flush. glXMakeCurrent and wglMakeCurrent does this
           implicitly, seemingly NSOpenGLContext::makeCurrentContext doesn't. */
        if ([NSOpenGLContext currentContext] != nil)
        {
            DEBUG_CLEAR_GL_ERRORS();
            glFlush();
            DEBUG_CHECK_GL_ERROR("glFlush");
        }
#endif
    	[NSOpenGLContext clearCurrentContext];
    }

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewMakeCurrentContext: returns\n"));
}

#ifndef IN_VMSVGA3D

GLboolean cocoaViewNeedsEmptyPresent(NativeNSViewRef pView)
{
    COCOA_LOG_FLOW(("cocoaViewNeedsEmptyPresent: pView=%p\n", (void *)pView));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    GLboolean fNeedsPresent = [(OverlayView *)pView vboxNeedsEmptyPresent];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewNeedsEmptyPresent: returns %d\n", fNeedsPresent));
    return fNeedsPresent;
}

void cocoaViewSetVisibleRegion(NativeNSViewRef pView, GLint cRects, const GLint *paRects)
{
    COCOA_LOG_FLOW(("cocoaViewSetVisibleRegion: pView=%p cRects=%d paRects=%p)\n", (void *)pView, cRects, (void const *)paRects));
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

    [(OverlayView *)pView setVisibleRegions:cRects paRects:paRects];

    [pPool release];
    COCOA_LOG_FLOW(("cocoaViewSetVisibleRegion: returns\n"));
}

#endif /* IN_VMSVGA3D */

#ifdef IN_VMSVGA3D
/*
 * VMSVGA3D interface.
 */

VMSVGA3DCOCOA_DECL(bool) vmsvga3dCocoaCreateViewAndContext(NativeNSViewRef *ppView, NativeNSOpenGLContextRef *ppCtx,
                                                           NativeNSViewRef pParentView, uint32_t cx, uint32_t cy,
                                                           NativeNSOpenGLContextRef pSharedCtx, bool fOtherProfile)
{
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    GLbitfield fVisParams = CR_ALPHA_BIT | CR_DEPTH_BIT | CR_DOUBLE_BIT | (fOtherProfile ? VMSVGA3D_NON_DEFAULT_PROFILE_BIT : 0);
    bool fRc = false;

    cocoaGLCtxCreate(ppCtx, fVisParams, pSharedCtx);
    if (*ppCtx)
    {
        cocoaViewCreate(ppView, NULL, pParentView, fVisParams);
        if (*ppView)
        {
            cocoaViewSetSize(*ppView, cx, cy);
            [(OverlayView *)*ppView setGLCtx: *ppCtx];
            fRc = true;
        }
        else
            [*ppCtx release];
    }

    [pPool release];
    return fRc;
}

VMSVGA3DCOCOA_DECL(void) vmsvga3dCocoaDestroyViewAndContext(NativeNSViewRef pView, NativeNSOpenGLContextRef pCtx)
{
    cocoaGLCtxDestroy(pCtx);
    cocoaViewDestroy(pView);
}

VMSVGA3DCOCOA_DECL(void) vmsvga3dCocoaViewSetPosition(NativeNSViewRef pView, NativeNSViewRef pParentView, int x, int y)
{
    cocoaViewSetPosition(pView, pParentView, x, y);
}

VMSVGA3DCOCOA_DECL(void) vmsvga3dCocoaViewSetSize(NativeNSViewRef pView, int w, int h)
{
    cocoaViewSetSize(pView, w, h);
}

VMSVGA3DCOCOA_DECL(void) vmsvga3dCocoaViewMakeCurrentContext(NativeNSViewRef pView, NativeNSOpenGLContextRef pCtx)
{
    Assert(!pView || [(OverlayView *)pView glCtx] == pCtx || [(OverlayView *)pView glCtx] == nil);
    cocoaViewMakeCurrentContext(pView, pCtx);
}

VMSVGA3DCOCOA_DECL(void) vmsvga3dCocoaSwapBuffers(NativeNSViewRef pView, NativeNSOpenGLContextRef pCtx)
{
# if 1
    Assert([(OverlayView *)pView glCtx] == pCtx);
    Assert([pCtx view] == pView);
    cocoaViewDisplay(pView);
# else
    DEBUG_FUNC_ENTER();
    NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];
    
    Assert([(OverlayView *)pView glCtx] == pCtx);
    Assert([pCtx view] == pView);

    [pCtx flushBuffer];

    [pPool release];
    DEBUG_FUNC_LEAVE();
# endif
}

#endif /* IN_VMSVGA3D */
