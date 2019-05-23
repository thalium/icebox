/* $Id: HGCMThread.h $ */
/** @file
 * HGCMThread - Host-Guest Communication Manager worker threads header.
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

#ifndef __HGCMThread_h__
#define __HGCMThread_h__

#include <VBox/types.h>

#include "HGCMObjects.h"

/** A handle for HGCM message. */
typedef uint32_t HGCMMSGHANDLE;

/** A handle for HGCM worker threads. */
typedef uint32_t HGCMTHREADHANDLE;

/* Forward declaration of message core class. */
class HGCMMsgCore;

/** @todo comment */

typedef HGCMMsgCore *FNHGCMNEWMSGALLOC(uint32_t u32MsgId);
typedef FNHGCMNEWMSGALLOC *PFNHGCMNEWMSGALLOC;

/** Function that is called after message processing by worker thread,
 *  or if an error occurred during message handling after successfully
 *  posting (hgcmMsgPost) the message to worker thread.
 *
 * @param result    Return code either from the service which actually processed the message
 *                  or from HGCM.
 * @param pMsgCore  Pointer to just processed message.
 */
typedef DECLCALLBACK(void) HGCMMSGCALLBACK (int32_t result, HGCMMsgCore *pMsgCore);
typedef HGCMMSGCALLBACK *PHGCMMSGCALLBACK;

/* Forward declaration of the worker thread class. */
class HGCMThread;

/** HGCM core message. */
class HGCMMsgCore: public HGCMObject
{
    private:
        friend class HGCMThread;

        /** Version of message header. */
        uint32_t m_u32Version;

        /** Thread the message belongs to, referenced by the message. */
        HGCMThread *m_pThread;

        /** Message number/identifier. */
        uint32_t m_u32Msg;

        /** Callback function pointer. */
        PHGCMMSGCALLBACK m_pfnCallback;

        /** Next element in a message queue. */
        HGCMMsgCore *m_pNext;
        /** @todo seems not necessary. Previous element in a message queue. */
        HGCMMsgCore *m_pPrev;

        /** Various internal flags. */
        uint32_t m_fu32Flags;

        /** Result code for a Send */
        int32_t m_rcSend;

        void InitializeCore (uint32_t u32MsgId, HGCMTHREADHANDLE hThread);

    protected:
        virtual ~HGCMMsgCore ();

    public:
        HGCMMsgCore () : HGCMObject(HGCMOBJ_MSG) {};

        uint32_t MsgId (void) { return m_u32Msg; };

        HGCMThread *Thread (void) { return m_pThread; };

        /** Initialize message after it was allocated. */
        virtual void Initialize (void) {};

        /** Uninitialize message. */
        virtual void Uninitialize (void) {};

};


/** HGCM worker thread function.
 *
 *  @param ThreadHandle  Handle of the thread.
 *  @param pvUser        User specified thread parameter.
 */
typedef DECLCALLBACK(void) FNHGCMTHREAD (HGCMTHREADHANDLE ThreadHandle, void *pvUser);
typedef FNHGCMTHREAD *PFNHGCMTHREAD;


/**
 * Thread API.
 * Based on thread handles. Internals of a thread are not exposed to users.
 */

/** Initialize threads.
 *
 * @return VBox error code
 */
int hgcmThreadInit (void);
void hgcmThreadUninit (void);


/** Create a HGCM worker thread.
 *
 * @param pHandle       Where to store the returned worker thread handle.
 * @param pszThreadName Name of the thread, needed by runtime.
 * @param pfnThread     The worker thread function.
 * @param pvUser        A pointer passed to worker thread.
 *
 * @return VBox error code
 */
int hgcmThreadCreate (HGCMTHREADHANDLE *pHandle, const char *pszThreadName, PFNHGCMTHREAD pfnThread, void *pvUser);

/** Wait for termination of a HGCM worker thread.
 *
 * @param handle The HGCM thread handle.
 *
 * @return VBox error code
 */
int hgcmThreadWait (HGCMTHREADHANDLE handle);

/** Allocate a message to be posted to HGCM worker thread.
 *
 * @param hThread       Worker thread handle.
 * @param pHandle       Where to store the returned message handle.
 * @param u32MsgId      Message identifier.
 * @param pfnNewMessage New message allocation callback.
 *
 * @return VBox error code
 */
int hgcmMsgAlloc (HGCMTHREADHANDLE hThread, HGCMMSGHANDLE *pHandle, uint32_t u32MsgId, PFNHGCMNEWMSGALLOC pfnNewMessage);

/** Post a message to HGCM worker thread.
 *
 * @param hMsg          Message handle.
 * @param pfnCallback   Message completion callback.
 *
 * @return VBox error code
 */
int hgcmMsgPost (HGCMMSGHANDLE hMsg, PHGCMMSGCALLBACK pfnCallback);

/** Send a message to HGCM worker thread.
 *  The function will return after message is processed by thread.
 *
 * @param hMsg          Message handle.
 *
 * @return VBox error code
 */
int hgcmMsgSend (HGCMMSGHANDLE hMsg);


/* Wait for and get a message.
 *
 * @param hThread       The thread handle.
 * @param ppMsg         Where to store returned message pointer.
 *
 * @return VBox error code
 *
 * @thread worker thread
 */
int hgcmMsgGet (HGCMTHREADHANDLE hThread, HGCMMsgCore **ppMsg);


/** Worker thread has processed a message previously obtained with hgcmMsgGet.
 *
 * @param pMsg          Processed message pointer.
 * @param result        Result code, VBox erro code.
 *
 * @return VBox error code
 *
 * @thread worker thread
 */
void hgcmMsgComplete (HGCMMsgCore *pMsg, int32_t result);


#endif /* __HGCMThread_h__ */
