/* $Id: Framebuffer.cpp $ */
/** @file
 * VBoxSDL - Implementation of VBoxSDLFB (SDL framebuffer) class
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/stream.h>
#include <iprt/env.h>

#ifdef RT_OS_OS2
# undef RT_MAX
// from <iprt/cdefs.h>
# define RT_MAX(Value1, Value2)  ((Value1) >= (Value2) ? (Value1) : (Value2))
#endif

using namespace com;

#define LOG_GROUP LOG_GROUP_GUI
#include <VBox/err.h>
#include <VBox/log.h>

#include "VBoxSDL.h"
#include "Framebuffer.h"
#include "Ico64x01.h"

#if defined(RT_OS_WINDOWS) || defined(RT_OS_LINUX)
# ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4121) /* warning C4121: 'SDL_SysWMmsg' : alignment of a member was sensitive to packing*/
# endif
# include <SDL_syswm.h>           /* for SDL_GetWMInfo() */
# ifdef _MSC_VER
#  pragma warning(pop)
# endif
#endif

#if defined(VBOX_WITH_XPCOM)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI(VBoxSDLFB, IFramebuffer)
NS_DECL_CLASSINFO(VBoxSDLFB)
NS_IMPL_THREADSAFE_ISUPPORTS2_CI(VBoxSDLFBOverlay, IFramebufferOverlay, IFramebuffer)
NS_DECL_CLASSINFO(VBoxSDLFBOverlay)
#endif

#ifdef VBOX_SECURELABEL
/* function pointers */
extern "C"
{
DECLSPEC int (SDLCALL *pTTF_Init)(void);
DECLSPEC TTF_Font* (SDLCALL *pTTF_OpenFont)(const char *file, int ptsize);
DECLSPEC SDL_Surface* (SDLCALL *pTTF_RenderUTF8_Solid)(TTF_Font *font, const char *text, SDL_Color fg);
DECLSPEC SDL_Surface* (SDLCALL *pTTF_RenderUTF8_Blended)(TTF_Font *font, const char *text, SDL_Color fg);
DECLSPEC void (SDLCALL *pTTF_CloseFont)(TTF_Font *font);
DECLSPEC void (SDLCALL *pTTF_Quit)(void);
}
#endif /* VBOX_SECURELABEL */

static bool gfSdlInitialized = false;           /**< if SDL was initialized */
static SDL_Surface *gWMIcon = NULL;             /**< the application icon */
static RTNATIVETHREAD gSdlNativeThread = NIL_RTNATIVETHREAD; /**< the SDL thread */

//
// Constructor / destructor
//

VBoxSDLFB::VBoxSDLFB()
{
}

HRESULT VBoxSDLFB::FinalConstruct()
{
    return 0;
}

void VBoxSDLFB::FinalRelease()
{
    return;
}

/**
 * SDL framebuffer constructor. It is called from the main
 * (i.e. SDL) thread. Therefore it is safe to use SDL calls
 * here.
 * @param fFullscreen    flag whether we start in fullscreen mode
 * @param fResizable     flag whether the SDL window should be resizable
 * @param fShowSDLConfig flag whether we print out SDL settings
 * @param fKeepHostRes   flag whether we switch the host screen resolution
 *                       when switching to fullscreen or not
 * @param iFixedWidth    fixed SDL width (-1 means not set)
 * @param iFixedHeight   fixed SDL height (-1 means not set)
 */
HRESULT VBoxSDLFB::init(uint32_t uScreenId,
                     bool fFullscreen, bool fResizable, bool fShowSDLConfig,
                     bool fKeepHostRes, uint32_t u32FixedWidth,
                     uint32_t u32FixedHeight, uint32_t u32FixedBPP,
                     bool fUpdateImage)
{
    int rc;
    LogFlow(("VBoxSDLFB::VBoxSDLFB\n"));

    mScreenId       = uScreenId;
    mfUpdateImage   = fUpdateImage;
    mScreen         = NULL;
#ifdef VBOX_WITH_SDL13
    mWindow         = 0;
    mTexture        = 0;
#endif
    mSurfVRAM       = NULL;
    mfInitialized   = false;
    mfFullscreen    = fFullscreen;
    mfKeepHostRes   = fKeepHostRes;
    mTopOffset      = 0;
    mfResizable     = fResizable;
    mfShowSDLConfig = fShowSDLConfig;
    mFixedSDLWidth  = u32FixedWidth;
    mFixedSDLHeight = u32FixedHeight;
    mFixedSDLBPP    = u32FixedBPP;
    mCenterXOffset  = 0;
    mCenterYOffset  = 0;
    /* Start with standard screen dimensions. */
    mGuestXRes      = 640;
    mGuestYRes      = 480;
    mPtrVRAM        = NULL;
    mBitsPerPixel   = 0;
    mBytesPerLine   = 0;
    mfSameSizeRequested = false;
#ifdef VBOX_SECURELABEL
    mLabelFont      = NULL;
    mLabelHeight    = 0;
    mLabelOffs      = 0;
#endif

    mfUpdates = false;

    rc = RTCritSectInit(&mUpdateLock);
    AssertMsg(rc == VINF_SUCCESS, ("Error from RTCritSectInit!\n"));

    resizeGuest();
    Assert(mScreen);
    mfInitialized = true;
#ifdef RT_OS_WINDOWS
    HRESULT hr = CoCreateFreeThreadedMarshaler(this, m_pUnkMarshaler.asOutParam());
    Log(("CoCreateFreeThreadedMarshaler hr %08X\n", hr)); NOREF(hr);
#endif

    return 0;
}

VBoxSDLFB::~VBoxSDLFB()
{
    LogFlow(("VBoxSDLFB::~VBoxSDLFB\n"));
    if (mSurfVRAM)
    {
        SDL_FreeSurface(mSurfVRAM);
        mSurfVRAM = NULL;
    }
    mScreen = NULL;

#ifdef VBOX_SECURELABEL
    if (mLabelFont)
        pTTF_CloseFont(mLabelFont);
    if (pTTF_Quit)
        pTTF_Quit();
#endif

    RTCritSectDelete(&mUpdateLock);
}

bool VBoxSDLFB::init(bool fShowSDLConfig)
{
    LogFlow(("VBoxSDLFB::init\n"));

    /* memorize the thread that inited us, that's the SDL thread */
    gSdlNativeThread = RTThreadNativeSelf();

#ifdef RT_OS_WINDOWS
    /* default to DirectX if nothing else set */
    if (!RTEnvExist("SDL_VIDEODRIVER"))
    {
        _putenv("SDL_VIDEODRIVER=directx");
//        _putenv("SDL_VIDEODRIVER=windib");
    }
#endif
#ifdef VBOXSDL_WITH_X11
    /* On some X servers the mouse is stuck inside the bottom right corner.
     * See http://wiki.clug.org.za/wiki/QEMU_mouse_not_working */
    RTEnvSet("SDL_VIDEO_X11_DGAMOUSE", "0");
#endif
    int rc = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE);
    if (rc != 0)
    {
        RTPrintf("SDL Error: '%s'\n", SDL_GetError());
        return false;
    }
    gfSdlInitialized = true;

    const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
    Assert(videoInfo);
    if (videoInfo)
    {
        /* output what SDL is capable of */
        if (fShowSDLConfig)
            RTPrintf("SDL capabilities:\n"
                     "  Hardware surface support:                    %s\n"
                     "  Window manager available:                    %s\n"
                     "  Screen to screen blits accelerated:          %s\n"
                     "  Screen to screen colorkey blits accelerated: %s\n"
                     "  Screen to screen alpha blits accelerated:    %s\n"
                     "  Memory to screen blits accelerated:          %s\n"
                     "  Memory to screen colorkey blits accelerated: %s\n"
                     "  Memory to screen alpha blits accelerated:    %s\n"
                     "  Color fills accelerated:                     %s\n"
                     "  Video memory in kilobytes:                   %d\n"
                     "  Optimal bpp mode:                            %d\n"
                     "SDL video driver:                              %s\n",
                         videoInfo->hw_available ? "yes" : "no",
                         videoInfo->wm_available ? "yes" : "no",
                         videoInfo->blit_hw ? "yes" : "no",
                         videoInfo->blit_hw_CC ? "yes" : "no",
                         videoInfo->blit_hw_A ? "yes" : "no",
                         videoInfo->blit_sw ? "yes" : "no",
                         videoInfo->blit_sw_CC ? "yes" : "no",
                         videoInfo->blit_sw_A ? "yes" : "no",
                         videoInfo->blit_fill ? "yes" : "no",
                         videoInfo->video_mem,
                         videoInfo->vfmt->BitsPerPixel,
                         RTEnvGet("SDL_VIDEODRIVER"));
    }

    if (12320 == g_cbIco64x01)
    {
        gWMIcon = SDL_AllocSurface(SDL_SWSURFACE, 64, 64, 24, 0xff, 0xff00, 0xff0000, 0);
        /** @todo make it as simple as possible. No PNM interpreter here... */
        if (gWMIcon)
        {
            memcpy(gWMIcon->pixels, g_abIco64x01+32, g_cbIco64x01-32);
            SDL_WM_SetIcon(gWMIcon, NULL);
        }
    }

    return true;
}

/**
 * Terminate SDL
 *
 * @remarks must be called from the SDL thread!
 */
void VBoxSDLFB::uninit()
{
    if (gfSdlInitialized)
    {
        AssertMsg(gSdlNativeThread == RTThreadNativeSelf(), ("Wrong thread! SDL is not threadsafe!\n"));
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        if (gWMIcon)
        {
            SDL_FreeSurface(gWMIcon);
            gWMIcon = NULL;
        }
    }
}

/**
 * Returns the current framebuffer width in pixels.
 *
 * @returns COM status code
 * @param   width Address of result buffer.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(Width)(ULONG *width)
{
    LogFlow(("VBoxSDLFB::GetWidth\n"));
    if (!width)
        return E_INVALIDARG;
    *width = mGuestXRes;
    return S_OK;
}

/**
 * Returns the current framebuffer height in pixels.
 *
 * @returns COM status code
 * @param   height Address of result buffer.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(Height)(ULONG *height)
{
    LogFlow(("VBoxSDLFB::GetHeight\n"));
    if (!height)
        return E_INVALIDARG;
    *height = mGuestYRes;
    return S_OK;
}

/**
 * Return the current framebuffer color depth.
 *
 * @returns COM status code
 * @param   bitsPerPixel Address of result variable
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(BitsPerPixel)(ULONG *bitsPerPixel)
{
    LogFlow(("VBoxSDLFB::GetBitsPerPixel\n"));
    if (!bitsPerPixel)
        return E_INVALIDARG;
    /* get the information directly from the surface in use */
    Assert(mSurfVRAM);
    *bitsPerPixel = (ULONG)(mSurfVRAM ? mSurfVRAM->format->BitsPerPixel : 0);
    return S_OK;
}

/**
 * Return the current framebuffer line size in bytes.
 *
 * @returns COM status code.
 * @param   lineSize Address of result variable.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(BytesPerLine)(ULONG *bytesPerLine)
{
    LogFlow(("VBoxSDLFB::GetBytesPerLine\n"));
    if (!bytesPerLine)
        return E_INVALIDARG;
    /* get the information directly from the surface */
    Assert(mSurfVRAM);
    *bytesPerLine = (ULONG)(mSurfVRAM ? mSurfVRAM->pitch : 0);
    return S_OK;
}

STDMETHODIMP VBoxSDLFB::COMGETTER(PixelFormat) (BitmapFormat_T *pixelFormat)
{
    if (!pixelFormat)
        return E_POINTER;
    *pixelFormat = BitmapFormat_BGR;
    return S_OK;
}

/**
 * Returns by how many pixels the guest should shrink its
 * video mode height values.
 *
 * @returns COM status code.
 * @param   heightReduction Address of result variable.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(HeightReduction)(ULONG *heightReduction)
{
    if (!heightReduction)
        return E_POINTER;
#ifdef VBOX_SECURELABEL
    *heightReduction = mLabelHeight;
#else
    *heightReduction = 0;
#endif
    return S_OK;
}

/**
 * Returns a pointer to an alpha-blended overlay used for displaying status
 * icons above the framebuffer.
 *
 * @returns COM status code.
 * @param   aOverlay The overlay framebuffer.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(Overlay)(IFramebufferOverlay **aOverlay)
{
    if (!aOverlay)
        return E_POINTER;
    /* Not yet implemented */
    *aOverlay = 0;
    return S_OK;
}

/**
 * Returns handle of window where framebuffer context is being drawn
 *
 * @returns COM status code.
 * @param   winId Handle of associated window.
 */
STDMETHODIMP VBoxSDLFB::COMGETTER(WinId)(LONG64 *winId)
{
    if (!winId)
        return E_POINTER;
#ifdef RT_OS_DARWIN
    if (mWinId == NULL) /* (In case it failed the first time.) */
        mWinId = (intptr_t)VBoxSDLGetDarwinWindowId();
#endif
    *winId = mWinId;
    return S_OK;
}

STDMETHODIMP VBoxSDLFB::COMGETTER(Capabilities)(ComSafeArrayOut(FramebufferCapabilities_T, aCapabilities))
{
    if (ComSafeArrayOutIsNull(aCapabilities))
        return E_POINTER;

    com::SafeArray<FramebufferCapabilities_T> caps;

    if (mfUpdateImage)
    {
        caps.resize(1);
        caps[0] = FramebufferCapabilities_UpdateImage;
    }
    else
    {
        /* No caps to return. */
    }

    caps.detachTo(ComSafeArrayOutArg(aCapabilities));
    return S_OK;
}

/**
 * Notify framebuffer of an update.
 *
 * @returns COM status code
 * @param   x        Update region upper left corner x value.
 * @param   y        Update region upper left corner y value.
 * @param   w        Update region width in pixels.
 * @param   h        Update region height in pixels.
 * @param   finished Address of output flag whether the update
 *                   could be fully processed in this call (which
 *                   has to return immediately) or VBox should wait
 *                   for a call to the update complete API before
 *                   continuing with display updates.
 */
STDMETHODIMP VBoxSDLFB::NotifyUpdate(ULONG x, ULONG y,
                                     ULONG w, ULONG h)
{
    /*
     * The input values are in guest screen coordinates.
     */
    LogFlow(("VBoxSDLFB::NotifyUpdate: x = %d, y = %d, w = %d, h = %d\n",
             x, y, w, h));

#ifdef VBOXSDL_WITH_X11
    /*
     * SDL does not allow us to make this call from any other thread than
     * the main SDL thread (which initialized the video mode). So we have
     * to send an event to the main SDL thread and process it there. For
     * sake of simplicity, we encode all information in the event parameters.
     */
    SDL_Event event;
    event.type       = SDL_USEREVENT;
    event.user.code  = mScreenId;
    event.user.type  = SDL_USER_EVENT_UPDATERECT;
    // 16 bit is enough for coordinates
    event.user.data1 = (void*)(uintptr_t)(x << 16 | y);
    event.user.data2 = (void*)(uintptr_t)(w << 16 | h);
    PushNotifyUpdateEvent(&event);
#else /* !VBOXSDL_WITH_X11 */
    update(x, y, w, h, true /* fGuestRelative */);
#endif /* !VBOXSDL_WITH_X11 */

    return S_OK;
}

STDMETHODIMP VBoxSDLFB::NotifyUpdateImage(ULONG aX,
                                          ULONG aY,
                                          ULONG aWidth,
                                          ULONG aHeight,
                                          ComSafeArrayIn(BYTE, aImage))
{
    LogFlow(("NotifyUpdateImage: %d,%d %dx%d\n", aX, aY, aWidth, aHeight));

    com::SafeArray<BYTE> image(ComSafeArrayInArg(aImage));

    /* Copy to mSurfVRAM. */
    SDL_Rect srcRect;
    SDL_Rect dstRect;
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = (uint16_t)aWidth;
    srcRect.h = (uint16_t)aHeight;
    dstRect.x = (int16_t)aX;
    dstRect.y = (int16_t)aY;
    dstRect.w = (uint16_t)aWidth;
    dstRect.h = (uint16_t)aHeight;

    const uint32_t Rmask = 0x00FF0000, Gmask = 0x0000FF00, Bmask = 0x000000FF, Amask = 0;
    SDL_Surface *surfSrc = SDL_CreateRGBSurfaceFrom(image.raw(), aWidth, aHeight, 32, aWidth * 4,
                                                    Rmask, Gmask, Bmask, Amask);
    if (surfSrc)
    {
        RTCritSectEnter(&mUpdateLock);
        if (mfUpdates)
            SDL_BlitSurface(surfSrc, &srcRect, mSurfVRAM, &dstRect);
        RTCritSectLeave(&mUpdateLock);

        SDL_FreeSurface(surfSrc);
    }

    return NotifyUpdate(aX, aY, aWidth, aHeight);
}

extern ComPtr<IDisplay> gpDisplay;

STDMETHODIMP VBoxSDLFB::NotifyChange(ULONG aScreenId,
                                     ULONG aXOrigin,
                                     ULONG aYOrigin,
                                     ULONG aWidth,
                                     ULONG aHeight)
{
    LogRel(("NotifyChange: %d %d,%d %dx%d\n",
            aScreenId, aXOrigin, aYOrigin, aWidth, aHeight));

    ComPtr<IDisplaySourceBitmap> pSourceBitmap;
    if (!mfUpdateImage)
        gpDisplay->QuerySourceBitmap(aScreenId, pSourceBitmap.asOutParam());

    RTCritSectEnter(&mUpdateLock);

    /* Disable screen updates. */
    mfUpdates = false;

    if (mfUpdateImage)
    {
        mGuestXRes   = aWidth;
        mGuestYRes   = aHeight;
        mPtrVRAM     = NULL;
        mBitsPerPixel = 0;
        mBytesPerLine = 0;
    }
    else
    {
        /* Save the new bitmap. */
        mpPendingSourceBitmap = pSourceBitmap;
    }

    RTCritSectLeave(&mUpdateLock);

    SDL_Event event;
    event.type       = SDL_USEREVENT;
    event.user.type  = SDL_USER_EVENT_NOTIFYCHANGE;
    event.user.code  = mScreenId;

    PushSDLEventForSure(&event);

    RTThreadYield();

    return S_OK;
}

/**
 * Returns whether we like the given video mode.
 *
 * @returns COM status code
 * @param   width     video mode width in pixels
 * @param   height    video mode height in pixels
 * @param   bpp       video mode bit depth in bits per pixel
 * @param   supported pointer to result variable
 */
STDMETHODIMP VBoxSDLFB::VideoModeSupported(ULONG width, ULONG height, ULONG bpp, BOOL *supported)
{
    RT_NOREF(bpp);

    if (!supported)
        return E_POINTER;

    /* are constraints set? */
    if (   (   (mMaxScreenWidth != ~(uint32_t)0)
            && (width > mMaxScreenWidth))
        || (   (mMaxScreenHeight != ~(uint32_t)0)
            && (height > mMaxScreenHeight)))
    {
        /* nope, we don't want that (but still don't freak out if it is set) */
#ifdef DEBUG
        printf("VBoxSDL::VideoModeSupported: we refused mode %dx%dx%d\n", width, height, bpp);
#endif
        *supported = false;
    }
    else
    {
        /* anything will do */
        *supported = true;
    }
    return S_OK;
}

STDMETHODIMP VBoxSDLFB::GetVisibleRegion(BYTE *aRectangles, ULONG aCount,
                                         ULONG *aCountCopied)
{
    PRTRECT rects = (PRTRECT)aRectangles;

    if (!rects)
        return E_POINTER;

    /// @todo

    NOREF(aCount);
    NOREF(aCountCopied);

    return S_OK;
}

STDMETHODIMP VBoxSDLFB::SetVisibleRegion(BYTE *aRectangles, ULONG aCount)
{
    PRTRECT rects = (PRTRECT)aRectangles;

    if (!rects)
        return E_POINTER;

    /// @todo

    NOREF(aCount);

    return S_OK;
}

STDMETHODIMP VBoxSDLFB::ProcessVHWACommand(BYTE *pCommand)
{
    RT_NOREF(pCommand);
    return E_NOTIMPL;
}

STDMETHODIMP VBoxSDLFB::Notify3DEvent(ULONG uType, ComSafeArrayIn(BYTE, aData))
{
    RT_NOREF(uType); ComSafeArrayNoRef(aData);
    return E_NOTIMPL;
}

//
// Internal public methods
//

/* This method runs on the main SDL thread. */
void VBoxSDLFB::notifyChange(ULONG aScreenId)
{
    /* Disable screen updates. */
    RTCritSectEnter(&mUpdateLock);

    if (!mfUpdateImage && mpPendingSourceBitmap.isNull())
    {
        /* Do nothing. Change event already processed. */
        RTCritSectLeave(&mUpdateLock);
        return;
    }

    /* Release the current bitmap and keep the pending one. */
    mpSourceBitmap = mpPendingSourceBitmap;
    mpPendingSourceBitmap.setNull();

    RTCritSectLeave(&mUpdateLock);

    if (mpSourceBitmap.isNull())
    {
        mPtrVRAM      = NULL;
        mBitsPerPixel = 32;
        mBytesPerLine = mGuestXRes * 4;
    }
    else
    {
        BYTE *pAddress = NULL;
        ULONG ulWidth = 0;
        ULONG ulHeight = 0;
        ULONG ulBitsPerPixel = 0;
        ULONG ulBytesPerLine = 0;
        BitmapFormat_T bitmapFormat = BitmapFormat_Opaque;

        mpSourceBitmap->QueryBitmapInfo(&pAddress,
                                        &ulWidth,
                                        &ulHeight,
                                        &ulBitsPerPixel,
                                        &ulBytesPerLine,
                                        &bitmapFormat);

        if (   mGuestXRes    == ulWidth
            && mGuestYRes    == ulHeight
            && mBitsPerPixel == ulBitsPerPixel
            && mBytesPerLine == ulBytesPerLine
            && mPtrVRAM == pAddress
           )
        {
            mfSameSizeRequested = true;
        }
        else
        {
            mfSameSizeRequested = false;
        }

        mGuestXRes   = ulWidth;
        mGuestYRes   = ulHeight;
        mPtrVRAM     = pAddress;
        mBitsPerPixel = ulBitsPerPixel;
        mBytesPerLine = ulBytesPerLine;
    }

    resizeGuest();

    gpDisplay->InvalidateAndUpdateScreen(aScreenId);
}

/**
 * Method that does the actual resize of the guest framebuffer and
 * then changes the SDL framebuffer setup.
 */
void VBoxSDLFB::resizeGuest()
{
    LogFlowFunc (("mGuestXRes: %d, mGuestYRes: %d\n", mGuestXRes, mGuestYRes));
    AssertMsg(gSdlNativeThread == RTThreadNativeSelf(),
              ("Wrong thread! SDL is not threadsafe!\n"));

    RTCritSectEnter(&mUpdateLock);

    const uint32_t Rmask = 0x00FF0000, Gmask = 0x0000FF00, Bmask = 0x000000FF, Amask = 0;

    /* first free the current surface */
    if (mSurfVRAM)
    {
        SDL_FreeSurface(mSurfVRAM);
        mSurfVRAM = NULL;
    }

    if (mPtrVRAM)
    {
        /* Create a source surface from the source bitmap. */
        mSurfVRAM = SDL_CreateRGBSurfaceFrom(mPtrVRAM, mGuestXRes, mGuestYRes, mBitsPerPixel,
                                             mBytesPerLine, Rmask, Gmask, Bmask, Amask);
        LogFlow(("VBoxSDL:: using the source bitmap\n"));
    }
    else
    {
        mSurfVRAM = SDL_CreateRGBSurface(SDL_SWSURFACE, mGuestXRes, mGuestYRes, mBitsPerPixel,
                                         Rmask, Gmask, Bmask, Amask);
        LogFlow(("VBoxSDL:: using SDL_SWSURFACE\n"));
    }
    LogFlow(("VBoxSDL:: created VRAM surface %p\n", mSurfVRAM));

    if (mfSameSizeRequested)
    {
        mfSameSizeRequested = false;
        LogFlow(("VBoxSDL:: the same resolution requested, skipping the resize.\n"));
    }
    else
    {
        /* now adjust the SDL resolution */
        resizeSDL();
    }

    /* Enable screen updates. */
    mfUpdates = true;

    RTCritSectLeave(&mUpdateLock);

    repaint();
}

/**
 * Sets SDL video mode. This is independent from guest video
 * mode changes.
 *
 * @remarks Must be called from the SDL thread!
 */
void VBoxSDLFB::resizeSDL(void)
{
    LogFlow(("VBoxSDL:resizeSDL\n"));

    /*
     * We request a hardware surface from SDL so that we can perform
     * accelerated system memory to VRAM blits. The way video handling
     * works it that on the one hand we have the screen surface from SDL
     * and on the other hand we have a software surface that we create
     * using guest VRAM memory for linear modes and using SDL allocated
     * system memory for text and non linear graphics modes. We never
     * directly write to the screen surface but always use SDL blitting
     * functions to blit from our system memory surface to the VRAM.
     * Therefore, SDL can take advantage of hardware acceleration.
     */
    int sdlFlags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;
#ifndef RT_OS_OS2 /* doesn't seem to work for some reason... */
    if (mfResizable)
        sdlFlags |= SDL_RESIZABLE;
#endif
    if (mfFullscreen)
        sdlFlags |= SDL_FULLSCREEN;

    /*
     * Now we have to check whether there are video mode restrictions
     */
    SDL_Rect **modes;
    /* Get available fullscreen/hardware modes */
    modes = SDL_ListModes(NULL, sdlFlags);
    Assert(modes != NULL);
    /* -1 means that any mode is possible (usually non fullscreen) */
    if (modes != (SDL_Rect **)-1)
    {
        /*
         * according to the SDL documentation, the API guarantees that
         * the modes are sorted from larger to smaller, so we just
         * take the first entry as the maximum.
         */
        mMaxScreenWidth  = modes[0]->w;
        mMaxScreenHeight = modes[0]->h;
    }
    else
    {
        /* no restriction */
        mMaxScreenWidth  = ~(uint32_t)0;
        mMaxScreenHeight = ~(uint32_t)0;
    }

    uint32_t newWidth;
    uint32_t newHeight;

    /* reset the centering offsets */
    mCenterXOffset = 0;
    mCenterYOffset = 0;

    /* we either have a fixed SDL resolution or we take the guest's */
    if (mFixedSDLWidth != ~(uint32_t)0)
    {
        newWidth  = mFixedSDLWidth;
        newHeight = mFixedSDLHeight;
    }
    else
    {
        newWidth  = RT_MIN(mGuestXRes, mMaxScreenWidth);
#ifdef VBOX_SECURELABEL
        newHeight = RT_MIN(mGuestYRes + mLabelHeight, mMaxScreenHeight);
#else
        newHeight = RT_MIN(mGuestYRes, mMaxScreenHeight);
#endif
    }

    /* we don't have any extra space by default */
    mTopOffset = 0;

#if defined(VBOX_WITH_SDL13)
    int sdlWindowFlags = SDL_WINDOW_SHOWN;
    if (mfResizable)
        sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
    if (!mWindow)
    {
        SDL_DisplayMode desktop_mode;
        int x = 40 + mScreenId * 20;
        int y = 40 + mScreenId * 15;

        SDL_GetDesktopDisplayMode(&desktop_mode);
        /* create new window */

        char szTitle[64];
        RTStrPrintf(szTitle, sizeof(szTitle), "SDL window %d", mScreenId);
        mWindow = SDL_CreateWindow(szTitle, x, y,
                                   newWidth, newHeight, sdlWindowFlags);
        if (SDL_CreateRenderer(mWindow, -1,
                               SDL_RENDERER_SINGLEBUFFER | SDL_RENDERER_PRESENTDISCARD) < 0)
            AssertReleaseFailed();

        SDL_GetRendererInfo(&mRenderInfo);

        mTexture = SDL_CreateTexture(desktop_mode.format,
                                     SDL_TEXTUREACCESS_STREAMING, newWidth, newHeight);
        if (!mTexture)
            AssertReleaseFailed();
    }
    else
    {
        int w, h;
        uint32_t format;
        int access;

        /* resize current window */
        SDL_GetWindowSize(mWindow, &w, &h);

        if (w != (int)newWidth || h != (int)newHeight)
            SDL_SetWindowSize(mWindow, newWidth, newHeight);

        SDL_QueryTexture(mTexture, &format, &access, &w, &h);
        SDL_SelectRenderer(mWindow);
        SDL_DestroyTexture(mTexture);
        mTexture = SDL_CreateTexture(format, access, newWidth, newHeight);
        if (!mTexture)
            AssertReleaseFailed();
    }

    void *pixels;
    int pitch;
    int w, h, bpp;
    uint32_t Rmask, Gmask, Bmask, Amask;
    uint32_t format;

    if (SDL_QueryTexture(mTexture, &format, NULL, &w, &h) < 0)
        AssertReleaseFailed();

    if (!SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask))
        AssertReleaseFailed();

    if (SDL_QueryTexturePixels(mTexture, &pixels, &pitch) == 0)
    {
        mScreen = SDL_CreateRGBSurfaceFrom(pixels, w, h, bpp, pitch,
                                           Rmask, Gmask, Bmask, Amask);
    }
    else
    {
        mScreen = SDL_CreateRGBSurface(0, w, h, bpp, Rmask, Gmask, Bmask, Amask);
        AssertReleaseFailed();
    }

    SDL_SetClipRect(mScreen, NULL);

#else
    /*
     * Now set the screen resolution and get the surface pointer
     * @todo BPP is not supported!
     */
    mScreen = SDL_SetVideoMode(newWidth, newHeight, 0, sdlFlags);

    /*
     * Set the Window ID. Currently used for OpenGL accelerated guests.
     */
# if defined (RT_OS_WINDOWS)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWMInfo(&info))
        mWinId = (intptr_t) info.window;
# elif defined (RT_OS_LINUX)
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWMInfo(&info))
        mWinId = (LONG64) info.info.x11.wmwindow;
# elif defined(RT_OS_DARWIN)
    mWinId = (intptr_t)VBoxSDLGetDarwinWindowId();
# else
    /* XXX ignore this for other architectures */
# endif
#endif
#ifdef VBOX_SECURELABEL
    /*
     * For non fixed SDL resolution, the above call tried to add the label height
     * to the guest height. If it worked, we have an offset. If it didn't the below
     * code will try again with the original guest resolution.
     */
    if (mFixedSDLWidth == ~(uint32_t)0)
    {
        /* if it didn't work, then we have to go for the original resolution and paint over the guest */
        if (!mScreen)
        {
            mScreen = SDL_SetVideoMode(newWidth, newHeight - mLabelHeight, 0, sdlFlags);
        }
        else
        {
            /* we now have some extra space */
            mTopOffset = mLabelHeight;
        }
    }
    else
    {
        /* in case the guest resolution is small enough, we do have a top offset */
        if (mFixedSDLHeight - mGuestYRes >= mLabelHeight)
            mTopOffset = mLabelHeight;

        /* we also might have to center the guest picture */
        if (mFixedSDLWidth > mGuestXRes)
            mCenterXOffset = (mFixedSDLWidth - mGuestXRes) / 2;
        if (mFixedSDLHeight > mGuestYRes + mLabelHeight)
            mCenterYOffset = (mFixedSDLHeight - (mGuestYRes + mLabelHeight)) / 2;
    }
#endif
    AssertMsg(mScreen, ("Error: SDL_SetVideoMode failed!\n"));
    if (mScreen)
    {
#ifdef VBOX_WIN32_UI
        /* inform the UI code */
        resizeUI(mScreen->w, mScreen->h);
#endif
        if (mfShowSDLConfig)
            RTPrintf("Resized to %dx%d, screen surface type: %s\n", mScreen->w, mScreen->h,
                     ((mScreen->flags & SDL_HWSURFACE) == 0) ? "software" : "hardware");
    }
}

/**
 * Update specified framebuffer area. The coordinates can either be
 * relative to the guest framebuffer or relative to the screen.
 *
 * @remarks Must be called from the SDL thread on Linux!
 * @param   x              left column
 * @param   y              top row
 * @param   w              width in pixels
 * @param   h              height in pixels
 * @param   fGuestRelative flag whether the above values are guest relative or screen relative;
 */
void VBoxSDLFB::update(int x, int y, int w, int h, bool fGuestRelative)
{
#ifdef VBOXSDL_WITH_X11
    AssertMsg(gSdlNativeThread == RTThreadNativeSelf(), ("Wrong thread! SDL is not threadsafe!\n"));
#endif
    RTCritSectEnter(&mUpdateLock);
    Log(("Updates %d, %d,%d %dx%d\n", mfUpdates, x, y, w, h));
    if (!mfUpdates)
    {
        RTCritSectLeave(&mUpdateLock);
        return;
    }

    Assert(mScreen);
    Assert(mSurfVRAM);
    if (!mScreen || !mSurfVRAM)
    {
        RTCritSectLeave(&mUpdateLock);
        return;
    }

    /* the source and destination rectangles */
    SDL_Rect srcRect;
    SDL_Rect dstRect;

    /* this is how many pixels we have to cut off from the height for this specific blit */
    int yCutoffGuest = 0;

#ifdef VBOX_SECURELABEL
    bool fPaintLabel = false;
    /* if we have a label and no space for it, we have to cut off a bit */
    if (mLabelHeight && !mTopOffset)
    {
        if (y < (int)mLabelHeight)
            yCutoffGuest = mLabelHeight - y;
    }
#endif

    /**
     * If we get a SDL window relative update, we
     * just perform a full screen update to keep things simple.
     *
     * @todo improve
     */
    if (!fGuestRelative)
    {
#ifdef VBOX_SECURELABEL
        /* repaint the label if necessary */
        if (y < (int)mLabelHeight)
            fPaintLabel = true;
#endif
        x = 0;
        w = mGuestXRes;
        y = 0;
        h = mGuestYRes;
    }

    srcRect.x = x;
    srcRect.y = y + yCutoffGuest;
    srcRect.w = w;
    srcRect.h = RT_MAX(0, h - yCutoffGuest);

    /*
     * Destination rectangle is just offset by the label height.
     * There are two cases though: label height is added to the
     * guest resolution (mTopOffset == mLabelHeight; yCutoffGuest == 0)
     * or the label cuts off a portion of the guest screen (mTopOffset == 0;
     * yCutoffGuest >= 0)
     */
    dstRect.x = x + mCenterXOffset;
#ifdef VBOX_SECURELABEL
    dstRect.y = RT_MAX(mLabelHeight, y + yCutoffGuest + mTopOffset) + mCenterYOffset;
#else
    dstRect.y = y + yCutoffGuest + mTopOffset + mCenterYOffset;
#endif
    dstRect.w = w;
    dstRect.h = RT_MAX(0, h - yCutoffGuest);

    /*
     * Now we just blit
     */
    SDL_BlitSurface(mSurfVRAM, &srcRect, mScreen, &dstRect);
    /* hardware surfaces don't need update notifications */
#if defined(VBOX_WITH_SDL13)
    AssertRelease(mScreen->flags & SDL_PREALLOC);
    SDL_SelectRenderer(mWindow);
    SDL_DirtyTexture(mTexture, 1, &dstRect);
    AssertRelease(mRenderInfo.flags & SDL_RENDERER_PRESENTCOPY);
    SDL_RenderCopy(mTexture, &dstRect, &dstRect);
    SDL_RenderPresent();
#else
    if ((mScreen->flags & SDL_HWSURFACE) == 0)
        SDL_UpdateRect(mScreen, dstRect.x, dstRect.y, dstRect.w, dstRect.h);
#endif

#ifdef VBOX_SECURELABEL
    if (fPaintLabel)
        paintSecureLabel(0, 0, 0, 0, false);
#endif
    RTCritSectLeave(&mUpdateLock);
}

/**
 * Repaint the whole framebuffer
 *
 * @remarks Must be called from the SDL thread!
 */
void VBoxSDLFB::repaint()
{
    AssertMsg(gSdlNativeThread == RTThreadNativeSelf(), ("Wrong thread! SDL is not threadsafe!\n"));
    LogFlow(("VBoxSDLFB::repaint\n"));
    update(0, 0, mScreen->w, mScreen->h, false /* fGuestRelative */);
}

/**
 * Toggle fullscreen mode
 *
 * @remarks Must be called from the SDL thread!
 */
void VBoxSDLFB::setFullscreen(bool fFullscreen)
{
    AssertMsg(gSdlNativeThread == RTThreadNativeSelf(), ("Wrong thread! SDL is not threadsafe!\n"));
    LogFlow(("VBoxSDLFB::SetFullscreen: fullscreen: %d\n", fFullscreen));
    mfFullscreen = fFullscreen;
    /* only change the SDL resolution, do not touch the guest framebuffer */
    resizeSDL();
    repaint();
}

/**
 * Return the geometry of the host. This isn't very well tested but it seems
 * to work at least on Linux hosts.
 */
void VBoxSDLFB::getFullscreenGeometry(uint32_t *width, uint32_t *height)
{
    SDL_Rect **modes;

    /* Get available fullscreen/hardware modes */
    modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
    Assert(modes != NULL);
    /* -1 means that any mode is possible (usually non fullscreen) */
    if (modes != (SDL_Rect **)-1)
    {
        /*
         * According to the SDL documentation, the API guarantees that the modes
         * are sorted from larger to smaller, so we just take the first entry as
         * the maximum.
         *
         * XXX Crude Xinerama hack :-/
         */
        if (   modes[0]->w > (16*modes[0]->h/9)
            && modes[1]
            && modes[1]->h == modes[0]->h)
        {
            *width  = modes[1]->w;
            *height = modes[1]->h;
        }
        else
        {
            *width  = modes[0]->w;
            *height = modes[0]->w;
        }
    }
}

#ifdef VBOX_SECURELABEL

/**
 * Setup the secure labeling parameters
 *
 * @returns         VBox status code
 * @param height    height of the secure label area in pixels
 * @param font      file path fo the TrueType font file
 * @param pointsize font size in points
 */
int VBoxSDLFB::initSecureLabel(uint32_t height, char *font, uint32_t pointsize, uint32_t labeloffs)
{
    LogFlow(("VBoxSDLFB:initSecureLabel: new offset: %d pixels, new font: %s, new pointsize: %d\n",
              height, font, pointsize));
    mLabelHeight = height;
    mLabelOffs = labeloffs;
    Assert(font);
    pTTF_Init();
    mLabelFont = pTTF_OpenFont(font, pointsize);
    if (!mLabelFont)
    {
        AssertMsgFailed(("Failed to open TTF font file %s\n", font));
        return VERR_OPEN_FAILED;
    }
    mSecureLabelColorFG = 0x0000FF00;
    mSecureLabelColorBG = 0x00FFFF00;
    repaint();
    return VINF_SUCCESS;
}

/**
 * Set the secure label text and repaint the label
 *
 * @param   text UTF-8 string of new label
 * @remarks must be called from the SDL thread!
 */
void VBoxSDLFB::setSecureLabelText(const char *text)
{
    mSecureLabelText = text;
    paintSecureLabel(0, 0, 0, 0, true);
}

/**
 * Sets the secure label background color.
 *
 * @param   colorFG encoded RGB value for text
 * @param   colorBG encored RGB value for background
 * @remarks must be called from the SDL thread!
 */
void VBoxSDLFB::setSecureLabelColor(uint32_t colorFG, uint32_t colorBG)
{
    mSecureLabelColorFG = colorFG;
    mSecureLabelColorBG = colorBG;
    paintSecureLabel(0, 0, 0, 0, true);
}

/**
 * Paint the secure label if required
 *
 * @param   fForce Force the repaint
 * @remarks must be called from the SDL thread!
 */
void VBoxSDLFB::paintSecureLabel(int x, int y, int w, int h, bool fForce)
{
    RT_NOREF(x, w, h);
# ifdef VBOXSDL_WITH_X11
    AssertMsg(gSdlNativeThread == RTThreadNativeSelf(), ("Wrong thread! SDL is not threadsafe!\n"));
# endif
    /* only when the function is present */
    if (!pTTF_RenderUTF8_Solid)
        return;
    /* check if we can skip the paint */
    if (!fForce && ((uint32_t)y > mLabelHeight))
    {
        return;
    }
    /* first fill the background */
    SDL_Rect rect = {0, 0, (Uint16)mScreen->w, (Uint16)mLabelHeight};
    SDL_FillRect(mScreen, &rect, SDL_MapRGB(mScreen->format,
                                            (mSecureLabelColorBG & 0x00FF0000) >> 16,   /* red   */
                                            (mSecureLabelColorBG & 0x0000FF00) >> 8,   /* green */
                                            mSecureLabelColorBG & 0x000000FF)); /* blue  */

    /* now the text */
    if (    mLabelFont != NULL
         && !mSecureLabelText.isEmpty()
       )
    {
        SDL_Color clrFg = {(uint8_t)((mSecureLabelColorFG & 0x00FF0000) >> 16),
                           (uint8_t)((mSecureLabelColorFG & 0x0000FF00) >> 8),
                           (uint8_t)( mSecureLabelColorFG & 0x000000FF      ), 0};
        SDL_Surface *sText = (pTTF_RenderUTF8_Blended != NULL)
                                 ? pTTF_RenderUTF8_Blended(mLabelFont, mSecureLabelText.c_str(), clrFg)
                                 : pTTF_RenderUTF8_Solid(mLabelFont, mSecureLabelText.c_str(), clrFg);
        rect.x = 10;
        rect.y = mLabelOffs;
        SDL_BlitSurface(sText, NULL, mScreen, &rect);
        SDL_FreeSurface(sText);
    }
    /* make sure to update the screen */
    SDL_UpdateRect(mScreen, 0, 0, mScreen->w, mLabelHeight);
}

#endif /* VBOX_SECURELABEL */

// IFramebufferOverlay
///////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor for the VBoxSDLFBOverlay class (IFramebufferOverlay implementation)
 *
 * @param x       Initial X offset for the overlay
 * @param y       Initial Y offset for the overlay
 * @param width   Initial width for the overlay
 * @param height  Initial height for the overlay
 * @param visible Whether the overlay is initially visible
 * @param alpha   Initial alpha channel value for the overlay
 */
VBoxSDLFBOverlay::VBoxSDLFBOverlay(ULONG x, ULONG y, ULONG width, ULONG height,
                                   BOOL visible, VBoxSDLFB *aParent) :
                                   mOverlayX(x), mOverlayY(y), mOverlayWidth(width),
                                   mOverlayHeight(height), mOverlayVisible(visible),
                                   mParent(aParent)
{}

/**
 * Destructor for the VBoxSDLFBOverlay class.
 */
VBoxSDLFBOverlay::~VBoxSDLFBOverlay()
{
    SDL_FreeSurface(mBlendedBits);
    SDL_FreeSurface(mOverlayBits);
}

/**
 * Perform any initialisation of the overlay that can potentially fail
 *
 * @returns S_OK on success or the reason for the failure
 */
HRESULT VBoxSDLFBOverlay::init()
{
    mBlendedBits = SDL_CreateRGBSurface(SDL_ANYFORMAT, mOverlayWidth, mOverlayHeight, 32,
                                        0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    AssertMsgReturn(mBlendedBits != NULL, ("Failed to create an SDL surface\n"),
                    E_OUTOFMEMORY);
    mOverlayBits = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, mOverlayWidth,
                                        mOverlayHeight, 32, 0x00ff0000, 0x0000ff00,
                                        0x000000ff, 0xff000000);
    AssertMsgReturn(mOverlayBits != NULL, ("Failed to create an SDL surface\n"),
                    E_OUTOFMEMORY);
    return S_OK;
}

/**
 * Returns the current overlay X offset in pixels.
 *
 * @returns COM status code
 * @param   x Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(X)(ULONG *x)
{
    LogFlow(("VBoxSDLFBOverlay::GetX\n"));
    if (!x)
        return E_INVALIDARG;
    *x = mOverlayX;
    return S_OK;
}

/**
 * Returns the current overlay height in pixels.
 *
 * @returns COM status code
 * @param   height Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Y)(ULONG *y)
{
    LogFlow(("VBoxSDLFBOverlay::GetY\n"));
    if (!y)
        return E_INVALIDARG;
    *y = mOverlayY;
    return S_OK;
}

/**
 * Returns the current overlay width in pixels.  In fact, this returns the line size.
 *
 * @returns COM status code
 * @param   width Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Width)(ULONG *width)
{
    LogFlow(("VBoxSDLFBOverlay::GetWidth\n"));
    if (!width)
        return E_INVALIDARG;
    *width = mOverlayBits->pitch;
    return S_OK;
}

/**
 * Returns the current overlay line size in pixels.
 *
 * @returns COM status code
 * @param   lineSize Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(BytesPerLine)(ULONG *bytesPerLine)
{
    LogFlow(("VBoxSDLFBOverlay::GetBytesPerLine\n"));
    if (!bytesPerLine)
        return E_INVALIDARG;
    *bytesPerLine = mOverlayBits->pitch;
    return S_OK;
}

/**
 * Returns the current overlay height in pixels.
 *
 * @returns COM status code
 * @param   height Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Height)(ULONG *height)
{
    LogFlow(("VBoxSDLFBOverlay::GetHeight\n"));
    if (!height)
        return E_INVALIDARG;
    *height = mOverlayHeight;
    return S_OK;
}

/**
 * Returns whether the overlay is currently visible.
 *
 * @returns COM status code
 * @param   visible Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Visible)(BOOL *visible)
{
    LogFlow(("VBoxSDLFBOverlay::GetVisible\n"));
    if (!visible)
        return E_INVALIDARG;
    *visible = mOverlayVisible;
    return S_OK;
}

/**
 * Sets whether the overlay is currently visible.
 *
 * @returns COM status code
 * @param   visible New value.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMSETTER(Visible)(BOOL visible)
{
    LogFlow(("VBoxSDLFBOverlay::SetVisible\n"));
    mOverlayVisible = visible;
    return S_OK;
}

/**
 * Returns the value of the global alpha channel.
 *
 * @returns COM status code
 * @param   alpha Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Alpha)(ULONG *alpha)
{
    RT_NOREF(alpha);
    LogFlow(("VBoxSDLFBOverlay::GetAlpha\n"));
    return E_NOTIMPL;
}

/**
 * Sets whether the overlay is currently visible.
 *
 * @returns COM status code
 * @param   alpha new value.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMSETTER(Alpha)(ULONG alpha)
{
    RT_NOREF(alpha);
    LogFlow(("VBoxSDLFBOverlay::SetAlpha\n"));
    return E_NOTIMPL;
}

/**
 * Returns the current colour depth.  In fact, this is always 32bpp.
 *
 * @returns COM status code
 * @param   bitsPerPixel Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(BitsPerPixel)(ULONG *bitsPerPixel)
{
    LogFlow(("VBoxSDLFBOverlay::GetBitsPerPixel\n"));
    if (!bitsPerPixel)
        return E_INVALIDARG;
    *bitsPerPixel = 32;
    return S_OK;
}

/**
 * Returns the current pixel format.  In fact, this is always RGB.
 *
 * @returns COM status code
 * @param   pixelFormat Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(PixelFormat)(ULONG *pixelFormat)
{
    LogFlow(("VBoxSDLFBOverlay::GetPixelFormat\n"));
    if (!pixelFormat)
        return E_INVALIDARG;
    *pixelFormat = BitmapFormat_BGR;
    return S_OK;
}

/**
 * Returns whether the guest VRAM is used directly.  In fact, this is always FALSE.
 *
 * @returns COM status code
 * @param   usesGuestVRAM Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(UsesGuestVRAM)(BOOL *usesGuestVRAM)
{
    LogFlow(("VBoxSDLFBOverlay::GetUsesGuestVRAM\n"));
    if (!usesGuestVRAM)
        return E_INVALIDARG;
    *usesGuestVRAM = FALSE;
    return S_OK;
}

/**
 * Returns the height reduction.  In fact, this is always 0.
 *
 * @returns COM status code
 * @param   heightReduction Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(HeightReduction)(ULONG *heightReduction)
{
    LogFlow(("VBoxSDLFBOverlay::GetHeightReduction\n"));
    if (!heightReduction)
        return E_INVALIDARG;
    *heightReduction = 0;
    return S_OK;
}

/**
 * Returns the overlay for this framebuffer.  Obviously, we return NULL here.
 *
 * @returns COM status code
 * @param   overlay Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(Overlay)(IFramebufferOverlay **aOverlay)
{
    LogFlow(("VBoxSDLFBOverlay::GetOverlay\n"));
    if (!aOverlay)
        return E_INVALIDARG;
    *aOverlay = 0;
    return S_OK;
}

/**
 * Returns associated window handle. We return NULL here.
 *
 * @returns COM status code
 * @param   winId Address of result buffer.
 */
STDMETHODIMP VBoxSDLFBOverlay::COMGETTER(WinId)(LONG64 *winId)
{
    LogFlow(("VBoxSDLFBOverlay::GetWinId\n"));
    if (!winId)
        return E_INVALIDARG;
    *winId = 0;
    return S_OK;
}


/**
 * Lock the overlay.  This should not be used - lock the parent IFramebuffer instead.
 *
 * @returns COM status code
 */
STDMETHODIMP VBoxSDLFBOverlay::Lock()
{
    LogFlow(("VBoxSDLFBOverlay::Lock\n"));
    AssertMsgFailed(("You should not attempt to lock an IFramebufferOverlay object -\n"
                     "lock the parent IFramebuffer object instead.\n"));
    return E_NOTIMPL;
}

/**
 * Unlock the overlay.
 *
 * @returns COM status code
 */
STDMETHODIMP VBoxSDLFBOverlay::Unlock()
{
    LogFlow(("VBoxSDLFBOverlay::Unlock\n"));
    AssertMsgFailed(("You should not attempt to lock an IFramebufferOverlay object -\n"
                     "lock the parent IFramebuffer object instead.\n"));
    return E_NOTIMPL;
}

/**
 * Change the X and Y co-ordinates of the overlay area.
 *
 * @returns COM status code
 * @param   x New X co-ordinate.
 * @param   y New Y co-ordinate.
 */
STDMETHODIMP VBoxSDLFBOverlay::Move(ULONG x, ULONG y)
{
    mOverlayX = x;
    mOverlayY = y;
    return S_OK;
}

/**
 * Notify the overlay that a section of the framebuffer has been redrawn.
 *
 * @returns COM status code
 * @param   x        X co-ordinate of upper left corner of modified area.
 * @param   y        Y co-ordinate of upper left corner of modified area.
 * @param   w        Width of modified area.
 * @param   h        Height of modified area.
 * @retval  finished Set if the operation has completed.
 *
 * All we do here is to send a request to the parent to update the affected area,
 * translating between our co-ordinate system and the parent's.  It would be have
 * been better to call the parent directly, but such is life.  We leave bounds
 * checking to the parent.
 */
STDMETHODIMP VBoxSDLFBOverlay::NotifyUpdate(ULONG x, ULONG y,
                            ULONG w, ULONG h)
{
    return mParent->NotifyUpdate(x + mOverlayX, y + mOverlayY, w, h);
}

/**
 * Change the dimensions of the overlay.
 *
 * @returns COM status code
 * @param   pixelFormat Must be BitmapFormat_BGR.
 * @param   vram        Must be NULL.
 * @param   lineSize    Ignored.
 * @param   w           New overlay width.
 * @param   h           New overlay height.
 * @retval  finished    Set if the operation has completed.
 */
STDMETHODIMP VBoxSDLFBOverlay::RequestResize(ULONG aScreenId, ULONG pixelFormat, ULONG vram,
                                             ULONG bitsPerPixel, ULONG bytesPerLine,
                                             ULONG w, ULONG h, BOOL *finished)
{
    RT_NOREF(aScreenId, bytesPerLine, finished);
    AssertReturn(pixelFormat == BitmapFormat_BGR, E_INVALIDARG);
    AssertReturn(vram == 0, E_INVALIDARG);
    AssertReturn(bitsPerPixel == 32, E_INVALIDARG);
    mOverlayWidth = w;
    mOverlayHeight = h;
    SDL_FreeSurface(mOverlayBits);
    mBlendedBits = SDL_CreateRGBSurface(SDL_ANYFORMAT, mOverlayWidth, mOverlayHeight, 32,
                                        0x00ff0000, 0x0000ff00, 0x000000ff, 0);
    AssertMsgReturn(mBlendedBits != NULL, ("Failed to create an SDL surface\n"),
                    E_OUTOFMEMORY);
    mOverlayBits = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, mOverlayWidth,
                                        mOverlayHeight, 32, 0x00ff0000, 0x0000ff00,
                                        0x000000ff, 0xff000000);
    AssertMsgReturn(mOverlayBits != NULL, ("Failed to create an SDL surface\n"),
                    E_OUTOFMEMORY);
    return S_OK;
}

/**
 * Returns whether we like the given video mode.
 *
 * @returns COM status code
 * @param   width     video mode width in pixels
 * @param   height    video mode height in pixels
 * @param   bpp       video mode bit depth in bits per pixel
 * @retval  supported pointer to result variable
 *
 * Basically, we support anything with 32bpp.
 */
STDMETHODIMP VBoxSDLFBOverlay::VideoModeSupported(ULONG width, ULONG height, ULONG bpp, BOOL *supported)
{
    RT_NOREF(width, height);
    if (!supported)
        return E_POINTER;
    if (bpp == 32)
        *supported = true;
    else
        *supported = false;
    return S_OK;
}
