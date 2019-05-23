/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packspu.h"
#include "cr_packfunctions.h"
#include "state/cr_statefuncs.h"
#include "cr_string.h"
#include "packspu_proto.h"
#include "cr_mem.h"
#include <locale.h>

static GLubyte gpszExtensions[10000];
#ifdef CR_OPENGL_VERSION_2_0
static GLubyte gpszShadingVersion[255]="";
#endif

static void GetString(GLenum name, GLubyte *pszStr)
{
    GET_THREAD(thread);
    int writeback = 1;

    if (pack_spu.swap)
        crPackGetStringSWAP(name, pszStr, &writeback);
    else
        crPackGetString(name, pszStr, &writeback);
    packspuFlush( (void *) thread );

    CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
}

static GLfloat
GetVersionString(void)
{
    static GLboolean fInitialized = GL_FALSE;
    static GLfloat version = 0.;

    if (!fInitialized)
    {
        GLubyte return_value[100];

        GetString(GL_VERSION, return_value);
        CRASSERT(crStrlen((char *)return_value) < 100);

        version = crStrToFloat((char *) return_value);
        version = crStateComputeVersion(version);

        fInitialized = GL_TRUE;
    }

    return version;
}

static const GLubyte *
GetExtensions(void)
{
    static GLboolean fInitialized = GL_FALSE;
    if (!fInitialized)
    {
        GLubyte return_value[10*1000];
        const GLubyte *extensions, *ext;
        GET_THREAD(thread);
        int writeback = 1;

        if (pack_spu.swap)
        {
            crPackGetStringSWAP( GL_EXTENSIONS, return_value, &writeback );
        }
        else
        {
            crPackGetString( GL_EXTENSIONS, return_value, &writeback );
        }
        packspuFlush( (void *) thread );

        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);

        CRASSERT(crStrlen((char *)return_value) < 10*1000);

        /* OK, we got the result from the server.  Now we have to
         * intersect is with the set of extensions that Chromium understands
         * and tack on the Chromium-specific extensions.
         */
        extensions = return_value;
        ext = crStateMergeExtensions(1, &extensions);

#ifdef Linux
        /*@todo
         *That's a hack to allow running Unity, it uses libnux which is calling extension functions
         *without checking if it's being supported/exported.
         *glActiveStencilFaceEXT seems to be actually supported but the extension string isn't exported (for ex. on ATI HD4870),
         *which leads to libglew setting function pointer to NULL and crashing Unity.
         */
        sprintf((char*)gpszExtensions, "%s GL_EXT_stencil_two_side", ext);
#else
        sprintf((char*)gpszExtensions, "%s", ext);
#endif
        fInitialized = GL_TRUE;
    }

    return gpszExtensions;
}

#ifdef WINDOWS
static bool packspuRunningUnderWine(void)
{
    return NULL != GetModuleHandle("wined3d.dll") || NULL != GetModuleHandle("wined3dwddm.dll") || NULL != GetModuleHandle("wined3dwddm-x86.dll");
}
#endif

const GLubyte * PACKSPU_APIENTRY packspu_GetString( GLenum name )
{
    GET_CONTEXT(ctx);

    switch(name)
    {
        case GL_EXTENSIONS:
            return GetExtensions();
        case GL_VERSION:
#if 0 && defined(WINDOWS)
            if (packspuRunningUnderWine())
            {
                GetString(GL_REAL_VERSION, ctx->pszRealVersion);
                return ctx->pszRealVersion;               
            }
            else
#endif
            {
                char *oldlocale;
                float version;

                oldlocale = setlocale(LC_NUMERIC, NULL);
                oldlocale = crStrdup(oldlocale);
                setlocale(LC_NUMERIC, "C");

                version = GetVersionString();
                sprintf((char*)ctx->glVersion, "%.1f Chromium %s", version, CR_VERSION_STRING);

                if (oldlocale)
                {
                    setlocale(LC_NUMERIC, oldlocale);
                    crFree(oldlocale);
                }

                return ctx->glVersion;
            }
        case GL_VENDOR:
#ifdef WINDOWS
            if (packspuRunningUnderWine())
            {
                GetString(GL_REAL_VENDOR, ctx->pszRealVendor);
                return ctx->pszRealVendor;
            }
            else
#endif
            {
                return crStateGetString(name);
            }
        case GL_RENDERER:
#ifdef WINDOWS
            if (packspuRunningUnderWine())
            {
                GetString(GL_REAL_RENDERER, ctx->pszRealRenderer);
                return ctx->pszRealRenderer;
            }
            else
#endif
            {
                return crStateGetString(name);
            }

#ifdef CR_OPENGL_VERSION_2_0
        case GL_SHADING_LANGUAGE_VERSION:
        {
            static GLboolean fInitialized = GL_FALSE;
            if (!fInitialized)
            {
                GetString(GL_SHADING_LANGUAGE_VERSION, gpszShadingVersion);
                fInitialized = GL_TRUE;
            }
            return gpszShadingVersion;
        }
#endif
#ifdef GL_CR_real_vendor_strings
        case GL_REAL_VENDOR:
            GetString(GL_REAL_VENDOR, ctx->pszRealVendor);
            return ctx->pszRealVendor;
        case GL_REAL_VERSION:
            GetString(GL_REAL_VERSION, ctx->pszRealVersion);
            return ctx->pszRealVersion;
        case GL_REAL_RENDERER:
            GetString(GL_REAL_RENDERER, ctx->pszRealRenderer);
            return ctx->pszRealRenderer;
#endif
        default:
            return crStateGetString(name);
    }
}

void packspuInitStrings()
{
    static GLboolean fInitialized = GL_FALSE;

    if (!fInitialized)
    {
        packspu_GetString(GL_EXTENSIONS);
        packspu_GetString(GL_VERSION);
        fInitialized = GL_TRUE;
    }
}
