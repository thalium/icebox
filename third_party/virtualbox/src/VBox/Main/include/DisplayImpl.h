/* $Id: DisplayImpl.h $ */
/** @file
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_DISPLAYIMPL
#define ____H_DISPLAYIMPL

#include "SchemaDefs.h"

#include <iprt/semaphore.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/VMMDev.h>
#include <VBoxVideo.h>
#include <VBox/vmm/pdmifs.h>
#include "DisplayWrap.h"

#ifdef VBOX_WITH_CROGL
# include <VBox/HostServices/VBoxCrOpenGLSvc.h>
#endif

#ifdef VBOX_WITH_VIDEOREC
# include "../src-client/VideoRec.h"
struct VIDEORECCONTEXT;
#endif

#include "DisplaySourceBitmapWrap.h"


class Console;

typedef struct _DISPLAYFBINFO
{
    /* The following 3 fields (u32Offset, u32MaxFramebufferSize and u32InformationSize)
     * are not used by the current HGSMI. They are needed for backward compatibility with
     * pre-HGSMI additions.
     */
    uint32_t u32Offset;
    uint32_t u32MaxFramebufferSize;
    uint32_t u32InformationSize;

    ComPtr<IFramebuffer> pFramebuffer;
    com::Guid framebufferId;
    ComPtr<IDisplaySourceBitmap> pSourceBitmap;
    bool fDisabled;

    uint32_t u32Caps;

    struct
    {
        ComPtr<IDisplaySourceBitmap> pSourceBitmap;
        uint8_t *pu8Address;
        uint32_t cbLine;
    } updateImage;

    LONG xOrigin;
    LONG yOrigin;

    ULONG w;
    ULONG h;

    uint16_t u16BitsPerPixel;
    uint8_t *pu8FramebufferVRAM;
    uint32_t u32LineSize;

    uint16_t flags;

    VBOXVIDEOINFOHOSTEVENTS *pHostEvents;

    /** The framebuffer has default format and must be updates immediately. */
    bool fDefaultFormat;

#ifdef VBOX_WITH_HGSMI
    bool fVBVAEnabled;
    bool fVBVAForceResize;
    bool fRenderThreadMode;
    VBVAHOSTFLAGS RT_UNTRUSTED_VOLATILE_GUEST *pVBVAHostFlags;
#endif /* VBOX_WITH_HGSMI */

#ifdef VBOX_WITH_CROGL
    struct
    {
        bool fPending;
        ULONG x;
        ULONG y;
        ULONG width;
        ULONG height;
    } pendingViewportInfo;
#endif /* VBOX_WITH_CROGL */

#ifdef VBOX_WITH_VIDEOREC
    struct
    {
        ComPtr<IDisplaySourceBitmap> pSourceBitmap;
    } videoRec;
#endif /* VBOX_WITH_VIDEOREC */
} DISPLAYFBINFO;

/* The legacy VBVA (VideoAccel) data.
 *
 * Backward compatibility with the guest additions 3.x or older.
 */
typedef struct VIDEOACCEL
{
    VBVAMEMORY *pVbvaMemory;
    bool        fVideoAccelEnabled;

    uint8_t    *pu8VbvaPartial;
    uint32_t    cbVbvaPartial;

    /* Old guest additions (3.x and older) use both VMMDev and DevVGA refresh timer
     * to process the VBVABUFFER memory. Therefore the legacy VBVA (VideoAccel) host
     * code can be executed concurrently by VGA refresh timer and the guest VMMDev
     * request in SMP VMs. The semaphore serialized this.
     */
    RTSEMXROADS hXRoadsVideoAccel;

} VIDEOACCEL;

class DisplayMouseInterface
{
public:
    virtual HRESULT i_getScreenResolution(ULONG cScreen, ULONG *pcx,
                                          ULONG *pcy, ULONG *pcBPP, LONG *pXOrigin, LONG *pYOrigin) = 0;
    virtual void i_getFramebufferDimensions(int32_t *px1, int32_t *py1,
                                            int32_t *px2, int32_t *py2) = 0;
    virtual HRESULT i_reportHostCursorCapabilities(uint32_t fCapabilitiesAdded, uint32_t fCapabilitiesRemoved) = 0;
    virtual HRESULT i_reportHostCursorPosition(int32_t x, int32_t y) = 0;
    virtual bool i_isInputMappingSet(void) = 0;
};

class VMMDev;

class ATL_NO_VTABLE Display :
    public DisplayWrap,
    public DisplayMouseInterface
{
public:

    DECLARE_EMPTY_CTOR_DTOR(Display)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Console *aParent);
    void uninit();
    int  i_registerSSM(PUVM pUVM);

    // public methods only for internal purposes
    int i_handleDisplayResize(unsigned uScreenId, uint32_t bpp, void *pvVRAM,
                              uint32_t cbLine, uint32_t w, uint32_t h, uint16_t flags,
                              int32_t xOrigin, int32_t yOrigin, bool fVGAResize);
    void i_handleDisplayUpdate(unsigned uScreenId, int x, int y, int w, int h);
    void i_handleUpdateVMMDevSupportsGraphics(bool fSupportsGraphics);
    void i_handleUpdateGuestVBVACapabilities(uint32_t fNewCapabilities);
    void i_handleUpdateVBVAInputMapping(int32_t xOrigin, int32_t yOrigin, uint32_t cx, uint32_t cy);
#ifdef VBOX_WITH_VIDEOHWACCEL
    int  i_handleVHWACommandProcess(int enmCmd, bool fGuestCmd, VBOXVHWACMD RT_UNTRUSTED_VOLATILE_GUEST *pCommand);
#endif
#ifdef VBOX_WITH_CRHGSMI
    void i_handleCrHgsmiCommandCompletion(int32_t result, uint32_t u32Function, PVBOXHGCMSVCPARM pParam);
    void i_handleCrHgsmiControlCompletion(int32_t result, uint32_t u32Function, PVBOXHGCMSVCPARM pParam);
    void i_handleCrHgsmiCommandProcess(VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd);
    void i_handleCrHgsmiControlProcess(VBOXVDMACMD_CHROMIUM_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl, uint32_t cbCtl);
#endif
#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    int  i_handleCrHgcmCtlSubmit(struct VBOXCRCMDCTL RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd,
                                 PFNCRCTLCOMPLETION pfnCompletion, void *pvCompletion);
    void  i_handleCrVRecScreenshotPerform(uint32_t uScreen,
                                          uint32_t x, uint32_t y, uint32_t uPixelFormat, uint32_t uBitsPerPixel,
                                          uint32_t uBytesPerLine, uint32_t uGuestWidth, uint32_t uGuestHeight,
                                          uint8_t *pu8BufferAddress, uint64_t u64TimeStamp);
    /** @todo r=bird: u64TimeStamp - using the 'u64' prefix add nothing.
     *        However, using one of the prefixes indicating the timestamp unit
     *        would be very valuable!  */
    bool i_handleCrVRecScreenshotBegin(uint32_t uScreen, uint64_t u64TimeStamp);
    void i_handleCrVRecScreenshotEnd(uint32_t uScreen, uint64_t u64TimeStamp);
    void i_handleVRecCompletion();
#endif

    int i_notifyCroglResize(PCVBVAINFOVIEW pView, PCVBVAINFOSCREEN pScreen, void *pvVRAM);

    int  i_saveVisibleRegion(uint32_t cRect, PRTRECT pRect);
    int  i_handleSetVisibleRegion(uint32_t cRect, PRTRECT pRect);
    int  i_handleQueryVisibleRegion(uint32_t *pcRects, PRTRECT paRects);

    void i_VideoAccelVRDP(bool fEnable);

    /* Legacy video acceleration requests coming from the VGA refresh timer. */
    int  VideoAccelEnableVGA(bool fEnable, VBVAMEMORY *pVbvaMemory);

    /* Legacy video acceleration requests coming from VMMDev. */
    int  VideoAccelEnableVMMDev(bool fEnable, VBVAMEMORY *pVbvaMemory);
    void VideoAccelFlushVMMDev(void);

#ifdef VBOX_WITH_VIDEOREC
    PVIDEORECCFG             i_videoRecGetConfig(void) { return &mVideoRecCfg; }
    VIDEORECFEATURES         i_videoRecGetFeatures(void);
    bool                     i_videoRecStarted(void);
# ifdef VBOX_WITH_AUDIO_VIDEOREC
    int                      i_videoRecConfigureAudioDriver(const Utf8Str& strAdapter, unsigned uInstance, unsigned uLUN, bool fAttach);
# endif
    static DECLCALLBACK(int) i_videoRecConfigure(Display *pThis, PVIDEORECCFG pCfg, bool fAttachDetach, unsigned *puLUN);
    int                      i_videoRecSendAudio(const void *pvData, size_t cbData, uint64_t uDurationMs);
    int                      i_videoRecStart(void);
    void                     i_videoRecStop(void);
    void                     i_videoRecScreenChanged(unsigned uScreenId);
#endif

    void i_notifyPowerDown(void);

    // DisplayMouseInterface methods
    virtual HRESULT i_getScreenResolution(ULONG cScreen, ULONG *pcx,
                                          ULONG *pcy, ULONG *pcBPP, LONG *pXOrigin, LONG *pYOrigin)
    {
        return getScreenResolution(cScreen, pcx, pcy, pcBPP, pXOrigin, pYOrigin, NULL);
    }
    virtual void i_getFramebufferDimensions(int32_t *px1, int32_t *py1,
                                            int32_t *px2, int32_t *py2);
    virtual HRESULT i_reportHostCursorCapabilities(uint32_t fCapabilitiesAdded, uint32_t fCapabilitiesRemoved);
    virtual HRESULT i_reportHostCursorPosition(int32_t x, int32_t y);
    virtual bool i_isInputMappingSet(void)
    {
        return cxInputMapping != 0 && cyInputMapping != 0;
    }

    static const PDMDRVREG  DrvReg;

private:
    // Wrapped IDisplay properties
    virtual HRESULT getGuestScreenLayout(std::vector<ComPtr<IGuestScreenInfo> > &aGuestScreenLayout);

    // Wrapped IDisplay methods
    virtual HRESULT getScreenResolution(ULONG aScreenId,
                                        ULONG *aWidth,
                                        ULONG *aHeight,
                                        ULONG *aBitsPerPixel,
                                        LONG *aXOrigin,
                                        LONG *aYOrigin,
                                        GuestMonitorStatus_T *aGuestMonitorStatus);
    virtual HRESULT attachFramebuffer(ULONG aScreenId,
                                      const ComPtr<IFramebuffer> &aFramebuffer,
                                      com::Guid &aId);
    virtual HRESULT detachFramebuffer(ULONG aScreenId,
                                      const com::Guid &aId);
    virtual HRESULT queryFramebuffer(ULONG aScreenId,
                                     ComPtr<IFramebuffer> &aFramebuffer);
    virtual HRESULT setVideoModeHint(ULONG aDisplay,
                                     BOOL aEnabled,
                                     BOOL aChangeOrigin,
                                     LONG aOriginX,
                                     LONG aOriginY,
                                     ULONG aWidth,
                                     ULONG aHeight,
                                     ULONG aBitsPerPixel);
    virtual HRESULT setSeamlessMode(BOOL aEnabled);
    virtual HRESULT takeScreenShot(ULONG aScreenId,
                                   BYTE *aAddress,
                                   ULONG aWidth,
                                   ULONG aHeight,
                                   BitmapFormat_T aBitmapFormat);
    virtual HRESULT takeScreenShotToArray(ULONG aScreenId,
                                          ULONG aWidth,
                                          ULONG aHeight,
                                          BitmapFormat_T aBitmapFormat,
                                          std::vector<BYTE> &aScreenData);
    virtual HRESULT drawToScreen(ULONG aScreenId,
                                 BYTE *aAddress,
                                 ULONG aX,
                                 ULONG aY,
                                 ULONG aWidth,
                                 ULONG aHeight);
    virtual HRESULT invalidateAndUpdate();
    virtual HRESULT invalidateAndUpdateScreen(ULONG aScreenId);
    virtual HRESULT completeVHWACommand(BYTE *aCommand);
    virtual HRESULT viewportChanged(ULONG aScreenId,
                                    ULONG aX,
                                    ULONG aY,
                                    ULONG aWidth,
                                    ULONG aHeight);
    virtual HRESULT querySourceBitmap(ULONG aScreenId,
                                      ComPtr<IDisplaySourceBitmap> &aDisplaySourceBitmap);
    virtual HRESULT notifyScaleFactorChange(ULONG aScreenId,
                                            ULONG aScaleFactorWMultiplied,
                                            ULONG aScaleFactorHMultiplied);
    virtual HRESULT notifyHiDPIOutputPolicyChange(BOOL fUnscaledHiDPI);
    virtual HRESULT setScreenLayout(ScreenLayoutMode_T aScreenLayoutMode,
                                    const std::vector<ComPtr<IGuestScreenInfo> > &aGuestScreenInfo);
    virtual HRESULT detachScreens(const std::vector<LONG> &aScreenIds);

    // Wrapped IEventListener properties

    // Wrapped IEventListener methods
    virtual HRESULT handleEvent(const ComPtr<IEvent> &aEvent);

    // other internal methods
    HRESULT takeScreenShotWorker(ULONG aScreenId,
                                 BYTE *aAddress,
                                 ULONG aWidth,
                                 ULONG aHeight,
                                 BitmapFormat_T aBitmapFormat,
                                 ULONG *pcbOut);
    int processVBVAResize(PCVBVAINFOVIEW pView, PCVBVAINFOSCREEN pScreen, void *pvVRAM, bool fResetInputMapping);

#ifdef VBOX_WITH_CRHGSMI
    void i_setupCrHgsmiData(void);
    void i_destructCrHgsmiData(void);
#endif

#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    int i_crViewportNotify(ULONG aScreenId, ULONG x, ULONG y, ULONG width, ULONG height);
#endif

    static DECLCALLBACK(void*) i_drvQueryInterface(PPDMIBASE pInterface, const char *pszIID);
    static DECLCALLBACK(int)   i_drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void)  i_drvDestruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(int)   i_displayResizeCallback(PPDMIDISPLAYCONNECTOR pInterface, uint32_t bpp, void *pvVRAM,
                                                       uint32_t cbLine, uint32_t cx, uint32_t cy);
    static DECLCALLBACK(void)  i_displayUpdateCallback(PPDMIDISPLAYCONNECTOR pInterface,
                                                       uint32_t x, uint32_t y, uint32_t cx, uint32_t cy);
    static DECLCALLBACK(void)  i_displayRefreshCallback(PPDMIDISPLAYCONNECTOR pInterface);
    static DECLCALLBACK(void)  i_displayResetCallback(PPDMIDISPLAYCONNECTOR pInterface);
    static DECLCALLBACK(void)  i_displayLFBModeChangeCallback(PPDMIDISPLAYCONNECTOR pInterface, bool fEnabled);
    static DECLCALLBACK(void)  i_displayProcessAdapterDataCallback(PPDMIDISPLAYCONNECTOR pInterface,
                                                                   void *pvVRAM, uint32_t u32VRAMSize);
    static DECLCALLBACK(void)  i_displayProcessDisplayDataCallback(PPDMIDISPLAYCONNECTOR pInterface,
                                                                   void *pvVRAM, unsigned uScreenId);

#ifdef VBOX_WITH_VIDEOHWACCEL
    static DECLCALLBACK(int)  i_displayVHWACommandProcess(PPDMIDISPLAYCONNECTOR pInterface, int enmCmd, bool fGuestCmd,
                                                          VBOXVHWACMD RT_UNTRUSTED_VOLATILE_GUEST *pCommand);
#endif

#ifdef VBOX_WITH_CRHGSMI
    static DECLCALLBACK(void)  i_displayCrHgsmiCommandProcess(PPDMIDISPLAYCONNECTOR pInterface,
                                                              VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_GUEST *pCmd,
                                                              uint32_t cbCmd);
    static DECLCALLBACK(void)  i_displayCrHgsmiControlProcess(PPDMIDISPLAYCONNECTOR pInterface,
                                                              VBOXVDMACMD_CHROMIUM_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl,
                                                              uint32_t cbCtl);

    static DECLCALLBACK(void)  i_displayCrHgsmiCommandCompletion(int32_t result, uint32_t u32Function, PVBOXHGCMSVCPARM pParam,
                                                                 void *pvContext);
    static DECLCALLBACK(void)  i_displayCrHgsmiControlCompletion(int32_t result, uint32_t u32Function, PVBOXHGCMSVCPARM pParam,
                                                                 void *pvContext);
#endif
#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    static DECLCALLBACK(int)  i_displayCrHgcmCtlSubmit(PPDMIDISPLAYCONNECTOR pInterface, struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd,
                                                       PFNCRCTLCOMPLETION pfnCompletion, void *pvCompletion);
    static DECLCALLBACK(void) i_displayCrHgcmCtlSubmitCompletion(int32_t result, uint32_t u32Function, PVBOXHGCMSVCPARM pParam,
                                                                 void *pvContext);
#endif
#ifdef VBOX_WITH_HGSMI
    static DECLCALLBACK(int)   i_displayVBVAEnable(PPDMIDISPLAYCONNECTOR pInterface, unsigned uScreenId,
                                                   VBVAHOSTFLAGS RT_UNTRUSTED_VOLATILE_GUEST *pHostFlags, bool fRenderThreadMode);
    static DECLCALLBACK(void)  i_displayVBVADisable(PPDMIDISPLAYCONNECTOR pInterface, unsigned uScreenId);
    static DECLCALLBACK(void)  i_displayVBVAUpdateBegin(PPDMIDISPLAYCONNECTOR pInterface, unsigned uScreenId);
    static DECLCALLBACK(void)  i_displayVBVAUpdateProcess(PPDMIDISPLAYCONNECTOR pInterface, unsigned uScreenId,
                                                          struct VBVACMDHDR const RT_UNTRUSTED_VOLATILE_GUEST *pCmd, size_t cbCmd);
    static DECLCALLBACK(void)  i_displayVBVAUpdateEnd(PPDMIDISPLAYCONNECTOR pInterface, unsigned uScreenId, int32_t x, int32_t y,
                                                      uint32_t cx, uint32_t cy);
    static DECLCALLBACK(int)   i_displayVBVAResize(PPDMIDISPLAYCONNECTOR pInterface, PCVBVAINFOVIEW pView,
                                                   PCVBVAINFOSCREEN pScreen, void *pvVRAM,
                                                   bool fResetInputMapping);
    static DECLCALLBACK(int)   i_displayVBVAMousePointerShape(PPDMIDISPLAYCONNECTOR pInterface, bool fVisible, bool fAlpha,
                                                              uint32_t xHot, uint32_t yHot, uint32_t cx, uint32_t cy,
                                                              const void *pvShape);
    static DECLCALLBACK(void)  i_displayVBVAGuestCapabilityUpdate(PPDMIDISPLAYCONNECTOR pInterface, uint32_t fCapabilities);

    static DECLCALLBACK(void)  i_displayVBVAInputMappingUpdate(PPDMIDISPLAYCONNECTOR pInterface, int32_t xOrigin, int32_t yOrigin,
                                                               uint32_t cx, uint32_t cy);
#endif

#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    static DECLCALLBACK(void) i_displayCrVRecScreenshotPerform(void *pvCtx, uint32_t uScreen,
                                                               uint32_t x, uint32_t y,
                                                               uint32_t uBitsPerPixel, uint32_t uBytesPerLine,
                                                               uint32_t uGuestWidth, uint32_t uGuestHeight,
                                                               uint8_t *pu8BufferAddress, uint64_t u64TimeStamp);
    static DECLCALLBACK(bool) i_displayCrVRecScreenshotBegin(void *pvCtx, uint32_t uScreen, uint64_t u64TimeStamp);
    static DECLCALLBACK(void) i_displayCrVRecScreenshotEnd(void *pvCtx, uint32_t uScreen, uint64_t u64TimeStamp);

    static DECLCALLBACK(void) i_displayVRecCompletion(struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd, int rc, void *pvCompletion);
#endif
    static DECLCALLBACK(void) i_displayCrCmdFree(struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd, int rc, void *pvCompletion);

    static DECLCALLBACK(void) i_displaySSMSaveScreenshot(PSSMHANDLE pSSM, void *pvUser);
    static DECLCALLBACK(int)  i_displaySSMLoadScreenshot(PSSMHANDLE pSSM, void *pvUser, uint32_t uVersion, uint32_t uPass);
    static DECLCALLBACK(void) i_displaySSMSave(PSSMHANDLE pSSM, void *pvUser);
    static DECLCALLBACK(int)  i_displaySSMLoad(PSSMHANDLE pSSM, void *pvUser, uint32_t uVersion, uint32_t uPass);

    Console * const         mParent;
    /** Pointer to the associated display driver. */
    struct DRVMAINDISPLAY   *mpDrv;

    unsigned mcMonitors;
    /** Input mapping rectangle top left X relative to the first screen. */
    int32_t     xInputMappingOrigin;
    /** Input mapping rectangle top left Y relative to the first screen. */
    int32_t     yInputMappingOrigin;
    uint32_t    cxInputMapping;  /**< Input mapping rectangle width. */
    uint32_t    cyInputMapping;  /**< Input mapping rectangle height. */
    DISPLAYFBINFO maFramebuffers[SchemaDefs::MaxGuestMonitors];
    /** Does the VMM device have the "supports graphics" capability set?
     *  Does not go into the saved state as it is refreshed on restore. */
    bool        mfVMMDevSupportsGraphics;
    /** Mirror of the current guest VBVA capabilities. */
    uint32_t    mfGuestVBVACapabilities;
    /** Mirror of the current host cursor capabilities. */
    uint32_t    mfHostCursorCapabilities;

    bool mfSourceBitmapEnabled;
    bool volatile fVGAResizing;

    /** Are we in seamless mode?  Not saved, as we exit seamless on saving. */
    bool        mfSeamlessEnabled;
    /** Last set seamless visible region, number of rectangles. */
    uint32_t    mcRectVisibleRegion;
    /** Last set seamless visible region, data.  Freed on final clean-up. */
    PRTRECT     mpRectVisibleRegion;

    bool        mfVideoAccelVRDP;
    uint32_t    mfu32SupportedOrders;
    int32_t volatile mcVideoAccelVRDPRefs;

    /** Accelerate3DEnabled = true && GraphicsControllerType == VBoxVGA. */
    bool        mfIsCr3DEnabled;

#ifdef VBOX_WITH_CROGL
    bool        mfCrOglDataHidden;
#endif

#ifdef VBOX_WITH_CRHGSMI
    /* for fast host hgcm calls */
    HGCMCVSHANDLE mhCrOglSvc;
    RTCRITSECTRW mCrOglLock;
#endif
#ifdef VBOX_WITH_CROGL
    CR_MAIN_INTERFACE mCrOglCallbacks;
    volatile uint32_t mfCrOglVideoRecState;
    CRVBOXHGCMTAKESCREENSHOT mCrOglScreenshotData;
    VBOXCRCMDCTL_HGCM mCrOglScreenshotCtl;
#endif

    /* The legacy VBVA data and methods. */
    VIDEOACCEL mVideoAccelLegacy;

    int  i_VideoAccelEnable(bool fEnable, VBVAMEMORY *pVbvaMemory, PPDMIDISPLAYPORT pUpPort);
    void i_VideoAccelFlush(PPDMIDISPLAYPORT pUpPort);
    bool i_VideoAccelAllowed(void);

    int  i_videoAccelRefreshProcess(PPDMIDISPLAYPORT pUpPort);
    int  i_videoAccelEnable(bool fEnable, VBVAMEMORY *pVbvaMemory, PPDMIDISPLAYPORT pUpPort);
    int  i_videoAccelFlush(PPDMIDISPLAYPORT pUpPort);

    /* Legacy pre-HGSMI handlers. */
    void processAdapterData(void *pvVRAM, uint32_t u32VRAMSize);
    void processDisplayData(void *pvVRAM, unsigned uScreenId);

    /** Serializes access to mVideoAccelLegacy and mfVideoAccelVRDP, etc between VRDP and Display. */
    RTCRITSECT           mVideoAccelLock;

#ifdef VBOX_WITH_VIDEOREC
    /* Serializes access to video recording source bitmaps. */
    RTCRITSECT           mVideoRecLock;
    /** The current video recording configuration being used. */
    VIDEORECCFG          mVideoRecCfg;
    /** The video recording context. */
    VIDEORECCONTEXT     *mpVideoRecCtx;
    /** Array which defines which screens are being enabled for recording. */
    bool                 maVideoRecEnabled[SchemaDefs::MaxGuestMonitors];
#endif

public:

    static int i_displayTakeScreenshotEMT(Display *pDisplay, ULONG aScreenId, uint8_t **ppbData, size_t *pcbData,
                                          uint32_t *pcx, uint32_t *pcy, bool *pfMemFree);
#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    static BOOL  i_displayCheckTakeScreenshotCrOgl(Display *pDisplay, ULONG aScreenId, uint8_t *pbData,
                                                   uint32_t u32Width, uint32_t u32Height);
    int i_crCtlSubmit(struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd, PFNCRCTLCOMPLETION pfnCompletion, void *pvCompletion);
    int i_crCtlSubmitSync(struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd);
    /* copies the given command and submits it asynchronously,
     * i.e. the pCmd data may be discarded right after the call returns */
    int i_crCtlSubmitAsyncCmdCopy(struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd);
    /* performs synchronous request processing if 3D backend has something to display
     * this is primarily to work-around 3d<->main thread deadlocks on OSX
     * in case of async completion, the command is coppied to the allocated buffer,
     * freeded on command completion
     * can be used for "notification" commands, when client is not interested in command result,
     * that must synchronize with 3D backend only when some 3D data is displayed.
     * The routine does NOT provide any info on whether command is processed asynchronously or not */
    int i_crCtlSubmitSyncIfHasDataForScreen(uint32_t u32ScreenID, struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd);
#endif

private:
    static int i_InvalidateAndUpdateEMT(Display *pDisplay, unsigned uId, bool fUpdateAll);
    static int i_drawToScreenEMT(Display *pDisplay, ULONG aScreenId, BYTE *address, ULONG x, ULONG y, ULONG width, ULONG height);

    void i_updateGuestGraphicsFacility(void);

#if defined(VBOX_WITH_HGCM) && defined(VBOX_WITH_CROGL)
    int i_crOglWindowsShow(bool fShow);
#endif

#ifdef VBOX_WITH_HGSMI
    volatile uint32_t mu32UpdateVBVAFlags;
#endif

private:
    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(Display); /* Shuts up MSC warning C4625. */
};

/* The legacy VBVA helpers. */
int videoAccelConstruct(VIDEOACCEL *pVideoAccel);
void videoAccelDestroy(VIDEOACCEL *pVideoAccel);
void i_vbvaSetMemoryFlags(VBVAMEMORY *pVbvaMemory,
                          bool fVideoAccelEnabled,
                          bool fVideoAccelVRDP,
                          uint32_t fu32SupportedOrders,
                          DISPLAYFBINFO *paFBInfos,
                          unsigned cFBInfos);
int videoAccelEnterVGA(VIDEOACCEL *pVideoAccel);
void videoAccelLeaveVGA(VIDEOACCEL *pVideoAccel);
int videoAccelEnterVMMDev(VIDEOACCEL *pVideoAccel);
void videoAccelLeaveVMMDev(VIDEOACCEL *pVideoAccel);


/* helper function, code in DisplayResampleImage.cpp */
void BitmapScale32(uint8_t *dst, int dstW, int dstH,
                   const uint8_t *src, int iDeltaLine, int srcW, int srcH);

/* helper function, code in DisplayPNGUtul.cpp */
int DisplayMakePNG(uint8_t *pbData, uint32_t cx, uint32_t cy,
                   uint8_t **ppu8PNG, uint32_t *pcbPNG, uint32_t *pcxPNG, uint32_t *pcyPNG,
                     uint8_t fLimitSize);

class ATL_NO_VTABLE DisplaySourceBitmap:
    public DisplaySourceBitmapWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(DisplaySourceBitmap)

    HRESULT FinalConstruct();
    void FinalRelease();

    /* Public initializer/uninitializer for internal purposes only. */
    HRESULT init(ComObjPtr<Display> pDisplay, unsigned uScreenId, DISPLAYFBINFO *pFBInfo);
    void uninit();

    bool i_usesVRAM(void) { return m.pu8Allocated == NULL; }

private:
    // wrapped IDisplaySourceBitmap properties
    virtual HRESULT getScreenId(ULONG *aScreenId);

    // wrapped IDisplaySourceBitmap methods
    virtual HRESULT queryBitmapInfo(BYTE **aAddress,
                                    ULONG *aWidth,
                                    ULONG *aHeight,
                                    ULONG *aBitsPerPixel,
                                    ULONG *aBytesPerLine,
                                    BitmapFormat_T *aBitmapFormat);

    int initSourceBitmap(unsigned aScreenId, DISPLAYFBINFO *pFBInfo);

    struct Data
    {
        ComObjPtr<Display> pDisplay;
        unsigned uScreenId;
        DISPLAYFBINFO *pFBInfo;

        uint8_t *pu8Allocated;

        uint8_t *pu8Address;
        ULONG ulWidth;
        ULONG ulHeight;
        ULONG ulBitsPerPixel;
        ULONG ulBytesPerLine;
        BitmapFormat_T bitmapFormat;
    };

    Data m;
};

#endif // !____H_DISPLAYIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
