/* $Id: DrvHostDSound.cpp $ */
/** @file
 * Windows host backend driver using DirectSound.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>
#include <iprt/win/windows.h>
#include <dsound.h>

#include <iprt/alloc.h>
#include <iprt/uuid.h>

#include "DrvAudio.h"
#include "VBoxDD.h"
#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
# include <new> /* For bad_alloc. */
# include "VBoxMMNotificationClient.h"
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/*
 * IDirectSound* interface uses HRESULT status codes and the driver callbacks use
 * the IPRT status codes. To minimize HRESULT->IPRT conversion most internal functions
 * in the driver return HRESULT and conversion is done in the driver callbacks.
 *
 * Naming convention:
 * 'dsound*' functions return IPRT status code;
 * 'directSound*' - return HRESULT.
 */

/*
 * Optional release logging, which a user can turn on with the
 * 'VBoxManage debugvm' command.
 * Debug logging still uses the common Log* macros from IPRT.
 * Messages which always should go to the release log use LogRel.
 */
/* General code behavior. */
#define DSLOG(a)    do { LogRel2(a); } while(0)
/* Something which produce a lot of logging during playback/recording. */
#define DSLOGF(a)   do { LogRel3(a); } while(0)
/* Important messages like errors. Limited in the default release log to avoid log flood. */
#define DSLOGREL(a) \
    do {  \
        static int8_t s_cLogged = 0; \
        if (s_cLogged < 8) { \
            ++s_cLogged; \
            LogRel(a); \
        } else DSLOG(a); \
    } while (0)


/** Maximum number of attempts to restore the sound buffer before giving up. */
#define DRV_DSOUND_RESTORE_ATTEMPTS_MAX         3

/** Makes DRVHOSTDSOUND out of PDMIHOSTAUDIO. */
#define PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface) \
    ( (PDRVHOSTDSOUND)((uintptr_t)pInterface - RT_OFFSETOF(DRVHOSTDSOUND, IHostAudio)) )


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/* Dynamically load dsound.dll. */
typedef HRESULT WINAPI FNDIRECTSOUNDENUMERATEW(LPDSENUMCALLBACKW pDSEnumCallback, PVOID pContext);
typedef FNDIRECTSOUNDENUMERATEW *PFNDIRECTSOUNDENUMERATEW;
typedef HRESULT WINAPI FNDIRECTSOUNDCAPTUREENUMERATEW(LPDSENUMCALLBACKW pDSEnumCallback, PVOID pContext);
typedef FNDIRECTSOUNDCAPTUREENUMERATEW *PFNDIRECTSOUNDCAPTUREENUMERATEW;
typedef HRESULT WINAPI FNDIRECTSOUNDCAPTURECREATE8(LPCGUID lpcGUID, LPDIRECTSOUNDCAPTURE8 *lplpDSC, LPUNKNOWN pUnkOuter);
typedef FNDIRECTSOUNDCAPTURECREATE8 *PFNDIRECTSOUNDCAPTURECREATE8;

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
# define VBOX_DSOUND_MAX_EVENTS 3

typedef enum DSOUNDEVENT
{
    DSOUNDEVENT_NOTIFY = 0,
    DSOUNDEVENT_INPUT,
    DSOUNDEVENT_OUTPUT,
 } DSOUNDEVENT;
#endif /* VBOX_WITH_AUDIO_DEVICE_CALLBACKS */

typedef struct DSOUNDHOSTCFG
{
    DWORD   cbBufferIn;
    DWORD   cbBufferOut;
    RTUUID  uuidPlay;
    LPCGUID pGuidPlay;
    RTUUID  uuidCapture;
    LPCGUID pGuidCapture;
} DSOUNDHOSTCFG, *PDSOUNDHOSTCFG;

typedef struct DSOUNDSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    /** Buffer alignment. */
    uint8_t            uAlign;
    /** Whether this stream is in an enable state on the DirectSound side. */
    bool               fEnabled;
    union
    {
        struct
        {
            /** The actual DirectSound Buffer (DSB) used for the capturing.
             *  This is a secondary buffer and is used as a streaming buffer. */
            LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB;
            /** Current read offset (in bytes) within the DSB. */
            DWORD                       offReadPos;
            /** Size (in bytes) of the DirectSound buffer. */
            DWORD                       cbBufSize;
            HRESULT                     hrLastCapture;
        } In;
        struct
        {
            /** The actual DirectSound Buffer (DSB) used for playback.
             *  This is a secondary buffer and is used as a streaming buffer. */
            LPDIRECTSOUNDBUFFER8        pDSB;
            /** Current write offset (in bytes) within the DSB. */
            DWORD                       offWritePos;
            /** Offset of last play cursor within the DSB when checked for pending. */
            DWORD                       offPlayCursorLastPending;
            /** Offset of last play cursor within the DSB when last played. */
            DWORD                       offPlayCursorLastPlayed;
            /** Total amount (in bytes) written. */
            uint64_t                    cbWritten;
            /** Size (in bytes) of the DirectSound buffer. */
            DWORD                       cbBufSize;
            /** Flag indicating whether playback was (re)started. */
            bool                        fRestartPlayback;
        } Out;
    };
} DSOUNDSTREAM, *PDSOUNDSTREAM;

typedef struct DRVHOSTDSOUND
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS                  pDrvIns;
    /** Our audio host audio interface. */
    PDMIHOSTAUDIO               IHostAudio;
    /** Critical section to serialize access. */
    RTCRITSECT                  CritSect;
    /** List of found host input devices. */
    RTLISTANCHOR                lstDevInput;
    /** List of found host output devices. */
    RTLISTANCHOR                lstDevOutput;
    /** DirectSound configuration options. */
    DSOUNDHOSTCFG               cfg;
    /** Whether this backend supports any audio input. */
    bool                        fEnabledIn;
    /** Whether this backend supports any audio output. */
    bool                        fEnabledOut;
    /** The Direct Sound playback interface. */
    LPDIRECTSOUND8              pDS;
    /** The Direct Sound capturing interface. */
    LPDIRECTSOUNDCAPTURE8       pDSC;
#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
    VBoxMMNotificationClient   *m_pNotificationClient;
#endif
#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /** Callback function to the upper driver.
     *  Can be NULL if not being used / registered. */
    PFNPDMHOSTAUDIOCALLBACK     pfnCallback;
#endif
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    /** Pointer to the audio connector interface of the driver/device above us. */
    PPDMIAUDIOCONNECTOR         pUpIAudioConnector;
    /** Stopped indicator. */
    bool                        fStopped;
    /** Shutdown indicator. */
    bool                        fShutdown;
    /** Notification thread. */
    RTTHREAD                    Thread;
    /** Array of events to wait for in notification thread. */
    HANDLE                      aEvents[VBOX_DSOUND_MAX_EVENTS];
    /** Number of events to wait for in notification thread.
     *  Must not exceed VBOX_DSOUND_MAX_EVENTS. */
    uint8_t                     cEvents;
    /** Pointer to the input stream. */
    PDSOUNDSTREAM               pDSStrmIn;
    /** Pointer to the output stream. */
    PDSOUNDSTREAM               pDSStrmOut;
#endif
} DRVHOSTDSOUND, *PDRVHOSTDSOUND;

/** No flags specified. */
#define DSOUNDENUMCBFLAGS_NONE          0
/** (Release) log found devices. */
#define DSOUNDENUMCBFLAGS_LOG           RT_BIT(0)

/**
 * Callback context for enumeration callbacks
 */
typedef struct DSOUNDENUMCBCTX
{
    /** Pointer to host backend driver. */
    PDRVHOSTDSOUND      pDrv;
    /** Enumeration flags. */
    uint32_t            fFlags;
    /** Number of found input devices. */
    uint8_t             cDevIn;
    /** Number of found output devices. */
    uint8_t             cDevOut;
} DSOUNDENUMCBCTX, *PDSOUNDENUMCBCTX;

typedef struct DSOUNDDEV
{
    RTLISTNODE  Node;
    char       *pszName;
    GUID        Guid;
} DSOUNDDEV, *PDSOUNDDEV;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static HRESULT  directSoundPlayRestore(PDRVHOSTDSOUND pThis, LPDIRECTSOUNDBUFFER8 pDSB);
static HRESULT  directSoundCaptureStop(PDSOUNDSTREAM pStreamDS);

static void     dsoundDeviceRemove(PDSOUNDDEV pDev);
static int      dsoundDevicesEnumerate(PDRVHOSTDSOUND pThis, PPDMAUDIOBACKENDCFG pCfg);
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
static int      dsoundNotifyThread(PDRVHOSTDSOUND pThis, bool fShutdown);
#endif
static void     dsoundUpdateStatusInternal(PDRVHOSTDSOUND pThis);
static void     dsoundUpdateStatusInternalEx(PDRVHOSTDSOUND pThis, PPDMAUDIOBACKENDCFG pCfg, uint32_t fEnum);


static DWORD dsoundRingDistance(DWORD offEnd, DWORD offBegin, DWORD cSize)
{
    AssertReturn(offEnd <= cSize,   0);
    AssertReturn(offBegin <= cSize, 0);

    return offEnd >= offBegin ? offEnd - offBegin : cSize - offBegin + offEnd;
}


static int dsoundWaveFmtFromCfg(PPDMAUDIOSTREAMCFG pCfg, PWAVEFORMATEX pFmt)
{
    AssertPtrReturn(pCfg, VERR_INVALID_POINTER);
    AssertPtrReturn(pFmt, VERR_INVALID_POINTER);

    RT_BZERO(pFmt, sizeof(WAVEFORMATEX));

    pFmt->wFormatTag      = WAVE_FORMAT_PCM;
    pFmt->nChannels       = pCfg->Props.cChannels;
    pFmt->nSamplesPerSec  = pCfg->Props.uHz;
    pFmt->nAvgBytesPerSec = pCfg->Props.uHz << (pCfg->Props.cChannels == 2 ? 1: 0);
    pFmt->nBlockAlign     = 1 << (pCfg->Props.cChannels == 2 ? 1 : 0);
    pFmt->cbSize          = 0; /* No extra data specified. */

    switch (pCfg->Props.cBits)
    {
        case 8:
            pFmt->wBitsPerSample = 8;
            break;

        case 16:
            pFmt->wBitsPerSample = 16;
            pFmt->nAvgBytesPerSec <<= 1;
            pFmt->nBlockAlign <<= 1;
            break;

        case 32:
            pFmt->wBitsPerSample = 32;
            pFmt->nAvgBytesPerSec <<= 2;
            pFmt->nBlockAlign <<= 2;
            break;

        default:
            AssertMsgFailed(("Wave format for %RU8 bits not supported\n", pCfg->Props.cBits));
            return VERR_NOT_SUPPORTED;
    }

    return VINF_SUCCESS;
}


/**
 * Retrieves the number of free bytes available for writing to a DirectSound output stream.
 *
 * @return  IPRT status code. VERR_NOT_AVAILABLE if unable to determine or the buffer was not recoverable.
 * @param   pThis               Host audio driver instance.
 * @param   pStreamDS           DirectSound output stream to retrieve number for.
 * @param   pdwFree             Where to return the free amount on success.
 */
static int dsoundGetFreeOut(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, DWORD *pdwFree)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pStreamDS, VERR_INVALID_POINTER);
    AssertPtrReturn(pdwFree,   VERR_INVALID_POINTER);

    Assert(pStreamDS->pCfg->enmDir == PDMAUDIODIR_OUT); /* Paranoia. */

    LPDIRECTSOUNDBUFFER8 pDSB = pStreamDS->Out.pDSB;
    if (!pDSB)
    {
        AssertPtr(pDSB);
        return VERR_INVALID_POINTER;
    }

    HRESULT hr = S_OK;

    /* Get the current play position which is used for calculating the free space in the buffer. */
    for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
    {
        DWORD cbPlayCursor;
        hr = IDirectSoundBuffer8_GetCurrentPosition(pDSB, &cbPlayCursor, NULL /* cbWriteCursor */);
        if (SUCCEEDED(hr))
        {
            *pdwFree = pStreamDS->Out.cbBufSize
                     - dsoundRingDistance(pStreamDS->Out.offWritePos, cbPlayCursor, pStreamDS->Out.cbBufSize);

            return VINF_SUCCESS;
        }

        if (hr != DSERR_BUFFERLOST) /** @todo MSDN doesn't state this error for GetCurrentPosition(). */
            break;

        LogFunc(("Getting playing position failed due to lost buffer, restoring ...\n"));

        directSoundPlayRestore(pThis, pDSB);
    }

    if (hr != DSERR_BUFFERLOST) /* Avoid log flooding if the error is still there. */
        DSLOGREL(("DSound: Getting current playback position failed with %Rhrc\n", hr));

    LogFunc(("Failed with %Rhrc\n", hr));

    return VERR_NOT_AVAILABLE;
}


static char *dsoundGUIDToUtf8StrA(LPCGUID pGUID)
{
    if (pGUID)
    {
        LPOLESTR lpOLEStr;
        HRESULT hr = StringFromCLSID(*pGUID, &lpOLEStr);
        if (SUCCEEDED(hr))
        {
            char *pszGUID;
            int rc = RTUtf16ToUtf8(lpOLEStr, &pszGUID);
            CoTaskMemFree(lpOLEStr);

            return RT_SUCCESS(rc) ? pszGUID : NULL;
        }
    }

    return RTStrDup("{Default device}");
}


/**
 * Clears the list of the host's playback + capturing devices.
 *
 * @param   pThis               Host audio driver instance.
 */
static void dsoundDevicesClear(PDRVHOSTDSOUND pThis)
{
    AssertPtrReturnVoid(pThis);

    PDSOUNDDEV pDev, pDevNext;
    RTListForEachSafe(&pThis->lstDevInput, pDev, pDevNext, DSOUNDDEV, Node)
        dsoundDeviceRemove(pDev);

    Assert(RTListIsEmpty(&pThis->lstDevInput));

    RTListForEachSafe(&pThis->lstDevOutput, pDev, pDevNext, DSOUNDDEV, Node)
        dsoundDeviceRemove(pDev);

    Assert(RTListIsEmpty(&pThis->lstDevOutput));
}


static HRESULT directSoundPlayRestore(PDRVHOSTDSOUND pThis, LPDIRECTSOUNDBUFFER8 pDSB)
{
    RT_NOREF(pThis);
    HRESULT hr = IDirectSoundBuffer8_Restore(pDSB);
    if (FAILED(hr))
        DSLOGREL(("DSound: Restoring playback buffer failed with %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundPlayUnlock(PDRVHOSTDSOUND pThis, LPDIRECTSOUNDBUFFER8 pDSB,
                                     PVOID pv1, PVOID pv2,
                                     DWORD cb1, DWORD cb2)
{
    RT_NOREF(pThis);
    HRESULT hr = IDirectSoundBuffer8_Unlock(pDSB, pv1, cb1, pv2, cb2);
    if (FAILED(hr))
        DSLOGREL(("DSound: Unlocking playback buffer failed with %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundCaptureUnlock(LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB,
                                        PVOID pv1, PVOID pv2,
                                        DWORD cb1, DWORD cb2)
{
    HRESULT hr = IDirectSoundCaptureBuffer8_Unlock(pDSCB, pv1, cb1, pv2, cb2);
    if (FAILED(hr))
        DSLOGREL(("DSound: Unlocking capture buffer failed with %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundPlayLock(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                   DWORD dwOffset, DWORD dwBytes,
                                   PVOID *ppv1, PVOID *ppv2,
                                   DWORD *pcb1, DWORD *pcb2,
                                   DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    AssertCompile(DRV_DSOUND_RESTORE_ATTEMPTS_MAX > 0);
    for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
    {
        *ppv1 = *ppv2 = NULL;
        *pcb1 = *pcb2 = 0;
        hr = IDirectSoundBuffer8_Lock(pStreamDS->Out.pDSB, dwOffset, dwBytes, ppv1, pcb1, ppv2, pcb2, dwFlags);
        if (SUCCEEDED(hr))
        {
            if (   (!*ppv1 || !(*pcb1 & pStreamDS->uAlign))
                && (!*ppv2 || !(*pcb2 & pStreamDS->uAlign)) )
                return S_OK;
            DSLOGREL(("DSound: Locking playback buffer returned misaligned buffer: cb1=%#RX32, cb2=%#RX32 (alignment: %#RX32)\n",
                      *pcb1, *pcb2, pStreamDS->uAlign));
            directSoundPlayUnlock(pThis, pStreamDS->Out.pDSB, *ppv1, *ppv2, *pcb1, *pcb2);
            return E_FAIL;
        }

        if (hr != DSERR_BUFFERLOST)
            break;

        LogFlowFunc(("Locking failed due to lost buffer, restoring ...\n"));
        directSoundPlayRestore(pThis, pStreamDS->Out.pDSB);
    }

    DSLOGREL(("DSound: Locking playback buffer failed with %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundCaptureLock(PDSOUNDSTREAM pStreamDS,
                                      DWORD dwOffset, DWORD dwBytes,
                                      PVOID *ppv1, PVOID *ppv2,
                                      DWORD *pcb1, DWORD *pcb2,
                                      DWORD dwFlags)
{
    PVOID pv1 = NULL;
    PVOID pv2 = NULL;
    DWORD cb1 = 0;
    DWORD cb2 = 0;

    HRESULT hr = IDirectSoundCaptureBuffer8_Lock(pStreamDS->In.pDSCB, dwOffset, dwBytes,
                                                 &pv1, &cb1, &pv2, &cb2, dwFlags);
    if (FAILED(hr))
    {
        DSLOGREL(("DSound: Locking capture buffer failed with %Rhrc\n", hr));
        return hr;
    }

    if (   (pv1 && (cb1 & pStreamDS->uAlign))
        || (pv2 && (cb2 & pStreamDS->uAlign)))
    {
        DSLOGREL(("DSound: Locking capture buffer returned misaligned buffer: cb1=%RI32, cb2=%RI32 (alignment: %RU32)\n",
                  cb1, cb2, pStreamDS->uAlign));
        directSoundCaptureUnlock(pStreamDS->In.pDSCB, pv1, pv2, cb1, cb2);
        return E_FAIL;
    }

    *ppv1 = pv1;
    *ppv2 = pv2;
    *pcb1 = cb1;
    *pcb2 = cb2;

    return S_OK;
}


/*
 * DirectSound playback
 */

static void directSoundPlayInterfaceDestroy(PDRVHOSTDSOUND pThis)
{
    if (pThis->pDS)
    {
        LogFlowFuncEnter();

        IDirectSound8_Release(pThis->pDS);
        pThis->pDS = NULL;
    }
}


static HRESULT directSoundPlayInterfaceCreate(PDRVHOSTDSOUND pThis)
{
    if (pThis->pDS != NULL)
        return S_OK;

    LogFlowFuncEnter();

    HRESULT hr = CoCreateInstance(CLSID_DirectSound8, NULL, CLSCTX_ALL,
                                  IID_IDirectSound8, (void **)&pThis->pDS);
    if (FAILED(hr))
    {
        DSLOGREL(("DSound: Creating playback instance failed with %Rhrc\n", hr));
    }
    else
    {
        hr = IDirectSound8_Initialize(pThis->pDS, pThis->cfg.pGuidPlay);
        if (SUCCEEDED(hr))
        {
            HWND hWnd = GetDesktopWindow();
            hr = IDirectSound8_SetCooperativeLevel(pThis->pDS, hWnd, DSSCL_PRIORITY);
            if (FAILED(hr))
                DSLOGREL(("DSound: Setting cooperative level for window %p failed with %Rhrc\n", hWnd, hr));
        }

        if (FAILED(hr))
        {
            if (hr == DSERR_NODRIVER) /* Usually means that no playback devices are attached. */
                DSLOGREL(("DSound: DirectSound playback is currently unavailable\n"));
            else
                DSLOGREL(("DSound: DirectSound playback initialization failed with %Rhrc\n", hr));

            directSoundPlayInterfaceDestroy(pThis);
        }
    }

    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundPlayClose(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr = S_OK;

    if (pStreamDS->Out.pDSB)
    {
        DSLOG(("DSound: Closing playback stream %p, buffer %p\n", pStreamDS, pStreamDS->Out.pDSB));

        hr = IDirectSoundBuffer8_Stop(pStreamDS->Out.pDSB);
        if (SUCCEEDED(hr))
        {
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
            if (pThis->aEvents[DSOUNDEVENT_OUTPUT] != NULL)
            {
                CloseHandle(pThis->aEvents[DSOUNDEVENT_OUTPUT]);
                pThis->aEvents[DSOUNDEVENT_OUTPUT] = NULL;

                if (pThis->cEvents)
                    pThis->cEvents--;

                pThis->pDSStrmOut = NULL;
            }

            int rc2 = dsoundNotifyThread(pThis, false /* fShutdown */);
            AssertRC(rc2);
#endif
            IDirectSoundBuffer8_Release(pStreamDS->Out.pDSB);
            pStreamDS->Out.pDSB = NULL;
        }
    }

    if (FAILED(hr))
        DSLOGREL(("DSound: Stopping playback stream %p failed with %Rhrc\n", pStreamDS, hr));

    return hr;
}


static HRESULT directSoundPlayOpen(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                   PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);
    AssertPtrReturn(pCfgReq,   E_POINTER);
    AssertPtrReturn(pCfgAcq,   E_POINTER);

    DSLOG(("DSound: Opening playback stream %p: cbBufferOut=%ld, uHz=%RU32, cChannels=%RU8, cBits=%RU8, fSigned=%RTbool\n",
           pStreamDS,
           pThis->cfg.cbBufferOut,
           pCfgReq->Props.uHz,
           pCfgReq->Props.cChannels,
           pCfgReq->Props.cBits,
           pCfgReq->Props.fSigned));

    if (pStreamDS->Out.pDSB != NULL)
    {
        /* Should not happen but be forgiving. */
        DSLOGREL(("DSound: Playback buffer already exists\n"));
        directSoundPlayClose(pThis, pStreamDS);
    }

    WAVEFORMATEX wfx;
    int rc = dsoundWaveFmtFromCfg(pCfgReq, &wfx);
    if (RT_FAILURE(rc))
        return E_INVALIDARG;

    dsoundUpdateStatusInternal(pThis);

    HRESULT hr = directSoundPlayInterfaceCreate(pThis);
    if (FAILED(hr))
        return hr;

    do /* To use breaks. */
    {
        LPDIRECTSOUNDBUFFER pDSB = NULL;

        DSBUFFERDESC bd;
        RT_ZERO(bd);
        bd.dwSize      = sizeof(bd);
        bd.lpwfxFormat = &wfx;

        /*
         * As we reuse our (secondary) buffer for playing out data as it comes in,
         * we're using this buffer as a so-called streaming buffer.
         *
         * See https://msdn.microsoft.com/en-us/library/windows/desktop/ee419014(v=vs.85).aspx
         *
         * However, as we do not want to use memory on the sound device directly
         * (as most modern audio hardware on the host doesn't have this anyway),
         * we're *not* going to use DSBCAPS_STATIC for that.
         *
         * Instead we're specifying DSBCAPS_LOCSOFTWARE, as this fits the bill
         * of copying own buffer data to our secondary's Direct Sound buffer.
         */
        bd.dwFlags     = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
        bd.dwFlags    |= DSBCAPS_CTRLPOSITIONNOTIFY;
#endif
        bd.dwBufferBytes = pThis->cfg.cbBufferOut;

        hr = IDirectSound8_CreateSoundBuffer(pThis->pDS, &bd, &pDSB, NULL);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Creating playback sound buffer failed with %Rhrc\n", hr));
            break;
        }

        /* "Upgrade" to IDirectSoundBuffer8 interface. */
        hr = IDirectSoundBuffer_QueryInterface(pDSB, IID_IDirectSoundBuffer8, (PVOID *)&pStreamDS->Out.pDSB);
        IDirectSoundBuffer_Release(pDSB);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Querying playback sound buffer interface failed with %Rhrc\n", hr));
            break;
        }

        /*
         * Query the actual parameters.
         */
        hr = IDirectSoundBuffer8_GetFormat(pStreamDS->Out.pDSB, &wfx, sizeof(wfx), NULL);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Getting playback format failed with %Rhrc\n", hr));
            break;
        }

        DSBCAPS bc;
        RT_ZERO(bc);
        bc.dwSize = sizeof(bc);

        hr = IDirectSoundBuffer8_GetCaps(pStreamDS->Out.pDSB, &bc);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Getting playback capabilities failed with %Rhrc\n", hr));
            break;
        }

        DSLOG(("DSound: Playback format:\n"
               "  dwBufferBytes   = %RI32\n"
               "  dwFlags         = 0x%x\n"
               "  wFormatTag      = %RI16\n"
               "  nChannels       = %RI16\n"
               "  nSamplesPerSec  = %RU32\n"
               "  nAvgBytesPerSec = %RU32\n"
               "  nBlockAlign     = %RI16\n"
               "  wBitsPerSample  = %RI16\n"
               "  cbSize          = %RI16\n",
               bc.dwBufferBytes,
               bc.dwFlags,
               wfx.wFormatTag,
               wfx.nChannels,
               wfx.nSamplesPerSec,
               wfx.nAvgBytesPerSec,
               wfx.nBlockAlign,
               wfx.wBitsPerSample,
               wfx.cbSize));

        if (bc.dwBufferBytes & pStreamDS->uAlign)
            DSLOGREL(("DSound: Playback capabilities returned misaligned buffer: size %RU32, alignment %RU32\n",
                      bc.dwBufferBytes, pStreamDS->uAlign + 1));

        if (bc.dwBufferBytes != pThis->cfg.cbBufferOut)
            DSLOGREL(("DSound: Playback buffer size mismatched: DirectSound %RU32, requested %RU32 bytes\n",
                      bc.dwBufferBytes, pThis->cfg.cbBufferOut));

        /*
         * Initial state.
         * dsoundPlayStart initializes part of it to make sure that Stop/Start continues with a correct
         * playback buffer position.
         */
        pStreamDS->Out.cbBufSize = bc.dwBufferBytes;
        DSLOG(("DSound: cMaxSamplesInBuffer=%RU32\n", pStreamDS->Out.cbBufSize));

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
        /*
         * Install notification.
         */
        pThis->aEvents[DSOUNDEVENT_OUTPUT] = CreateEvent(NULL /* Security attribute */,
                                                         FALSE /* bManualReset */, FALSE /* bInitialState */,
                                                         NULL /* lpName */);
        if (pThis->aEvents[DSOUNDEVENT_OUTPUT] == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DSLOGREL(("DSound: CreateEvent for output failed with %Rhrc\n", hr));
            break;
        }

        LPDIRECTSOUNDNOTIFY8 pNotify;
        hr = IDirectSoundNotify_QueryInterface(pStreamDS->Out.pDSB, IID_IDirectSoundNotify8, (PVOID *)&pNotify);
        if (SUCCEEDED(hr))
        {
            DSBPOSITIONNOTIFY dsBufPosNotify;
            RT_ZERO(dsBufPosNotify);
            dsBufPosNotify.dwOffset     = DSBPN_OFFSETSTOP;
            dsBufPosNotify.hEventNotify = pThis->aEvents[DSOUNDEVENT_OUTPUT];

            hr = IDirectSoundNotify_SetNotificationPositions(pNotify, 1 /* Count */, &dsBufPosNotify);
            if (FAILED(hr))
                DSLOGREL(("DSound: Setting playback position notification failed with %Rhrc\n", hr));

            IDirectSoundNotify_Release(pNotify);
        }
        else
            DSLOGREL(("DSound: Querying interface for position notification failed with %Rhrc\n", hr));

        if (FAILED(hr))
            break;

        pThis->pDSStrmOut = pStreamDS;

        Assert(pThis->cEvents < VBOX_DSOUND_MAX_EVENTS);
        pThis->cEvents++;

        /* Let the thread know. */
        dsoundNotifyThread(pThis, false /* fShutdown */);

        /* Trigger the just installed output notification. */
        hr = IDirectSoundBuffer8_Play(pStreamDS->Out.pDSB, 0, 0, 0);
        if (FAILED(hr))
            break;

#endif /* VBOX_WITH_AUDIO_DEVICE_CALLBACKS */

        pCfgAcq->cFrameBufferHint = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, pThis->cfg.cbBufferOut);

    } while (0);

    if (FAILED(hr))
        directSoundPlayClose(pThis, pStreamDS);

    return hr;
}


static void dsoundPlayClearSamples(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturnVoid(pStreamDS);

    AssertPtr(pStreamDS->pCfg);
    PPDMAUDIOPCMPROPS pProps = &pStreamDS->pCfg->Props;

    PVOID pv1, pv2;
    DWORD cb1, cb2;
    HRESULT hr = directSoundPlayLock(pThis, pStreamDS,
                                     0 /* dwOffset */, pStreamDS->Out.cbBufSize,
                                     &pv1, &pv2, &cb1, &cb2, DSBLOCK_ENTIREBUFFER);
    if (SUCCEEDED(hr))
    {
        DWORD len1 = PDMAUDIOPCMPROPS_B2F(pProps, cb1);
        DWORD len2 = PDMAUDIOPCMPROPS_B2F(pProps, cb2);

        if (pv1 && len1)
            DrvAudioHlpClearBuf(pProps, pv1, cb1, len1);

        if (pv2 && len2)
            DrvAudioHlpClearBuf(pProps, pv2, cb2, len2);

        directSoundPlayUnlock(pThis, pStreamDS->Out.pDSB, pv1, pv2, cb1, cb2);
    }
}


static HRESULT directSoundPlayGetStatus(PDRVHOSTDSOUND pThis, LPDIRECTSOUNDBUFFER8 pDSB, DWORD *pdwStatus)
{
    AssertPtrReturn(pThis, E_POINTER);
    AssertPtrReturn(pDSB,  E_POINTER);

    AssertPtrNull(pdwStatus);

    DWORD dwStatus = 0;
    HRESULT hr = E_FAIL;
    for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
    {
        hr = IDirectSoundBuffer8_GetStatus(pDSB, &dwStatus);
        if (   hr == DSERR_BUFFERLOST
            || (   SUCCEEDED(hr)
                && (dwStatus & DSBSTATUS_BUFFERLOST)))
        {
            LogFlowFunc(("Getting status failed due to lost buffer, restoring ...\n"));
            directSoundPlayRestore(pThis, pDSB);
        }
        else
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (pdwStatus)
            *pdwStatus = dwStatus;
    }
    else
        DSLOGREL(("DSound: Retrieving playback status failed with %Rhrc\n", hr));

    return hr;
}


static HRESULT directSoundPlayStop(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr = S_OK;

    if (pStreamDS->Out.pDSB)
    {
        if (pStreamDS->fEnabled)
        {
            DSLOG(("DSound: Stopping playback\n"));

            hr = IDirectSoundBuffer8_Stop(pStreamDS->Out.pDSB);
            if (FAILED(hr))
            {
                hr = directSoundPlayRestore(pThis, pStreamDS->Out.pDSB);
                if (FAILED(hr))
                    hr = IDirectSoundBuffer8_Stop(pStreamDS->Out.pDSB);
            }

            if (SUCCEEDED(hr))
                pStreamDS->fEnabled = false;
        }
    }

    if (FAILED(hr))
        DSLOGREL(("DSound: Stopping playback failed with %Rhrc\n", hr));

    return hr;
}


static HRESULT directSoundPlayStart(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr;
    if (pStreamDS->Out.pDSB != NULL)
    {
        DWORD dwStatus;
        hr = directSoundPlayGetStatus(pThis, pStreamDS->Out.pDSB, &dwStatus);
        if (SUCCEEDED(hr))
        {
            if (dwStatus & DSBSTATUS_PLAYING)
            {
                DSLOG(("DSound: Already playing\n"));
            }
            else
            {
                dsoundPlayClearSamples(pThis, pStreamDS);

                pStreamDS->Out.fRestartPlayback = true;
                pStreamDS->fEnabled             = true;

                DSLOG(("DSound: Playback started\n"));

                /*
                 * The actual IDirectSoundBuffer8_Play call will be made in drvHostDSoundPlay,
                 * because it is necessary to put some samples into the buffer first.
                 */
            }
        }
    }
    else
        hr = E_UNEXPECTED;

    if (FAILED(hr))
        DSLOGREL(("DSound: Starting playback failed with %Rhrc\n", hr));

    return hr;
}

/*
 * DirectSoundCapture
 */

static LPCGUID dsoundCaptureSelectDevice(PDRVHOSTDSOUND pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, NULL);
    AssertPtrReturn(pCfg,  NULL);

    int rc = VINF_SUCCESS;

    LPCGUID pGUID = pThis->cfg.pGuidCapture;
    if (!pGUID)
    {
        PDSOUNDDEV pDev = NULL;

        switch (pCfg->DestSource.Source)
        {
            case PDMAUDIORECSOURCE_LINE:
                /*
                 * At the moment we're only supporting line-in in the HDA emulation,
                 * and line-in + mic-in in the AC'97 emulation both are expected
                 * to use the host's mic-in as well.
                 *
                 * So the fall through here is intentional for now.
                 */
            case PDMAUDIORECSOURCE_MIC:
            {
                RTListForEach(&pThis->lstDevInput, pDev, DSOUNDDEV, Node)
                {
                    if (RTStrIStr(pDev->pszName, "Mic")) /** @todo What is with non en_us windows versions? */
                        break;
                }

                if (RTListNodeIsDummy(&pThis->lstDevInput, pDev, DSOUNDDEV, Node))
                    pDev = NULL; /* Found nothing. */

                break;
            }

            default:
                AssertFailedStmt(rc = VERR_NOT_SUPPORTED);
                break;
        }

        if (   RT_SUCCESS(rc)
            && pDev)
        {
            DSLOG(("DSound: Guest source '%s' is using host recording device '%s'\n",
                   DrvAudioHlpRecSrcToStr(pCfg->DestSource.Source), pDev->pszName));

            pGUID = &pDev->Guid;
        }
    }

    if (RT_FAILURE(rc))
    {
        LogRel(("DSound: Selecting recording device failed with %Rrc\n", rc));
        return NULL;
    }

    char *pszGUID = dsoundGUIDToUtf8StrA(pGUID);

    /* This always has to be in the release log. */
    LogRel(("DSound: Guest source '%s' is using host recording device with GUID '%s'\n",
            DrvAudioHlpRecSrcToStr(pCfg->DestSource.Source), pszGUID ? pszGUID: "{?}"));

    if (pszGUID)
    {
        RTStrFree(pszGUID);
        pszGUID = NULL;
    }

    return pGUID;
}

/**
 * Destroys the DirectSound capturing interface.
 *
 * @return  IPRT status code.
 * @param   pThis               Driver instance to destroy capturing interface for.
 */
static void directSoundCaptureInterfaceDestroy(PDRVHOSTDSOUND pThis)
{
    if (pThis->pDSC)
    {
        LogFlowFuncEnter();

        IDirectSoundCapture_Release(pThis->pDSC);
        pThis->pDSC = NULL;
    }
}

/**
 * Creates the DirectSound capturing interface.
 *
 * @return  IPRT status code.
 * @param   pThis               Driver instance to create the capturing interface for.
 * @param   pCfg                Audio stream to use for creating the capturing interface.
 */
static HRESULT directSoundCaptureInterfaceCreate(PDRVHOSTDSOUND pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pCfg,      E_POINTER);

    if (pThis->pDSC)
        return S_OK;

    LogFlowFuncEnter();

    HRESULT hr = CoCreateInstance(CLSID_DirectSoundCapture8, NULL, CLSCTX_ALL,
                                  IID_IDirectSoundCapture8, (void **)&pThis->pDSC);
    if (FAILED(hr))
    {
        DSLOGREL(("DSound: Creating capture instance failed with %Rhrc\n", hr));
    }
    else
    {
        LPCGUID pGUID = dsoundCaptureSelectDevice(pThis, pCfg);
        /* pGUID can be NULL when using the default device. */

        hr = IDirectSoundCapture_Initialize(pThis->pDSC, pGUID);
        if (FAILED(hr))
        {
            if (hr == DSERR_NODRIVER) /* Usually means that no capture devices are attached. */
                DSLOGREL(("DSound: Capture device currently is unavailable\n"));
            else
                DSLOGREL(("DSound: Initializing capturing device failed with %Rhrc\n", hr));

            directSoundCaptureInterfaceDestroy(pThis);
        }
    }

    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundCaptureClose(PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr = S_OK;

    if (   pStreamDS
        && pStreamDS->In.pDSCB)
    {
        DSLOG(("DSound: Closing capturing stream %p, buffer %p\n", pStreamDS, pStreamDS->In.pDSCB));

        hr = directSoundCaptureStop(pStreamDS);
        if (SUCCEEDED(hr))
        {
            IDirectSoundCaptureBuffer8_Release(pStreamDS->In.pDSCB);
            pStreamDS->In.pDSCB = NULL;
        }
        else
            DSLOGREL(("DSound: Stopping capture buffer failed with %Rhrc\n", hr));
    }

    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundCaptureOpen(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                      PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);
    AssertPtrReturn(pCfgReq,   E_POINTER);
    AssertPtrReturn(pCfgAcq,   E_POINTER);

    DSLOG(("DSound: Opening capturing stream %p: cbBufferIn=%ld, uHz=%RU32, cChannels=%RU8, cBits=%RU8, fSigned=%RTbool\n",
           pStreamDS,
           pThis->cfg.cbBufferIn,
           pCfgReq->Props.uHz,
           pCfgReq->Props.cChannels,
           pCfgReq->Props.cBits,
           pCfgReq->Props.fSigned));

    if (pStreamDS->In.pDSCB != NULL)
    {
        /* Should not happen but be forgiving. */
        DSLOGREL(("DSound: DirectSoundCaptureBuffer already exists\n"));
        directSoundCaptureClose(pStreamDS);
    }

    WAVEFORMATEX wfx;
    int rc = dsoundWaveFmtFromCfg(pCfgReq, &wfx);
    if (RT_FAILURE(rc))
        return E_INVALIDARG;

    dsoundUpdateStatusInternalEx(pThis, NULL /* Cfg */, DSOUNDENUMCBFLAGS_LOG /* fEnum */);

    HRESULT hr = directSoundCaptureInterfaceCreate(pThis, pCfgReq);
    if (FAILED(hr))
        return hr;

    do /* To use breaks. */
    {
        LPDIRECTSOUNDCAPTUREBUFFER pDSCB = NULL;

        DSCBUFFERDESC bd;
        RT_ZERO(bd);

        bd.dwSize        = sizeof(bd);
        bd.lpwfxFormat   = &wfx;
        bd.dwBufferBytes = pThis->cfg.cbBufferIn;

        hr = IDirectSoundCapture_CreateCaptureBuffer(pThis->pDSC, &bd, &pDSCB, NULL);
        if (FAILED(hr))
        {
            if (hr == E_ACCESSDENIED)
            {
                DSLOGREL(("DSound: Capturing input from host not possible, access denied\n"));
            }
            else
                DSLOGREL(("DSound: Creating capture buffer failed with %Rhrc\n", hr));
            break;
        }

        hr = IDirectSoundCaptureBuffer_QueryInterface(pDSCB, IID_IDirectSoundCaptureBuffer8, (void **)&pStreamDS->In.pDSCB);
        IDirectSoundCaptureBuffer_Release(pDSCB);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Querying interface for capture buffer failed with %Rhrc\n", hr));
            break;
        }

        /*
         * Query the actual parameters.
         */
        DWORD offByteReadPos = 0;
        hr = IDirectSoundCaptureBuffer8_GetCurrentPosition(pStreamDS->In.pDSCB, NULL, &offByteReadPos);
        if (FAILED(hr))
        {
            offByteReadPos = 0;
            DSLOGREL(("DSound: Getting capture position failed with %Rhrc\n", hr));
        }

        RT_ZERO(wfx);
        hr = IDirectSoundCaptureBuffer8_GetFormat(pStreamDS->In.pDSCB, &wfx, sizeof(wfx), NULL);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Getting capture format failed with %Rhrc\n", hr));
            break;
        }

        DSCBCAPS bc;
        RT_ZERO(bc);
        bc.dwSize = sizeof(bc);
        hr = IDirectSoundCaptureBuffer8_GetCaps(pStreamDS->In.pDSCB, &bc);
        if (FAILED(hr))
        {
            DSLOGREL(("Getting capture capabilities failed with %Rhrc\n", hr));
            break;
        }

        DSLOG(("DSound: Capture format:\n"
               "  dwBufferBytes   = %RI32\n"
               "  dwFlags         = 0x%x\n"
               "  wFormatTag      = %RI16\n"
               "  nChannels       = %RI16\n"
               "  nSamplesPerSec  = %RU32\n"
               "  nAvgBytesPerSec = %RU32\n"
               "  nBlockAlign     = %RI16\n"
               "  wBitsPerSample  = %RI16\n"
               "  cbSize          = %RI16\n",
               bc.dwBufferBytes,
               bc.dwFlags,
               wfx.wFormatTag,
               wfx.nChannels,
               wfx.nSamplesPerSec,
               wfx.nAvgBytesPerSec,
               wfx.nBlockAlign,
               wfx.wBitsPerSample,
               wfx.cbSize));

        if (bc.dwBufferBytes & pStreamDS->uAlign)
            DSLOGREL(("DSound: Capture GetCaps returned misaligned buffer: size %RU32, alignment %RU32\n",
                      bc.dwBufferBytes, pStreamDS->uAlign + 1));

        if (bc.dwBufferBytes != pThis->cfg.cbBufferIn)
            DSLOGREL(("DSound: Capture buffer size mismatched: DirectSound %RU32, requested %RU32 bytes\n",
                      bc.dwBufferBytes, pThis->cfg.cbBufferIn));

        /* Initial state: reading at the initial capture position, no error. */
        pStreamDS->In.offReadPos = offByteReadPos;
        pStreamDS->In.cbBufSize  = bc.dwBufferBytes;

        pStreamDS->In.hrLastCapture     = S_OK;

        DSLOG(("DSound: Opened capturing offReadPos=%RU32, cbBufSize=%RU32\n",
               pStreamDS->In.offReadPos, pStreamDS->In.cbBufSize));

        pCfgAcq->cFrameBufferHint = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, pThis->cfg.cbBufferIn);

    } while (0);

    if (FAILED(hr))
        directSoundCaptureClose(pStreamDS);

    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}


static HRESULT directSoundCaptureStop(PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr = S_OK;

    if (pStreamDS->In.pDSCB)
    {
        if (pStreamDS->fEnabled)
        {
            DSLOG(("DSound: Stopping capture\n"));

            hr = IDirectSoundCaptureBuffer_Stop(pStreamDS->In.pDSCB);
            if (SUCCEEDED(hr))
                pStreamDS->fEnabled = false;
        }
    }

    if (FAILED(hr))
        DSLOGREL(("DSound: Stopping capture buffer failed with %Rhrc\n", hr));

    return hr;
}


static HRESULT directSoundCaptureStart(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     VERR_INVALID_POINTER);
    AssertPtrReturn(pStreamDS, VERR_INVALID_POINTER);

    HRESULT hr;
    if (pStreamDS->In.pDSCB != NULL)
    {
        DWORD dwStatus;
        hr = IDirectSoundCaptureBuffer8_GetStatus(pStreamDS->In.pDSCB, &dwStatus);
        if (FAILED(hr))
        {
            DSLOGREL(("DSound: Retrieving capture status failed with %Rhrc\n", hr));
        }
        else
        {
            if (dwStatus & DSCBSTATUS_CAPTURING)
            {
                DSLOG(("DSound: Already capturing\n"));
            }
            else
            {
                DWORD fFlags = 0;
#ifndef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
                fFlags |= DSCBSTART_LOOPING;
#endif
                DSLOG(("DSound: Starting to capture\n"));
                hr = IDirectSoundCaptureBuffer8_Start(pStreamDS->In.pDSCB, fFlags);
                if (SUCCEEDED(hr))
                {
                    pStreamDS->fEnabled = true;
                }
                else
                    DSLOGREL(("DSound: Starting to capture failed with %Rhrc\n", hr));
            }
        }
    }
    else
        hr = E_UNEXPECTED;

    if (SUCCEEDED(hr))


    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}


static int dsoundDevAdd(PRTLISTANCHOR pList, LPGUID pGUID, LPCWSTR pwszDescription, PDSOUNDDEV *ppDev)
{
    AssertPtrReturn(pList, VERR_INVALID_POINTER);
    AssertPtrReturn(pGUID, VERR_INVALID_POINTER);
    AssertPtrReturn(pwszDescription, VERR_INVALID_POINTER);

    PDSOUNDDEV pDev = (PDSOUNDDEV)RTMemAlloc(sizeof(DSOUNDDEV));
    if (!pDev)
        return VERR_NO_MEMORY;

    int rc = RTUtf16ToUtf8(pwszDescription, &pDev->pszName);
    if (   RT_SUCCESS(rc)
        && pGUID)
    {
        memcpy(&pDev->Guid, pGUID, sizeof(GUID));
    }

    if (RT_SUCCESS(rc))
        RTListAppend(pList, &pDev->Node);

    if (ppDev)
        *ppDev = pDev;

    return rc;
}


static void dsoundDeviceRemove(PDSOUNDDEV pDev)
{
    if (pDev)
    {
        if (pDev->pszName)
        {
            RTStrFree(pDev->pszName);
            pDev->pszName = NULL;
        }

        RTListNodeRemove(&pDev->Node);

        RTMemFree(pDev);
        pDev = NULL;
    }
}


static void dsoundLogDevice(const char *pszType, LPGUID pGUID, LPCWSTR pwszDescription, LPCWSTR pwszModule)
{
    char *pszGUID = dsoundGUIDToUtf8StrA(pGUID);
    /* This always has to be in the release log. */
    LogRel(("DSound: %s: GUID: %s [%ls] (Module: %ls)\n", pszType, pszGUID ? pszGUID : "{?}", pwszDescription, pwszModule));
    RTStrFree(pszGUID);
}


static BOOL CALLBACK dsoundDevicesEnumCbPlayback(LPGUID pGUID, LPCWSTR pwszDescription, LPCWSTR pwszModule, PVOID lpContext)
{
    PDSOUNDENUMCBCTX pCtx = (PDSOUNDENUMCBCTX)lpContext;
    AssertPtrReturn(pCtx, FALSE);
    AssertPtrReturn(pCtx->pDrv, FALSE);

    if (!pGUID)
        return TRUE;

    AssertPtrReturn(pwszDescription, FALSE);
    /* Do not care about pwszModule. */

    if (pCtx->fFlags & DSOUNDENUMCBFLAGS_LOG)
        dsoundLogDevice("Output", pGUID, pwszDescription, pwszModule);

    int rc = dsoundDevAdd(&pCtx->pDrv->lstDevOutput,
                          pGUID, pwszDescription, NULL /* ppDev */);
    if (RT_FAILURE(rc))
        return FALSE; /* Abort enumeration. */

    pCtx->cDevOut++;

    return TRUE;
}


static BOOL CALLBACK dsoundDevicesEnumCbCapture(LPGUID pGUID, LPCWSTR pwszDescription, LPCWSTR pwszModule, PVOID lpContext)
{
    PDSOUNDENUMCBCTX pCtx = (PDSOUNDENUMCBCTX)lpContext;
    AssertPtrReturn(pCtx, FALSE);
    AssertPtrReturn(pCtx->pDrv, FALSE);

    if (!pGUID)
        return TRUE;

    if (pCtx->fFlags & DSOUNDENUMCBFLAGS_LOG)
        dsoundLogDevice("Input", pGUID, pwszDescription, pwszModule);

    int rc = dsoundDevAdd(&pCtx->pDrv->lstDevInput,
                          pGUID, pwszDescription, NULL /* ppDev */);
    if (RT_FAILURE(rc))
        return FALSE; /* Abort enumeration. */

    pCtx->cDevIn++;

    return TRUE;
}


/**
 * Does a (Re-)enumeration of the host's playback + capturing devices.
 *
 * @return  IPRT status code.
 * @param   pThis               Host audio driver instance.
 * @param   pEnmCtx             Enumeration context to use.
 * @param   fEnum               Enumeration flags.
 */
static int dsoundDevicesEnumerate(PDRVHOSTDSOUND pThis, PDSOUNDENUMCBCTX pEnmCtx, uint32_t fEnum)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pEnmCtx, VERR_INVALID_POINTER);

    dsoundDevicesClear(pThis);

    RTLDRMOD hDSound = NULL;
    int rc = RTLdrLoadSystem("dsound.dll", true /*fNoUnload*/, &hDSound);
    if (RT_SUCCESS(rc))
    {
        PFNDIRECTSOUNDENUMERATEW pfnDirectSoundEnumerateW = NULL;
        PFNDIRECTSOUNDCAPTUREENUMERATEW pfnDirectSoundCaptureEnumerateW = NULL;

        rc = RTLdrGetSymbol(hDSound, "DirectSoundEnumerateW", (void**)&pfnDirectSoundEnumerateW);
        if (RT_SUCCESS(rc))
            rc = RTLdrGetSymbol(hDSound, "DirectSoundCaptureEnumerateW", (void**)&pfnDirectSoundCaptureEnumerateW);

        if (RT_SUCCESS(rc))
        {
            HRESULT hr = pfnDirectSoundEnumerateW(&dsoundDevicesEnumCbPlayback, pEnmCtx);
            if (FAILED(hr))
                LogRel2(("DSound: Error enumerating host playback devices: %Rhrc\n", hr));

            hr = pfnDirectSoundCaptureEnumerateW(&dsoundDevicesEnumCbCapture, pEnmCtx);
            if (FAILED(hr))
                LogRel2(("DSound: Error enumerating host capturing devices: %Rhrc\n", hr));

            if (fEnum & DSOUNDENUMCBFLAGS_LOG)
            {
                LogRel2(("DSound: Found %RU8 host playback devices\n",  pEnmCtx->cDevOut));
                LogRel2(("DSound: Found %RU8 host capturing devices\n", pEnmCtx->cDevIn));
            }
        }

        RTLdrClose(hDSound);
    }
    else
    {
        /* No dsound.dll on this system. */
        LogRel2(("DSound: Could not load dsound.dll: %Rrc\n", rc));
    }

    return rc;
}


/**
 * Updates this host driver's internal status, according to the global, overall input/output
 * state and all connected (native) audio streams.
 *
 * @param   pThis               Host audio driver instance.
 * @param   pCfg                Where to store the backend configuration. Optional.
 * @param   fEnum               Enumeration flags.
 */
static void dsoundUpdateStatusInternalEx(PDRVHOSTDSOUND pThis, PPDMAUDIOBACKENDCFG pCfg, uint32_t fEnum)
{
    AssertPtrReturnVoid(pThis);
    /* pCfg is optional. */

    PDMAUDIOBACKENDCFG Cfg;
    RT_ZERO(Cfg);

    Cfg.cbStreamOut = sizeof(DSOUNDSTREAM);
    Cfg.cbStreamIn  = sizeof(DSOUNDSTREAM);

    DSOUNDENUMCBCTX cbCtx = { pThis, fEnum, 0, 0 };

    int rc = dsoundDevicesEnumerate(pThis, &cbCtx, fEnum);
    if (RT_SUCCESS(rc))
    {
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
        if (   pThis->fEnabledOut != RT_BOOL(cbCtx.cDevOut)
            || pThis->fEnabledIn  != RT_BOOL(cbCtx.cDevIn))
        {
            /** @todo Use a registered callback to the audio connector (e.g "OnConfigurationChanged") to
             *        let the connector know that something has changed within the host backend. */
        }
#else
        pThis->fEnabledOut = RT_BOOL(cbCtx.cDevOut);
        pThis->fEnabledIn  = RT_BOOL(cbCtx.cDevIn);
#endif

        Cfg.cMaxStreamsIn  = UINT32_MAX;
        Cfg.cMaxStreamsOut = UINT32_MAX;

        if (pCfg)
            memcpy(pCfg, &Cfg, sizeof(PDMAUDIOBACKENDCFG));
    }

    LogFlowFuncLeaveRC(rc);
}


static void dsoundUpdateStatusInternal(PDRVHOSTDSOUND pThis)
{
    dsoundUpdateStatusInternalEx(pThis, NULL /* pCfg */, 0 /* fEnum */);
}


static int dsoundCreateStreamOut(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                 PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    LogFlowFunc(("pStreamDS=%p, pCfgReq=%p\n", pStreamDS, pCfgReq));

    pStreamDS->Out.pDSB = NULL;
    pStreamDS->Out.offWritePos = 0;
    pStreamDS->Out.offPlayCursorLastPlayed = 0;
    pStreamDS->Out.offPlayCursorLastPending = 0;
    pStreamDS->Out.cbWritten = 0;
    pStreamDS->Out.fRestartPlayback = true;
    pStreamDS->Out.cbBufSize = 0;

    int rc = VINF_SUCCESS;

    /* Try to open playback in case the device is already there. */
    HRESULT hr = directSoundPlayOpen(pThis, pStreamDS, pCfgReq, pCfgAcq);
    if (FAILED(hr))
        rc = VERR_GENERAL_FAILURE; /** @todo Fudge! */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static int dsoundControlStreamOut(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    LogFlowFunc(("pStreamDS=%p, cmd=%d\n", pStreamDS, enmStreamCmd));

    int rc = VINF_SUCCESS;

    HRESULT hr;
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
        {
            hr = directSoundPlayStart(pThis, pStreamDS);
            if (FAILED(hr))
            {
                hr = directSoundPlayClose(pThis, pStreamDS);
                if (SUCCEEDED(hr))
                {
                    PDMAUDIOSTREAMCFG CfgAcq;
                    hr = directSoundPlayOpen(pThis, pStreamDS, pStreamDS->pCfg /* pCfqReq */, &CfgAcq);
                    if (SUCCEEDED(hr))
                    {
                        DrvAudioHlpStreamCfgFree(pStreamDS->pCfg);

                        pStreamDS->pCfg = DrvAudioHlpStreamCfgDup(&CfgAcq);
                        AssertPtr(pStreamDS->pCfg);

                        /** @todo What to do if the format has changed? */
                    }
                }
                if (SUCCEEDED(hr))
                    hr = directSoundPlayStart(pThis, pStreamDS);
            }

            if (FAILED(hr))
                rc = VERR_NOT_SUPPORTED;
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            AssertPtr(pThis->pDS);

            hr = directSoundPlayStop(pThis, pStreamDS);
            if (FAILED(hr))
                rc = VERR_NOT_SUPPORTED;
            break;
        }

        default:
        {
            AssertMsgFailed(("Invalid command: %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
int drvHostDSoundStreamPlay(PPDMIHOSTAUDIO pInterface,
                            PPDMAUDIOBACKENDSTREAM pStream, const void *pvBuf, uint32_t cxBuf, uint32_t *pcxWritten)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);
    /* pcxWritten is optional. */

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    int rc = VINF_SUCCESS;

    uint32_t cbWrittenTotal = 0;

#ifdef DEBUG_andy
    LogFlowFuncEnter();
#endif

    do /* to use 'break' */
    {
        AssertPtr(pStreamDS->pCfg);
        PPDMAUDIOPCMPROPS pProps = &pStreamDS->pCfg->Props;

        DWORD cbFree;
        rc = dsoundGetFreeOut(pThis, pStreamDS, &cbFree);
        if (RT_FAILURE(rc))
            break;

        if (pStreamDS->Out.fRestartPlayback == false)
        {
            DWORD offPlayCursor, offWriteCursor;
            HRESULT hr = IDirectSoundBuffer8_GetCurrentPosition(pStreamDS->Out.pDSB, &offPlayCursor, &offWriteCursor);
            if (SUCCEEDED(hr))
            {
                uint32_t cbPending;
                if (pStreamDS->Out.offPlayCursorLastPlayed <= offPlayCursor)
                    cbPending = offPlayCursor - pStreamDS->Out.offPlayCursorLastPlayed;
                else
                    cbPending = pStreamDS->Out.cbBufSize - pStreamDS->Out.offPlayCursorLastPlayed + offPlayCursor;

                pStreamDS->Out.cbWritten               -= RT_MIN(pStreamDS->Out.cbWritten, cbPending);
                pStreamDS->Out.offPlayCursorLastPlayed  = offPlayCursor;
            }
        }

        /*
         * Check for full buffer, do not allow the offPlayWritePos to catch cbPlayPos during playback,
         * i.e. always leave a free space for 1 audio sample.
         */
        const DWORD cbSample = PDMAUDIOPCMPROPS_F2B(pProps, 1);
        if (cbFree < cbSample)
            break;
        Assert(cbFree >= cbSample);
        cbFree     -= cbSample;

        uint32_t cbLive = cxBuf;

        /* Do not write more than available space in the DirectSound playback buffer. */
        cbLive  = RT_MIN(cbFree, cbLive);
        cbLive &= ~pStreamDS->uAlign;

        if (!cbLive)
            break;

        LPDIRECTSOUNDBUFFER8 pDSB = pStreamDS->Out.pDSB;
        AssertPtr(pDSB);

        PVOID pv1, pv2;
        DWORD cb1, cb2;
        HRESULT hr = directSoundPlayLock(pThis, pStreamDS, pStreamDS->Out.offWritePos, cbLive,
                                         &pv1, &pv2, &cb1, &cb2, 0 /* dwFlags */);
        if (FAILED(hr))
        {
            rc = VERR_ACCESS_DENIED;
            break;
        }

        AssertPtr(pv1);
        Assert(cb1);

        memcpy(pv1, pvBuf, cb1);
        cbWrittenTotal = cb1;

        if (pv2 && cb2) /* Buffer wrap-around? Write second part. */
        {
            memcpy(pv2, (uint8_t *)pvBuf + cb1, cb2);
            cbWrittenTotal += cb2;
        }

        Assert(cbLive == cb1 + cb2);

        directSoundPlayUnlock(pThis, pDSB, pv1, pv2, cb1, cb2);

        pStreamDS->Out.offWritePos = (pStreamDS->Out.offWritePos + cbWrittenTotal) % pStreamDS->Out.cbBufSize;
        pStreamDS->Out.cbWritten  += cbWrittenTotal;

        DSLOGF(("DSound: %RU32/%RU32, buffer write pos %ld, rc=%Rrc\n",
                cbWrittenTotal, cbLive, pStreamDS->Out.offWritePos, rc));

        if (pStreamDS->Out.fRestartPlayback)
        {
            /*
             * The playback has been just started.
             * Some samples of the new sound have been copied to the buffer
             * and it can start playing.
             */
            pStreamDS->Out.fRestartPlayback = false;

            DWORD fFlags = 0;
#ifndef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
            fFlags |= DSCBSTART_LOOPING;
#endif
            for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
            {
                hr = IDirectSoundBuffer8_Play(pStreamDS->Out.pDSB, 0, 0, fFlags);
                if (   SUCCEEDED(hr)
                    || hr != DSERR_BUFFERLOST)
                    break;
                else
                {
                    LogFlowFunc(("Restarting playback failed due to lost buffer, restoring ...\n"));
                    directSoundPlayRestore(pThis, pStreamDS->Out.pDSB);
                }
            }

            if (FAILED(hr))
            {
                DSLOGREL(("DSound: Starting playback failed with %Rhrc\n", hr));
                rc = VERR_NOT_SUPPORTED;
                break;
            }
        }

    } while (0);

    if (RT_SUCCESS(rc))
    {
        if (pcxWritten)
            *pcxWritten = cbWrittenTotal;
    }
    else
        dsoundUpdateStatusInternal(pThis);

    return rc;
}


static int dsoundDestroyStreamOut(PDRVHOSTDSOUND pThis, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDSOUNDSTREAM pStreamDS = (PDSOUNDSTREAM)pStream;

    directSoundPlayClose(pThis, pStreamDS);

    return VINF_SUCCESS;
}

static int dsoundCreateStreamIn(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    LogFunc(("pStreamDS=%p, pCfgReq=%p, enmRecSource=%s\n",
             pStreamDS, pCfgReq, DrvAudioHlpRecSrcToStr(pCfgReq->DestSource.Source)));

    /* Init the stream structure and save relevant information to it. */
    pStreamDS->In.offReadPos    = 0;
    pStreamDS->In.cbBufSize     = 0;
    pStreamDS->In.pDSCB         = NULL;
    pStreamDS->In.hrLastCapture = S_OK;

    int rc = VINF_SUCCESS;

    /* Try to open capture in case the device is already there. */
    HRESULT hr = directSoundCaptureOpen(pThis, pStreamDS, pCfgReq, pCfgAcq);
    if (FAILED(hr))
        rc = VERR_GENERAL_FAILURE; /** @todo Fudge! */

    return rc;
}

static int dsoundControlStreamIn(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    LogFlowFunc(("pStreamDS=%p, enmStreamCmd=%ld\n", pStreamDS, enmStreamCmd));

    int rc = VINF_SUCCESS;

    HRESULT hr;
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        case PDMAUDIOSTREAMCMD_RESUME:
        {
            /* Try to start capture. If it fails, then reopen and try again. */
            hr = directSoundCaptureStart(pThis, pStreamDS);
            if (FAILED(hr))
            {
                hr = directSoundCaptureClose(pStreamDS);
                if (SUCCEEDED(hr))
                {
                    PDMAUDIOSTREAMCFG CfgAcq;
                    hr = directSoundCaptureOpen(pThis, pStreamDS, pStreamDS->pCfg /* pCfgReq */, &CfgAcq);
                    if (SUCCEEDED(hr))
                    {
                        DrvAudioHlpStreamCfgFree(pStreamDS->pCfg);

                        pStreamDS->pCfg = DrvAudioHlpStreamCfgDup(&CfgAcq);
                        AssertPtr(pStreamDS->pCfg);

                        /** @todo What to do if the format has changed? */

                        hr = directSoundCaptureStart(pThis, pStreamDS);
                    }
                }
            }

            if (FAILED(hr))
                rc = VERR_NOT_SUPPORTED;
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            AssertPtr(pThis->pDSC);

            directSoundCaptureStop(pStreamDS);

            /* Return success in any case, as stopping the capture can fail if
             * the capture buffer is not around anymore.
             *
             * This can happen if the host's capturing device has been changed suddenly. */
            rc = VINF_SUCCESS;
            break;
        }

        default:
        {
            AssertMsgFailed(("Invalid command %ld\n", enmStreamCmd));
            rc = VERR_INVALID_PARAMETER;
            break;
        }
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
int drvHostDSoundStreamCapture(PPDMIHOSTAUDIO pInterface,
                               PPDMAUDIOBACKENDSTREAM pStream, void *pvBuf, uint32_t cxBuf, uint32_t *pcxRead)
{

    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    AssertReturn(cxBuf,         VERR_INVALID_PARAMETER);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB = pStreamDS->In.pDSCB;
    AssertPtr(pDSCB);

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;

    do
    {
        if (pDSCB == NULL)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        /* Get DirectSound capture position in bytes. */
        DWORD offCurPos;
        HRESULT hr = IDirectSoundCaptureBuffer_GetCurrentPosition(pDSCB, NULL, &offCurPos);
        if (FAILED(hr))
        {
            if (hr != pStreamDS->In.hrLastCapture)
            {
                DSLOGREL(("DSound: Getting capture position failed with %Rhrc\n", hr));
                pStreamDS->In.hrLastCapture = hr;
            }

            rc = VERR_NOT_AVAILABLE;
            break;
        }

        pStreamDS->In.hrLastCapture = hr;

        if (offCurPos & pStreamDS->uAlign)
            DSLOGF(("DSound: Misaligned capture read position %ld (alignment: %RU32)\n",
                    offCurPos, pStreamDS->uAlign + 1));

        /* Number of samples available in the DirectSound capture buffer. */
        DWORD cbToCapture = dsoundRingDistance(offCurPos, pStreamDS->In.offReadPos, pStreamDS->In.cbBufSize);
        if (cbToCapture == 0)
            break;

        if (cxBuf == 0)
        {
            DSLOGF(("DSound: Capture buffer full\n"));
            break;
        }

        DSLOGF(("DSound: Capture cxBuf=%RU32, offCurPos=%ld, offReadPos=%ld, cbToCapture=%ld\n",
                cxBuf, offCurPos, pStreamDS->In.offReadPos, cbToCapture));

        /* No need to fetch more samples than mix buffer can receive. */
        cbToCapture = RT_MIN(cbToCapture, cxBuf);

        /* Lock relevant range in the DirectSound capture buffer. */
        PVOID pv1, pv2;
        DWORD cb1, cb2;
        hr = directSoundCaptureLock(pStreamDS,
                                    pStreamDS->In.offReadPos,  /* dwOffset */
                                    cbToCapture,               /* dwBytes */
                                    &pv1, &pv2, &cb1, &cb2,
                                    0                          /* dwFlags */);
        if (FAILED(hr))
        {
            rc = VERR_ACCESS_DENIED;
            break;
        }

        if (pv1 && cb1)
        {
            memcpy((uint8_t *)pvBuf + cbReadTotal, pv1, cb1);
            cbReadTotal += cb1;
        }

        if (pv2 && cb2)
        {
            memcpy((uint8_t *)pvBuf + cbReadTotal, pv2, cb2);
            cbReadTotal += cb2;
        }

        directSoundCaptureUnlock(pDSCB, pv1, pv2, cb1, cb2);

        if (RT_SUCCESS(rc))
        {
            pStreamDS->In.offReadPos = (pStreamDS->In.offReadPos + cbReadTotal)
                                     % pStreamDS->In.cbBufSize;
            DSLOGF(("DSound: Captured %ld bytes (%RU32 total)\n", cbToCapture, cbReadTotal));
        }

    } while (0);

    if (RT_SUCCESS(rc))
    {
        if (pcxRead)
            *pcxRead = cbReadTotal;
    }
    else
        dsoundUpdateStatusInternal(pThis);

    return rc;
}

static int dsoundDestroyStreamIn(PDSOUNDSTREAM pStreamDS)
{
    directSoundCaptureClose(pStreamDS);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
int drvHostDSoundGetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);

    PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);

    dsoundUpdateStatusInternalEx(pThis, pBackendCfg, 0 /* fEnum */);

    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS

static int dsoundNotifyThread(PDRVHOSTDSOUND pThis, bool fShutdown)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    if (fShutdown)
    {
        LogFlowFunc(("Shutting down thread ...\n"));
        pThis->fShutdown = fShutdown;
    }

    /* Set the notification event so that the thread is being notified. */
    BOOL fRc = SetEvent(pThis->aEvents[DSOUNDEVENT_NOTIFY]);
    Assert(fRc);

    return VINF_SUCCESS;
}


static DECLCALLBACK(int) dsoundNotificationThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PDRVHOSTDSOUND pThis = (PDRVHOSTDSOUND)pvUser;
    AssertPtr(pThis);

    LogFlowFuncEnter();

    /* Let caller know that we're done initializing, regardless of the result. */
    int rc = RTThreadUserSignal(hThreadSelf);
    AssertRC(rc);

    do
    {
        HANDLE aEvents[VBOX_DSOUND_MAX_EVENTS];
        DWORD  cEvents = 0;
        for (uint8_t i = 0; i < VBOX_DSOUND_MAX_EVENTS; i++)
        {
            if (pThis->aEvents[i])
                aEvents[cEvents++] = pThis->aEvents[i];
        }
        Assert(cEvents);

        LogFlowFunc(("Waiting: cEvents=%ld\n", cEvents));

        DWORD dwObj = WaitForMultipleObjects(cEvents, aEvents, FALSE /* bWaitAll */, INFINITE);
        switch (dwObj)
        {
            case WAIT_FAILED:
            {
                rc = VERR_CANCELLED;
                break;
            }

            case WAIT_TIMEOUT:
            {
                rc = VERR_TIMEOUT;
                break;
            }

            default:
            {
                dwObj = WAIT_OBJECT_0 + cEvents - 1;
                if (aEvents[dwObj] == pThis->aEvents[DSOUNDEVENT_NOTIFY])
                {
                    LogFlowFunc(("Notify\n"));
                }
                else if (aEvents[dwObj] == pThis->aEvents[DSOUNDEVENT_INPUT])
                {

                }
                else if (aEvents[dwObj] == pThis->aEvents[DSOUNDEVENT_OUTPUT])
                {
                    DWORD cbFree;
                    rc = dsoundGetFreeOut(pThis->pDSStream, &cbFree);
                    if (   RT_SUCCESS(rc)
                        && cbFree)
                    {
                        PDMAUDIOCBDATA_DATA_OUTPUT Out;
                        Out.cbInFree     = cbFree;
                        Out.cbOutWritten = 0;

                        while (!Out.cbOutWritten)
                        {
                            rc = pThis->pUpIAudioConnector->pfnCallback(pThis->pUpIAudioConnector,
                                                                        PDMAUDIOCALLBACKTYPE_OUTPUT, &Out, sizeof(Out));
                            if (RT_FAILURE(rc))
                                break;
                            RTThreadSleep(100);
                        }

                        LogFlowFunc(("Output: cbBuffer=%ld, cbFree=%ld, cbWritten=%RU32, rc=%Rrc\n",
                                     cbBuffer, cbFree, Out.cbOutWritten, rc));
                    }
                }
                break;
            }
        }

        if (pThis->fShutdown)
            break;

    } while (RT_SUCCESS(rc));

    pThis->fStopped = true;

    LogFlowFunc(("Exited with fShutdown=%RTbool, rc=%Rrc\n", pThis->fShutdown, rc));
    return rc;
}

#endif /* VBOX_WITH_AUDIO_DEVICE_CALLBACKS */


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
void drvHostDSoundShutdown(PPDMIHOSTAUDIO pInterface)
{
    PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);

    LogFlowFuncEnter();

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    int rc = dsoundNotifyThread(pThis, true /* fShutdown */);
    AssertRC(rc);

    int rcThread;
    rc = RTThreadWait(pThis->Thread,  15 * 1000 /* 15s timeout */, &rcThread);
    LogFlowFunc(("rc=%Rrc, rcThread=%Rrc\n", rc, rcThread));

    Assert(pThis->fStopped);

    if (pThis->aEvents[DSOUNDEVENT_NOTIFY])
    {
        CloseHandle(pThis->aEvents[DSOUNDEVENT_NOTIFY]);
        pThis->aEvents[DSOUNDEVENT_NOTIFY] = NULL;
    }
#else
    RT_NOREF_PV(pThis);
#endif

    LogFlowFuncLeave();
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnInit}
 */
static DECLCALLBACK(int) drvHostDSoundInit(PPDMIHOSTAUDIO pInterface)
{
    PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    LogFlowFuncEnter();

    int rc;

    /* Verify that IDirectSound is available. */
    LPDIRECTSOUND pDirectSound = NULL;
    HRESULT hr = CoCreateInstance(CLSID_DirectSound, NULL, CLSCTX_ALL, IID_IDirectSound, (void **)&pDirectSound);
    if (SUCCEEDED(hr))
    {
        IDirectSound_Release(pDirectSound);

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
        /* Create notification event. */
        pThis->aEvents[DSOUNDEVENT_NOTIFY] = CreateEvent(NULL /* Security attribute */,
                                                         FALSE /* bManualReset */, FALSE /* bInitialState */,
                                                         NULL /* lpName */);
        Assert(pThis->aEvents[DSOUNDEVENT_NOTIFY] != NULL);

        /* Start notification thread. */
        rc = RTThreadCreate(&pThis->Thread, dsoundNotificationThread,
                            pThis /*pvUser*/, 0 /*cbStack*/,
                            RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "dsoundNtfy");
        if (RT_SUCCESS(rc))
        {
            /* Wait for the thread to initialize. */
            rc = RTThreadUserWait(pThis->Thread, RT_MS_1MIN);
            if (RT_FAILURE(rc))
                DSLOGREL(("DSound: Waiting for thread to initialize failed with rc=%Rrc\n", rc));
        }
    else
            DSLOGREL(("DSound: Creating thread failed with rc=%Rrc\n", rc));
#else
        rc = VINF_SUCCESS;
#endif

        dsoundUpdateStatusInternalEx(pThis, NULL /* pCfg */, DSOUNDENUMCBFLAGS_LOG /* fEnum */);
    }
    else
    {
        DSLOGREL(("DSound: DirectSound not available: %Rhrc\n", hr));
        rc = VERR_NOT_SUPPORTED;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}


static LPCGUID dsoundConfigQueryGUID(PCFGMNODE pCfg, const char *pszName, RTUUID *pUuid)
{
    LPCGUID pGuid = NULL;

    char *pszGuid = NULL;
    int rc = CFGMR3QueryStringAlloc(pCfg, pszName, &pszGuid);
    if (RT_SUCCESS(rc))
    {
        rc = RTUuidFromStr(pUuid, pszGuid);
        if (RT_SUCCESS(rc))
            pGuid = (LPCGUID)&pUuid;
        else
            DSLOGREL(("DSound: Error parsing device GUID for device '%s': %Rrc\n", pszName, rc));

        RTStrFree(pszGuid);
    }

    return pGuid;
}


static int dsoundConfigInit(PDRVHOSTDSOUND pThis, PCFGMNODE pCfg)
{
    unsigned int uBufsizeOut, uBufsizeIn;

    CFGMR3QueryUIntDef(pCfg, "BufsizeOut", &uBufsizeOut, _16K);
    CFGMR3QueryUIntDef(pCfg, "BufsizeIn",  &uBufsizeIn,  _16K);
    pThis->cfg.cbBufferOut = uBufsizeOut;
    pThis->cfg.cbBufferIn  = uBufsizeIn;

    pThis->cfg.pGuidPlay    = dsoundConfigQueryGUID(pCfg, "DeviceGuidOut", &pThis->cfg.uuidPlay);
    pThis->cfg.pGuidCapture = dsoundConfigQueryGUID(pCfg, "DeviceGuidIn",  &pThis->cfg.uuidCapture);

    DSLOG(("DSound: Configuration cbBufferIn=%ld, cbBufferOut=%ld, DeviceGuidOut {%RTuuid}, DeviceGuidIn {%RTuuid}\n",
           pThis->cfg.cbBufferIn,
           pThis->cfg.cbBufferOut,
           &pThis->cfg.uuidPlay,
           &pThis->cfg.uuidCapture));

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostDSoundGetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(enmDir);
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostDSoundStreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                   PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq,    VERR_INVALID_POINTER);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    int rc;
    if (pCfgReq->enmDir == PDMAUDIODIR_IN)
        rc = dsoundCreateStreamIn(pThis,  pStreamDS, pCfgReq, pCfgAcq);
    else
        rc = dsoundCreateStreamOut(pThis, pStreamDS, pCfgReq, pCfgAcq);

    if (RT_SUCCESS(rc))
    {
        pStreamDS->pCfg = DrvAudioHlpStreamCfgDup(pCfgAcq);
        if (!pStreamDS->pCfg)
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostDSoundStreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (!pStreamDS->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamDS->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = dsoundDestroyStreamIn(pStreamDS);
    else
        rc = dsoundDestroyStreamOut(pThis, pStreamDS);

    if (RT_SUCCESS(rc))
    {
        DrvAudioHlpStreamCfgFree(pStreamDS->pCfg);
        pStreamDS->pCfg = NULL;
    }

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostDSoundStreamControl(PPDMIHOSTAUDIO pInterface,
                                                    PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (!pStreamDS->pCfg) /* Not (yet) configured? Skip. */
        return VINF_SUCCESS;

    int rc;
    if (pStreamDS->pCfg->enmDir == PDMAUDIODIR_IN)
        rc = dsoundControlStreamIn(pThis,  pStreamDS, enmStreamCmd);
    else
        rc = dsoundControlStreamOut(pThis, pStreamDS, enmStreamCmd);

    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostDSoundStreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, PDMAUDIOSTREAMSTS_FLAG_NONE);

    PDSOUNDSTREAM pStreamDS = (PDSOUNDSTREAM)pStream;

    if (pStreamDS->fEnabled)
        return UINT32_MAX;

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostDSoundStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, PDMAUDIOSTREAMSTS_FLAG_NONE);
    AssertPtrReturn(pStream,    PDMAUDIOSTREAMSTS_FLAG_NONE);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (pStreamDS->fEnabled)
    {
        DWORD cbFree;
        int rc = dsoundGetFreeOut(pThis, pStreamDS, &cbFree);
        if (   RT_SUCCESS(rc)
            && cbFree)
        {
            Log3Func(("cbFree=%ld\n", cbFree));
            return (uint32_t)cbFree;
        }
    }

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetPending}
 */
static DECLCALLBACK(uint32_t) drvHostDSoundStreamGetPending(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, 0);

    PDRVHOSTDSOUND pThis     = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);
    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (pStreamDS->pCfg->enmDir == PDMAUDIODIR_OUT)
    {
        DWORD dwStatus;
        HRESULT hr = directSoundPlayGetStatus(pThis, pStreamDS->Out.pDSB, &dwStatus);
        if (hr != DS_OK)
            return 0;

         if (!(dwStatus & DSBSTATUS_PLAYING))
            return 0;

         DWORD offPlayCursor, offWriteCursor;
         hr = IDirectSoundBuffer8_GetCurrentPosition(pStreamDS->Out.pDSB, &offPlayCursor, &offWriteCursor);
         if (SUCCEEDED(hr))
         {
             uint32_t cbPending;
             if (pStreamDS->Out.offPlayCursorLastPending <= offPlayCursor)
                 cbPending = offPlayCursor - pStreamDS->Out.offPlayCursorLastPending;
             else
                 cbPending = pStreamDS->Out.cbBufSize - pStreamDS->Out.offPlayCursorLastPending + offPlayCursor;

             pStreamDS->Out.cbWritten                -= RT_MIN(pStreamDS->Out.cbWritten, cbPending);
             pStreamDS->Out.offPlayCursorLastPending  = offPlayCursor;

            LogFunc(("offPlayCursor=%RU32, offWriteCursor=%RU32\n", offPlayCursor, offWriteCursor));
            LogFunc(("offPlayWritePos=%RU32, cbWritten=%RU64, cbPending=%RU32\n",
                     pStreamDS->Out.offWritePos, pStreamDS->Out.cbWritten, cbPending));

            /*
             * As we operate a DirectSound secondary *streaming* buffer which loops over
             * the data repeatedly until stopped, we have to make at least an estimate when we're actually
             * done playing the written data on the host.
             */
            return pStreamDS->Out.cbWritten;
        }
        else
            LogFunc(("Failed with %Rhrc\n", hr));
    }
    /* Note: For input streams we never have pending data left. */

    return 0;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTREAMSTS) drvHostDSoundStreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, PDMAUDIOSTREAMSTS_FLAG_NONE);

    PDSOUNDSTREAM pStreamDS = (PDSOUNDSTREAM)pStream;

    if (!pStreamDS->pCfg) /* Not (yet) configured? Skip. */
        return PDMAUDIOSTREAMSTS_FLAG_NONE;

    PDMAUDIOSTREAMSTS strmSts = PDMAUDIOSTREAMSTS_FLAG_INITIALIZED;
    if (pStreamDS->pCfg->enmDir == PDMAUDIODIR_IN)
    {
        if (pStreamDS->fEnabled)
            strmSts |= PDMAUDIOSTREAMSTS_FLAG_ENABLED;
    }
    else
    {
        if (pStreamDS->fEnabled)
            strmSts |= PDMAUDIOSTREAMSTS_FLAG_ENABLED;
    }

    return strmSts;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvHostDSoundStreamIterate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    /* Nothing to do here for DSound. */
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnSetCallback}
 */
static DECLCALLBACK(int) drvHostDSoundSetCallback(PPDMIHOSTAUDIO pInterface, PFNPDMHOSTAUDIOCALLBACK pfnCallback)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    /* pfnCallback will be handled below. */

    PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_SUCCESS(rc))
    {
        LogFunc(("pfnCallback=%p\n", pfnCallback));

        if (pfnCallback) /* Register. */
        {
            Assert(pThis->pfnCallback == NULL);
            pThis->pfnCallback = pfnCallback;

#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
            if (pThis->m_pNotificationClient)
                pThis->m_pNotificationClient->RegisterCallback(pThis->pDrvIns, pfnCallback);
#endif
        }
        else /* Unregister. */
        {
            if (pThis->pfnCallback)
                pThis->pfnCallback = NULL;

#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
            if (pThis->m_pNotificationClient)
                pThis->m_pNotificationClient->UnregisterCallback();
#endif
        }

        int rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);
    }

    return rc;
}
#endif


/*********************************************************************************************************************************
*   PDMDRVINS::IBase Interface                                                                                                   *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostDSoundQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS     pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTDSOUND pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTDSOUND);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


/*********************************************************************************************************************************
*   PDMDRVREG Interface                                                                                                          *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNPDMDRVDESTRUCT, pfnDestruct}
 */
static DECLCALLBACK(void) drvHostDSoundDestruct(PPDMDRVINS pDrvIns)
{
    PDRVHOSTDSOUND pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTDSOUND);
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    LogFlowFuncEnter();

#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
    if (pThis->m_pNotificationClient)
    {
        pThis->m_pNotificationClient->Dispose();
        pThis->m_pNotificationClient->Release();

        pThis->m_pNotificationClient = NULL;
    }
#endif

    if (pThis->pDrvIns)
        CoUninitialize();

    int rc2 = RTCritSectDelete(&pThis->CritSect);
    AssertRC(rc2);

    LogFlowFuncLeave();
}


/**
 * @callback_method_impl{FNPDMDRVCONSTRUCT,
 *      Construct a DirectSound Audio driver instance.}
 */
static DECLCALLBACK(int) drvHostDSoundConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDRVHOSTDSOUND pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTDSOUND);

    LogRel(("Audio: Initializing DirectSound audio driver\n"));

    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DSLOGREL(("DSound: CoInitializeEx failed with %Rhrc\n", hr));
        return VERR_NOT_SUPPORTED;
    }

    /*
     * Init basic data members and interfaces.
     */
    pThis->pDrvIns                   = pDrvIns;
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface = drvHostDSoundQueryInterface;
    /* IHostAudio */
    PDMAUDIO_IHOSTAUDIO_CALLBACKS(drvHostDSound);
    pThis->IHostAudio.pfnStreamGetPending  = drvHostDSoundStreamGetPending;

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /* This backend supports host audio callbacks. */
    pThis->IHostAudio.pfnSetCallback       = drvHostDSoundSetCallback;
    pThis->pfnCallback               = NULL;
#endif

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    /*
     * Get the IAudioConnector interface of the above driver/device.
     */
    pThis->pUpIAudioConnector = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIAUDIOCONNECTOR);
    if (!pThis->pUpIAudioConnector)
    {
        AssertMsgFailed(("Configuration error: No audio connector interface above!\n"));
        return VERR_PDM_MISSING_INTERFACE_ABOVE;
    }
#endif

    /*
     * Init the static parts.
     */
    RTListInit(&pThis->lstDevInput);
    RTListInit(&pThis->lstDevOutput);

    pThis->fEnabledIn  = false;
    pThis->fEnabledOut = false;
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    pThis->fStopped    = false;
    pThis->fShutdown   = false;

    RT_ZERO(pThis->aEvents);
    pThis->cEvents = 0;
#endif

    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
    try
    {
        pThis->m_pNotificationClient = new VBoxMMNotificationClient();

        HRESULT hr = pThis->m_pNotificationClient->Initialize();
        if (FAILED(hr))
            rc = VERR_AUDIO_BACKEND_INIT_FAILED;
    }
    catch (std::bad_alloc &ex)
    {
        NOREF(ex);
        rc = VERR_NO_MEMORY;
    }
#endif

    if (RT_SUCCESS(rc))
    {
        /*
         * Initialize configuration values.
         */
        rc = dsoundConfigInit(pThis, pCfg);
        if (RT_SUCCESS(rc))
            rc = RTCritSectInit(&pThis->CritSect);
    }

    return rc;
}


/**
 * PDM driver registration.
 */
const PDMDRVREG g_DrvHostDSound =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "DSoundAudio",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "DirectSound Audio host driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTDSOUND),
    /* pfnConstruct */
    drvHostDSoundConstruct,
    /* pfnDestruct */
    drvHostDSoundDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

