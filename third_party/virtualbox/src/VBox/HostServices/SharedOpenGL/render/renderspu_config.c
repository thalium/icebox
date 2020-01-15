/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "renderspu.h"

#include "cr_string.h"
#include "cr_mem.h"
#include "cr_error.h"

#include <iprt/env.h>


void renderspuSetVBoxConfiguration( RenderSPU *render_spu )
{
    const char *display = NULL;
    int a;

    for (a=0; a<256; a++)
    {
        render_spu->lut8[0][a] = 
        render_spu->lut8[1][a] = 
        render_spu->lut8[2][a] = a;
    }
    render_spu->use_lut8 = 0;

    crFree( render_spu->window_title );
    render_spu->window_title = crStrdup("Chromium Render SPU");
    render_spu->defaultX = 0;
    render_spu->defaultY = 0;
    render_spu->defaultWidth = 0;
    render_spu->defaultHeight = 0;
    render_spu->fullscreen = 0;
    render_spu->resizable = 0;
    render_spu->ontop = 1;
    render_spu->borderless = 1;
    render_spu->default_visual = CR_RGB_BIT | CR_DOUBLE_BIT | CR_DEPTH_BIT;
#if defined(GLX)
    render_spu->try_direct = 1;
    render_spu->force_direct = 0;
#endif
    render_spu->render_to_app_window = 0;
    render_spu->render_to_crut_window = 0;
    render_spu->drawCursor = 0;
    render_spu->gather_userbuf_size = 0;
    render_spu->use_lut8 = 0;
    render_spu->num_swap_clients = 1;
    render_spu->nvSwapGroup = 0;
    render_spu->ignore_papi = 0;
    render_spu->ignore_window_moves = 0;
    render_spu->pbufferWidth = 0;
    render_spu->pbufferHeight = 0;
    render_spu->use_glxchoosevisual = 1;
    render_spu->draw_bbox = 0;

    display = RTEnvGet("DISPLAY");
    if (display)
        crStrncpy(render_spu->display_string, display, sizeof(render_spu->display_string));
    else
        crStrcpy(render_spu->display_string, ""); /* empty string */

    /* Some initialization that doesn't really have anything to do
     * with configuration but which was done here before:
     */
    render_spu->use_L2 = 0;
    render_spu->cursorX = 0;
    render_spu->cursorY = 0;
#if defined(GLX)
    render_spu->sync = 0;
#endif

    /* Config of "render force present main thread" (currently implemented by glx and wgl). */
    {
        const char *forcePresent = RTEnvGet("CR_RENDER_FORCE_PRESENT_MAIN_THREAD");
        if (forcePresent)
            render_spu->force_present_main_thread = crStrToInt(forcePresent) ? 1 : 0;
        else
        {
#if defined(GLX)
            /* Customer needed this for avoiding system 3D driver bugs. */
            render_spu->force_present_main_thread = 1;
#else
            render_spu->force_present_main_thread = 0;
#endif
        }
    }
}

