/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "renderspu.h"

#include "cr_string.h"
#include "cr_mem.h"
#include "cr_error.h"
#include "cr_environment.h"
#include "cr_url.h"


static void set_window_geometry( RenderSPU *render_spu, const char *response )
{
    int x, y, w, h;
    CRASSERT(response[0] == '[');
    sscanf( response, "[ %d, %d, %d, %d ]", &x, &y, &w, &h );
    render_spu->defaultX = (int) x;
    render_spu->defaultY = (int) y;
    render_spu->defaultWidth = (unsigned int) w;
    render_spu->defaultHeight = (unsigned int) h;
}

static void set_default_visual( RenderSPU *render_spu, const char *response )
{
    if (crStrlen(response) > 0) {
        if (crStrstr(response, "rgb"))
                render_spu->default_visual |= CR_RGB_BIT;
        if (crStrstr(response, "alpha"))
                render_spu->default_visual |= CR_ALPHA_BIT;
        if (crStrstr(response, "z") || crStrstr(response, "depth"))
                render_spu->default_visual |= CR_DEPTH_BIT;
        if (crStrstr(response, "stencil"))
                render_spu->default_visual |= CR_STENCIL_BIT;
        if (crStrstr(response, "accum"))
                render_spu->default_visual |= CR_ACCUM_BIT;
        if (crStrstr(response, "stereo"))
                render_spu->default_visual |= CR_STEREO_BIT;
        if (crStrstr(response, "multisample"))
                render_spu->default_visual |= CR_MULTISAMPLE_BIT;
        if (crStrstr(response, "double"))
                render_spu->default_visual |= CR_DOUBLE_BIT;
        if (crStrstr(response, "pbuffer"))
                render_spu->default_visual |= CR_PBUFFER_BIT;
    }
}

static void set_display_string( RenderSPU *render_spu, const char *response )
{
    if (!crStrcmp(response, "DEFAULT")) {
        const char *display = crGetenv("DISPLAY");
        if (display)
            crStrncpy(render_spu->display_string,
                                display,
                                sizeof(render_spu->display_string));
        else
            crStrcpy(render_spu->display_string, ""); /* empty string */
    }
    else {
        crStrncpy(render_spu->display_string,
                            response,
                            sizeof(render_spu->display_string));
    }
}

static void set_fullscreen( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->fullscreen) );
}

static void set_on_top( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->ontop) );
}

static void set_system_gl_path( RenderSPU *render_spu, const char *response )
{
    if (crStrlen(response) > 0)
        crSetenv( "CR_SYSTEM_GL_PATH", response );
}

static void set_title( RenderSPU *render_spu, const char *response )
{
    crFree( render_spu->window_title );
    render_spu->window_title = crStrdup( response );
}

#if defined(GLX)
static void set_try_direct( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->try_direct) );
}

static void set_force_direct( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->force_direct) );
}
#endif /* GLX */

static void render_to_app_window( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->render_to_app_window) );
}

static void render_to_crut_window( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->render_to_crut_window) );
}

static void resizable( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->resizable) );
}

static void set_borderless( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->borderless) );
}

static void set_cursor( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->drawCursor) );
}

static void gather_url( RenderSPU *render_spu, const char *response )
{
    char protocol[4096], hostname[4096];
    unsigned short port;
    
    if (!crParseURL(response, protocol, hostname, &port, 0))
    {
        crError( "Malformed URL: \"%s\"", response );
    }

    render_spu->gather_port = port;
}

static void gather_userbuf( RenderSPU *render_spu, const char *response )
{
    sscanf( response, "%d", &(render_spu->gather_userbuf_size) );
}

static void set_lut8( RenderSPU *render_spu, const char *response )
{
    int a;  
    char **lut;
    
    if (!response[0]) return;

    lut = crStrSplit(response, ",");
    if (!lut) return;

    for (a=0; a<256; a++)
    {
        render_spu->lut8[0][a]  = crStrToInt(lut[a]);
        render_spu->lut8[1][a]  = crStrToInt(lut[256+a]);
        render_spu->lut8[2][a]  = crStrToInt(lut[512+a]);
    }

    crFreeStrings(lut);

    render_spu->use_lut8 = 1;
}

static void set_master_url ( RenderSPU *render_spu, char *response )
{
    if (response[0])
        render_spu->swap_master_url = crStrdup( response );
    else
        render_spu->swap_master_url = NULL;
}

static void set_is_master ( RenderSPU *render_spu, char *response )
{
    render_spu->is_swap_master = crStrToInt( response );
}

static void set_num_clients ( RenderSPU *render_spu, char *response )
{
    render_spu->num_swap_clients = crStrToInt( response );
}

static void set_use_osmesa ( RenderSPU *render_spu, char *response )
{
    int val = crStrToInt( response );
#ifdef USE_OSMESA
    render_spu->use_osmesa = val;
#else
    if (val != 0)
        crError( "renderspu with Conf(use_osmesa, 1) but not compiled with -DUSE_OSMESA");
#endif
}

static void set_nv_swap_group( RenderSPU *render_spu, char *response )
{
    render_spu->nvSwapGroup = crStrToInt( response );
    if (render_spu->nvSwapGroup < 0)
        render_spu->nvSwapGroup = 0;
}

static void set_ignore_papi( RenderSPU *render_spu, char *response )
{
    render_spu->ignore_papi = crStrToInt( response );
}

static void set_ignore_window_moves( RenderSPU *render_spu, char *response )
{
    render_spu->ignore_window_moves = crStrToInt( response );
}

static void set_pbuffer_size( RenderSPU *render_spu, const char *response )
{
    CRASSERT(response[0] == '[');
    sscanf( response, "[ %d, %d ]",
                    &render_spu->pbufferWidth, &render_spu->pbufferHeight);
}

static void set_use_glxchoosevisual( RenderSPU *render_spu, char *response )
{
    render_spu->use_glxchoosevisual = crStrToInt( response );
}

static void set_draw_bbox( RenderSPU *render_spu, char *response )
{
    render_spu->draw_bbox = crStrToInt( response );
}



/* option, type, nr, default, min, max, title, callback
 */
SPUOptions renderSPUOptions[] = {
    { "title", CR_STRING, 1, "Chromium Render SPU", NULL, NULL, 
        "Window Title", (SPUOptionCB)set_title },

    { "window_geometry", CR_INT, 4, "[0, 0, 256, 256]", "[0, 0, 1, 1]", NULL, 
        "Default Window Geometry (x,y,w,h)", (SPUOptionCB)set_window_geometry },

    { "fullscreen", CR_BOOL, 1, "0", NULL, NULL, 
        "Full-screen Window", (SPUOptionCB)set_fullscreen },

    { "resizable", CR_BOOL, 1, "0", NULL, NULL,
        "Resizable Window", (SPUOptionCB)resizable },

    { "on_top", CR_BOOL, 1, "0", NULL, NULL, 
        "Display on Top", (SPUOptionCB)set_on_top },

    { "borderless", CR_BOOL, 1, "0", NULL, NULL,
        "Borderless Window", (SPUOptionCB) set_borderless },

    { "default_visual", CR_STRING, 1, "rgb, double, depth", NULL, NULL,
        "Default GL Visual", (SPUOptionCB) set_default_visual },

#if defined(GLX)
    { "try_direct", CR_BOOL, 1, "1", NULL, NULL, 
        "Try Direct Rendering", (SPUOptionCB)set_try_direct  },

    { "force_direct", CR_BOOL, 1, "0", NULL, NULL, 
        "Force Direct Rendering", (SPUOptionCB)set_force_direct },
#endif

    { "render_to_app_window", CR_BOOL, 1, "0", NULL, NULL,
        "Render to Application window", (SPUOptionCB)render_to_app_window },

    { "render_to_crut_window", CR_BOOL, 1, "0", NULL, NULL,
        "Render to CRUT window", (SPUOptionCB)render_to_crut_window },

    { "show_cursor", CR_BOOL, 1, "0", NULL, NULL,
        "Show Software Cursor", (SPUOptionCB) set_cursor },

    { "system_gl_path", CR_STRING, 1, "", NULL, NULL, 
        "System GL Path", (SPUOptionCB)set_system_gl_path },

    { "display_string", CR_STRING, 1, "DEFAULT", NULL, NULL, 
        "X Display String", (SPUOptionCB)set_display_string },

    { "gather_url", CR_STRING, 1, "", NULL, NULL,
        "Gatherer URL", (SPUOptionCB)gather_url},

    { "gather_userbuf_size", CR_INT, 1, "0", NULL, NULL,
        "Size of Buffer to Allocate for Gathering", (SPUOptionCB)gather_userbuf},

    { "lut8", CR_STRING, 1, "", NULL, NULL,
        "8 bit RGB LUT", (SPUOptionCB)set_lut8},

    { "swap_master_url", CR_STRING, 1, "", NULL, NULL,
        "The URL to the master swapper", (SPUOptionCB)set_master_url },

    { "is_swap_master", CR_BOOL, 1, "0", NULL, NULL,
        "Is this the swap master", (SPUOptionCB)set_is_master },

    { "num_swap_clients", CR_INT, 1, "1", NULL, NULL,
        "How many swaps to wait on", (SPUOptionCB)set_num_clients },

    { "use_osmesa", CR_BOOL, 1, "0", NULL, NULL,
        "Use offscreen rendering with Mesa", (SPUOptionCB)set_use_osmesa },
     
    { "nv_swap_group", CR_INT, 1, "0", NULL, NULL,
        "NVIDIA Swap Group Number", (SPUOptionCB) set_nv_swap_group },

    { "ignore_papi", CR_BOOL, 1, "0", NULL, NULL,
        "Ignore Barrier and Semaphore calls", (SPUOptionCB) set_ignore_papi },

    { "ignore_window_moves", CR_BOOL, 1, "0", NULL, NULL,
        "Ignore crWindowPosition calls", (SPUOptionCB) set_ignore_window_moves },

    { "pbuffer_size", CR_INT, 2, "[0, 0]", "[0, 0]", NULL,
        "Maximum PBuffer Size", (SPUOptionCB) set_pbuffer_size },

    { "use_glxchoosevisual", CR_BOOL, 1, "1", NULL, NULL,
        "Use glXChooseVisual", (SPUOptionCB) set_use_glxchoosevisual },

    { "draw_bbox", CR_BOOL, 1, "0", NULL, NULL,
        "Draw Bounding Boxes", (SPUOptionCB) set_draw_bbox },
    { NULL, CR_BOOL, 0, NULL, NULL, NULL, NULL, NULL },
};


void renderspuSetVBoxConfiguration( RenderSPU *render_spu )
{
    int a;

    for (a=0; a<256; a++)
    {
        render_spu->lut8[0][a] = 
        render_spu->lut8[1][a] = 
        render_spu->lut8[2][a] = a;
    }
    render_spu->use_lut8 = 0;

    set_title(render_spu, "Chromium Render SPU");
    set_window_geometry(render_spu, "[0, 0, 0, 0]");
    set_fullscreen(render_spu, "0");
    resizable(render_spu, "0");
    set_on_top(render_spu, "1");
    set_borderless(render_spu, "1");
    set_default_visual(render_spu, "rgb, double, depth");
#if defined(GLX)
    set_try_direct(render_spu, "1");
    set_force_direct(render_spu, "0");
#endif
    render_to_app_window(render_spu, "0");
    render_to_crut_window(render_spu, "0");
    set_cursor(render_spu, "0");
    set_system_gl_path(render_spu, "");
    set_display_string(render_spu, "DEFAULT");
    gather_url(render_spu, "");
    gather_userbuf(render_spu, "0");
    set_lut8(render_spu, "");
    set_master_url(render_spu, "");
    set_is_master(render_spu, "0");
    set_num_clients(render_spu, "1");
    set_use_osmesa(render_spu, "0");
    set_nv_swap_group(render_spu, "0");
    set_ignore_papi(render_spu, "0");
    set_ignore_window_moves(render_spu, "0");
    set_pbuffer_size(render_spu, "[0, 0]");
    set_use_glxchoosevisual(render_spu, "1");
    set_draw_bbox(render_spu, "0");

    render_spu->swap_mtu = 1024 * 500;

    /* Some initialization that doesn't really have anything to do
     * with configuration but which was done here before:
     */
    render_spu->use_L2 = 0;
    render_spu->cursorX = 0;
    render_spu->cursorY = 0;
#if defined(GLX)
    render_spu->sync = 0;
#endif
}

