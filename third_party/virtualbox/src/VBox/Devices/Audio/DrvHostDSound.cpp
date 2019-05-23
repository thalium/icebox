/* $Id: DrvHostDSound.cpp $ */
/** @file
 * Windows host backend driver using DirectSound.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
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
#include <iprt/system.h>
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
/** Default input latency (in ms). */
#define DRV_DSOUND_DEFAULT_LATENCY_MS_IN        50
/** Default output latency (in ms). */
#define DRV_DSOUND_DEFAULT_LATENCY_MS_OUT       50

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

#define VBOX_DSOUND_MAX_EVENTS 3

typedef enum DSOUNDEVENT
{
    DSOUNDEVENT_NOTIFY = 0,
    DSOUNDEVENT_INPUT,
    DSOUNDEVENT_OUTPUT,
} DSOUNDEVENT;

typedef struct DSOUNDHOSTCFG
{
    unsigned int    msLatencyIn;
    unsigned int    msLatencyOut;
    RTUUID          uuidPlay;
    LPCGUID         pGuidPlay;
    RTUUID          uuidCapture;
    LPCGUID         pGuidCapture;
} DSOUNDHOSTCFG, *PDSOUNDHOSTCFG;

typedef struct DSOUNDSTREAM
{
    /** The stream's acquired configuration. */
    PPDMAUDIOSTREAMCFG pCfg;
    /** Buffer alignment. */
    uint8_t            uAlign;
    /** Whether this stream is in an enable state on the DirectSound side. */
    bool               fEnabled;
    /** The stream's critical section for synchronizing access. */
    RTCRITSECT         CritSect;
    /** The internal playback / capturing buffer. */
    PRTCIRCBUF         pCircBuf;
    /** Size (in bytes) of the DirectSound buffer.
     *  Note: This in *not* the size of the circular buffer above! */
    DWORD              cbBufSize;
    union
    {
        struct
        {
            /** The actual DirectSound Buffer (DSB) used for the capturing.
             *  This is a secondary buffer and is used as a streaming buffer. */
            LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB;
            /** Current read offset (in bytes) within the DSB. */
            DWORD                       offReadPos;
            /** Number of buffer overruns happened. Used for logging. */
            uint8_t                     cOverruns;
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
            /** Total amount (in bytes) played (to the DirectSound buffer). */
            uint64_t                    cbPlayed;
            /** Flag indicating whether playback was (re)started. */
            bool                        fFirstPlayback;
            /** Timestamp (in ms) of When the last playback has happened. */
            uint64_t                    tsLastPlayMs;
            /** Number of buffer underruns happened. Used for logging. */
            uint8_t                     cUnderruns;
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
    DSOUNDHOSTCFG               Cfg;
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
    /** Stopped indicator. */
    bool                        fStopped;
    /** Shutdown indicator. */
    bool                        fShutdown;
    /** Notification thread. */
    RTTHREAD                    Thread;
    /** Array of events to wait for in notification thread. */
    HANDLE                      aEvents[VBOX_DSOUND_MAX_EVENTS];
    /** Pointer to the input stream. */
    PDSOUNDSTREAM               pDSStrmIn;
    /** Pointer to the output stream. */
    PDSOUNDSTREAM               pDSStrmOut;
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
static HRESULT  directSoundPlayStop(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, bool fFlush);
static HRESULT  directSoundCaptureStop(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, bool fFlush);

static void     dsoundDeviceRemove(PDSOUNDDEV pDev);
static int      dsoundDevicesEnumerate(PDRVHOSTDSOUND pThis, PPDMAUDIOBACKENDCFG pCfg);

static int      dsoundNotifyThread(PDRVHOSTDSOUND pThis, bool fShutdown);

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
    pFmt->wBitsPerSample  = pCfg->Props.cBits;
    pFmt->nSamplesPerSec  = pCfg->Props.uHz;
    pFmt->nBlockAlign     = pFmt->nChannels * pFmt->wBitsPerSample / 8;
    pFmt->nAvgBytesPerSec = pFmt->nSamplesPerSec * pFmt->nBlockAlign;
    pFmt->cbSize          = 0; /* No extra data specified. */

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
        DWORD cbPlayCursor, cbWriteCursor;
        hr = IDirectSoundBuffer8_GetCurrentPosition(pDSB, &cbPlayCursor, &cbWriteCursor);
        if (SUCCEEDED(hr))
        {
            int32_t cbDiff = cbWriteCursor - cbPlayCursor;
            if (cbDiff < 0)
                cbDiff += pStreamDS->cbBufSize;

            int32_t cbFree = cbPlayCursor - pStreamDS->Out.offWritePos;
            if (cbFree < 0)
                cbFree += pStreamDS->cbBufSize;

            if (cbFree > (int32_t)pStreamDS->cbBufSize - cbDiff)
            {
                pStreamDS->Out.offWritePos = cbWriteCursor;
                cbFree = pStreamDS->cbBufSize - cbDiff;
            }

            /* When starting to use a DirectSound buffer, cbPlayCursor and cbWriteCursor
             * both point at position 0, so we won't be able to detect how many bytes
             * are writable that way.
             *
             * So use our per-stream written indicator to see if we just started a stream. */
            if (pStreamDS->Out.cbWritten == 0)
                cbFree = pStreamDS->cbBufSize;

            DSLOGREL(("DSound: cbPlayCursor=%RU32, cbWriteCursor=%RU32, offWritePos=%RU32 -> cbFree=%RI32\n",
                      cbPlayCursor, cbWriteCursor, pStreamDS->Out.offWritePos, cbFree));

            *pdwFree = cbFree;

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
        DSLOG(("DSound: Restoring playback buffer\n"));
    else
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
    AssertReturn(dwBytes, VERR_INVALID_PARAMETER);

    HRESULT hr = E_FAIL;
    AssertCompile(DRV_DSOUND_RESTORE_ATTEMPTS_MAX > 0);
    for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
    {
        PVOID pv1, pv2;
        DWORD cb1, cb2;
        hr = IDirectSoundBuffer8_Lock(pStreamDS->Out.pDSB, dwOffset, dwBytes, &pv1, &cb1, &pv2, &cb2, dwFlags);
        if (SUCCEEDED(hr))
        {
            if (   (!pv1 || !(cb1 & pStreamDS->uAlign))
                && (!pv2 || !(cb2 & pStreamDS->uAlign)))
            {
                if (ppv1)
                    *ppv1 = pv1;
                if (ppv2)
                    *ppv2 = pv2;
                if (pcb1)
                    *pcb1 = cb1;
                if (pcb2)
                    *pcb2 = cb2;
                return S_OK;
            }
            DSLOGREL(("DSound: Locking playback buffer returned misaligned buffer: cb1=%#RX32, cb2=%#RX32 (alignment: %#RX32)\n",
                      *pcb1, *pcb2, pStreamDS->uAlign));
            directSoundPlayUnlock(pThis, pStreamDS->Out.pDSB, pv1, pv2, cb1, cb2);
            return E_FAIL;
        }

        if (hr != DSERR_BUFFERLOST)
            break;

        LogFlowFunc(("Locking failed due to lost buffer, restoring ...\n"));
        directSoundPlayRestore(pThis, pStreamDS->Out.pDSB);
    }

    DSLOGREL(("DSound: Locking playback buffer failed with %Rhrc (dwOff=%ld, dwBytes=%ld)\n", hr, dwOffset, dwBytes));
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
        hr = IDirectSound8_Initialize(pThis->pDS, pThis->Cfg.pGuidPlay);
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

    LogFlowFuncEnter();

    HRESULT hr = directSoundPlayStop(pThis, pStreamDS, true /* fFlush */);
    if (FAILED(hr))
        return hr;

    DSLOG(("DSound: Closing playback stream\n"));

    if (pStreamDS->pCircBuf)
        Assert(RTCircBufUsed(pStreamDS->pCircBuf) == 0);

    if (SUCCEEDED(hr))
    {
        RTCritSectEnter(&pThis->CritSect);

        if (pStreamDS->pCircBuf)
        {
            RTCircBufDestroy(pStreamDS->pCircBuf);
            pStreamDS->pCircBuf = NULL;
        }

        if (pStreamDS->Out.pDSB)
        {
            IDirectSoundBuffer8_Release(pStreamDS->Out.pDSB);
            pStreamDS->Out.pDSB = NULL;
        }

        pThis->pDSStrmOut = NULL;

        RTCritSectLeave(&pThis->CritSect);

        int rc2 = dsoundNotifyThread(pThis, false /* fShutdown */);
        AssertRC(rc2);
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

    LogFlowFuncEnter();

    Assert(pStreamDS->Out.pDSB == NULL);

    DSLOG(("DSound: Opening playback stream (uHz=%RU32, cChannels=%RU8, cBits=%RU8, fSigned=%RTbool)\n",
           pCfgReq->Props.uHz,
           pCfgReq->Props.cChannels,
           pCfgReq->Props.cBits,
           pCfgReq->Props.fSigned));

    WAVEFORMATEX wfx;
    int rc = dsoundWaveFmtFromCfg(pCfgReq, &wfx);
    if (RT_FAILURE(rc))
        return E_INVALIDARG;

    DSLOG(("DSound: Requested playback format:\n"
           "  wFormatTag      = %RI16\n"
           "  nChannels       = %RI16\n"
           "  nSamplesPerSec  = %RU32\n"
           "  nAvgBytesPerSec = %RU32\n"
           "  nBlockAlign     = %RI16\n"
           "  wBitsPerSample  = %RI16\n"
           "  cbSize          = %RI16\n",
           wfx.wFormatTag,
           wfx.nChannels,
           wfx.nSamplesPerSec,
           wfx.nAvgBytesPerSec,
           wfx.nBlockAlign,
           wfx.wBitsPerSample,
           wfx.cbSize));

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
        bd.dwFlags    |= DSBCAPS_CTRLPOSITIONNOTIFY;

        bd.dwBufferBytes = DrvAudioHlpMsToBytes(&pCfgReq->Props, pThis->Cfg.msLatencyOut);

        DSLOG(("DSound: Playback buffer is %ld bytes\n", bd.dwBufferBytes));

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
         * Query the actual parameters set for this stream.
         * Those might be different than the initially requested parameters.
         */
        RT_ZERO(wfx);
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

        DSLOG(("DSound: Acquired playback format:\n"
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

        /*
         * Initial state.
         * dsoundPlayStart initializes part of it to make sure that Stop/Start continues with a correct
         * playback buffer position.
         */
        pStreamDS->cbBufSize = bc.dwBufferBytes;

        RTCritSectEnter(&pThis->CritSect);

        rc = RTCircBufCreate(&pStreamDS->pCircBuf, pStreamDS->cbBufSize * 2 /* Double buffering */);
        AssertRC(rc);

        /*
         * Install notification.
         */
        LPDIRECTSOUNDNOTIFY8 pNotify;
        hr = IDirectSoundNotify_QueryInterface(pStreamDS->Out.pDSB, IID_IDirectSoundNotify8, (PVOID *)&pNotify);
        if (SUCCEEDED(hr))
        {
            DSBPOSITIONNOTIFY dsPosNotify[3];
            RT_ZERO(dsPosNotify);

            dsPosNotify[0].dwOffset     = 0;
            dsPosNotify[0].hEventNotify = pThis->aEvents[DSOUNDEVENT_OUTPUT];

            dsPosNotify[1].dwOffset     = float(pStreamDS->cbBufSize * 0.3);
            dsPosNotify[1].hEventNotify = pThis->aEvents[DSOUNDEVENT_OUTPUT];

            dsPosNotify[2].dwOffset     = float(pStreamDS->cbBufSize * 0.6);
            dsPosNotify[2].hEventNotify = pThis->aEvents[DSOUNDEVENT_OUTPUT];

            hr = IDirectSoundNotify_SetNotificationPositions(pNotify, RT_ELEMENTS(dsPosNotify), dsPosNotify);
            if (FAILED(hr))
                DSLOGREL(("DSound: Setting playback position notification failed with %Rhrc\n", hr));

            IDirectSoundNotify_Release(pNotify);

            pThis->pDSStrmOut = pStreamDS;
        }
        else
            DSLOGREL(("DSound: Querying interface for position notification failed with %Rhrc\n", hr));

        RTCritSectLeave(&pThis->CritSect);

        pCfgAcq->cFrameBufferHint = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, pStreamDS->cbBufSize);

    } while (0);

    if (FAILED(hr))
        directSoundPlayClose(pThis, pStreamDS);

    return hr;
}

static void dsoundPlayClearBuffer(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturnVoid(pStreamDS);

    AssertPtr(pStreamDS->pCfg);
    PPDMAUDIOPCMPROPS pProps = &pStreamDS->pCfg->Props;

    HRESULT hr = IDirectSoundBuffer_SetCurrentPosition(pStreamDS->Out.pDSB, 0 /* Position */);
    if (FAILED(hr))
        DSLOGREL(("DSound: Setting current position to 0 when clearing buffer failed with %Rhrc\n", hr));

    PVOID pv1;
    hr = directSoundPlayLock(pThis, pStreamDS,
                             0 /* dwOffset */, pStreamDS->cbBufSize,
                             &pv1, NULL, 0, 0, DSBLOCK_ENTIREBUFFER);
    if (SUCCEEDED(hr))
    {
        DrvAudioHlpClearBuf(pProps, pv1, pStreamDS->cbBufSize, PDMAUDIOPCMPROPS_B2F(pProps, pStreamDS->cbBufSize));

        directSoundPlayUnlock(pThis, pStreamDS->Out.pDSB, pv1, NULL, 0, 0);

        /* Make sure to get the last playback position and current write position from DirectSound again.
         * Those positions in theory could have changed, re-fetch them to be sure. */
        hr = IDirectSoundBuffer_GetCurrentPosition(pStreamDS->Out.pDSB,
                                                   &pStreamDS->Out.offPlayCursorLastPlayed, &pStreamDS->Out.offWritePos);
        if (FAILED(hr))
            DSLOGREL(("DSound: Re-fetching current position when clearing buffer failed with %Rhrc\n", hr));
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

static HRESULT directSoundPlayStop(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, bool fFlush)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    HRESULT hr = S_OK;

    if (pStreamDS->Out.pDSB)
    {
        if (pStreamDS->fEnabled)
        {
            DSLOG(("DSound: %s playback\n", fFlush ? "Stopping" : "Pausing"));

            hr = IDirectSoundBuffer8_Stop(pStreamDS->Out.pDSB);
            if (FAILED(hr))
            {
                hr = directSoundPlayRestore(pThis, pStreamDS->Out.pDSB);
                if (FAILED(hr))
                    hr = IDirectSoundBuffer8_Stop(pStreamDS->Out.pDSB);
            }

            pStreamDS->fEnabled = false;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (   fFlush
            && pStreamDS->pCircBuf)
        {
            RTCircBufReset(pStreamDS->pCircBuf);
        }
    }

    if (FAILED(hr))
        DSLOGREL(("DSound: %s playback failed with %Rhrc\n", fFlush ? "Stopping" : "Pausing", hr));

    return hr;
}


static HRESULT directSoundPlayStart(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    Assert(pStreamDS->fEnabled == false);

    pStreamDS->fEnabled           = true;
    /* Note: Playing back via DirectSound starts in the notification thread
     *       once enough audio output data is available. */

    pStreamDS->Out.fFirstPlayback = true;
    pStreamDS->Out.cUnderruns     = 0;

    DSLOG(("DSound: Playback started\n"));

    return S_OK;
}

/*
 * DirectSoundCapture
 */

static LPCGUID dsoundCaptureSelectDevice(PDRVHOSTDSOUND pThis, PPDMAUDIOSTREAMCFG pCfg)
{
    AssertPtrReturn(pThis, NULL);
    AssertPtrReturn(pCfg,  NULL);

    int rc = VINF_SUCCESS;

    LPCGUID pGUID = pThis->Cfg.pGuidCapture;
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

static HRESULT directSoundCaptureClose(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    LogFlowFuncEnter();

    HRESULT hr = directSoundCaptureStop(pThis, pStreamDS, true /* fFlush */);
    if (FAILED(hr))
        return hr;

    if (   pStreamDS
        && pStreamDS->In.pDSCB)
    {
        DSLOG(("DSound: Closing capturing stream\n"));

        IDirectSoundCaptureBuffer8_Release(pStreamDS->In.pDSCB);
        pStreamDS->In.pDSCB = NULL;
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

    LogFlowFuncEnter();

    Assert(pStreamDS->In.pDSCB == NULL);

    DSLOG(("DSound: Opening capturing stream (uHz=%RU32, cChannels=%RU8, cBits=%RU8, fSigned=%RTbool)\n",
           pCfgReq->Props.uHz,
           pCfgReq->Props.cChannels,
           pCfgReq->Props.cBits,
           pCfgReq->Props.fSigned));

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
        DSCBUFFERDESC bd;
        RT_ZERO(bd);

        bd.dwSize        = sizeof(bd);
        bd.lpwfxFormat   = &wfx;
        bd.dwBufferBytes = DrvAudioHlpMsToBytes(&pCfgReq->Props, pThis->Cfg.msLatencyIn);

        DSLOG(("DSound: Capture buffer is %ld bytes\n", bd.dwBufferBytes));

        LPDIRECTSOUNDCAPTUREBUFFER pDSCB;
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
            DSLOGREL(("DSound: Getting capture capabilities failed with %Rhrc\n", hr));
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

        /* Initial state: reading at the initial capture position, no error. */
        pStreamDS->In.offReadPos = 0;
        pStreamDS->cbBufSize     = bc.dwBufferBytes;

        rc = RTCircBufCreate(&pStreamDS->pCircBuf, pStreamDS->cbBufSize * 2 /* Double buffering */);
        AssertRC(rc);

        /*
         * Install notification.
         */
        LPDIRECTSOUNDNOTIFY8 pNotify;
        hr = IDirectSoundNotify_QueryInterface(pStreamDS->In.pDSCB, IID_IDirectSoundNotify8, (PVOID *)&pNotify);
        if (SUCCEEDED(hr))
        {
            DSBPOSITIONNOTIFY dsPosNotify[3];
            RT_ZERO(dsPosNotify);

            dsPosNotify[0].dwOffset     = 0;
            dsPosNotify[0].hEventNotify = pThis->aEvents[DSOUNDEVENT_INPUT];

            dsPosNotify[1].dwOffset     = float(pStreamDS->cbBufSize * 0.3);
            dsPosNotify[1].hEventNotify = pThis->aEvents[DSOUNDEVENT_INPUT];

            dsPosNotify[2].dwOffset     = float(pStreamDS->cbBufSize * 0.6);
            dsPosNotify[2].hEventNotify = pThis->aEvents[DSOUNDEVENT_INPUT];

            hr = IDirectSoundNotify_SetNotificationPositions(pNotify, 3 /* Count */, dsPosNotify);
            if (FAILED(hr))
                DSLOGREL(("DSound: Setting capture position notification failed with %Rhrc\n", hr));

            IDirectSoundNotify_Release(pNotify);

            pThis->pDSStrmIn = pStreamDS;
        }

        pCfgAcq->cFrameBufferHint = PDMAUDIOSTREAMCFG_B2F(pCfgAcq, pStreamDS->cbBufSize);

    } while (0);

    if (FAILED(hr))
        directSoundCaptureClose(pThis, pStreamDS);

    LogFlowFunc(("Returning %Rhrc\n", hr));
    return hr;
}

static HRESULT directSoundCaptureStop(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS, bool fFlush)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    RT_NOREF(pThis);

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

    if (SUCCEEDED(hr))
    {
        if (   fFlush
            && pStreamDS->pCircBuf)
        {
            RTCircBufReset(pStreamDS->pCircBuf);
        }
    }

    if (FAILED(hr))
        DSLOGREL(("DSound: Stopping capture buffer failed with %Rhrc\n", hr));

    return hr;
}

static HRESULT directSoundCaptureStart(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    AssertPtrReturn(pThis,     E_POINTER);
    AssertPtrReturn(pStreamDS, E_POINTER);

    Assert(pStreamDS->fEnabled == false);

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
                const DWORD fFlags = DSCBSTART_LOOPING;

                DSLOG(("DSound: Starting to capture\n"));
                hr = IDirectSoundCaptureBuffer8_Start(pStreamDS->In.pDSCB, fFlags);
                if (SUCCEEDED(hr))
                {
                    pStreamDS->fEnabled = true;

                    pStreamDS->In.offReadPos = 0;
                    pStreamDS->In.cOverruns  = 0;
                }
                else
                    DSLOGREL(("DSound: Starting to capture failed with %Rhrc\n", hr));
            }
        }
    }
    else
        hr = E_UNEXPECTED;

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
#if 0
        if (   pThis->fEnabledOut != RT_BOOL(cbCtx.cDevOut)
            || pThis->fEnabledIn  != RT_BOOL(cbCtx.cDevIn))
        {
            /** @todo Use a registered callback to the audio connector (e.g "OnConfigurationChanged") to
             *        let the connector know that something has changed within the host backend. */
        }
#endif
        pThis->fEnabledOut = RT_BOOL(cbCtx.cDevOut);
        pThis->fEnabledIn  = RT_BOOL(cbCtx.cDevIn);

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

    pStreamDS->cbBufSize = 0;

    pStreamDS->Out.pDSB = NULL;
    pStreamDS->Out.offWritePos = 0;
    pStreamDS->Out.offPlayCursorLastPlayed = 0;
    pStreamDS->Out.offPlayCursorLastPending = 0;
    pStreamDS->Out.cbWritten = 0;
    pStreamDS->Out.cbPlayed = 0;
    pStreamDS->Out.fFirstPlayback = true;
    pStreamDS->Out.tsLastPlayMs = 0;

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
        {
            hr = directSoundPlayStart(pThis, pStreamDS);
            if (FAILED(hr))
                rc = VERR_NOT_SUPPORTED; /** @todo Fix this. */
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            hr = directSoundPlayStart(pThis, pStreamDS);
            if (SUCCEEDED(hr))
            {
                BOOL fRc = SetEvent(pThis->aEvents[DSOUNDEVENT_OUTPUT]);
                RT_NOREF(fRc);
                Assert(fRc);
            }

            if (FAILED(hr))
                rc = VERR_NOT_SUPPORTED; /** @todo Fix this. */
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            hr = directSoundPlayStop(pThis, pStreamDS, enmStreamCmd == PDMAUDIOSTREAMCMD_DISABLE /* fFlush */);
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

    uint8_t   *pbBuf    = (uint8_t *)pvBuf;
    PRTCIRCBUF pCircBuf = pStreamDS->pCircBuf;

    uint32_t cbToPlay = RT_MIN(cxBuf, (uint32_t)RTCircBufFree(pCircBuf));
    while (cbToPlay)
    {
        void *pvChunk;
        size_t cbChunk;
        RTCircBufAcquireWriteBlock(pCircBuf, cbToPlay, &pvChunk, &cbChunk);

        if (cbChunk)
        {
            memcpy(pvChunk, pbBuf, cbChunk);

            pbBuf     += cbChunk;
            Assert(cbToPlay >= cbChunk);
            cbToPlay  -= (uint32_t)cbChunk;

            cbWrittenTotal += (uint32_t)cbChunk;
        }

        RTCircBufReleaseWriteBlock(pCircBuf, cbChunk);
    }

    Assert(cbWrittenTotal <= cxBuf);

    pStreamDS->Out.cbWritten += cbWrittenTotal;

    if (   pStreamDS->Out.fFirstPlayback
        && pStreamDS->Out.cbWritten >= pStreamDS->cbBufSize)
    {
        BOOL fRc = SetEvent(pThis->aEvents[DSOUNDEVENT_OUTPUT]);
        RT_NOREF(fRc);
        Assert(fRc);
    }

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

    LogFlowFuncEnter();

    HRESULT hr = directSoundPlayStop(pThis, pStreamDS, true /* fFlush */);
    if (SUCCEEDED(hr))
    {
        hr = directSoundPlayClose(pThis, pStreamDS);
        if (FAILED(hr))
            return VERR_GENERAL_FAILURE; /** @todo Fix. */
    }

    return VINF_SUCCESS;
}

static int dsoundCreateStreamIn(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS,
                                PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    LogFunc(("pStreamDS=%p, pCfgReq=%p, enmRecSource=%s\n",
             pStreamDS, pCfgReq, DrvAudioHlpRecSrcToStr(pCfgReq->DestSource.Source)));

    /* Init the stream structure and save relevant information to it. */
    pStreamDS->cbBufSize     = 0;

    pStreamDS->In.pDSCB      = NULL;
    pStreamDS->In.offReadPos = 0;
    pStreamDS->In.cOverruns  = 0;

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
                hr = directSoundCaptureClose(pThis, pStreamDS);
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

            directSoundCaptureStop(pThis, pStreamDS,
                                   enmStreamCmd == PDMAUDIOSTREAMCMD_DISABLE /* fFlush */);

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

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;

    uint32_t cbToRead = RT_MIN((uint32_t)RTCircBufUsed(pStreamDS->pCircBuf), cxBuf);
    while (cbToRead)
    {
        void   *pvChunk;
        size_t  cbChunk;
        RTCircBufAcquireReadBlock(pStreamDS->pCircBuf, cbToRead, &pvChunk, &cbChunk);

        if (cbChunk)
        {
            memcpy((uint8_t *)pvBuf + cbReadTotal, pvChunk, cbChunk);
            cbReadTotal += (uint32_t)cbChunk;
            Assert(cbToRead >= cbChunk);
            cbToRead    -= (uint32_t)cbChunk;
        }

        RTCircBufReleaseReadBlock(pStreamDS->pCircBuf, cbChunk);
    }

    if (RT_SUCCESS(rc))
    {
        if (pcxRead)
            *pcxRead = cbReadTotal;
    }
    else
        dsoundUpdateStatusInternal(pThis);

    return rc;
}

static int dsoundDestroyStreamIn(PDRVHOSTDSOUND pThis, PDSOUNDSTREAM pStreamDS)
{
    LogFlowFuncEnter();

    directSoundCaptureClose(pThis, pStreamDS);

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
    RT_NOREF(fRc);
    Assert(fRc);

    return VINF_SUCCESS;
}


static DECLCALLBACK(int) dsoundThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PDRVHOSTDSOUND pThis = (PDRVHOSTDSOUND)pvUser;
    AssertPtr(pThis);

    LogFlowFuncEnter();

    /* Let caller know that we're done initializing, regardless of the result. */
    int rc = RTThreadUserSignal(hThreadSelf);
    AssertRC(rc);

    for (;;)
    {
        RTCritSectEnter(&pThis->CritSect);

        HANDLE aEvents[VBOX_DSOUND_MAX_EVENTS];
        DWORD  cEvents = 0;
        for (uint8_t i = 0; i < VBOX_DSOUND_MAX_EVENTS; i++)
        {
            if (pThis->aEvents[i])
                aEvents[cEvents++] = pThis->aEvents[i];
        }
        Assert(cEvents);

        RTCritSectLeave(&pThis->CritSect);

        DWORD dwObj = WaitForMultipleObjects(cEvents, aEvents, FALSE /* bWaitAll */, INFINITE);

        RTCritSectEnter(&pThis->CritSect);

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

            case WAIT_OBJECT_0:
            case WAIT_OBJECT_0 + 1:
            case WAIT_OBJECT_0 + 2:
            case WAIT_OBJECT_0 + 3:
            case WAIT_OBJECT_0 + 4:
            {
                dwObj -= WAIT_OBJECT_0;

                if (dwObj == DSOUNDEVENT_NOTIFY)
                {
                    Log3Func(("Notify\n"));
                }
                else if (dwObj == DSOUNDEVENT_INPUT)
                {
                    PDSOUNDSTREAM pStreamDS = pThis->pDSStrmIn;

                    if (   !pStreamDS
                        || !pStreamDS->fEnabled)
                    {
                        Log3Func(("Skipping capture\n"));
                        break;
                    }

                    LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB = pStreamDS->In.pDSCB;
                    AssertPtr(pDSCB);

                    DWORD offCaptureCursor;
                    HRESULT hr = IDirectSoundCaptureBuffer_GetCurrentPosition(pDSCB, NULL, &offCaptureCursor);
                    if (FAILED(hr))
                        break;

                    DWORD cbUsed = dsoundRingDistance(offCaptureCursor, pStreamDS->In.offReadPos, pStreamDS->cbBufSize);

                    PRTCIRCBUF pCircBuf = pStreamDS->pCircBuf;
                    AssertPtr(pCircBuf);

                    uint32_t cbFree = (uint32_t)RTCircBufFree(pCircBuf);
                    if (   !cbFree
                        || pStreamDS->In.cOverruns < 32) /** @todo Make this configurable. */
                    {
                        DSLOG(("DSound: Warning: Capture buffer full, skipping to record data (%RU32 bytes)\n", cbUsed));
                        pStreamDS->In.cOverruns++;
                    }

                    DWORD cbToCapture = RT_MIN(cbUsed, cbFree);

                    Log3Func(("cbUsed=%ld, cbToCapture=%ld\n", cbUsed, cbToCapture));

                    while (cbToCapture)
                    {
                        void  *pvBuf;
                        size_t cbBuf;
                        RTCircBufAcquireWriteBlock(pCircBuf, cbToCapture, &pvBuf, &cbBuf);

                        if (cbBuf)
                        {
                            PVOID pv1, pv2;
                            DWORD cb1, cb2;
                            hr = directSoundCaptureLock(pStreamDS, pStreamDS->In.offReadPos, (DWORD)cbBuf,
                                                        &pv1, &pv2, &cb1, &cb2, 0 /* dwFlags */);
                            if (FAILED(hr))
                                break;

                            AssertPtr(pv1);
                            Assert(cb1);

                            memcpy(pvBuf, pv1, cb1);

                            if (pv2 && cb2) /* Buffer wrap-around? Write second part. */
                                memcpy((uint8_t *)pvBuf + cb1, pv2, cb2);

                            directSoundCaptureUnlock(pDSCB, pv1, pv2, cb1, cb2);

                            pStreamDS->In.offReadPos = (pStreamDS->In.offReadPos + cb1 + cb2) % pStreamDS->cbBufSize;

                            Assert(cbToCapture >= cbBuf);
                            cbToCapture -= (uint32_t)cbBuf;
                        }

#ifdef VBOX_AUDIO_DEBUG_DUMP_PCM_DATA
                        if (cbBuf)
                        {
                            RTFILE fh;
                            int rc2 = RTFileOpen(&fh, VBOX_AUDIO_DEBUG_DUMP_PCM_DATA_PATH "dsoundCapture.pcm",
                                                 RTFILE_O_OPEN_CREATE | RTFILE_O_APPEND | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
                            if (RT_SUCCESS(rc2))
                            {
                                RTFileWrite(fh, pvBuf, cbBuf, NULL);
                                RTFileClose(fh);
                            }
                        }
#endif
                        RTCircBufReleaseWriteBlock(pCircBuf, cbBuf);
                    }
                }
                else if (dwObj == DSOUNDEVENT_OUTPUT)
                {
                    PDSOUNDSTREAM pStreamDS = pThis->pDSStrmOut;

                    if (   !pStreamDS
                        || !pStreamDS->fEnabled)
                    {
                        Log3Func(("Skipping playback\n"));
                        break;
                    }

                    LPDIRECTSOUNDBUFFER8 pDSB = pStreamDS->Out.pDSB;
                    AssertPtr(pDSB);

                    HRESULT hr;

                    DWORD offPlayCursor, offWriteCursor;
                    hr = IDirectSoundBuffer8_GetCurrentPosition(pDSB, &offPlayCursor, &offWriteCursor);
                    if (FAILED(hr))
                        break;

                    DWORD cbFree, cbRemaining;
                    if (pStreamDS->Out.fFirstPlayback)
                    {
                        cbRemaining = 0;
                        cbFree      = dsoundRingDistance(pStreamDS->Out.offWritePos, 0, pStreamDS->cbBufSize);
                    }
                    else
                    {
                        cbFree      = dsoundRingDistance(offPlayCursor, pStreamDS->Out.offWritePos, pStreamDS->cbBufSize);
                        cbRemaining = dsoundRingDistance(pStreamDS->Out.offWritePos, offPlayCursor, pStreamDS->cbBufSize);
                    }

                    PRTCIRCBUF pCircBuf = pStreamDS->pCircBuf;
                    AssertPtr(pCircBuf);

                    DWORD cbUsed   = (uint32_t)RTCircBufUsed(pCircBuf);
                    DWORD cbToPlay = RT_MIN(cbFree, cbUsed);

                    if (   !cbUsed
                        && pStreamDS->Out.cbWritten
                        && pStreamDS->Out.cUnderruns < 32) /** @todo Make this configurable. */
                    {
                        //DSLOG(("DSound: Warning: No more playback data available within time (%RU32 bytes free)\n", cbFree));
                        Log3Func(("Underrun (cbFree=%ld, cbRemaining=%ld, cbUsed=%ld, cbToPlay=%ld, offPlay=%ld, offWrite=%ld)\n",
                                  cbFree, cbRemaining, cbUsed, cbToPlay, offPlayCursor, offWriteCursor));
                        pStreamDS->Out.cUnderruns++;
                    }

                    while (cbToPlay)
                    {
                        void  *pvBuf;
                        size_t cbBuf;
                        RTCircBufAcquireReadBlock(pCircBuf, cbToPlay, &pvBuf, &cbBuf);

                        if (cbBuf)
                        {
                            PVOID pv1, pv2;
                            DWORD cb1, cb2;
                            hr = directSoundPlayLock(pThis, pStreamDS, pStreamDS->Out.offWritePos, (DWORD)cbBuf,
                                                     &pv1, &pv2, &cb1, &cb2, 0 /* dwFlags */);
                            if (FAILED(hr))
                                break;

                            AssertPtr(pv1);
                            Assert(cb1);

                            memcpy(pv1, pvBuf, cb1);

                            if (pv2 && cb2) /* Buffer wrap-around? Write second part. */
                                memcpy(pv2, (uint8_t *)pvBuf + cb1, cb2);

                            directSoundPlayUnlock(pThis, pDSB, pv1, pv2, cb1, cb2);

                            pStreamDS->Out.offWritePos = (pStreamDS->Out.offWritePos + cb1 + cb2) % pStreamDS->cbBufSize;

                            Assert(cbToPlay >= cbBuf);
                            cbToPlay -= (uint32_t)cbBuf;

                            pStreamDS->Out.cbPlayed += cb1 + cb2;
                        }

                        RTCircBufReleaseReadBlock(pCircBuf, cbBuf);
                    }

                    if (   pStreamDS->Out.fFirstPlayback
                        && RTCircBufUsed(pCircBuf) >= pStreamDS->cbBufSize)
                    {
                        DWORD fFlags = DSCBSTART_LOOPING;

                        for (unsigned i = 0; i < DRV_DSOUND_RESTORE_ATTEMPTS_MAX; i++)
                        {
                            hr = IDirectSoundBuffer8_Play(pDSB, 0, 0, fFlags);
                            if (   SUCCEEDED(hr)
                                || hr != DSERR_BUFFERLOST)
                                break;
                            else
                            {
                                LogFunc(("Restarting playback failed due to lost buffer, restoring ...\n"));
                                directSoundPlayRestore(pThis, pDSB);
                            }
                        }

                        if (SUCCEEDED(hr))
                        {
                            DSLOG(("DSound: Started playing output\n"));
                            pStreamDS->Out.fFirstPlayback = false;
                        }
                    }
                }
                break;
            }

            default:
                AssertFailed();
                break;
        }

        RTCritSectLeave(&pThis->CritSect);

        if (pThis->fShutdown)
            break;

    } /* for */

    pThis->fStopped = true;

    LogFlowFunc(("Exited with fShutdown=%RTbool, rc=%Rrc\n", pThis->fShutdown, rc));
    return rc;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnShutdown}
 */
void drvHostDSoundShutdown(PPDMIHOSTAUDIO pInterface)
{
    PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);

    LogFlowFuncEnter();

    int rc = dsoundNotifyThread(pThis, true /* fShutdown */);
    AssertRC(rc);

    int rcThread;
    rc = RTThreadWait(pThis->Thread,  15 * 1000 /* 15s timeout */, &rcThread);
    LogFlowFunc(("rc=%Rrc, rcThread=%Rrc\n", rc, rcThread));

    Assert(pThis->fStopped);

    for (int i = 0; i < VBOX_DSOUND_MAX_EVENTS; i++)
    {
        if (pThis->aEvents[i])
            CloseHandle(pThis->aEvents[i]);
    }

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

        /* Create notification events. */
        for (int i = 0; i < VBOX_DSOUND_MAX_EVENTS; i++)
        {
            pThis->aEvents[i] = CreateEvent(NULL /* Security attribute */,
                                            FALSE /* bManualReset */, FALSE /* bInitialState */,
                                            NULL /* lpName */);
            Assert(pThis->aEvents[i] != NULL);
        }

        /* Start notification thread. */
        rc = RTThreadCreate(&pThis->Thread, dsoundThread,
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
    CFGMR3QueryUIntDef(pCfg, "LatencyMsIn",  &pThis->Cfg.msLatencyIn,  DRV_DSOUND_DEFAULT_LATENCY_MS_IN);
    CFGMR3QueryUIntDef(pCfg, "LatencyMsOut", &pThis->Cfg.msLatencyOut, DRV_DSOUND_DEFAULT_LATENCY_MS_OUT);

    pThis->Cfg.pGuidPlay    = dsoundConfigQueryGUID(pCfg, "DeviceGuidOut", &pThis->Cfg.uuidPlay);
    pThis->Cfg.pGuidCapture = dsoundConfigQueryGUID(pCfg, "DeviceGuidIn",  &pThis->Cfg.uuidCapture);

    DSLOG(("DSound: Configuration: Input latency = %ums, Output latency = %ums, DeviceGuidOut {%RTuuid}, DeviceGuidIn {%RTuuid}\n",
           pThis->Cfg.msLatencyIn,
           pThis->Cfg.msLatencyOut,
           &pThis->Cfg.uuidPlay,
           &pThis->Cfg.uuidCapture));

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
        if (pStreamDS->pCfg)
        {
            rc = RTCritSectInit(&pStreamDS->CritSect);
        }
        else
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
        rc = dsoundDestroyStreamIn(pThis, pStreamDS);
    else
        rc = dsoundDestroyStreamOut(pThis, pStreamDS);

    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectDelete(&pStreamDS->CritSect);

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

    if (   pStreamDS->fEnabled
        && pStreamDS->pCircBuf)
    {
        return (uint32_t)RTCircBufUsed(pStreamDS->pCircBuf);
    }

    return 0;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostDSoundStreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    AssertPtrReturn(pInterface, PDMAUDIOSTREAMSTS_FLAG_NONE);
    AssertPtrReturn(pStream,    PDMAUDIOSTREAMSTS_FLAG_NONE);

    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (pStreamDS->fEnabled)
        return (uint32_t)RTCircBufFree(pStreamDS->pCircBuf);

    return 0;
}

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetPending}
 */
static DECLCALLBACK(uint32_t) drvHostDSoundStreamGetPending(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pStream, 0);

    PDSOUNDSTREAM  pStreamDS = (PDSOUNDSTREAM)pStream;

    if (pStreamDS->pCfg->enmDir == PDMAUDIODIR_OUT)
    {
        uint32_t cbPending = 0;

        /* Any uncommitted data left? */
        if (pStreamDS->pCircBuf)
            cbPending = (uint32_t)RTCircBufUsed(pStreamDS->pCircBuf);

        /* Check if we have committed data which still needs to be played by
         * by DirectSound's streaming buffer. */
        if (!cbPending)
        {
            const uint64_t tsNowMs = RTTimeMilliTS();
            if (pStreamDS->Out.tsLastPlayMs == 0)
                pStreamDS->Out.tsLastPlayMs = tsNowMs;

            PDRVHOSTDSOUND pThis = PDMIHOSTAUDIO_2_DRVHOSTDSOUND(pInterface);

            const uint64_t diffLastPlayMs = tsNowMs - pStreamDS->Out.tsLastPlayMs;
            const uint64_t msThreshold    = pThis->Cfg.msLatencyOut;

            Log2Func(("diffLastPlayMs=%RU64ms\n", diffLastPlayMs));

            cbPending = (diffLastPlayMs >= msThreshold) ? 0 : 1;
        }

        return cbPending;
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
    pThis->IHostAudio.pfnSetCallback = drvHostDSoundSetCallback;
    pThis->pfnCallback               = NULL;
#endif

    /*
     * Init the static parts.
     */
    RTListInit(&pThis->lstDevInput);
    RTListInit(&pThis->lstDevOutput);

    pThis->fEnabledIn  = false;
    pThis->fEnabledOut = false;
    pThis->fStopped    = false;
    pThis->fShutdown   = false;

    RT_ZERO(pThis->aEvents);

    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_AUDIO_MMNOTIFICATION_CLIENT
    bool fUseNotificationClient = false;

    char szOSVersion[32];
    rc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szOSVersion, sizeof(szOSVersion));
    if (RT_SUCCESS(rc))
    {
        /* IMMNotificationClient is available starting at Windows Vista. */
        if (RTStrVersionCompare(szOSVersion, "6.0") >= 0)
            fUseNotificationClient = true;
    }

    if (fUseNotificationClient)
    {
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
    }

    LogRel2(("DSound: Notification client is %s\n", fUseNotificationClient ? "enabled" : "disabled"));
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

