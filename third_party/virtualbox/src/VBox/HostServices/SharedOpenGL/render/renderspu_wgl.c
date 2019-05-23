/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */


#define WIN32_LEAN_AND_MEAN
#include <iprt/win/windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "cr_environment.h"
#include "cr_error.h"
#include "cr_string.h"
#include "renderspu.h"
#include "cr_mem.h"


/* IAT patcher stuff */
#define RVA2PTR(_t, _base, _off) ((_t*)(((uint8_t*)(_base)) + (_off)))

int renderspuIatPatcherGetImportAddress(HMODULE hModule, LPCSTR pszLib, LPCSTR pszName, void** ppAdr)
{
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNtHdr;
    PIMAGE_IMPORT_DESCRIPTOR pImportDr;
    DWORD rvaImport;

    crDebug("searching entry %s from %s", pszName, pszLib);

    *ppAdr = 0;

    if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        crWarning("invalid dos signature");
        return VERR_INVALID_HANDLE;
    }
    pNtHdr = RVA2PTR(IMAGE_NT_HEADERS, pDosHdr, pDosHdr->e_lfanew);
    if (pNtHdr->Signature != IMAGE_NT_SIGNATURE)
    {
        crWarning("invalid nt signature");
        return VERR_INVALID_HANDLE;
    }
    rvaImport = pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!rvaImport)
    {
        crWarning("no imports found");
        return VERR_NOT_FOUND;
    }
    pImportDr =  RVA2PTR(IMAGE_IMPORT_DESCRIPTOR, pDosHdr, rvaImport);

    for ( ;pImportDr->TimeDateStamp != 0 || pImportDr->Name != 0; ++pImportDr)
    {
        DWORD rvaINT, rvaIAT;
        PIMAGE_THUNK_DATA pINT, pIAT;
        LPCSTR pszLibCur = RVA2PTR(char, pDosHdr, pImportDr->Name);
        if (stricmp(pszLibCur, pszLib))
            continue;

        /* got the necessary lib! */
        crDebug("got info for lib");

        rvaINT = pImportDr->OriginalFirstThunk;
        rvaIAT = pImportDr->FirstThunk;

        if (!rvaINT  || !rvaIAT)
        {
            crWarning("either rvaINT(0x%x) or rvaIAT(0x%x) are NULL, nothing found!", rvaINT, rvaIAT);
            return VERR_NOT_FOUND;
        }

        pINT = RVA2PTR(IMAGE_THUNK_DATA, pDosHdr, rvaINT);
        pIAT = RVA2PTR(IMAGE_THUNK_DATA, pDosHdr, rvaIAT);

        for ( ; pINT->u1.AddressOfData; ++pINT, ++pIAT)
        {
            PIMAGE_IMPORT_BY_NAME pIbn;

            if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))
                continue;

            pIbn = RVA2PTR(IMAGE_IMPORT_BY_NAME, pDosHdr, pINT->u1.AddressOfData);

            if (stricmp(pszName, (char*)pIbn->Name))
                continue;

            *ppAdr = &pIAT->u1.Function;

            crDebug("search succeeded!");
            return VINF_SUCCESS;
        }
    }

    crDebug("not found");
    return VERR_NOT_FOUND;
}

int renderspuIatPatcherPatchEntry(void *pvEntry, void *pvValue, void **ppvOldVal)
{
    void **ppfn = (void**)pvEntry;
    DWORD dwOldProtect = 0;

    if (!VirtualProtect(pvEntry, sizeof (pvEntry), PAGE_READWRITE, &dwOldProtect))
    {
        crWarning("VirtualProtect 1 failed, %d", GetLastError());
        return VERR_ACCESS_DENIED;
    }

    if (ppvOldVal)
        *ppvOldVal = *ppfn;
    *ppfn = pvValue;

    if (!VirtualProtect(pvEntry, sizeof (pvEntry), dwOldProtect, &dwOldProtect))
    {
        crWarning("VirtualProtect 2 failed, %d.. ignoring", GetLastError());
    }

    return VINF_SUCCESS;
}


int renderspuIatPatcherPatchFunction(HMODULE hModule, LPCSTR pszLib, LPCSTR pszName, void* pfn)
{
    void* pAdr;
    int rc = renderspuIatPatcherGetImportAddress(hModule, pszLib, pszName, &pAdr);
    if (RT_FAILURE(rc))
    {
        crDebug("renderspuIatPatcherGetImportAddress failed, %d", rc);
        return rc;
    }

    rc = renderspuIatPatcherPatchEntry(pAdr, pfn, NULL);
    if (RT_FAILURE(rc))
    {
        crWarning("renderspuIatPatcherPatchEntry failed, %d", rc);
        return rc;
    }

    return VINF_SUCCESS;
}

/* patch */
static HWND __stdcall renderspuAtiQuirk_GetForegroundWindow()
{
    crDebug("renderspuAtiQuirk_GetForegroundWindow");
    return NULL;
}


#define CRREG_MAXKEYNAME 8
static int renderspuAtiQuirk_GetICDDriverList(char *pBuf, DWORD cbBuf, DWORD *pcbResult)
{
    static LPCSTR aValueNames[] = {"OpenGLVendorName", "OpenGLDriverName"};
    char *pBufPos = pBuf;
    DWORD cbBufRemain = cbBuf, cbTotal = 0;
    HKEY hKey, hSubkey;
    DWORD dwIndex = 0;
    int i;
    int rc = VINF_SUCCESS;
    char NameBuf[CRREG_MAXKEYNAME];
    LONG lRc;

    if (pcbResult)
        *pcbResult = 0;

    lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E968-E325-11CE-BFC1-08002BE10318}",
            0, /* reserved*/
            KEY_READ,
            &hKey);
    if (ERROR_SUCCESS != lRc)
    {
        crDebug("RegOpenKeyEx 1 failed, %d", lRc);
        return VERR_OPEN_FAILED;
    }

    for ( ; ; ++dwIndex)
    {
        lRc = RegEnumKeyA(hKey, dwIndex, NameBuf, CRREG_MAXKEYNAME);
        if (lRc == ERROR_NO_MORE_ITEMS)
            break;
        if (lRc == ERROR_MORE_DATA)
            continue;
        if (lRc != ERROR_SUCCESS)
        {
            crWarning("RegEnumKeyA failed, %d", lRc);
            continue;
        }

        lRc = RegOpenKeyEx(hKey,
                NameBuf,
                0, /* reserved*/
                KEY_READ,
                &hSubkey);
        if (ERROR_SUCCESS != lRc)
        {
            crDebug("RegOpenKeyEx 2 failed, %d", lRc);
            RegCloseKey(hKey);
            return VERR_OPEN_FAILED;
        }

        for (i = 0; i < RT_ELEMENTS(aValueNames); ++i)
        {
            DWORD cbCur = cbBufRemain;
            DWORD type;
            lRc = RegQueryValueExA(hSubkey, aValueNames[i], NULL, /* reserved*/
                    &type,
                    (PBYTE)pBufPos, &cbCur);
            /* exclude second null termination */
            --cbCur;

            if (ERROR_MORE_DATA == lRc)
            {
                if (REG_MULTI_SZ != type)
                {
                    crWarning("unexpected data type! %d", type);
                    continue;
                }
                rc = VERR_BUFFER_OVERFLOW;
                pBufPos = NULL;
                cbBufRemain = 0;
                CRASSERT(cbCur > 0 && cbCur < UINT32_MAX/2);
                cbTotal += cbCur;
                continue;
            }
            if (ERROR_SUCCESS != lRc)
            {
                crDebug("RegQueryValueExA failed, %d", lRc);
                continue;
            }

            if (REG_MULTI_SZ != type)
            {
                crWarning("unexpected data type! %d", type);
                continue;
            }

            /* succeeded */
            CRASSERT(cbCur > 0 && cbCur < UINT32_MAX/2);
            pBufPos += cbCur;
            cbBufRemain -= cbCur;
            cbTotal += cbCur;
            CRASSERT(cbBufRemain < UINT32_MAX/2);
        }

        RegCloseKey(hSubkey);
    }

    RegCloseKey(hKey);

    if (cbTotal)
    {
        /* include second null termination */
        CRASSERT(!pBufPos || pBufPos[0] == '\0');
        ++cbTotal;
    }

    if (pcbResult)
        *pcbResult = cbTotal;

    return rc;
}

static int renderspuAtiQuirk_ApplyForModule(LPCSTR pszAtiDll)
{
    int rc;
    HMODULE hAtiDll;

    crDebug("renderspuAtiQuirk_ApplyForModule (%s)", pszAtiDll);

    hAtiDll = GetModuleHandleA(pszAtiDll);
    if (!hAtiDll)
    {
        crDebug("GetModuleHandle failed, %d", GetLastError());
        return VERR_NOT_FOUND;
    }

    rc = renderspuIatPatcherPatchFunction(hAtiDll, "user32.dll", "GetForegroundWindow", (void*)renderspuAtiQuirk_GetForegroundWindow);
    if (RT_FAILURE(rc))
    {
        crDebug("renderspuIatPatcherPatchFunction failed, %d", rc);
        return rc;
    }

    crDebug("renderspuAtiQuirk_ApplyForModule SUCCEEDED!");
    crInfo("ATI Fullscreen quirk patch SUCCEEDED!");

    return VINF_SUCCESS;
}

static LPCSTR renderspuRegMultiSzNextVal(LPCSTR pszBuf)
{
    pszBuf += strlen(pszBuf) + sizeof (pszBuf[0]);

    if (pszBuf[0] == '\0')
        return NULL;

    return pszBuf;
}

static LPCSTR renderspuRegMultiSzCurVal(LPCSTR pszBuf)
{
    if (pszBuf[0] == '\0')
        return NULL;

    return pszBuf;
}


static int renderspuAtiQuirk_Apply()
{
    char aBuf[4096];
    DWORD cbResult = 0;
    LPCSTR pszVal;
    int rc;

    crDebug("renderspuAtiQuirk_Apply..");

    rc = renderspuAtiQuirk_GetICDDriverList(aBuf, sizeof (aBuf), &cbResult);
    if (RT_FAILURE(rc))
    {
        crDebug("renderspuAtiQuirk_GetICDDriverList failed, rc(%d)", rc);
        return rc;
    }

    for (pszVal = renderspuRegMultiSzCurVal(aBuf);
            pszVal;
            pszVal = renderspuRegMultiSzNextVal(pszVal))
    {
        renderspuAtiQuirk_ApplyForModule(pszVal);
    }

    return VINF_SUCCESS;
}

static GLboolean renderspuAtiQuirk_Needed()
{
    const char * pszString = render_spu.ws.glGetString(GL_VENDOR);
    if (pszString && strstr(pszString, "ATI"))
        return GL_TRUE;
    pszString = render_spu.ws.glGetString(GL_RENDERER);
    if (pszString && strstr(pszString, "ATI"))
        return GL_TRUE;
    return GL_FALSE;
}

static void renderspuAtiQuirk_ChkApply()
{
    static GLboolean fChecked = GL_FALSE;
    if (fChecked)
        return;

    fChecked = GL_TRUE;
    if (!renderspuAtiQuirk_Needed())
        return;

    crInfo("This is an ATI card, taking care of fullscreen..");

    /*
     * ATI WDDM-based graphics have an issue with rendering fullscreen.
     * See public tickets #9775 & #9267 .
     * Namely ATI drivers check whether ogl window is foreground and fullscreen
     * and if so - do D3DKMTSetDisplayMode for ogl surface,
     * which prevented any other data from being displayed, no matter what.
     *
     * Here we check whether we're using an ATI card and if so, patch the ogl ICD driver's IAT
     * to replace GetForegroundWindow reference with our renderspuAtiQuirk_GetForegroundWindow,
     * which always returns NULL.
     */
    renderspuAtiQuirk_Apply();
}

#define WINDOW_NAME window->title

static BOOL
bSetupPixelFormat( HDC hdc, GLbitfield visAttribs );

GLboolean renderspu_SystemInitVisual( VisualInfo *visual )
{
    if (visual->visAttribs & CR_PBUFFER_BIT) {
        crWarning("Render SPU: PBuffers not support on Windows yet.");
    }

    /* In the windows world, we need a window before a context.
     * Use the device_context as a marker to do just that */

    return TRUE;
}

void renderspu_SystemDestroyWindow( WindowInfo *window )
{
    VBOX_RENDERSPU_DESTROY_WINDOW vrdw;

    CRASSERT(window);

    /*DestroyWindow( window->hWnd );*/

    vrdw.hWnd = window->hWnd;

    if (render_spu.dwWinThreadId)
    {
        PostThreadMessage(render_spu.dwWinThreadId, WM_VBOX_RENDERSPU_DESTROY_WINDOW, 0, (LPARAM) &vrdw);
        WaitForSingleObject(render_spu.hWinThreadReadyEvent, INFINITE);
    }
    else
    {
        crError("Render SPU: window thread is not running");
    }

    window->hWnd = NULL;
    window->visual = NULL;
    if (window->hRgn)
    {
        DeleteObject(window->hRgn);
        window->hRgn = NULL;
    }
}

static LONG WINAPI
MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    /* int w,h; */

    switch ( uMsg ) {
        case WM_PAINT:
        {
            WindowInfo *pWindow = (WindowInfo *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (pWindow)
            {
                const struct VBOXVR_SCR_COMPOSITOR * pCompositor;

                pCompositor = renderspuVBoxCompositorAcquire(pWindow);
                if (pCompositor)
                {
                    HDC hDC;
                    PAINTSTRUCT Paint;

                    Assert(pWindow->device_context);
                    hDC = BeginPaint(pWindow->hWnd, &Paint);
                    if (hDC)
                    {
                        BOOL bRc;
                        pWindow->redraw_device_context = hDC;

                        renderspuVBoxPresentCompositionGeneric(pWindow, pCompositor, NULL, 1, true);

                        bRc = EndPaint(pWindow->hWnd, &Paint);

                        pWindow->redraw_device_context = NULL;

                        renderspuVBoxCompositorRelease(pWindow);

                        if (!bRc)
                        {
                            DWORD winEr = GetLastError();
                            crWarning("EndPaint failed, winEr %d", winEr);
                        }
                    }
                    else
                    {
                        DWORD winEr = GetLastError();
                        crWarning("BeginPaint failed, winEr %d", winEr);
                    }
                }
            }
            break;
        }
        case WM_SIZE:
            /* w = LOWORD( lParam ); 
             * h = HIWORD( lParam ); */

            /* glViewport( 0, 0, w, h ); */
#if 0
            glViewport( -render_spu.mural_x, -render_spu.mural_y, 
                    render_spu.mural_width, render_spu.mural_height );
            glScissor( -render_spu.mural_x, -render_spu.mural_y, 
                    render_spu.mural_width, render_spu.mural_height );
#endif
            break;

        case WM_CLOSE:
            crWarning( "Render SPU: caught WM_CLOSE -- quitting." );
            exit( 0 );
            break;

        case WM_DESTROY:
            crDebug("Render SPU: caught WM_DESTROY for our window %x", hWnd);
            break;

        case WM_NCHITTEST:
            crDebug("WM_NCHITTEST");
            return HTNOWHERE;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

static BOOL
bSetupPixelFormatEXT( HDC hdc, GLbitfield visAttribs)
{
    PIXELFORMATDESCRIPTOR ppfd;
    int pixelFormat;
    int attribList[100];
    float fattribList[] = { 0.0, 0.0 };
    int numFormats;
    int i = 0;
    BOOL vis;

    CRASSERT(visAttribs & CR_RGB_BIT);  /* anybody need color index */

    crWarning("Render SPU: Using WGL_EXT_pixel_format to select visual ID.");

    attribList[i++] = WGL_DRAW_TO_WINDOW_EXT;
    attribList[i++] = GL_TRUE;
    attribList[i++] = WGL_ACCELERATION_EXT;
    attribList[i++] = WGL_FULL_ACCELERATION_EXT;
    attribList[i++] = WGL_COLOR_BITS_EXT;
    attribList[i++] = 24;
    attribList[i++] = WGL_RED_BITS_EXT;
    attribList[i++] = 1;
    attribList[i++] = WGL_GREEN_BITS_EXT;
    attribList[i++] = 1;
    attribList[i++] = WGL_BLUE_BITS_EXT;
    attribList[i++] = 1;

    crWarning("Render SPU: Visual chosen is... RGB");

    if (visAttribs & CR_ALPHA_BIT)
    {
        attribList[i++] = WGL_ALPHA_BITS_EXT;
        attribList[i++] = 1;
        crWarning("A");
    }

    crWarning(", ");

    if (visAttribs & CR_DOUBLE_BIT) {
        attribList[i++] = WGL_DOUBLE_BUFFER_EXT;
        attribList[i++] = GL_TRUE;
        crWarning("DB, ");
    }

    if (visAttribs & CR_STEREO_BIT) {
        attribList[i++] = WGL_STEREO_EXT;
        attribList[i++] = GL_TRUE;
        crWarning("Stereo, ");
    }

    if (visAttribs & CR_DEPTH_BIT)
    {
        attribList[i++] = WGL_DEPTH_BITS_EXT;
        attribList[i++] = 1;
        crWarning("Z, ");
    }

    if (visAttribs & CR_STENCIL_BIT)
    {
        attribList[i++] = WGL_STENCIL_BITS_EXT;
        attribList[i++] = 1;
        crWarning("Stencil, ");
    }

    if (visAttribs & CR_ACCUM_BIT)
    {
        attribList[i++] = WGL_ACCUM_RED_BITS_EXT;
        attribList[i++] = 1;
        attribList[i++] = WGL_ACCUM_GREEN_BITS_EXT;
        attribList[i++] = 1;
        attribList[i++] = WGL_ACCUM_BLUE_BITS_EXT;
        attribList[i++] = 1;
        crWarning("Accum, ");
        if (visAttribs & CR_ALPHA_BIT)
        {
            attribList[i++] = WGL_ACCUM_ALPHA_BITS_EXT;
            attribList[i++] = 1;
            crWarning("Accum Alpha, ");
        }
    }

    if (visAttribs & CR_MULTISAMPLE_BIT)
    {
        attribList[i++] = WGL_SAMPLE_BUFFERS_EXT;
        attribList[i++] = 1;
        attribList[i++] = WGL_SAMPLES_EXT;
        attribList[i++] = 4;
        crWarning("Multisample, ");
    }

    crWarning("\n");

    /* End the list */
    attribList[i++] = 0;
    attribList[i++] = 0;

    vis = render_spu.ws.wglChoosePixelFormatEXT( hdc, attribList, fattribList, 1, &pixelFormat, &numFormats);

    crDebug("Render SPU: wglChoosePixelFormatEXT (vis 0x%x, LastError 0x%x, pixelFormat 0x%x", vis, GetLastError(), pixelFormat);

#ifdef VBOX_CR_SERVER_FORCE_WGL
    render_spu.ws.wglSetPixelFormat( hdc, pixelFormat, &ppfd );
#else
    SetPixelFormat( hdc, pixelFormat, &ppfd );
#endif

    crDebug("Render SPU: wglSetPixelFormat (Last error 0x%x)", GetLastError());

    return vis;
}

static BOOL
bSetupPixelFormatNormal( HDC hdc, GLbitfield visAttribs )
{
    PIXELFORMATDESCRIPTOR pfd = { 
        sizeof(PIXELFORMATDESCRIPTOR),  /*  size of this pfd */
        1,                              /* version number */
        PFD_DRAW_TO_WINDOW |            /* support window */
        PFD_SUPPORT_OPENGL,             /* support OpenGL */
        PFD_TYPE_RGBA,                  /* RGBA type */
        24,                             /* 24-bit color depth */
        0, 0, 0, 0, 0, 0,               /* color bits ignored */
        0,                              /* no alpha buffer */
        0,                              /* shift bit ignored */
        0,                              /* no accumulation buffer */
        0, 0, 0, 0,                     /* accum bits ignored */
        0,                              /* set depth buffer  */
        0,                              /* set stencil buffer */
        0,                              /* no auxiliary buffer */
        PFD_MAIN_PLANE,                 /* main layer */
        0,                              /* reserved */
        0, 0, 0                         /* layer masks ignored */
    }; 
    PIXELFORMATDESCRIPTOR *ppfd = &pfd; 
    char s[1000];
    GLbitfield b = 0;
    int pixelformat;

    renderspuMakeVisString( visAttribs, s );

    crDebug( "Render SPU: WGL wants these visual capabilities: %s", s);

    /* These really come into play with sort-last configs */
    if (visAttribs & CR_DEPTH_BIT)
        ppfd->cDepthBits = 24;
    if (visAttribs & CR_ACCUM_BIT)
        ppfd->cAccumBits = 16;
    if (visAttribs & CR_RGB_BIT)
        ppfd->cColorBits = 24;
    if (visAttribs & CR_STENCIL_BIT)
        ppfd->cStencilBits = 8;
    if (visAttribs & CR_ALPHA_BIT)
        ppfd->cAlphaBits = 8;
    if (visAttribs & CR_DOUBLE_BIT)
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;
    if (visAttribs & CR_STEREO_BIT)
        ppfd->dwFlags |= PFD_STEREO;

    /* 
     * We call the wgl functions directly if the SPU was loaded
     * by our faker library, otherwise we have to call the GDI
     * versions.
     */
#ifdef VBOX_CR_SERVER_FORCE_WGL
    if (crGetenv( "CR_WGL_DO_NOT_USE_GDI" ) != NULL)
    {
        pixelformat = render_spu.ws.wglChoosePixelFormat( hdc, ppfd );
        /* doing this twice is normal Win32 magic */
        pixelformat = render_spu.ws.wglChoosePixelFormat( hdc, ppfd );
        if ( pixelformat == 0 ) 
        {
            crError( "render_spu.ws.wglChoosePixelFormat failed" );
        }
        if ( !render_spu.ws.wglSetPixelFormat( hdc, pixelformat, ppfd ) ) 
        {
            crError( "render_spu.ws.wglSetPixelFormat failed" );
        }
    
        render_spu.ws.wglDescribePixelFormat( hdc, pixelformat, sizeof(*ppfd), ppfd );
    }
    else
#endif
    {
        /* Okay, we were loaded manually.  Call the GDI functions. */
        pixelformat = ChoosePixelFormat( hdc, ppfd );
        /* doing this twice is normal Win32 magic */
        pixelformat = ChoosePixelFormat( hdc, ppfd );
        if ( pixelformat == 0 ) 
        {
            crError( "ChoosePixelFormat failed" );
        }
        if ( !SetPixelFormat( hdc, pixelformat, ppfd ) ) 
        {
            crError( "SetPixelFormat failed (Error 0x%x)", GetLastError() );
        }
        
        DescribePixelFormat( hdc, pixelformat, sizeof(*ppfd), ppfd );
    }


    if (ppfd->cDepthBits > 0)
        b |= CR_DEPTH_BIT;
    if (ppfd->cAccumBits > 0)
        b |= CR_ACCUM_BIT;
    if (ppfd->cColorBits > 8)
        b |= CR_RGB_BIT;
    if (ppfd->cStencilBits > 0)
        b |= CR_STENCIL_BIT;
    if (ppfd->cAlphaBits > 0)
        b |= CR_ALPHA_BIT;
    if (ppfd->dwFlags & PFD_DOUBLEBUFFER)
        b |= CR_DOUBLE_BIT;
    if (ppfd->dwFlags & PFD_STEREO)
        b |= CR_STEREO_BIT;

    renderspuMakeVisString( b, s );

    crDebug( "Render SPU: WGL chose these visual capabilities: %s", s);
    return TRUE;
}

static BOOL
bSetupPixelFormat( HDC hdc, GLbitfield visAttribs )
{
    /* According to http://www.opengl.org/resources/faq/technical/mswindows.htm
       we shouldn't be using wgl functions to setup pixel formats unless we're loading ICD driver.
       In particular, bSetupPixelFormatEXT bugs with Intel drivers.
     */
    return bSetupPixelFormatNormal(hdc, visAttribs);
}

GLboolean renderspu_SystemCreateWindow( VisualInfo *visual, GLboolean showIt, WindowInfo *window )
{
    HDESK     desktop;
    HINSTANCE hinstance;
    WNDCLASS  wc;
    DWORD     window_style;
    int       window_plus_caption_width;
    int       window_plus_caption_height;

    window->hRgn = NULL;
    window->visual = visual;
    window->nativeWindow = 0;

    if ( render_spu.use_L2 )
    {
        crWarning( "Going fullscreen because we think we're using Lightning-2." );
        render_spu.fullscreen = 1;
    }

    /*
     * Begin Windows / WGL code
     */

    hinstance = GetModuleHandle( NULL );
    if (!hinstance)
    {
        crError( "Render SPU: Couldn't get a handle to my module." );
        return GL_FALSE;
    }
    crDebug( "Render SPU: Got the module handle: 0x%x", hinstance );

    /* If we were launched from a service, telnet, or rsh, we need to
     * get the input desktop.  */

    desktop = OpenInputDesktop( 0, FALSE,
            DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
            DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
            DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
            DESKTOP_SWITCHDESKTOP | GENERIC_WRITE );

    if ( !desktop )
    {
        crError( "Render SPU: Couldn't acquire input desktop" );
        return GL_FALSE;
    }
    crDebug( "Render SPU: Got the desktop: 0x%x", desktop );

    if ( !SetThreadDesktop( desktop ) )
    {
        /* If this function fails, it's probably because 
         * it's already been called (i.e., the render SPU 
         * is bolted to an application?) */

        /*crError( "Couldn't set thread to input desktop" ); */
    }
    crDebug( "Render SPU: Set the thread desktop -- this might have failed." );

    if ( !GetClassInfo(hinstance, WINDOW_NAME, &wc) ) 
    {
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = (WNDPROC) MainWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
        wc.hCursor = LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WINDOW_NAME;

        if ( !RegisterClass( &wc ) )
        {
            crError( "Render SPU: Couldn't register window class -- you're not trying "
                    "to do multi-pipe stuff on windows, are you?\n\nNote --"
                    "This error message is from 1997 and probably doesn't make"
                    "any sense any more, but it's nostalgic for Humper." );
            return GL_FALSE;
        }
        crDebug( "Render SPU: Registered the class" );
    }
    crDebug( "Render SPU: Got the class information" );

    /* Full screen window should be a popup (undecorated) window */
#if 1
    window_style = ( render_spu.fullscreen ? WS_POPUP : WS_CAPTION );
#else
    window_style = ( WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN );
    window_style |= WS_SYSMENU;
#endif

    crDebug( "Render SPU: Fullscreen: %s", render_spu.fullscreen ? "yes" : "no");

    if ( render_spu.fullscreen )
    {
#if 0

        int smCxFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME );
        int smCyFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME ) + 1;
        int smCyCaption = GetSystemMetrics( SM_CYCAPTION );

        window->BltInfo.width = GetSystemMetrics( SM_CXSCREEN ) ;
        window->BltInfo.height = GetSystemMetrics( SM_CYSCREEN ) ;

        crDebug( "Render SPU: Window Dims: %d, %d", window->BltInfo.width, window->BltInfo.height );

        window->x = render_spu->defaultX - smCxFixedFrame - 1;
        window->y = render_spu->defaultY - smCyFixedFrame - smCyCaption;

        window_plus_caption_width = window->BltInfo.width + 2 * smCxFixedFrame;
        window_plus_caption_height = window->BltInfo.height + 2 * smCyFixedFrame + smCyCaption;

#else
        /* Since it's undecorated, we don't have to do anything fancy
         * with these parameters. */

        window->BltInfo.width = GetSystemMetrics( SM_CXSCREEN ) ;
        window->BltInfo.height = GetSystemMetrics( SM_CYSCREEN ) ;
        window->x = 0;
        window->y = 0;
        window_plus_caption_width = window->BltInfo.width;
        window_plus_caption_height = window->BltInfo.height;

#endif
    }
    else
    {
        /* CreateWindow takes the size of the entire window, so we add
         * in the size necessary for the frame and the caption. */

        int smCxFixedFrame, smCyFixedFrame, smCyCaption;
        smCxFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME );
        crDebug( "Render SPU: Got the X fixed frame" );
        smCyFixedFrame = GetSystemMetrics( SM_CYFIXEDFRAME );
        crDebug( "Render SPU: Got the Y fixed frame" );
        smCyCaption = GetSystemMetrics( SM_CYCAPTION );
        crDebug( "Render SPU: Got the Caption " );

        window_plus_caption_width = window->BltInfo.width + 2 * smCxFixedFrame;
        window_plus_caption_height = window->BltInfo.height + 2 * smCyFixedFrame + smCyCaption;

        window->x = render_spu.defaultX - smCxFixedFrame;
        window->y = render_spu.defaultY - smCyFixedFrame - smCyCaption;
    }

    crDebug( "Render SPU: Creating the window: (%d,%d), (%d,%d)", render_spu.defaultX, render_spu.defaultY, window_plus_caption_width, window_plus_caption_height );
    window->hWnd = CreateWindow( WINDOW_NAME, WINDOW_NAME,
            window_style,
            window->x, window->y,
            window_plus_caption_width,
            window_plus_caption_height,
            NULL, NULL, hinstance, &render_spu );

    if ( !window->hWnd )
    {
        crError( "Render SPU: Create Window failed!  That's almost certainly terrible." );
        return GL_FALSE;
    }

    window->visible = showIt;

    if (!showIt)
    {
        renderspu_SystemShowWindow( window, 0 );
        if (window->BltInfo.height <= 0 || window->BltInfo.width <= 0)
        {
            renderspu_SystemWindowSize(window,
                    window->BltInfo.width > 0 ? window->BltInfo.width : 4,
                    window->BltInfo.height > 0 ? window->BltInfo.height : 4);
        }
    }
    else
    {
        crDebug( "Render SPU: Showing the window" );
        crDebug("renderspu_SystemCreateWindow: showwindow: %x", window->hWnd);
    }

    CRASSERT(!window->visible == !showIt);

    /* Intel drivers require a window to be visible for proper 3D rendering,
     * so set it visible and handle the visibility with visible regions (see below) */
    ShowWindow( window->hWnd, SW_SHOWNORMAL );

    SetForegroundWindow( window->hWnd );

    SetWindowPos( window->hWnd, HWND_TOP, window->x, window->y,
              window_plus_caption_width, window_plus_caption_height,
              ( render_spu.fullscreen ? (SWP_SHOWWINDOW |
                         SWP_NOSENDCHANGING | 
                         SWP_NOREDRAW | 
                         SWP_NOACTIVATE ) :
                         0 ) );

    if ( render_spu.fullscreen )
        ShowCursor( FALSE );

    window->device_context = GetDC( window->hWnd );
    if (!window->device_context)
    {
        DWORD winEr = GetLastError();
        crWarning("GetDC failed, winEr %d", winEr);
    }

    crDebug( "Render SPU: Got the DC: 0x%x", window->device_context );

    if ( !bSetupPixelFormat( window->device_context, visual->visAttribs ) )
    {
        crError( "Render SPU: Couldn't set up the device context!  Yikes!" );
        return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean renderspu_SystemVBoxCreateWindow( VisualInfo *visual, GLboolean showIt, WindowInfo *window )
{
#if 0
    HDESK     desktop;
#endif
    HINSTANCE hinstance;
    WNDCLASS  wc;
    DWORD     window_style;
    int       window_plus_caption_width;
    int       window_plus_caption_height;

    window->hRgn = NULL;
    window->visual = visual;
    window->nativeWindow = 0;

    if ( render_spu.use_L2 )
    {
        crWarning( "Going fullscreen because we think we're using Lightning-2." );
        render_spu.fullscreen = 1;
    }

    /*
     * Begin Windows / WGL code
     */

    hinstance = GetModuleHandle( NULL );
    if (!hinstance)
    {
        crError( "Render SPU: Couldn't get a handle to my module." );
        return GL_FALSE;
    }
    crDebug( "Render SPU: Got the module handle: 0x%x", hinstance );

#if 0
    /* If we were launched from a service, telnet, or rsh, we need to
     * get the input desktop.  */

    desktop = OpenInputDesktop( 0, FALSE,
            DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
            DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
            DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
            DESKTOP_SWITCHDESKTOP | GENERIC_WRITE );

    if ( !desktop )
    {
        crError( "Render SPU: Couldn't acquire input desktop" );
        return GL_FALSE;
    }
    crDebug( "Render SPU: Got the desktop: 0x%x", desktop );

    if ( !SetThreadDesktop( desktop ) )
    {
        /* If this function fails, it's probably because 
         * it's already been called (i.e., the render SPU 
         * is bolted to an application?) */

        /*crError( "Couldn't set thread to input desktop" ); */
    }
    crDebug( "Render SPU: Set the thread desktop -- this might have failed." );
#endif

    if ( !GetClassInfo(hinstance, WINDOW_NAME, &wc) ) 
    {
        wc.style = CS_OWNDC; // | CS_PARENTDC;
        wc.lpfnWndProc = (WNDPROC) MainWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hinstance;
        wc.hIcon = NULL; //LoadIcon( NULL, IDI_APPLICATION );
        wc.hCursor = NULL; //LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = WINDOW_NAME;                       

        if ( !RegisterClass( &wc ) )
        {
            crError( "Render SPU: Couldn't register window class -- you're not trying "
                    "to do multi-pipe stuff on windows, are you?\n\nNote --"
                    "This error message is from 1997 and probably doesn't make"
                    "any sense any more, but it's nostalgic for Humper." );
            return GL_FALSE;
        }
        crDebug( "Render SPU: Registered the class" );
    }
    crDebug( "Render SPU: Got the class information" );

    /* Full screen window should be a popup (undecorated) window */
#if 1
    window_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DISABLED;
    if (render_spu_parent_window_id)
    {
        window_style |= WS_CHILD;
    }
#else
    window_style = ( WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN );
    window_style |= WS_SYSMENU;
#endif

    crDebug( "Render SPU: Fullscreen: %s", render_spu.fullscreen ? "yes" : "no");

    if ( render_spu.fullscreen )
    {
#if 0

        int smCxFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME );
        int smCyFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME ) + 1;
        int smCyCaption = GetSystemMetrics( SM_CYCAPTION );

        window->BltInfo.width = GetSystemMetrics( SM_CXSCREEN ) ;
        window->BltInfo.height = GetSystemMetrics( SM_CYSCREEN ) ;

        crDebug( "Render SPU: Window Dims: %d, %d", window->BltInfo.width, window->BltInfo.height );

        window->x = render_spu->defaultX - smCxFixedFrame - 1;
        window->y = render_spu->defaultY - smCyFixedFrame - smCyCaption;

        window_plus_caption_width = window->BltInfo.width + 2 * smCxFixedFrame;
        window_plus_caption_height = window->BltInfo.height + 2 * smCyFixedFrame + smCyCaption;

#else
        /* Since it's undecorated, we don't have to do anything fancy
         * with these parameters. */

        window->BltInfo.width = GetSystemMetrics( SM_CXSCREEN ) ;
        window->BltInfo.height = GetSystemMetrics( SM_CYSCREEN ) ;
        window->x = 0;
        window->y = 0;
        window_plus_caption_width = window->BltInfo.width;
        window_plus_caption_height = window->BltInfo.height;

#endif
    }
    else
    {
        /* CreateWindow takes the size of the entire window, so we add
         * in the size necessary for the frame and the caption. */
        int smCxFixedFrame, smCyFixedFrame, smCyCaption;
        smCxFixedFrame = GetSystemMetrics( SM_CXFIXEDFRAME );
        crDebug( "Render SPU: Got the X fixed frame" );
        smCyFixedFrame = GetSystemMetrics( SM_CYFIXEDFRAME );
        crDebug( "Render SPU: Got the Y fixed frame" );
        smCyCaption = GetSystemMetrics( SM_CYCAPTION );
        crDebug( "Render SPU: Got the Caption " );

        window_plus_caption_width = window->BltInfo.width + 2 * smCxFixedFrame;
        window_plus_caption_height = window->BltInfo.height + 2 * smCyFixedFrame + smCyCaption;

        window->x = render_spu.defaultX;
        window->y = render_spu.defaultY;
    }

    crDebug( "Render SPU: Creating the window: (%d,%d), (%d,%d)", render_spu.defaultX, render_spu.defaultY, window_plus_caption_width, window_plus_caption_height );
    /*window->hWnd = CreateWindowEx( WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY,
            WINDOW_NAME, WINDOW_NAME,
            window_style,
            window->x, window->y,
            window->BltInfo.width,
            window->BltInfo.height,
            (void*) render_spu_parent_window_id, NULL, hinstance, &render_spu );*/
    {
        CREATESTRUCT cs;

        cs.lpCreateParams = window;

        cs.dwExStyle    = WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY;
        cs.lpszName     = WINDOW_NAME;
        cs.lpszClass    = WINDOW_NAME;
        cs.style        = window_style;
        cs.x            = window->x;
        cs.y            = window->y;
        cs.cx           = window->BltInfo.width;
        cs.cy           = window->BltInfo.height;
        cs.hwndParent   = (void*) render_spu_parent_window_id;
        cs.hMenu        = NULL;
        cs.hInstance    = hinstance;

        if (render_spu.dwWinThreadId)
        {
            DWORD res;
            int cnt=0;

            if (!PostThreadMessage(render_spu.dwWinThreadId, WM_VBOX_RENDERSPU_CREATE_WINDOW, 0, (LPARAM) &cs))
            {
                crError("Render SPU: PostThreadMessage failed with %i", GetLastError());
                return GL_FALSE;
            }

            do
            {
                res = WaitForSingleObject(render_spu.hWinThreadReadyEvent, 1000);
                cnt++;
            }
            while ((res!=WAIT_OBJECT_0) && (cnt<10));

            crDebug("Render SPU: window thread waited %i secs", cnt);

            if (res!=WAIT_OBJECT_0)
            {
                crError("Render SPU: window thread not responded after %i tries", cnt);
                return GL_FALSE;
            }
        }
        else
        {
            crError("Render SPU: window thread is not running");
            return GL_FALSE;
        }
    }

    if ( !window->hWnd )
    {
        crError( "Render SPU: Create Window failed!  That's almost certainly terrible." );
        return GL_FALSE;
    }

    window->visible = 1;

    if (!showIt)
    {
        renderspu_SystemShowWindow( window, 0 );
        if (window->BltInfo.height <= 0 || window->BltInfo.width <= 0)
        {
            renderspu_SystemWindowSize(window,
                    window->BltInfo.width > 0 ? window->BltInfo.width : 4,
                    window->BltInfo.height > 0 ? window->BltInfo.height : 4);
        }
    }
    else
    {
#ifdef DEBUG_misha
        crWarning( "Render SPU: Showing the window" );
#else
        crDebug( "Render SPU: Showing the window" );
#endif
        crDebug("renderspu_SystemCreateWindow: showwindow: %x", window->hWnd);
    }

    CRASSERT(!window->visible == !showIt);

    /* Intel drivers require a window to be visible for proper 3D rendering,
     * so set it visible and handle the visibility with visible regions (see below) */
    if (window->BltInfo.Base.id != CR_RENDER_DEFAULT_WINDOW_ID)
    {
        ShowWindow( window->hWnd, SW_SHOWNORMAL );
    }
    else
    {
        CRASSERT(!showIt);
        /* dummy window is always hidden in any way */
    }

    //SetForegroundWindow( visual->hWnd );

    SetWindowPos( window->hWnd, HWND_TOP, window->x, window->y,
                  window->BltInfo.width, window->BltInfo.height,
                  ( render_spu.fullscreen ? 
                    (SWP_SHOWWINDOW | SWP_NOSENDCHANGING | SWP_NOREDRAW | SWP_NOACTIVATE ) : SWP_NOACTIVATE 
                  ) );
    crDebug("Render SPU: SetWindowPos (%x, %d, %d, %d, %d)", window->hWnd, 
            window->x, window->y, window->BltInfo.width, window->BltInfo.height);

    if ( render_spu.fullscreen )
        ShowCursor( FALSE );

    window->device_context = GetDC( window->hWnd );
    if (!window->device_context)
    {
        DWORD winEr = GetLastError();
        crWarning("GetDC failed, winEr %d", winEr);
    }

    crDebug( "Render SPU: Got the DC: 0x%x", window->device_context );

    if ( !bSetupPixelFormat( window->device_context, visual->visAttribs ) )
    {
        crError( "Render SPU: Couldn't set up the device context!  Yikes!" );
        return GL_FALSE;
    }

    /* set the window pointer data at the last step to ensure our WM_PAINT callback does not do anything until we are fully initialized */
    {
        LONG_PTR oldVal = SetWindowLongPtr(window->hWnd, GWLP_USERDATA, (LONG_PTR)window);
        DWORD winEr = GetLastError();
        Assert(!oldVal && winEr == NO_ERROR);
    }

    return GL_TRUE;
}

static void renderspuWindowRgnApply(WindowInfo *window)
{
    HRGN hRgn = window->hRgn;
    if (hRgn)
    {
        /* note: according to the docs, SetWindowRgn owns the regions after it is called,
         * and the regions will be freed automatically when needed,
         * i.e. the caller should not do that.
         * this is why we need to make a copy of the regions to be passed in */

        int result;
        hRgn = CreateRectRgn(0, 0, 0, 0);
        if (!hRgn)
        {
            WARN(("CreateRectRgn failed"));
            return;
        }

        result =  CombineRgn(hRgn, window->hRgn, NULL, RGN_COPY);
        if (result == ERROR)
        {
            WARN(("CombineRgn failed"));
            return;
        }
    }

    SetWindowRgn(window->hWnd, hRgn, true);
}

/* Either show or hide the render SPU's window. */
void renderspu_SystemShowWindow( WindowInfo *window, GLboolean showIt )
{
    if (showIt)
    {
        crDebug("SHOW renderspu_SystemShowWindow: %x", window->hWnd);
        renderspuWindowRgnApply(window);
    }
    else
    {
        HRGN hRgn;
        crDebug("HIDE renderspu_SystemShowWindow: %x", window->hWnd);
        hRgn = CreateRectRgn(0, 0, 0, 0);
        SetWindowRgn(window->hWnd, hRgn, true);
        /* note: according to the docs, SetWindowRgn owns the regions after it is called,
         * and the regions will be freed automatically when needed,
         * i.e. the caller should not do that */
    }
    window->visible = showIt;
}

void renderspu_SystemVBoxPresentComposition( WindowInfo *window, const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry )
{
    /* the CR_RENDER_FORCE_PRESENT_MAIN_THREAD is actually inherited from cocoa backend impl,
     * here it forces rendering in WinCmd thread rather than a Main thread.
     * it is used for debugging only in any way actually.
     * @todo: change to some more generic macro name */
#ifndef CR_RENDER_FORCE_PRESENT_MAIN_THREAD
    const struct VBOXVR_SCR_COMPOSITOR *pCompositor;
    /* we do not want to be blocked with the GUI thread here, so only draw her eif we are really able to do that w/o bllocking */
    int rc = renderspuVBoxCompositorTryAcquire(window, &pCompositor);
    if (RT_SUCCESS(rc))
    {
        renderspuVBoxPresentCompositionGeneric(window, pCompositor, pChangedEntry, 0, false);
        renderspuVBoxCompositorRelease(window);
    }
    else if (rc == VERR_SEM_BUSY)
#endif
    {
        render_spu.self.Flush();
        renderspuVBoxPresentBlitterEnsureCreated(window, 0);
        RedrawWindow(window->hWnd, NULL, NULL, RDW_INTERNALPAINT);
    }
#ifndef CR_RENDER_FORCE_PRESENT_MAIN_THREAD
    else
    {
        /* this is somewhat we do not expect */
        crWarning("renderspuVBoxCompositorTryAcquire failed rc %d", rc);
    }
#endif
}

GLboolean renderspu_SystemCreateContext( VisualInfo *visual, ContextInfo *context, ContextInfo *sharedContext )
{
    (void) sharedContext;
    context->visual = visual;

    /* Found a visual, so we're o.k. to create the context now */
    if (0/*visual->device_context*/) {

        //crDebug( "Render SPU: Using the DC: 0x%x", visual->device_context );

        //context->hRC = render_spu.ws.wglCreateContext( visual->device_context );
        if (!context->hRC)
        {
            crError( "Render SPU: wglCreateContext failed (error 0x%x)", GetLastError() );
            return GL_FALSE;
        }
    } else {
        crDebug( "Render SPU: Delaying DC creation " );
        context->hRC = NULL;    /* create it later in makecurrent */
    }


    return GL_TRUE;
}

void renderspu_SystemDestroyContext( ContextInfo *context )
{
    render_spu.ws.wglDeleteContext( context->hRC );
    context->hRC = NULL;
}

static GLboolean renderspuChkActivateSharedContext(ContextInfo *sharedContext)
{
    WindowInfo *window;

    if (sharedContext->hRC)
        return GL_TRUE;

    CRASSERT(sharedContext->BltInfo.Base.id);

    if (sharedContext->shared)
        renderspuChkActivateSharedContext(sharedContext->shared);

    window = renderspuGetDummyWindow(sharedContext->visual->visAttribs);
    if (!window)
    {
        crError("renderspuChkActivateSharedContext: renderspuGetDummyWindow failed!");
        return GL_FALSE;
    }

    CRASSERT(window->device_context);

    crDebug( "Render SPU: renderspuChkActivateSharedContext: made the DC: 0x%x", window->device_context );

    sharedContext->hRC = render_spu.ws.wglCreateContext(window->device_context);
    if (!sharedContext->hRC)
    {
        crError( "Render SPU: (renderspuChkActivateSharedContext) Couldn't create the context for the window (error 0x%x)", GetLastError() );
        return GL_FALSE;
    }

    return GL_TRUE;
}

void renderspu_SystemMakeCurrent( WindowInfo *window, GLint nativeWindow, ContextInfo *context )
{
    CRASSERT(render_spu.ws.wglMakeCurrent);

    if (context && window) {
        if (window->visual != context->visual) {
            /*
             * XXX have to revisit this issue!!!
             *
             * But for now we destroy the current window
             * and re-create it with the context's visual abilities
             */

            /*@todo Chromium has no correct code to remove window ids and associated info from 
             * various tables. This is hack which just hides the root case.
             */
            crWarning("Recreating window in renderspu_SystemMakeCurrent\n");
            renderspu_SystemDestroyWindow( window );
            renderspu_SystemVBoxCreateWindow( context->visual, window->visible, window );
        }

        if (0/*render_spu.render_to_app_window && nativeWindow*/)
        {
            /* The render_to_app_window option 
             * is set and we've got a nativeWindow
             * handle, save the handle for 
             * later calls to swapbuffers().
             *
             * NOTE: This doesn't work, except 
             * for software driven Mesa.
             * We'd need to object link the 
             * crappfaker and crserver to be able to share
             * the HDC values between processes.. FIXME!
             */
            if (context->shared)
            {
                /* first make sure we have shared context created */
                renderspuChkActivateSharedContext(context->shared);
            }

            window->nativeWindow = (HDC) nativeWindow;
            if (context->hRC == 0) {
                context->hRC = render_spu.ws.wglCreateContext( window->nativeWindow );
                if (!context->hRC)
                {
                    crError( "(MakeCurrent) Couldn't create the context for the window (error 0x%x)", GetLastError() );
                }
            }

            if (context->shared
                    && context->shared->hRC
                    && context->hRC)
            {
                /* share lists */
                render_spu.ws.wglShareLists(context->shared->hRC, context->hRC);
            }

            render_spu.ws.wglMakeCurrent( window->nativeWindow, context->hRC );
        }
        else
        {
            if (!context->hRC) {
                CRASSERT(!nativeWindow);
                if (context->shared)
                {
                    /* first make sure we have shared context created */
                    renderspuChkActivateSharedContext(context->shared);
                }

                context->hRC = render_spu.ws.wglCreateContext(window->device_context);
                if (!context->hRC)
                {
                    crError( "Render SPU: (MakeCurrent) Couldn't create the context for the window (error 0x%x)", GetLastError() );
                }

                if (context->shared
                        && context->shared->hRC
                        && context->hRC)
                {
                    /* share lists */
                    BOOL bRc = render_spu.ws.wglShareLists(context->shared->hRC, context->hRC);
                    if (!bRc)
                    {
                        DWORD winEr = GetLastError();
                        crWarning("wglShareLists failed, winEr %d", winEr);
                    }
                }

                /*Requery ext function pointers, we skip dummy ctx as it should never be used with ext functions*/
                if (0 && context->BltInfo.Base.id)
                {
                    int numFuncs, i;
                    SPUNamedFunctionTable ext_table[1000];


                    crDebug("Default server ctx created, requerying ext functions");
                    /*requery ext functions*/
                    numFuncs = renderspuCreateFunctions(ext_table);
                    numFuncs += crLoadOpenGLExtensions( &render_spu.ws, ext_table+numFuncs);
                    CRASSERT(numFuncs < 1000);

                    /*change spu dispatch*/
                    crSPUChangeDispatch(&render_spu.self, ext_table);


                    /*cleanup temp table*/
                    for (i=0; i<numFuncs; ++i)
                    {
                        if (ext_table[i].name) crFree(ext_table[i].name);
                    }
                }
            }

            /*crDebug("MakeCurrent 0x%x, 0x%x", window->device_context, context->hRC);*/
            if (!render_spu.ws.wglMakeCurrent(!nativeWindow ? window->device_context : window->redraw_device_context, context->hRC))
            {
                DWORD err = GetLastError();
                crError("Render SPU: (MakeCurrent) failed to make 0x%x, 0x%x current with 0x%x error.", window->device_context, context->hRC, err);
            }
        }

        renderspuAtiQuirk_ChkApply();
    }
    else {
        render_spu.ws.wglMakeCurrent( 0, 0 );
    }
}

void renderspu_SystemWindowSize( WindowInfo *window, GLint w, GLint h )
{
    int winprop;
    CRASSERT(window);
    CRASSERT(window->visual);
    if ( render_spu.fullscreen )
        winprop = SWP_SHOWWINDOW | SWP_NOSENDCHANGING |
              SWP_NOREDRAW | SWP_NOACTIVATE;
    else 
        winprop = SWP_NOACTIVATE | SWP_DEFERERASE | SWP_NOSENDCHANGING | SWP_NOZORDER; //SWP_SHOWWINDOW;

    /*SetWindowRgn(window->hWnd, NULL, false);*/

    if (!SetWindowPos( window->hWnd, HWND_TOP,
            window->x, window->y, w, h, winprop )) {
        crWarning("!!!FAILED!!! Render SPU: SetWindowPos (%x, %d, %d, %d, %d)", window->hWnd, window->x, window->y, w, h);
    } else {
        crDebug("Render SPU: SetWindowSize (%x, %d, %d, %d, %d)", window->hWnd, window->x, window->y, w, h);
    }
    /* save the new size */
    window->BltInfo.width = w;
    window->BltInfo.height = h;
}


void renderspu_SystemGetWindowGeometry( WindowInfo *window, GLint *x, GLint *y, GLint *w, GLint *h )
{
    RECT rect;

    CRASSERT(window);
    CRASSERT(window->visual);

    GetClientRect( window->hWnd, &rect );
    *x = rect.left;
    *y = rect.top;
    *w = rect.right - rect.left;
    *h = rect.bottom - rect.top;
}


void renderspu_SystemGetMaxWindowSize( WindowInfo *window, GLint *w, GLint *h )
{
    /* XXX fix this */
    (void) window;
    *w = 1600;
    *h = 1200;
}


void renderspu_SystemWindowPosition( WindowInfo *window, GLint x, GLint y )
{
    int winprop;
    CRASSERT(window);
    CRASSERT(window->visual);
    if ( render_spu.fullscreen )
        winprop = SWP_SHOWWINDOW | SWP_NOSENDCHANGING |
              SWP_NOREDRAW | SWP_NOACTIVATE;
    else 
        winprop = SWP_NOACTIVATE | SWP_DEFERERASE | SWP_NOSENDCHANGING | SWP_NOZORDER; //SWP_SHOWWINDOW;

    /*SetWindowRgn(window->visual->hWnd, NULL, false);*/

    if (!SetWindowPos( window->hWnd, HWND_TOP, 
            x, y, window->BltInfo.width, window->BltInfo.height, winprop )) {
        crWarning("!!!FAILED!!! Render SPU: SetWindowPos (%x, %d, %d, %d, %d)", window->hWnd, x, y, window->BltInfo.width, window->BltInfo.height);
    } else {
        crDebug("Render SPU: SetWindowPos (%x, %d, %d, %d, %d)", window->hWnd,
                x, y, window->BltInfo.width, window->BltInfo.height);
    }
    /* save the new position */
    window->x = x;
    window->y = y;
}

GLboolean renderspu_SystemWindowNeedEmptyPresent(WindowInfo *window)
{
    return GL_FALSE;
}

void renderspu_SystemWindowVisibleRegion(WindowInfo *window, GLint cRects, const GLint* pRects)
{
    GLint i;
    HRGN hRgn, hTmpRgn;

    CRASSERT(window);
    CRASSERT(window->visual);

    if (window->hRgn)
    {
        DeleteObject(window->hRgn);
        window->hRgn = NULL;
    }

    hRgn = CreateRectRgn(0, 0, 0, 0);

    for (i=0; i<cRects; i++)
    {
        hTmpRgn = CreateRectRgn(pRects[4*i], pRects[4*i+1], pRects[4*i+2], pRects[4*i+3]);
        CombineRgn(hRgn, hRgn, hTmpRgn, RGN_OR);
        DeleteObject(hTmpRgn);
    }

    window->hRgn = hRgn;

    if (window->visible)
        renderspuWindowRgnApply(window);

    crDebug("Render SPU: SetWindowRgn (%x, cRects=%i)", window->hWnd, cRects);
}

static void renderspuHandleWindowMessages( HWND hWnd )
{
    MSG msg;
    while ( PeekMessage( &msg, hWnd, 0, 0xffffffff, PM_REMOVE ) )
    {
        TranslateMessage( &msg );    
        DispatchMessage( &msg );
    }
    
    //BringWindowToTop( hWnd );
}

void renderspu_SystemSwapBuffers( WindowInfo *w, GLint flags )
{
    int return_value;

    /* peek at the windows message queue */
//    renderspuHandleWindowMessages( w->hWnd );

    /* render_to_app_window:
     * w->nativeWindow will only be non-zero if the
     * render_spu.render_to_app_window option is true and
     * MakeCurrent() recorded the nativeWindow handle in the WindowInfo
     * structure.
     */
    if (render_spu.render_to_app_window && w->nativeWindow) {
#ifdef VBOX_CR_SERVER_FORCE_WGL
        return_value = render_spu.ws.wglSwapBuffers( w->nativeWindow );
#else
        return_value = SwapBuffers( w->nativeWindow );
#endif
    } else {
        /*
        HRGN hRgn1, hRgn2, hRgn3;
        HWND hWndParent;
        LONG ws;

        hRgn1 = CreateRectRgn(0, 0, w->BltInfo.width, w->BltInfo.height);
        hRgn2 = CreateRectRgn(50, 50, 100, 100);
        hRgn3 = CreateRectRgn(0, 0, 0, 0);
        CombineRgn(hRgn3, hRgn1, hRgn2, RGN_DIFF);
        SetWindowRgn(w->visual->hWnd, hRgn3, true);
        DeleteObject(hRgn1);
        DeleteObject(hRgn2);

        hWndParent = GetParent(w->visual->hWnd);
        ws = GetWindowLong(hWndParent, GWL_STYLE);
        ws &= ~WS_CLIPCHILDREN;
        SetWindowLong(hWndParent, GWL_STYLE, ws);

        RECT rcClip;        

        rcClip.left = 50;
        rcClip.top  = 50;
        rcClip.right = 100;
        rcClip.bottom = 100;
        ValidateRect(w->visual->hWnd, &rcClip);

        return_value = GetClipBox(w->visual->device_context, &rcClip);
        crDebug("GetClipBox returned %d (NR=%d,SR=%d,CR=%d,ERR=%d)", 
                return_value, NULLREGION, SIMPLEREGION, COMPLEXREGION, ERROR);

        crDebug("rcClip(%d, %d, %d, %d)", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

        return_value = ExcludeClipRect(w->visual->device_context, 50, 50, 100, 100);
        crDebug("ExcludeClipRect returned %d (NR=%d,SR=%d,CR=%d,ERR=%d)", 
                return_value, NULLREGION, SIMPLEREGION, COMPLEXREGION, ERROR);

        return_value = GetClipBox(w->visual->device_context, &rcClip);
        crDebug("GetClipBox returned %d (NR=%d,SR=%d,CR=%d,ERR=%d)", 
                return_value, NULLREGION, SIMPLEREGION, COMPLEXREGION, ERROR);
        crDebug("rcClip(%d, %d, %d, %d)", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        */
#ifdef VBOX_CR_SERVER_FORCE_WGL
        return_value = render_spu.ws.wglSwapBuffers( w->device_context );
#else
        return_value = SwapBuffers( w->device_context );
#endif
    }
    if (!return_value)
    {
        /* GOD DAMN IT.  The latest versions of the NVIDIA drivers
        * return failure from wglSwapBuffers, but it works just fine.
        * WHAT THE HELL?! */

        crWarning( "wglSwapBuffers failed: return value of %d!", return_value);
    }
}

void renderspu_SystemReparentWindow(WindowInfo *window)
{
    SetParent(window->hWnd, (HWND)render_spu_parent_window_id);
}

int renderspu_SystemInit()
{
    return VINF_SUCCESS;
}

int renderspu_SystemTerm()
{
    return VINF_SUCCESS;
}

void renderspu_SystemDefaultSharedContextChanged(ContextInfo *fromContext, ContextInfo *toContext)
{

}

uint32_t renderspu_SystemPostprocessFunctions(SPUNamedFunctionTable *aFunctions, uint32_t cFunctions, uint32_t cTable)
{
    return cFunctions;
}
