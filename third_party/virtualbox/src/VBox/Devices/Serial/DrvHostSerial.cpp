/* $Id: DrvHostSerial.cpp $ */
/** @file
 * VBox stream I/O devices: Host serial driver
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
#define LOG_GROUP LOG_GROUP_DRV_HOST_SERIAL
#include <VBox/vmm/pdm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/pipe.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
# include <errno.h>
# ifdef RT_OS_SOLARIS
#  include <sys/termios.h>
# else
#  include <termios.h>
# endif
# include <sys/types.h>
# include <fcntl.h>
# include <string.h>
# include <unistd.h>
# ifdef RT_OS_DARWIN
#  include <sys/select.h>
# else
#  include <sys/poll.h>
# endif
# include <sys/ioctl.h>
# include <pthread.h>

# ifdef RT_OS_LINUX
/*
 * TIOCM_LOOP is not defined in the above header files for some reason but in asm/termios.h.
 * But inclusion of this file however leads to compilation errors because of redefinition of some
 * structs. That's why it is defined here until a better solution is found.
 */
#  ifndef TIOCM_LOOP
#   define TIOCM_LOOP 0x8000
#  endif
/* For linux custom baudrate code we also need serial_struct */
#  include <linux/serial.h>
# endif /* linux */

#elif defined(RT_OS_WINDOWS)
# include <iprt/win/windows.h>
#endif

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Char driver instance data.
 *
 * @implements  PDMICHARCONNECTOR
 */
typedef struct DRVHOSTSERIAL
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS                  pDrvIns;
    /** Pointer to the char port interface of the driver/device above us. */
    PPDMICHARPORT               pDrvCharPort;
    /** Our char interface. */
    PDMICHARCONNECTOR           ICharConnector;
    /** Receive thread. */
    PPDMTHREAD                  pRecvThread;
    /** Send thread. */
    PPDMTHREAD                  pSendThread;
    /** Status lines monitor thread. */
    PPDMTHREAD                  pMonitorThread;
    /** Send event semaphore */
    RTSEMEVENT                  SendSem;

    /** the device path */
    char                        *pszDevicePath;

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    /** the device handle */
    RTFILE                      hDeviceFile;
# ifdef RT_OS_DARWIN
    /** The device handle used for reading.
     * Used to prevent the read select from blocking the writes. */
    RTFILE                      hDeviceFileR;
# endif
    /** The read end of the control pipe */
    RTPIPE                      hWakeupPipeR;
    /** The write end of the control pipe */
    RTPIPE                      hWakeupPipeW;
    /** The current line status.
     * Used by the polling version of drvHostSerialMonitorThread.  */
    int                         fStatusLines;
#elif defined(RT_OS_WINDOWS)
    /** the device handle */
    HANDLE                      hDeviceFile;
    /** The event semaphore for waking up the receive thread */
    HANDLE                      hHaltEventSem;
    /** The event semaphore for overlapped receiving */
    HANDLE                      hEventRecv;
    /** For overlapped receiving */
    OVERLAPPED                  overlappedRecv;
    /** The event semaphore for overlapped sending */
    HANDLE                      hEventSend;
    /** For overlapped sending */
    OVERLAPPED                  overlappedSend;
#endif

    /** Internal send FIFO queue */
    uint8_t volatile            u8SendByte;
    bool volatile               fSending;
    uint8_t                     Alignment[2];

    /** Read/write statistics */
    STAMCOUNTER                 StatBytesRead;
    STAMCOUNTER                 StatBytesWritten;
#ifdef RT_OS_DARWIN
    /** The number of bytes we've dropped because the send queue
     * was full. */
    STAMCOUNTER                 StatSendOverflows;
#endif
} DRVHOSTSERIAL, *PDRVHOSTSERIAL;


/** Converts a pointer to DRVCHAR::ICharConnector to a PDRVHOSTSERIAL. */
#define PDMICHAR_2_DRVHOSTSERIAL(pInterface) RT_FROM_MEMBER(pInterface, DRVHOSTSERIAL, ICharConnector)


/* -=-=-=-=- IBase -=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostSerialQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS      pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTSERIAL  pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMICHARCONNECTOR, &pThis->ICharConnector);
    return NULL;
}


/* -=-=-=-=- ICharConnector -=-=-=-=- */

/** @interface_method_impl{PDMICHARCONNECTOR,pfnWrite} */
static DECLCALLBACK(int) drvHostSerialWrite(PPDMICHARCONNECTOR pInterface, const void *pvBuf, size_t cbWrite)
{
    PDRVHOSTSERIAL pThis = PDMICHAR_2_DRVHOSTSERIAL(pInterface);
    const uint8_t *pbBuffer = (const uint8_t *)pvBuf;

    LogFlow(("%s: pvBuf=%#p cbWrite=%d\n", __FUNCTION__, pvBuf, cbWrite));

    for (uint32_t i = 0; i < cbWrite; i++)
    {
        if (ASMAtomicXchgBool(&pThis->fSending, true))
            return VERR_BUFFER_OVERFLOW;

        pThis->u8SendByte = pbBuffer[i];
        RTSemEventSignal(pThis->SendSem);
        STAM_COUNTER_INC(&pThis->StatBytesWritten);
    }
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) drvHostSerialSetParameters(PPDMICHARCONNECTOR pInterface, unsigned Bps, char chParity, unsigned cDataBits, unsigned cStopBits)
{
    PDRVHOSTSERIAL pThis = PDMICHAR_2_DRVHOSTSERIAL(pInterface);
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    struct termios *termiosSetup;
    int baud_rate;
#elif defined(RT_OS_WINDOWS)
    LPDCB comSetup;
#endif

    LogFlow(("%s: Bps=%u chParity=%c cDataBits=%u cStopBits=%u\n", __FUNCTION__, Bps, chParity, cDataBits, cStopBits));

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    termiosSetup = (struct termios *)RTMemTmpAllocZ(sizeof(struct termios));

    /* Enable receiver */
    termiosSetup->c_cflag |= (CLOCAL | CREAD);

    switch (Bps)
    {
        case 50:
            baud_rate = B50;
            break;
        case 75:
            baud_rate = B75;
            break;
        case 110:
            baud_rate = B110;
            break;
        case 134:
            baud_rate = B134;
            break;
        case 150:
            baud_rate = B150;
            break;
        case 200:
            baud_rate = B200;
            break;
        case 300:
            baud_rate = B300;
            break;
        case 600:
            baud_rate = B600;
            break;
        case 1200:
            baud_rate = B1200;
            break;
        case 1800:
            baud_rate = B1800;
            break;
        case 2400:
            baud_rate = B2400;
            break;
        case 4800:
            baud_rate = B4800;
            break;
        case 9600:
            baud_rate = B9600;
            break;
        case 19200:
            baud_rate = B19200;
            break;
        case 38400:
            baud_rate = B38400;
            break;
        case 57600:
            baud_rate = B57600;
            break;
        case 115200:
            baud_rate = B115200;
            break;
        default:
#ifdef RT_OS_LINUX
            struct serial_struct serialStruct;
            if (ioctl(RTFileToNative(pThis->hDeviceFile), TIOCGSERIAL, &serialStruct) != -1)
            {
                serialStruct.custom_divisor = serialStruct.baud_base / Bps;
                if (!serialStruct.custom_divisor)
                    serialStruct.custom_divisor = 1;
                serialStruct.flags &= ~ASYNC_SPD_MASK;
                serialStruct.flags |= ASYNC_SPD_CUST;
                ioctl(RTFileToNative(pThis->hDeviceFile), TIOCSSERIAL, &serialStruct);
                baud_rate = B38400;
            }
            else
                baud_rate = B9600;
#else /* !RT_OS_LINUX */
            baud_rate = B9600;
#endif /* !RT_OS_LINUX */
    }

    cfsetispeed(termiosSetup, baud_rate);
    cfsetospeed(termiosSetup, baud_rate);

    switch (chParity)
    {
        case 'E':
            termiosSetup->c_cflag |= PARENB;
            break;
        case 'O':
            termiosSetup->c_cflag |= (PARENB | PARODD);
            break;
        case 'N':
            break;
        default:
            break;
    }

    switch (cDataBits)
    {
        case 5:
            termiosSetup->c_cflag |= CS5;
            break;
        case 6:
            termiosSetup->c_cflag |= CS6;
            break;
        case 7:
            termiosSetup->c_cflag |= CS7;
            break;
        case 8:
            termiosSetup->c_cflag |= CS8;
            break;
        default:
            break;
    }

    switch (cStopBits)
    {
        case 2:
            termiosSetup->c_cflag |= CSTOPB;
        default:
            break;
    }

    /* set serial port to raw input */
    termiosSetup->c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ECHOK | ISIG | IEXTEN);

    tcsetattr(RTFileToNative(pThis->hDeviceFile), TCSANOW, termiosSetup);
    RTMemTmpFree(termiosSetup);

#ifdef RT_OS_LINUX
    /*
     * XXX In Linux, if a thread calls tcsetattr while the monitor thread is
     * waiting in ioctl for a modem status change then 8250.c wrongly disables
     * modem irqs and so the monitor thread never gets released. The workaround
     * is to send a signal after each tcsetattr.
     */
    if (RT_LIKELY(pThis->pMonitorThread != NULL))
        RTThreadPoke(pThis->pMonitorThread->Thread);
#endif

#elif defined(RT_OS_WINDOWS)
    comSetup = (LPDCB)RTMemTmpAllocZ(sizeof(DCB));

    comSetup->DCBlength = sizeof(DCB);

    switch (Bps)
    {
        case 110:
            comSetup->BaudRate = CBR_110;
            break;
        case 300:
            comSetup->BaudRate = CBR_300;
            break;
        case 600:
            comSetup->BaudRate = CBR_600;
            break;
        case 1200:
            comSetup->BaudRate = CBR_1200;
            break;
        case 2400:
            comSetup->BaudRate = CBR_2400;
            break;
        case 4800:
            comSetup->BaudRate = CBR_4800;
            break;
        case 9600:
            comSetup->BaudRate = CBR_9600;
            break;
        case 14400:
            comSetup->BaudRate = CBR_14400;
            break;
        case 19200:
            comSetup->BaudRate = CBR_19200;
            break;
        case 38400:
            comSetup->BaudRate = CBR_38400;
            break;
        case 57600:
            comSetup->BaudRate = CBR_57600;
            break;
        case 115200:
            comSetup->BaudRate = CBR_115200;
            break;
        default:
            comSetup->BaudRate = CBR_9600;
    }

    comSetup->fBinary = TRUE;
    comSetup->fOutxCtsFlow = FALSE;
    comSetup->fOutxDsrFlow = FALSE;
    comSetup->fDtrControl = DTR_CONTROL_DISABLE;
    comSetup->fDsrSensitivity = FALSE;
    comSetup->fTXContinueOnXoff = TRUE;
    comSetup->fOutX = FALSE;
    comSetup->fInX = FALSE;
    comSetup->fErrorChar = FALSE;
    comSetup->fNull = FALSE;
    comSetup->fRtsControl = RTS_CONTROL_DISABLE;
    comSetup->fAbortOnError = FALSE;
    comSetup->wReserved = 0;
    comSetup->XonLim = 5;
    comSetup->XoffLim = 5;
    comSetup->ByteSize = cDataBits;

    switch (chParity)
    {
        case 'E':
            comSetup->Parity = EVENPARITY;
            break;
        case 'O':
            comSetup->Parity = ODDPARITY;
            break;
        case 'N':
            comSetup->Parity = NOPARITY;
            break;
        default:
            break;
    }

    switch (cStopBits)
    {
        case 1:
            comSetup->StopBits = ONESTOPBIT;
            break;
        case 2:
            comSetup->StopBits = TWOSTOPBITS;
            break;
        default:
            break;
    }

    comSetup->XonChar = 0;
    comSetup->XoffChar = 0;
    comSetup->ErrorChar = 0;
    comSetup->EofChar = 0;
    comSetup->EvtChar = 0;

    SetCommState(pThis->hDeviceFile, comSetup);
    RTMemTmpFree(comSetup);
#endif /* RT_OS_WINDOWS */

    return VINF_SUCCESS;
}

/* -=-=-=-=- receive thread -=-=-=-=- */

/**
 * Send thread loop.
 *
 * @returns VINF_SUCCESS.
 * @param   pDrvIns     PDM driver instance data.
 * @param   pThread     The PDM thread data.
 */
static DECLCALLBACK(int) drvHostSerialSendThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

#ifdef RT_OS_WINDOWS
    /* Make sure that the halt event semaphore is reset. */
    DWORD dwRet = WaitForSingleObject(pThis->hHaltEventSem, 0);

    HANDLE haWait[2];
    haWait[0] = pThis->hEventSend;
    haWait[1] = pThis->hHaltEventSem;
#endif

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        int rc = RTSemEventWait(pThis->SendSem, RT_INDEFINITE_WAIT);
        AssertRCBreak(rc);

        /*
         * Write the character to the host device.
         */
        while (pThread->enmState == PDMTHREADSTATE_RUNNING)
        {
            /* copy the send queue so we get a linear buffer with the maximal size. */
            uint8_t ch = pThis->u8SendByte;
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)

            size_t cbWritten;
            rc = RTFileWrite(pThis->hDeviceFile, &ch, 1, &cbWritten);
            if (rc == VERR_TRY_AGAIN)
                cbWritten = 0;
            if (cbWritten < 1 && (RT_SUCCESS(rc) || rc == VERR_TRY_AGAIN))
            {
                /* ok, block till the device is ready for more (O_NONBLOCK) effect. */
                rc = VINF_SUCCESS;
                while (pThread->enmState == PDMTHREADSTATE_RUNNING)
                {
                    /* wait */
                    fd_set WrSet;
                    FD_ZERO(&WrSet);
                    FD_SET(RTFileToNative(pThis->hDeviceFile), &WrSet);
                    fd_set XcptSet;
                    FD_ZERO(&XcptSet);
                    FD_SET(RTFileToNative(pThis->hDeviceFile), &XcptSet);
# ifdef DEBUG
                    uint64_t u64Now = RTTimeMilliTS();
# endif
                    rc = select(RTFileToNative(pThis->hDeviceFile) + 1, NULL, &WrSet, &XcptSet, NULL);
                    /** @todo check rc? */

# ifdef DEBUG
                    Log2(("select wait for %dms\n", RTTimeMilliTS() - u64Now));
# endif
                    /* try write more */
                    rc = RTFileWrite(pThis->hDeviceFile, &ch, 1, &cbWritten);
                    if (rc == VERR_TRY_AGAIN)
                        cbWritten = 0;
                    else if (RT_FAILURE(rc))
                        break;
                    else if (cbWritten >= 1)
                        break;
                    rc = VINF_SUCCESS;
                } /* wait/write loop */
            }

#elif defined(RT_OS_WINDOWS)
            /* perform an overlapped write operation. */
            DWORD cbWritten;
            memset(&pThis->overlappedSend, 0, sizeof(pThis->overlappedSend));
            pThis->overlappedSend.hEvent = pThis->hEventSend;
            if (!WriteFile(pThis->hDeviceFile, &ch, 1, &cbWritten, &pThis->overlappedSend))
            {
                dwRet = GetLastError();
                if (dwRet == ERROR_IO_PENDING)
                {
                    /*
                     * write blocked, wait for completion or wakeup...
                     */
                    dwRet = WaitForMultipleObjects(2, haWait, FALSE, INFINITE);
                    if (dwRet != WAIT_OBJECT_0)
                    {
                        AssertMsg(pThread->enmState != PDMTHREADSTATE_RUNNING, ("The halt event semaphore is set but the thread is still in running state\n"));
                        break;
                    }
                }
                else
                    rc = RTErrConvertFromWin32(dwRet);
            }

#endif /* RT_OS_WINDOWS */
            if (RT_FAILURE(rc))
            {
                LogRel(("HostSerial#%d: Serial Write failed with %Rrc; terminating send thread\n", pDrvIns->iInstance, rc));
                return rc;
            }
            ASMAtomicXchgBool(&pThis->fSending, false);
            break;
        } /* write loop */
    }

    return VINF_SUCCESS;
}

/**
 * Unblock the send thread so it can respond to a state change.
 *
 * @returns a VBox status code.
 * @param     pDrvIns     The driver instance.
 * @param     pThread     The send thread.
 */
static DECLCALLBACK(int) drvHostSerialWakeupSendThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    int rc;

    rc = RTSemEventSignal(pThis->SendSem);
    if (RT_FAILURE(rc))
        return rc;

#ifdef RT_OS_WINDOWS
    if (!SetEvent(pThis->hHaltEventSem))
        return RTErrConvertFromWin32(GetLastError());
#endif

    return VINF_SUCCESS;
}

/* -=-=-=-=- receive thread -=-=-=-=- */

/**
 * Receive thread loop.
 *
 * This thread pushes data from the host serial device up the driver
 * chain toward the serial device.
 *
 * @returns VINF_SUCCESS.
 * @param   pDrvIns     PDM driver instance data.
 * @param   pThread     The PDM thread data.
 */
static DECLCALLBACK(int) drvHostSerialRecvThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    uint8_t abBuffer[256];
    uint8_t *pbBuffer = NULL;
    size_t cbRemaining = 0; /* start by reading host data */
    int rc = VINF_SUCCESS;
    int rcThread = VINF_SUCCESS;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

#ifdef RT_OS_WINDOWS
    /* Make sure that the halt event semaphore is reset. */
    DWORD dwRet = WaitForSingleObject(pThis->hHaltEventSem, 0);

    HANDLE ahWait[2];
    ahWait[0] = pThis->hEventRecv;
    ahWait[1] = pThis->hHaltEventSem;
#endif

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        if (!cbRemaining)
        {
            /* Get a block of data from the host serial device. */

#if defined(RT_OS_DARWIN) /* poll is broken on x86 darwin, returns POLLNVAL. */
            fd_set RdSet;
            FD_ZERO(&RdSet);
            FD_SET(RTFileToNative(pThis->hDeviceFileR), &RdSet);
            FD_SET(RTPipeToNative(pThis->hWakeupPipeR), &RdSet);
            fd_set XcptSet;
            FD_ZERO(&XcptSet);
            FD_SET(RTFileToNative(pThis->hDeviceFile), &XcptSet);
            FD_SET(RTPipeToNative(pThis->hWakeupPipeR), &XcptSet);
# if 1 /* it seems like this select is blocking the write... */
            rc = select(RT_MAX(RTPipeToNative(pThis->hWakeupPipeR), RTFileToNative(pThis->hDeviceFileR)) + 1,
                        &RdSet, NULL, &XcptSet, NULL);
# else
            struct timeval tv = { 0, 1000 };
            rc = select(RTPipeToNative(pThis->hWakeupPipeR), RTFileToNative(pThis->hDeviceFileR) + 1,
                        &RdSet, NULL, &XcptSet, &tv);
# endif
            if (rc == -1)
            {
                int err = errno;
                rcThread = RTErrConvertFromErrno(err);
                LogRel(("HostSerial#%d: select failed with errno=%d / %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, err, rcThread));
                break;
            }

            /* this might have changed in the meantime */
            if (pThread->enmState != PDMTHREADSTATE_RUNNING)
                break;
            if (rc == 0)
                continue;

            /* drain the wakeup pipe */
            size_t cbRead;
            if (   FD_ISSET(RTPipeToNative(pThis->hWakeupPipeR), &RdSet)
                || FD_ISSET(RTPipeToNative(pThis->hWakeupPipeR), &XcptSet))
            {
                rc = RTPipeRead(pThis->hWakeupPipeR, abBuffer, 1, &cbRead);
                if (RT_FAILURE(rc))
                {
                    LogRel(("HostSerial#%d: draining the wakeup pipe failed with %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, rc));
                    rcThread = rc;
                    break;
                }
                continue;
            }

            /* read data from the serial port. */
            rc = RTFileRead(pThis->hDeviceFileR, abBuffer, sizeof(abBuffer), &cbRead);
            if (RT_FAILURE(rc))
            {
                LogRel(("HostSerial#%d: (1) Read failed with %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, rc));
                rcThread = rc;
                break;
            }
            cbRemaining = cbRead;

#elif defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)

            size_t cbRead;
            struct pollfd aFDs[2];
            aFDs[0].fd      = RTFileToNative(pThis->hDeviceFile);
            aFDs[0].events  = POLLIN;
            aFDs[0].revents = 0;
            aFDs[1].fd      = RTPipeToNative(pThis->hWakeupPipeR);
            aFDs[1].events  = POLLIN | POLLERR | POLLHUP;
            aFDs[1].revents = 0;
            rc = poll(aFDs, RT_ELEMENTS(aFDs), -1);
            if (rc < 0)
            {
                int err = errno;
                if (err == EINTR)
                {
                    /*
                     * EINTR errors should be harmless, even if they are not supposed to occur in our setup.
                     */
                    Log(("rc=%d revents=%#x,%#x errno=%p %s\n", rc, aFDs[0].revents, aFDs[1].revents, err, strerror(err)));
                    RTThreadYield();
                    continue;
                }

                rcThread = RTErrConvertFromErrno(err);
                LogRel(("HostSerial#%d: poll failed with errno=%d / %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, err, rcThread));
                break;
            }
            /* this might have changed in the meantime */
            if (pThread->enmState != PDMTHREADSTATE_RUNNING)
                break;
            if (rc > 0 && aFDs[1].revents)
            {
                if (aFDs[1].revents & (POLLHUP | POLLERR | POLLNVAL))
                    break;
                /* notification to terminate -- drain the pipe */
                RTPipeRead(pThis->hWakeupPipeR, &abBuffer, 1, &cbRead);
                continue;
            }
            rc = RTFileRead(pThis->hDeviceFile, abBuffer, sizeof(abBuffer), &cbRead);
            if (RT_FAILURE(rc))
            {
                /* don't terminate worker thread when data unavailable */
                if (rc == VERR_TRY_AGAIN)
                    continue;

                LogRel(("HostSerial#%d: (2) Read failed with %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, rc));
                rcThread = rc;
                break;
            }
            cbRemaining = cbRead;

#elif defined(RT_OS_WINDOWS)

            DWORD dwEventMask = 0;
            DWORD dwNumberOfBytesTransferred;

            memset(&pThis->overlappedRecv, 0, sizeof(pThis->overlappedRecv));
            pThis->overlappedRecv.hEvent = pThis->hEventRecv;

            if (!WaitCommEvent(pThis->hDeviceFile, &dwEventMask, &pThis->overlappedRecv))
            {
                dwRet = GetLastError();
                if (dwRet == ERROR_IO_PENDING)
                {
                    dwRet = WaitForMultipleObjects(2, ahWait, FALSE, INFINITE);
                    if (dwRet != WAIT_OBJECT_0)
                    {
                        /* notification to terminate */
                        AssertMsg(pThread->enmState != PDMTHREADSTATE_RUNNING, ("The halt event semaphore is set but the thread is still in running state\n"));
                        break;
                    }
                }
                else
                {
                    rcThread = RTErrConvertFromWin32(dwRet);
                    LogRel(("HostSerial#%d: Wait failed with error %Rrc; terminating the worker thread.\n", pDrvIns->iInstance, rcThread));
                    break;
                }
            }
            /* this might have changed in the meantime */
            if (pThread->enmState != PDMTHREADSTATE_RUNNING)
                break;

            /* Check the event */
            if (dwEventMask & EV_RXCHAR)
            {
                if (!ReadFile(pThis->hDeviceFile, abBuffer, sizeof(abBuffer), &dwNumberOfBytesTransferred, &pThis->overlappedRecv))
                {
                    dwRet = GetLastError();
                    if (dwRet == ERROR_IO_PENDING)
                    {
                        if (GetOverlappedResult(pThis->hDeviceFile, &pThis->overlappedRecv, &dwNumberOfBytesTransferred, TRUE))
                            dwRet = NO_ERROR;
                        else
                            dwRet = GetLastError();
                    }
                    if (dwRet != NO_ERROR)
                    {
                        rcThread = RTErrConvertFromWin32(dwRet);
                        LogRel(("HostSerial#%d: Read failed with error %Rrc; terminating the worker thread.\n", pDrvIns->iInstance, rcThread));
                        break;
                    }
                }
                cbRemaining = dwNumberOfBytesTransferred;
            }
            else if (dwEventMask & EV_BREAK)
            {
                Log(("HostSerial#%d: Detected break\n"));
                rc = pThis->pDrvCharPort->pfnNotifyBreak(pThis->pDrvCharPort);
            }
            else
            {
                /* The status lines have changed. Notify the device. */
                DWORD dwNewStatusLinesState = 0;
                uint32_t uNewStatusLinesState = 0;

                /* Get the new state */
                if (GetCommModemStatus(pThis->hDeviceFile, &dwNewStatusLinesState))
                {
                    if (dwNewStatusLinesState & MS_RLSD_ON)
                        uNewStatusLinesState |= PDMICHARPORT_STATUS_LINES_DCD;
                    if (dwNewStatusLinesState & MS_RING_ON)
                        uNewStatusLinesState |= PDMICHARPORT_STATUS_LINES_RI;
                    if (dwNewStatusLinesState & MS_DSR_ON)
                        uNewStatusLinesState |= PDMICHARPORT_STATUS_LINES_DSR;
                    if (dwNewStatusLinesState & MS_CTS_ON)
                        uNewStatusLinesState |= PDMICHARPORT_STATUS_LINES_CTS;
                    rc = pThis->pDrvCharPort->pfnNotifyStatusLinesChanged(pThis->pDrvCharPort, uNewStatusLinesState);
                    if (RT_FAILURE(rc))
                    {
                        /* Notifying device failed, continue but log it */
                        LogRel(("HostSerial#%d: Notifying device failed with error %Rrc; continuing.\n", pDrvIns->iInstance, rc));
                    }
                }
                else
                {
                    /* Getting new state failed, continue but log it */
                    LogRel(("HostSerial#%d: Getting status lines state failed with error %Rrc; continuing.\n", pDrvIns->iInstance, RTErrConvertFromWin32(GetLastError())));
                }
            }
#endif

            Log(("Read %d bytes.\n", cbRemaining));
            pbBuffer = abBuffer;
        }
        else
        {
            /* Send data to the guest. */
            size_t cbProcessed = cbRemaining;
            rc = pThis->pDrvCharPort->pfnNotifyRead(pThis->pDrvCharPort, pbBuffer, &cbProcessed);
            if (RT_SUCCESS(rc))
            {
                Assert(cbProcessed); Assert(cbProcessed <= cbRemaining);
                pbBuffer += cbProcessed;
                cbRemaining -= cbProcessed;
                STAM_COUNTER_ADD(&pThis->StatBytesRead, cbProcessed);
            }
            else if (rc == VERR_TIMEOUT)
            {
                /* Normal case, just means that the guest didn't accept a new
                 * character before the timeout elapsed. Just retry. */
                rc = VINF_SUCCESS;
            }
            else
            {
                LogRel(("HostSerial#%d: NotifyRead failed with %Rrc, terminating the worker thread.\n", pDrvIns->iInstance, rc));
                rcThread = rc;
                break;
            }
        }
    }

    return rcThread;
}

/**
 * Unblock the receive thread so it can respond to a state change.
 *
 * @returns a VBox status code.
 * @param     pDrvIns     The driver instance.
 * @param     pThread     The receive thread.
 */
static DECLCALLBACK(int) drvHostSerialWakeupRecvThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    size_t cbIgnored;
    return RTPipeWrite(pThis->hWakeupPipeW, "", 1, &cbIgnored);

#elif defined(RT_OS_WINDOWS)
    if (!SetEvent(pThis->hHaltEventSem))
        return RTErrConvertFromWin32(GetLastError());
    return VINF_SUCCESS;
#else
# error adapt me!
#endif
}

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
/* -=-=-=-=- Monitor thread -=-=-=-=- */

/**
 * Monitor thread loop.
 *
 * This thread monitors the status lines and notifies the device
 * if they change.
 *
 * @returns VINF_SUCCESS.
 * @param   pDrvIns     PDM driver instance data.
 * @param   pThread     The PDM thread data.
 */
static DECLCALLBACK(int) drvHostSerialMonitorThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    unsigned long const uStatusLinesToCheck = TIOCM_CAR | TIOCM_RNG | TIOCM_DSR | TIOCM_CTS;
#ifdef RT_OS_LINUX
    bool fPoll = false;
#endif

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    do
    {
        unsigned int statusLines;

        /*
         * Get the status line state.
         */
        int rcPsx = ioctl(RTFileToNative(pThis->hDeviceFile), TIOCMGET, &statusLines);
        if (rcPsx < 0)
        {
            PDMDrvHlpVMSetRuntimeError(pDrvIns, 0 /*fFlags*/, "DrvHostSerialFail",
                                       N_("Ioctl failed for serial host device '%s' (%Rrc). The device will not work properly"),
                                       pThis->pszDevicePath, RTErrConvertFromErrno(errno));
            break;
        }

        uint32_t newStatusLine = 0;

        if (statusLines & TIOCM_CAR)
            newStatusLine |= PDMICHARPORT_STATUS_LINES_DCD;
        if (statusLines & TIOCM_RNG)
            newStatusLine |= PDMICHARPORT_STATUS_LINES_RI;
        if (statusLines & TIOCM_DSR)
            newStatusLine |= PDMICHARPORT_STATUS_LINES_DSR;
        if (statusLines & TIOCM_CTS)
            newStatusLine |= PDMICHARPORT_STATUS_LINES_CTS;
        pThis->pDrvCharPort->pfnNotifyStatusLinesChanged(pThis->pDrvCharPort, newStatusLine);

        if (PDMTHREADSTATE_RUNNING != pThread->enmState)
            break;

# ifdef RT_OS_LINUX
        /*
         * Wait for status line change.
         *
         * XXX In Linux, if a thread calls tcsetattr while the monitor thread is
         * waiting in ioctl for a modem status change then 8250.c wrongly disables
         * modem irqs and so the monitor thread never gets released. The workaround
         * is to send a signal after each tcsetattr.
         *
         * TIOCMIWAIT doesn't work for the DSR line with TIOCM_DSR set
         * (see http://lxr.linux.no/#linux+v4.7/drivers/usb/class/cdc-acm.c#L949)
         * However as it is possible to query the line state we will not just clear
         * the TIOCM_DSR bit from the lines to check but resort to the polling
         * approach just like on other hosts.
         */
        if (!fPoll)
        {
            rcPsx = ioctl(RTFileToNative(pThis->hDeviceFile), TIOCMIWAIT, uStatusLinesToCheck);
            if (rcPsx < 0 && errno != EINTR)
            {
                LogRel(("Serial#%u: Failed to wait for status line change with rcPsx=%d errno=%d, switch to polling\n",
                        pDrvIns->iInstance, rcPsx, errno));
                fPoll = true;
                pThis->fStatusLines = statusLines;
            }
        }
        else
        {
            /* Poll for status line change. */
            if (!((statusLines ^ pThis->fStatusLines) & uStatusLinesToCheck))
                PDMR3ThreadSleep(pThread, 500); /* 0.5 sec */
            pThis->fStatusLines = statusLines;
        }
# else
        /* Poll for status line change. */
        if (!((statusLines ^ pThis->fStatusLines) & uStatusLinesToCheck))
            PDMR3ThreadSleep(pThread, 500); /* 0.5 sec */
        pThis->fStatusLines = statusLines;
# endif
    } while (PDMTHREADSTATE_RUNNING == pThread->enmState);

    return VINF_SUCCESS;
}

/**
 * Unblock the monitor thread so it can respond to a state change.
 * We need to execute this code exactly once during initialization.
 * But we don't want to block --- therefore this dedicated thread.
 *
 * @returns a VBox status code.
 * @param     pDrvIns     The driver instance.
 * @param     pThread     The send thread.
 */
static DECLCALLBACK(int) drvHostSerialWakeupMonitorThread(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
# ifdef RT_OS_LINUX
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    int rc = VINF_SUCCESS;

    rc = RTThreadPoke(pThread->Thread);
    if (RT_FAILURE(rc))
        PDMDrvHlpVMSetRuntimeError(pDrvIns, 0 /*fFlags*/, "DrvHostSerialFail",
                                    N_("Suspending serial monitor thread failed for serial device '%s' (%Rrc). The shutdown may take longer than expected"),
                                    pThis->pszDevicePath, RTErrConvertFromErrno(rc));

# else  /* !RT_OS_LINUX*/

    /* In polling mode there is nobody to wake up (PDMThread will cancel the sleep). */
    NOREF(pDrvIns);
    NOREF(pThread);

# endif /* RT_OS_LINUX */

    return VINF_SUCCESS;
}
#endif /* RT_OS_LINUX || RT_OS_DARWIN || RT_OS_SOLARIS */

/**
 * Set the modem lines.
 *
 * @returns VBox status code
 * @param pInterface        Pointer to the interface structure.
 * @param RequestToSend     Set to true if this control line should be made active.
 * @param DataTerminalReady Set to true if this control line should be made active.
 */
static DECLCALLBACK(int) drvHostSerialSetModemLines(PPDMICHARCONNECTOR pInterface, bool RequestToSend, bool DataTerminalReady)
{
    PDRVHOSTSERIAL pThis = PDMICHAR_2_DRVHOSTSERIAL(pInterface);

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    int modemStateSet = 0;
    int modemStateClear = 0;

    if (RequestToSend)
        modemStateSet |= TIOCM_RTS;
    else
        modemStateClear |= TIOCM_RTS;

    if (DataTerminalReady)
        modemStateSet |= TIOCM_DTR;
    else
        modemStateClear |= TIOCM_DTR;

    if (modemStateSet)
        ioctl(RTFileToNative(pThis->hDeviceFile), TIOCMBIS, &modemStateSet);

    if (modemStateClear)
        ioctl(RTFileToNative(pThis->hDeviceFile), TIOCMBIC, &modemStateClear);

#elif defined(RT_OS_WINDOWS)
    if (RequestToSend)
        EscapeCommFunction(pThis->hDeviceFile, SETRTS);
    else
        EscapeCommFunction(pThis->hDeviceFile, CLRRTS);

    if (DataTerminalReady)
        EscapeCommFunction(pThis->hDeviceFile, SETDTR);
    else
        EscapeCommFunction(pThis->hDeviceFile, CLRDTR);

#endif

    return VINF_SUCCESS;
}

/**
 * Sets the TD line into break condition.
 *
 * @returns VBox status code.
 * @param   pInterface  Pointer to the interface structure containing the called function pointer.
 * @param   fBreak      Set to true to let the device send a break false to put into normal operation.
 * @thread  Any thread.
 */
static DECLCALLBACK(int) drvHostSerialSetBreak(PPDMICHARCONNECTOR pInterface, bool fBreak)
{
    PDRVHOSTSERIAL pThis = PDMICHAR_2_DRVHOSTSERIAL(pInterface);

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    if (fBreak)
        ioctl(RTFileToNative(pThis->hDeviceFile), TIOCSBRK);
    else
        ioctl(RTFileToNative(pThis->hDeviceFile), TIOCCBRK);

#elif defined(RT_OS_WINDOWS)
    if (fBreak)
        SetCommBreak(pThis->hDeviceFile);
    else
        ClearCommBreak(pThis->hDeviceFile);
#endif

    return VINF_SUCCESS;
}

/* -=-=-=-=- driver interface -=-=-=-=- */

/**
 * Destruct a char driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that
 * any non-VM resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvHostSerialDestruct(PPDMDRVINS pDrvIns)
{
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    LogFlow(("%s: iInstance=%d\n", __FUNCTION__, pDrvIns->iInstance));
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    /* Empty the send queue */
    if (pThis->SendSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pThis->SendSem);
        pThis->SendSem = NIL_RTSEMEVENT;
    }

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)

    int rc = RTPipeClose(pThis->hWakeupPipeW); AssertRC(rc);
    pThis->hWakeupPipeW = NIL_RTPIPE;
    rc = RTPipeClose(pThis->hWakeupPipeR); AssertRC(rc);
    pThis->hWakeupPipeR = NIL_RTPIPE;

# if defined(RT_OS_DARWIN)
    if (pThis->hDeviceFileR != NIL_RTFILE)
    {
        if (pThis->hDeviceFileR != pThis->hDeviceFile)
        {
            rc = RTFileClose(pThis->hDeviceFileR);
            AssertRC(rc);
        }
        pThis->hDeviceFileR = NIL_RTFILE;
    }
# endif
    if (pThis->hDeviceFile != NIL_RTFILE)
    {
        rc = RTFileClose(pThis->hDeviceFile); AssertRC(rc);
        pThis->hDeviceFile = NIL_RTFILE;
    }

#elif defined(RT_OS_WINDOWS)
    CloseHandle(pThis->hEventRecv);
    CloseHandle(pThis->hEventSend);
    CancelIo(pThis->hDeviceFile);
    CloseHandle(pThis->hDeviceFile);

#endif

    if (pThis->pszDevicePath)
    {
        MMR3HeapFree(pThis->pszDevicePath);
        pThis->pszDevicePath = NULL;
    }
}

/**
 * Construct a char driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvHostSerialConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF1(fFlags);
    PDRVHOSTSERIAL pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTSERIAL);
    LogFlow(("%s: iInstance=%d\n", __FUNCTION__, pDrvIns->iInstance));
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    /*
     * Init basic data members and interfaces.
     */
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    pThis->hDeviceFile  = NIL_RTFILE;
# ifdef RT_OS_DARWIN
    pThis->hDeviceFileR = NIL_RTFILE;
# endif
    pThis->hWakeupPipeR = NIL_RTPIPE;
    pThis->hWakeupPipeW = NIL_RTPIPE;
#elif defined(RT_OS_WINDOWS)
    pThis->hEventRecv  = INVALID_HANDLE_VALUE;
    pThis->hEventSend  = INVALID_HANDLE_VALUE;
    pThis->hDeviceFile = INVALID_HANDLE_VALUE;
#endif
    pThis->SendSem     = NIL_RTSEMEVENT;
    /* IBase. */
    pDrvIns->IBase.pfnQueryInterface        = drvHostSerialQueryInterface;
    /* ICharConnector. */
    pThis->ICharConnector.pfnWrite          = drvHostSerialWrite;
    pThis->ICharConnector.pfnSetParameters  = drvHostSerialSetParameters;
    pThis->ICharConnector.pfnSetModemLines  = drvHostSerialSetModemLines;
    pThis->ICharConnector.pfnSetBreak       = drvHostSerialSetBreak;

    /*
     * Query configuration.
     */
    /* Device */
    int rc = CFGMR3QueryStringAlloc(pCfg, "DevicePath", &pThis->pszDevicePath);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: query for \"DevicePath\" string returned %Rra.\n", rc));
        return rc;
    }

    /*
     * Open the device
     */
#ifdef RT_OS_WINDOWS

    pThis->hHaltEventSem = CreateEvent(NULL, FALSE, FALSE, NULL);
    AssertReturn(pThis->hHaltEventSem != NULL, VERR_NO_MEMORY);

    pThis->hEventRecv = CreateEvent(NULL, FALSE, FALSE, NULL);
    AssertReturn(pThis->hEventRecv != NULL, VERR_NO_MEMORY);

    pThis->hEventSend = CreateEvent(NULL, FALSE, FALSE, NULL);
    AssertReturn(pThis->hEventSend != NULL, VERR_NO_MEMORY);

    HANDLE hFile = CreateFile(pThis->pszDevicePath,
                              GENERIC_READ | GENERIC_WRITE,
                              0, // must be opened with exclusive access
                              NULL, // no SECURITY_ATTRIBUTES structure
                              OPEN_EXISTING, // must use OPEN_EXISTING
                              FILE_FLAG_OVERLAPPED, // overlapped I/O
                              NULL); // no template file
    if (hFile == INVALID_HANDLE_VALUE)
        rc = RTErrConvertFromWin32(GetLastError());
    else
    {
        pThis->hDeviceFile = hFile;
        /* for overlapped read */
        if (!SetCommMask(hFile, EV_RXCHAR | EV_CTS | EV_DSR | EV_RING | EV_RLSD))
        {
            LogRel(("HostSerial#%d: SetCommMask failed with error %d.\n", pDrvIns->iInstance, GetLastError()));
            return VERR_FILE_IO_ERROR;
        }
        rc = VINF_SUCCESS;
    }

#else /* !RT_OS_WINDOWS */

    uint32_t fOpen = RTFILE_O_READWRITE | RTFILE_O_OPEN | RTFILE_O_DENY_NONE;
# ifdef RT_OS_LINUX
    /* This seems to be necessary on some Linux hosts, otherwise we hang here forever. */
    fOpen |= RTFILE_O_NON_BLOCK;
# endif
    rc = RTFileOpen(&pThis->hDeviceFile, pThis->pszDevicePath, fOpen);
# ifdef RT_OS_LINUX
    /* RTFILE_O_NON_BLOCK not supported? */
    if (rc == VERR_INVALID_PARAMETER)
        rc = RTFileOpen(&pThis->hDeviceFile, pThis->pszDevicePath, fOpen & ~RTFILE_O_NON_BLOCK);
# endif
# ifdef RT_OS_DARWIN
    if (RT_SUCCESS(rc))
        rc = RTFileOpen(&pThis->hDeviceFileR, pThis->pszDevicePath, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
# endif


#endif /* !RT_OS_WINDOWS */

    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Could not open host device %s, rc=%Rrc\n", pThis->pszDevicePath, rc));
        switch (rc)
        {
            case VERR_ACCESS_DENIED:
                return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
                                           N_("Cannot open host device '%s' for read/write access. Check the permissions "
                                              "of that device ('/bin/ls -l %s'): Most probably you need to be member "
                                              "of the device group. Make sure that you logout/login after changing "
                                              "the group settings of the current user"),
#else
                                           N_("Cannot open host device '%s' for read/write access. Check the permissions "
                                              "of that device"),
#endif
                                           pThis->pszDevicePath, pThis->pszDevicePath);
           default:
                return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                           N_("Failed to open host device '%s'"),
                                           pThis->pszDevicePath);
        }
    }

    /* Set to non blocking I/O */
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)

    fcntl(RTFileToNative(pThis->hDeviceFile), F_SETFL, O_NONBLOCK);
# ifdef RT_OS_DARWIN
    fcntl(RTFileToNative(pThis->hDeviceFileR), F_SETFL, O_NONBLOCK);
# endif
    rc = RTPipeCreate(&pThis->hWakeupPipeR, &pThis->hWakeupPipeW, 0 /*fFlags*/);
    AssertRCReturn(rc, rc);

#elif defined(RT_OS_WINDOWS)

    /* Set the COMMTIMEOUTS to get non blocking I/O */
    COMMTIMEOUTS comTimeout;

    comTimeout.ReadIntervalTimeout         = MAXDWORD;
    comTimeout.ReadTotalTimeoutMultiplier  = 0;
    comTimeout.ReadTotalTimeoutConstant    = 0;
    comTimeout.WriteTotalTimeoutMultiplier = 0;
    comTimeout.WriteTotalTimeoutConstant   = 0;

    SetCommTimeouts(pThis->hDeviceFile, &comTimeout);

#endif

    /*
     * Get the ICharPort interface of the above driver/device.
     */
    pThis->pDrvCharPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMICHARPORT);
    if (!pThis->pDrvCharPort)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_PDM_MISSING_INTERFACE_ABOVE, RT_SRC_POS, N_("HostSerial#%d has no char port interface above"), pDrvIns->iInstance);

    /*
     * Create the receive, send and monitor threads plus the related send semaphore.
     */
    rc = PDMDrvHlpThreadCreate(pDrvIns, &pThis->pRecvThread, pThis, drvHostSerialRecvThread, drvHostSerialWakeupRecvThread, 0, RTTHREADTYPE_IO, "SerRecv");
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, N_("HostSerial#%d cannot create receive thread"), pDrvIns->iInstance);

    rc = RTSemEventCreate(&pThis->SendSem);
    AssertRC(rc);

    rc = PDMDrvHlpThreadCreate(pDrvIns, &pThis->pSendThread, pThis, drvHostSerialSendThread, drvHostSerialWakeupSendThread, 0, RTTHREADTYPE_IO, "SerSend");
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, N_("HostSerial#%d cannot create send thread"), pDrvIns->iInstance);

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    /* Linux & darwin needs a separate thread which monitors the status lines. */
    int rcPsx = ioctl(RTFileToNative(pThis->hDeviceFile), TIOCMGET, &pThis->fStatusLines);
    if (!rcPsx)
    {
        rc = PDMDrvHlpThreadCreate(pDrvIns, &pThis->pMonitorThread, pThis, drvHostSerialMonitorThread, drvHostSerialWakeupMonitorThread, 0, RTTHREADTYPE_IO, "SerMon");
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS, N_("HostSerial#%d cannot create monitor thread"), pDrvIns->iInstance);
    }
    else
    {
        /* TIOCMGET is not supported for pseudo terminals so just silently skip it. */
        if (errno != ENOTTY)
            PDMDrvHlpVMSetRuntimeError(pDrvIns, 0 /*fFlags*/, "DrvHostSerialFail",
                                       N_("Trying to get the status lines state failed for serial host device '%s' (%Rrc). The device will not work properly"),
                                       pThis->pszDevicePath, RTErrConvertFromErrno(errno));
    }
#endif

    /*
     * Register release statistics.
     */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesWritten,    STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_BYTES, "Nr of bytes written",         "/Devices/HostSerial%d/Written", pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesRead,       STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_BYTES, "Nr of bytes read",            "/Devices/HostSerial%d/Read", pDrvIns->iInstance);
#ifdef RT_OS_DARWIN /* new Write code, not darwin specific. */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatSendOverflows,   STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_BYTES, "Nr of bytes overflowed",      "/Devices/HostSerial%d/SendOverflow", pDrvIns->iInstance);
#endif

    return VINF_SUCCESS;
}

/**
 * Char driver registration record.
 */
const PDMDRVREG g_DrvHostSerial =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "Host Serial",
        /* szRCMod */
    "",
    /* szR0Mod */
    "",
/* pszDescription */
    "Host serial driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_CHAR,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTSERIAL),
    /* pfnConstruct */
    drvHostSerialConstruct,
    /* pfnDestruct */
    drvHostSerialDestruct,
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

