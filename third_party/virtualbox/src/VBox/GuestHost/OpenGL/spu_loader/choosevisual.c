
#include "chromium.h"
#include "cr_spu.h"
#include "cr_error.h"
#include "cr_string.h"


/**
 * Wrappers for glXChooseVisual/etc.
 *
 * By using this function, the fake GLX, render SPU, tilesort SPU,
 * etc can be assured of getting the same GLX visual for a set of CR_*_BIT
 * visual flags.  This helps ensure that render_to_app_window will work
 * properly.
 */


#if defined(WINDOWS)
int
crChooseVisual(const crOpenGLInterface *ws, int visBits)
{
	/* placeholder */
	return 0;
}
#endif


#if defined(DARWIN)
int
crChooseVisual(const crOpenGLInterface *ws, int visBits)
{
	/* placeholder */
	return 0;
}
#endif


#if defined(GLX)

XVisualInfo *
crChooseVisual(const crOpenGLInterface *ws, Display *dpy, int screen,
							 GLboolean directColor, int visBits)
{
	XVisualInfo *vis;
	int errorBase, eventBase;

	if (ws->glXQueryExtension(dpy, &errorBase, &eventBase))
	{

		if (ws->glXChooseVisual)
		{
			/* Use the real OpenGL's glXChooseVisual function */
			int attribList[100];
			int i = 0;

			/* Build the attribute list */
			if (visBits & CR_RGB_BIT)
			{
				attribList[i++] = GLX_RGBA;
				attribList[i++] = GLX_RED_SIZE;
				attribList[i++] = 1;
				attribList[i++] = GLX_GREEN_SIZE;
				attribList[i++] = 1;
				attribList[i++] = GLX_BLUE_SIZE;
				attribList[i++] = 1;
			}

			if (visBits & CR_ALPHA_BIT)
			{
				attribList[i++] = GLX_ALPHA_SIZE;
				attribList[i++] = 1;
			}

			if (visBits & CR_DOUBLE_BIT)
			{
				attribList[i++] = GLX_DOUBLEBUFFER;
			}

			if (visBits & CR_STEREO_BIT)
			{
				attribList[i++] = GLX_STEREO;
			}

			if (visBits & CR_DEPTH_BIT)
			{
				attribList[i++] = GLX_DEPTH_SIZE;
				attribList[i++] = 1;
			}

			if (visBits & CR_STENCIL_BIT)
			{
				attribList[i++] = GLX_STENCIL_SIZE;
				attribList[i++] = 1;
			}

			if (visBits & CR_ACCUM_BIT)
			{
				attribList[i++] = GLX_ACCUM_RED_SIZE;
				attribList[i++] = 1;
				attribList[i++] = GLX_ACCUM_GREEN_SIZE;
				attribList[i++] = 1;
				attribList[i++] = GLX_ACCUM_BLUE_SIZE;
				attribList[i++] = 1;
				if (visBits & CR_ALPHA_BIT)
				{
					attribList[i++] = GLX_ACCUM_ALPHA_SIZE;
					attribList[i++] = 1;
				}
			}

			if (visBits & CR_MULTISAMPLE_BIT)
			{
				attribList[i++] = GLX_SAMPLE_BUFFERS_SGIS;
				attribList[i++] = 1;
				attribList[i++] = GLX_SAMPLES_SGIS;
				attribList[i++] = 4;
			}

			if (visBits & CR_OVERLAY_BIT)
			{
				attribList[i++] = GLX_LEVEL;
				attribList[i++] = 1;
			}

			if (directColor)
			{
				/* 
				 * See if we have have GLX_EXT_visual_info so we
				 * can grab a Direct Color visual
				 */
#ifdef GLX_EXT_visual_info
				if (crStrstr(ws->glXQueryExtensionsString(dpy, screen),
										"GLX_EXT_visual_info"))
				{
					attribList[i++] = GLX_X_VISUAL_TYPE_EXT;
					attribList[i++] = GLX_DIRECT_COLOR_EXT; 
				}
#endif  
			}

			/* End the list */
			attribList[i++] = None;

			vis = ws->glXChooseVisual(dpy, screen, attribList);
			return vis;
		}
		else
		{
			/* Don't use glXChooseVisual, use glXGetConfig.
			 *
			 * Here's the deal:
			 * Some (all?) versions of the libGL.so that's shipped with ATI's
			 * drivers aren't built with the -Bsymbolic flag.  That's bad.
			 *
			 * If we call the glXChooseVisual() function that's built into ATI's
			 * libGL, it in turn calls the glXGetConfig() function.  Now, there's
			 * a glXGetConfig function in libGL.so **AND** there's a glXGetConfig
			 * function in Chromium's libcrfaker.so library.  Unfortunately, the
			 * later one gets called instead of the former.  At this point, things
			 * go haywire.  If -Bsymbolic were used, this would not happen.
			 */
			XVisualInfo templateVis;
			long templateFlags;
			int count, i, visType;

			visType = directColor ? DirectColor : TrueColor;

			/* Get list of candidate visuals */
			templateFlags = VisualScreenMask | VisualClassMask;
			templateVis.screen = screen;
#if defined(__cplusplus) || defined(c_plusplus)
			templateVis.c_class = visType;
#else
			templateVis.class = visType;
#endif

			vis = XGetVisualInfo(dpy, templateFlags, &templateVis, &count);
			/* find first visual that's good enough */
			for (i = 0; i < count; i++)
			{
				int val;

				/* Need exact match on RGB, DOUBLEBUFFER, STEREO, LEVEL, MULTISAMPLE */
				ws->glXGetConfig(dpy, vis + i, GLX_RGBA, &val);
				if (((visBits & CR_RGB_BIT) && !val) ||
						(((visBits & CR_RGB_BIT) == 0) && val))
				{
					continue;
				}

				ws->glXGetConfig(dpy, vis + i, GLX_DOUBLEBUFFER, &val);
				if (((visBits & CR_DOUBLE_BIT) && !val) ||
						(((visBits & CR_DOUBLE_BIT) == 0) && val))
				{
					continue;
				}

				ws->glXGetConfig(dpy, vis + i, GLX_STEREO, &val);
				if (((visBits & CR_STEREO_BIT) && !val) ||
						(((visBits & CR_STEREO_BIT) == 0) && val))
				{
					continue;
				}

				ws->glXGetConfig(dpy, vis + i, GLX_LEVEL, &val);
				if (((visBits & CR_OVERLAY_BIT) && !val) ||
						(((visBits & CR_OVERLAY_BIT) == 0) && val))
				{
					continue;
				}

				ws->glXGetConfig(dpy, vis + i, GLX_SAMPLE_BUFFERS_SGIS, &val);
				if (visBits & CR_MULTISAMPLE_BIT)
				{
					if (!val)
						continue;
					ws->glXGetConfig(dpy, vis + i, GLX_SAMPLES_SGIS, &val);
					if (val < 4)
						continue;
				}
				else {
					/* don't want multisample */
					if (val)
						continue;
				}

				/* Need good enough for ALPHA, DEPTH, STENCIL, ACCUM */
				if (visBits & CR_ALPHA_BIT)
				{
					ws->glXGetConfig(dpy, vis + i, GLX_ALPHA_SIZE, &val);
					if (!val)
						continue;
				}

				if (visBits & CR_DEPTH_BIT)
				{
					ws->glXGetConfig(dpy, vis + i, GLX_DEPTH_SIZE, &val);
					if (!val)
						continue;
				}

				if (visBits & CR_STENCIL_BIT)
				{
					ws->glXGetConfig(dpy, vis + i, GLX_STENCIL_SIZE, &val);
					if (!val)
						continue;
				}

				if (visBits & CR_ACCUM_BIT)
				{
					ws->glXGetConfig(dpy, vis + i, GLX_ACCUM_RED_SIZE, &val);
					if (!val)
						continue;
					if (visBits & CR_ALPHA_BIT)
					{
						ws->glXGetConfig(dpy, vis + i, GLX_ACCUM_ALPHA_SIZE, &val);
						if (!val)
							continue;
					}
				}

				/* If we get here, we found a good visual.
				 * Now, we need to get a new XVisualInfo pointer in case the caller
				 * calls XFree on it.
				 */
				templateFlags = VisualScreenMask | VisualIDMask;
				templateVis.screen = screen;
				templateVis.visualid = vis[i].visual->visualid;
				XFree(vis); /* free the list */
				vis = XGetVisualInfo(dpy, templateFlags, &templateVis, &count);
				return vis;
			}

			/* if we get here, we failed to find a sufficient visual */
			return NULL;
		}
	}
	else
	{
		/* use Xlib instead of GLX */
		XVisualInfo templateVis, *best;
		long templateFlags;
		int i, count, visType;

		if (visBits & CR_RGB_BIT)
			visType = directColor ? DirectColor : TrueColor;
		else
			visType = PseudoColor;

		/* Get list of candidate visuals */
		templateFlags = VisualScreenMask | VisualClassMask;
		templateVis.screen = screen;
#if defined(__cplusplus) || defined(c_plusplus)
		templateVis.c_class = visType;
#else
		templateVis.class = visType;
#endif

		vis = XGetVisualInfo(dpy, templateFlags, &templateVis, &count);
		if (!vis)
			return NULL;

		/* okay, select the RGB visual with the most depth */
		best = vis + 0;
		for (i = 1; i < count; i++)
		{
			if (vis[i].depth > best->depth &&
					vis[i].bits_per_rgb > best->bits_per_rgb )
				best = vis + i;
		}

		if (best)
		{
			/* If we get here, we found a good visual.
			 * Now, we need to get a new XVisualInfo pointer in case the caller
			 * calls XFree on it.
			 */
			templateFlags = VisualScreenMask | VisualIDMask;
			templateVis.screen = screen;
			templateVis.visualid = best->visualid;
			XFree(vis); /* free the list */
			best = XGetVisualInfo(dpy, templateFlags, &templateVis, &count);
		}

		return best;
	}
}

#endif /* GLX */
