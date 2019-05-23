/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Minimal swrast-based dri loadable driver.
 *
 * Todo:
 *   -- Use malloced (rather than framebuffer) memory for backbuffer
 *   -- 32bpp is hardwired -- fix
 *
 * NOTES:
 *   -- No mechanism for cliprects or resize notification --
 *      assumes this is a fullscreen device.  
 *   -- No locking -- assumes this is the only driver accessing this 
 *      device.
 *   -- Doesn't (yet) make use of any acceleration or other interfaces
 *      provided by fb.  Would be entirely happy working against any 
 *	fullscreen interface.
 *   -- HOWEVER: only a small number of pixelformats are supported, and
 *      the mechanism for choosing between them makes some assumptions
 *      that may not be valid everywhere.
 */

#include "driver.h"
#include "drm.h"
#include "utils.h"
#include "drirenderbuffer.h"

#include "buffers.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "vbo/vbo.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"

#define need_GL_VERSION_1_3
#define need_GL_VERSION_1_4
#define need_GL_VERSION_1_5
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1

/* sw extensions for imaging */
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_convolution
#define need_GL_EXT_histogram
#define need_GL_SGI_color_table

/* sw extensions not associated with some GL version */
#define need_GL_ARB_shader_objects
#define need_GL_ARB_vertex_program
#define need_GL_APPLE_vertex_array_object
#define need_GL_ATI_fragment_shader
#define need_GL_EXT_depth_bounds_test
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_framebuffer_blit
#define need_GL_EXT_gpu_program_parameters
#define need_GL_EXT_paletted_texture
#define need_GL_IBM_multimode_draw_arrays
#define need_GL_MESA_resize_buffers
#define need_GL_NV_vertex_program
#define need_GL_NV_fragment_program

#include "extension_helper.h"

const struct dri_extension card_extensions[] =
{
    { "GL_VERSION_1_3",                 GL_VERSION_1_3_functions },
    { "GL_VERSION_1_4",                 GL_VERSION_1_4_functions },
    { "GL_VERSION_1_5",                 GL_VERSION_1_5_functions },
    { "GL_VERSION_2_0",                 GL_VERSION_2_0_functions },
    { "GL_VERSION_2_1",                 GL_VERSION_2_1_functions },

    { "GL_EXT_blend_color",             GL_EXT_blend_color_functions },
    { "GL_EXT_blend_minmax",            GL_EXT_blend_minmax_functions },
    { "GL_EXT_convolution",             GL_EXT_convolution_functions },
    { "GL_EXT_histogram",               GL_EXT_histogram_functions },
    { "GL_SGI_color_table",             GL_SGI_color_table_functions },

    { "GL_ARB_shader_objects",          GL_ARB_shader_objects_functions },
    { "GL_ARB_vertex_program",          GL_ARB_vertex_program_functions },
    { "GL_APPLE_vertex_array_object",   GL_APPLE_vertex_array_object_functions },
    { "GL_ATI_fragment_shader",         GL_ATI_fragment_shader_functions },
    { "GL_EXT_depth_bounds_test",       GL_EXT_depth_bounds_test_functions },
    { "GL_EXT_framebuffer_object",      GL_EXT_framebuffer_object_functions },
    { "GL_EXT_framebuffer_blit",        GL_EXT_framebuffer_blit_functions },
    { "GL_EXT_gpu_program_parameters",  GL_EXT_gpu_program_parameters_functions },
    { "GL_EXT_paletted_texture",        GL_EXT_paletted_texture_functions },
    { "GL_IBM_multimode_draw_arrays",   GL_IBM_multimode_draw_arrays_functions },
    { "GL_MESA_resize_buffers",         GL_MESA_resize_buffers_functions },
    { "GL_NV_vertex_program",           GL_NV_vertex_program_functions },
    { "GL_NV_fragment_program",         GL_NV_fragment_program_functions },
    { NULL,                             NULL }
};

void fbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis);

typedef struct {
   GLcontext *glCtx;		/* Mesa context */

   struct {
      __DRIcontextPrivate *context;	
      __DRIscreenPrivate *screen;	
      __DRIdrawablePrivate *drawable; /* drawable bound to this ctx */
   } dri;
   
} fbContext, *fbContextPtr;

#define FB_CONTEXT(ctx)		((fbContextPtr)(ctx->DriverCtx))


static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa dumb framebuffer";
      default:
         return NULL;
   }
}


static void
update_state( GLcontext *ctx, GLuint new_state )
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


/**
 * Called by ctx->Driver.GetBufferSize from in core Mesa to query the
 * current framebuffer size.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = FB_CONTEXT(ctx);

   *width  = fbmesa->dri.drawable->w;
   *height = fbmesa->dri.drawable->h;
}


static void
updateFramebufferSize(GLcontext *ctx)
{
   fbContextPtr fbmesa = FB_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->WinSysDrawBuffer;
   if (fbmesa->dri.drawable->w != fb->Width ||
       fbmesa->dri.drawable->h != fb->Height) {
      driUpdateFramebufferSize(ctx, fbmesa->dri.drawable);
   }
}

static void
viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   /* XXX this should be called after we acquire the DRI lock, not here */
   updateFramebufferSize(ctx);
}


static void
init_core_functions( struct dd_function_table *functions )
{
   functions->GetString = get_string;
   functions->UpdateState = update_state;
   functions->GetBufferSize = get_buffer_size;
   functions->Viewport = viewport;

   functions->Clear = _swrast_Clear;  /* could accelerate with blits */
}


/*
 * Generate code for span functions.
 */

/* 24-bit BGR */
#define NAME(PREFIX) PREFIX##_B8G8R8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 3;
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]; \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_B8G8R8A8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 4;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]; \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]; \
   DST[3] = 0xff
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]; \
   DST[ACOMP] = SRC[3]

#include "swrast/s_spantemp.h"


/* 16-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G6R5
#define RB_TYPE GLubyte
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 2;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 8) | (((VALUE[GCOMP]) & 0xfc) << 3) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 8) & 0xf8) | (((SRC[0]) >> 11) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 3) & 0xfc) | (((SRC[0]) >>  5) & 0x3) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 15-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G5R5
#define RB_TYPE GLubyte
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 2;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 7) | (((VALUE[GCOMP]) & 0xf8) << 2) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 7) & 0xf8) | (((SRC[0]) >> 10) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 2) & 0xf8) | (((SRC[0]) >>  5) & 0x7) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(PREFIX) PREFIX##_CI8
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]

#include "swrast/s_spantemp.h"



void
fbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   ASSERT(drb->Base.InternalFormat == GL_RGBA);
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow = get_row_B5G6R5;
         drb->Base.GetValues = get_values_B5G6R5;
         drb->Base.PutRow = put_row_B5G6R5;
         drb->Base.PutMonoRow = put_mono_row_B5G6R5;
         drb->Base.PutRowRGB = put_row_rgb_B5G6R5;
         drb->Base.PutValues = put_values_B5G6R5;
         drb->Base.PutMonoValues = put_mono_values_B5G6R5;
      }
      else if (vis->redBits == 5 && vis->greenBits == 5 && vis->blueBits == 5) {
         drb->Base.GetRow = get_row_B5G5R5;
         drb->Base.GetValues = get_values_B5G5R5;
         drb->Base.PutRow = put_row_B5G5R5;
         drb->Base.PutMonoRow = put_mono_row_B5G5R5;
         drb->Base.PutRowRGB = put_row_rgb_B5G5R5;
         drb->Base.PutValues = put_values_B5G5R5;
         drb->Base.PutMonoValues = put_mono_values_B5G5R5;
      }
      else if (vis->redBits == 8 && vis->greenBits == 8 && vis->blueBits == 8
               && vis->alphaBits == 8) {
         drb->Base.GetRow = get_row_B8G8R8A8;
         drb->Base.GetValues = get_values_B8G8R8A8;
         drb->Base.PutRow = put_row_B8G8R8A8;
         drb->Base.PutMonoRow = put_mono_row_B8G8R8A8;
         drb->Base.PutRowRGB = put_row_rgb_B8G8R8A8;
         drb->Base.PutValues = put_values_B8G8R8A8;
         drb->Base.PutMonoValues = put_mono_values_B8G8R8A8;
      }
      else if (vis->redBits == 8 && vis->greenBits == 8 && vis->blueBits == 8
               && vis->alphaBits == 0) {
         drb->Base.GetRow = get_row_B8G8R8;
         drb->Base.GetValues = get_values_B8G8R8;
         drb->Base.PutRow = put_row_B8G8R8;
         drb->Base.PutMonoRow = put_mono_row_B8G8R8;
         drb->Base.PutRowRGB = put_row_rgb_B8G8R8;
         drb->Base.PutValues = put_values_B8G8R8;
         drb->Base.PutMonoValues = put_mono_values_B8G8R8;
      }
      else if (vis->indexBits == 8) {
         drb->Base.GetRow = get_row_CI8;
         drb->Base.GetValues = get_values_CI8;
         drb->Base.PutRow = put_row_CI8;
         drb->Base.PutMonoRow = put_mono_row_CI8;
         drb->Base.PutValues = put_values_CI8;
         drb->Base.PutMonoValues = put_mono_values_CI8;
      }
   }
   else {
      /* hardware z/stencil/etc someday */
   }
}


static void
fbDestroyScreen( __DRIscreenPrivate *sPriv )
{
}


static const __DRIconfig **fbFillInModes(unsigned pixel_bits,
                                         unsigned depth_bits,
                                         unsigned stencil_bits,
                                         GLboolean have_back_buffer)
{
        unsigned deep = (depth_bits > 17);

        /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
         * enough to add support.  Basically, if a context is created with an
         * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
         * will never be used.
         */

        static const GLenum db_modes[2] = { GLX_NONE, GLX_SWAP_UNDEFINED_OML };
        uint8_t depth_bits_array[4];
        uint8_t stencil_bits_array[4];
        if(deep) {
                depth_bits_array[0] = 0;
                depth_bits_array[1] = 24;
                stencil_bits_array[0] = 0;
                stencil_bits_array[1] = 8;
        } else {
                depth_bits_array[0] = depth_bits;
                depth_bits_array[1] = 0;
                depth_bits_array[2] = depth_bits;
                depth_bits_array[3] = 0;
                stencil_bits_array[0] = 0;
                stencil_bits_array[1] = 0;
                stencil_bits_array[2] = 8;
                stencil_bits_array[3] = 8;
        }

        return driCreateConfigs(
                deep ? GL_RGBA : GL_RGB,
                deep ? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_SHORT_5_6_5,
                depth_bits_array,
                stencil_bits_array,
                deep ? 2 : 4,
                db_modes, 2);
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using legacy DRI.
 * 
 * return the __GLcontextModes supported by this driver
 */
static const __DRIconfig **fbInitScreen(__DRIscreenPrivate *psp)
{
   static const __DRIversion ddx_expected = { 1, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 0, 0 };


   if ( ! driCheckDriDdxDrmVersions2( "vboxvideo",
          & psp->dri_version, & dri_expected,
          & psp->ddx_version, & ddx_expected,
          & psp->drm_version, & drm_expected ) ) {
             return NULL;
          }
      
	     driInitExtensions( NULL, card_extensions, GL_FALSE );

	     return fbFillInModes( psp->fbBPP,
				(psp->fbBPP == 16) ? 16 : 24,
				(psp->fbBPP == 16) ? 0  : 8,
				1);
}

/* Create the device specific context.
 */
static GLboolean
fbCreateContext( const __GLcontextModes *glVisual,
		 __DRIcontextPrivate *driContextPriv,
		 void *sharedContextPrivate)
{
   fbContextPtr fbmesa;
   GLcontext *ctx, *shareCtx;
   struct dd_function_table functions;

   assert(glVisual);
   assert(driContextPriv);

   /* Allocate the Fb context */
   fbmesa = (fbContextPtr) _mesa_calloc( sizeof(*fbmesa) );
   if ( !fbmesa )
      return GL_FALSE;

   /* Init default driver functions then plug in our FBdev-specific functions
    */
   _mesa_init_driver_functions(&functions);
   init_core_functions(&functions);

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((fbContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   ctx = fbmesa->glCtx = _mesa_create_context(glVisual, shareCtx, 
					      &functions, (void *) fbmesa);
   if (!fbmesa->glCtx) {
      _mesa_free(fbmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = fbmesa;

   /* Create module contexts */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _swsetup_Wakeup( ctx );


   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(ctx);

   return GL_TRUE;
}


static void
fbDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = (fbContextPtr) driContextPriv->driverPrivate;
   fbContextPtr current = ctx ? FB_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (fbmesa == current) {
      _mesa_make_current(NULL, NULL, NULL);
   }

   /* Free fb context resources */
   if ( fbmesa ) {
      _swsetup_DestroyContext( fbmesa->glCtx );
      _tnl_DestroyContext( fbmesa->glCtx );
      _vbo_DestroyContext( fbmesa->glCtx );
      _swrast_DestroyContext( fbmesa->glCtx );

      /* free the Mesa context */
      fbmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( fbmesa->glCtx );

      _mesa_free( fbmesa );
   }
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
fbCreateBuffer( __DRIscreenPrivate *driScrnPriv,
		__DRIdrawablePrivate *driDrawPriv,
		const __GLcontextModes *mesaVis,
		GLboolean isPixmap )
{
   struct gl_framebuffer *mesa_framebuffer;
   
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = mesaVis->depthBits > 0;
      const GLboolean swAlpha = mesaVis->alphaBits > 0;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0;
      
      mesa_framebuffer = _mesa_create_framebuffer(mesaVis);
      if (!mesa_framebuffer)
         return 0;

      /* XXX double-check these parameters (bpp vs cpp, etc) */
      {
         driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA,
                                                   driScrnPriv->pFB,
                                                   driScrnPriv->fbBPP / 8,
                                                   driScrnPriv->fbOrigin,
                                                   driScrnPriv->fbStride,
                                                   driDrawPriv);
         fbSetSpanFunctions(drb, mesaVis);
         _mesa_add_renderbuffer(mesa_framebuffer,
                                BUFFER_FRONT_LEFT, &drb->Base);
      }
      if (mesaVis->doubleBufferMode) {
         /* XXX what are the correct origin/stride values? */
         GLvoid *backBuf = _mesa_malloc(driScrnPriv->fbStride
                                        * driScrnPriv->fbHeight);
         driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA,
                                                   backBuf,
                                                   driScrnPriv->fbBPP /8,
                                                   driScrnPriv->fbOrigin,
                                                   driScrnPriv->fbStride,
                                                   driDrawPriv);
         fbSetSpanFunctions(drb, mesaVis);
         _mesa_add_renderbuffer(mesa_framebuffer,
                                BUFFER_BACK_LEFT, &drb->Base);
      }

      _mesa_add_soft_renderbuffers(mesa_framebuffer,
                                   GL_FALSE, /* color */
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha, /* or always zero? */
                                   GL_FALSE /* aux */);
      
      driDrawPriv->driverPrivate = mesa_framebuffer;

      return 1;
   }
}


static void
fbDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}



/* If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static void
fbSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   struct gl_framebuffer *mesa_framebuffer = (struct gl_framebuffer *)dPriv->driverPrivate;
   struct gl_renderbuffer * front_renderbuffer = mesa_framebuffer->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
   void *frontBuffer = front_renderbuffer->Data;
   int currentPitch = ((driRenderbuffer *)front_renderbuffer)->pitch;
   void *backBuffer = mesa_framebuffer->Attachment[BUFFER_BACK_LEFT].Renderbuffer->Data;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      fbContextPtr fbmesa = (fbContextPtr) dPriv->driContextPriv->driverPrivate;
      GLcontext *ctx = fbmesa->glCtx;
      
      if (ctx->Visual.doubleBufferMode) {
	 int i;
	 int offset = 0;
         char *tmp = _mesa_malloc(currentPitch);

         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering commands */

         ASSERT(frontBuffer);
         ASSERT(backBuffer);

	 for (i = 0; i < dPriv->h; i++) {
            _mesa_memcpy(tmp, (char *) backBuffer + offset,
                         currentPitch);
            _mesa_memcpy((char *) frontBuffer + offset, tmp,
                          currentPitch);
            offset += currentPitch;
	 }
	    
	 _mesa_free(tmp);
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "fbSwapBuffers: drawable has no context!\n");
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
static GLboolean
fbMakeCurrent( __DRIcontextPrivate *driContextPriv,
	       __DRIdrawablePrivate *driDrawPriv,
	       __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      fbContextPtr newFbCtx = 
            (fbContextPtr) driContextPriv->driverPrivate;

      newFbCtx->dri.drawable = driDrawPriv;

      _mesa_make_current( newFbCtx->glCtx, 
                           driDrawPriv->driverPrivate,
                           driReadPriv->driverPrivate);
   } else {
      _mesa_make_current( NULL, NULL, NULL );
   }

   return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
static GLboolean
fbUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

const struct __DriverAPIRec driDriverAPI = {
   .InitScreen      = fbInitScreen,
   .DestroyScreen   = fbDestroyScreen,
   .CreateContext   = fbCreateContext,
   .DestroyContext  = fbDestroyContext,
   .CreateBuffer    = fbCreateBuffer,
   .DestroyBuffer   = fbDestroyBuffer,
   .SwapBuffers     = fbSwapBuffers,
   .MakeCurrent     = fbMakeCurrent,
   .UnbindContext   = fbUnbindContext,
};
