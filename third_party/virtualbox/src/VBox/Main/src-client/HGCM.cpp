/* $Id: HGCM.cpp $ */
/** @file
 * HGCM (Host-Guest Communication Manager)
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

#define LOG_GROUP LOG_GROUP_HGCM
#include "LoggingNew.h"

#include "HGCM.h"
#include "HGCMThread.h"

#include <VBox/err.h>
#include <VBox/hgcmsvc.h>
#include <VBox/vmm/ssm.h>
#include <VBox/sup.h>

#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/avl.h>
#include <iprt/critsect.h>
#include <iprt/asm.h>
#include <iprt/ldr.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>

#include <VBox/VMMDev.h>

/**
 * A service gets one thread, which synchronously delivers messages to
 * the service. This is good for serialization.
 *
 * Some services may want to process messages asynchronously, and will want
 * a next message to be delivered, while a previous message is still being
 * processed.
 *
 * The dedicated service thread delivers a next message when service
 * returns after fetching a previous one. The service will call a message
 * completion callback when message is actually processed. So returning
 * from the service call means only that the service is processing message.
 *
 * 'Message processed' condition is indicated by service, which call the
 * callback, even if the callback is called synchronously in the dedicated
 * thread.
 *
 * This message completion callback is only valid for Call requests.
 * Connect and Disconnect are processed synchronously by the service.
 */


/* The maximum allowed size of a service name in bytes. */
#define VBOX_HGCM_SVC_NAME_MAX_BYTES 1024

struct _HGCMSVCEXTHANDLEDATA
{
    char *pszServiceName;
    /* The service name follows. */
};

/** Internal helper service object. HGCM code would use it to
 *  hold information about services and communicate with services.
 *  The HGCMService is an (in future) abstract class that implements
 *  common functionality. There will be derived classes for specific
 *  service types.
 */

class HGCMService
{
    private:
        VBOXHGCMSVCHELPERS m_svcHelpers;

        static HGCMService *sm_pSvcListHead;
        static HGCMService *sm_pSvcListTail;

        static int sm_cServices;

        HGCMTHREADHANDLE m_thread;
        friend DECLCALLBACK(void) hgcmServiceThread(HGCMTHREADHANDLE ThreadHandle, void *pvUser);

        uint32_t volatile m_u32RefCnt;

        HGCMService *m_pSvcNext;
        HGCMService *m_pSvcPrev;

        char *m_pszSvcName;
        char *m_pszSvcLibrary;

        RTLDRMOD m_hLdrMod;
        PFNVBOXHGCMSVCLOAD m_pfnLoad;

        VBOXHGCMSVCFNTABLE m_fntable;

        uint32_t m_cClients;
        uint32_t m_cClientsAllocated;

        uint32_t *m_paClientIds;

#ifdef VBOX_WITH_CRHGSMI
        uint32_t m_cHandleAcquires;
#endif

        HGCMSVCEXTHANDLE m_hExtension;

        int loadServiceDLL(void);
        void unloadServiceDLL(void);

        /*
         * Main HGCM thread methods.
         */
        int instanceCreate(const char *pszServiceLibrary, const char *pszServiceName);
        void instanceDestroy(void);

        int saveClientState(uint32_t u32ClientId, PSSMHANDLE pSSM);
        int loadClientState(uint32_t u32ClientId, PSSMHANDLE pSSM);

        HGCMService();
        ~HGCMService() {};

        static DECLCALLBACK(void) svcHlpCallComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc);
        static DECLCALLBACK(void) svcHlpDisconnectClient(void *pvInstance, uint32_t u32ClientId);

    public:

        /*
         * Main HGCM thread methods.
         */
        static int LoadService(const char *pszServiceLibrary, const char *pszServiceName);
        void UnloadService(void);

        static void UnloadAll(void);

        static int ResolveService(HGCMService **ppsvc, const char *pszServiceName);
        void ReferenceService(void);
        void ReleaseService(void);

        static void Reset(void);

        static int SaveState(PSSMHANDLE pSSM);
        static int LoadState(PSSMHANDLE pSSM);

        int CreateAndConnectClient(uint32_t *pu32ClientIdOut, uint32_t u32ClientIdIn);
        int DisconnectClient(uint32_t u32ClientId, bool fFromService);

        int HostCall(uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM *paParms);

#ifdef VBOX_WITH_CRHGSMI
        int HandleAcquired();
        int HandleReleased();
        int HostFastCallAsync(uint32_t u32Function, VBOXHGCMSVCPARM *pParm, PHGCMHOSTFASTCALLCB pfnCompletion,
                              void *pvCompletion);
#endif

        uint32_t SizeOfClient(void) { return m_fntable.cbClient; };

        int RegisterExtension(HGCMSVCEXTHANDLE handle, PFNHGCMSVCEXT pfnExtension, void *pvExtension);
        void UnregisterExtension(HGCMSVCEXTHANDLE handle);

        /*
         * The service thread methods.
         */

        int GuestCall(PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmd, uint32_t u32ClientId,
                      uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM aParms[]);
};


class HGCMClient: public HGCMObject
{
    public:
        HGCMClient() : HGCMObject(HGCMOBJ_CLIENT), pService(NULL),
                        pvData(NULL) {};
        ~HGCMClient();

        int Init(HGCMService *pSvc);

        /** Service that the client is connected to. */
        HGCMService *pService;

        /** Client specific data. */
        void *pvData;
};

HGCMClient::~HGCMClient()
{
    if (pService->SizeOfClient() > 0)
        RTMemFree(pvData);
}

int HGCMClient::Init(HGCMService *pSvc)
{
    pService = pSvc;

    if (pService->SizeOfClient() > 0)
    {
        pvData = RTMemAllocZ(pService->SizeOfClient());

        if (!pvData)
        {
           return VERR_NO_MEMORY;
        }
    }

    return VINF_SUCCESS;
}


#define HGCM_CLIENT_DATA(pService, pClient)(pClient->pvData)



HGCMService *HGCMService::sm_pSvcListHead = NULL;
HGCMService *HGCMService::sm_pSvcListTail = NULL;
int HGCMService::sm_cServices = 0;

HGCMService::HGCMService()
    :
    m_thread     (0),
    m_u32RefCnt  (0),
    m_pSvcNext   (NULL),
    m_pSvcPrev   (NULL),
    m_pszSvcName (NULL),
    m_pszSvcLibrary (NULL),
    m_hLdrMod    (NIL_RTLDRMOD),
    m_pfnLoad    (NULL),
    m_cClients   (0),
    m_cClientsAllocated (0),
    m_paClientIds (NULL),
#ifdef VBOX_WITH_CRHGSMI
    m_cHandleAcquires (0),
#endif
    m_hExtension (NULL)
{
    RT_ZERO(m_fntable);
}


static bool g_fResetting = false;
static bool g_fSaveState = false;


/** Helper function to load a local service DLL.
 *
 *  @return VBox code
 */
int HGCMService::loadServiceDLL(void)
{
    LogFlowFunc(("m_pszSvcLibrary = %s\n", m_pszSvcLibrary));

    if (m_pszSvcLibrary == NULL)
    {
        return VERR_INVALID_PARAMETER;
    }

    RTERRINFOSTATIC ErrInfo;
    RTErrInfoInitStatic(&ErrInfo);

    int rc;

    if (RTPathHasPath(m_pszSvcLibrary))
        rc = SUPR3HardenedLdrLoadPlugIn(m_pszSvcLibrary, &m_hLdrMod, &ErrInfo.Core);
    else
        rc = SUPR3HardenedLdrLoadAppPriv(m_pszSvcLibrary, &m_hLdrMod, RTLDRLOAD_FLAGS_LOCAL, &ErrInfo.Core);

    if (RT_SUCCESS(rc))
    {
        LogFlowFunc(("successfully loaded the library.\n"));

        m_pfnLoad = NULL;

        rc = RTLdrGetSymbol(m_hLdrMod, VBOX_HGCM_SVCLOAD_NAME, (void**)&m_pfnLoad);

        if (RT_FAILURE(rc) || !m_pfnLoad)
        {
            Log(("HGCMService::loadServiceDLL: Error resolving the service entry point %s, rc = %d, m_pfnLoad = %p\n",
                 VBOX_HGCM_SVCLOAD_NAME, rc, m_pfnLoad));

            if (RT_SUCCESS(rc))
            {
                /* m_pfnLoad was NULL */
                rc = VERR_SYMBOL_NOT_FOUND;
            }
        }

        if (RT_SUCCESS(rc))
        {
            RT_ZERO(m_fntable);

            m_fntable.cbSize     = sizeof(m_fntable);
            m_fntable.u32Version = VBOX_HGCM_SVC_VERSION;
            m_fntable.pHelpers   = &m_svcHelpers;

            rc = m_pfnLoad(&m_fntable);

            LogFlowFunc(("m_pfnLoad rc = %Rrc\n", rc));

            if (RT_SUCCESS(rc))
            {
                if (   m_fntable.pfnUnload == NULL
                    || m_fntable.pfnConnect == NULL
                    || m_fntable.pfnDisconnect == NULL
                    || m_fntable.pfnCall == NULL
                   )
                {
                    Log(("HGCMService::loadServiceDLL: at least one of function pointers is NULL\n"));

                    rc = VERR_INVALID_PARAMETER;

                    if (m_fntable.pfnUnload)
                    {
                        m_fntable.pfnUnload(m_fntable.pvService);
                    }
                }
            }
        }
    }
    else
    {
        LogRel(("HGCM: Failed to load the service library: [%s], rc = %Rrc - %s. The service will be not available.\n",
                m_pszSvcLibrary, rc, ErrInfo.Core.pszMsg));
        m_hLdrMod = NIL_RTLDRMOD;
    }

    if (RT_FAILURE(rc))
    {
        unloadServiceDLL();
    }

    return rc;
}

/** Helper function to free a local service DLL.
 *
 *  @return VBox code
 */
void HGCMService::unloadServiceDLL(void)
{
    if (m_hLdrMod)
    {
        RTLdrClose(m_hLdrMod);
    }

    RT_ZERO(m_fntable);
    m_pfnLoad = NULL;
    m_hLdrMod = NIL_RTLDRMOD;
}

/*
 * Messages processed by service threads. These threads only call the service entry points.
 */

#define SVC_MSG_LOAD       (0)  /* Load the service library and call VBOX_HGCM_SVCLOAD_NAME entry point. */
#define SVC_MSG_UNLOAD     (1)  /* call pfnUnload and unload the service library. */
#define SVC_MSG_CONNECT    (2)  /* pfnConnect */
#define SVC_MSG_DISCONNECT (3)  /* pfnDisconnect */
#define SVC_MSG_GUESTCALL  (4)  /* pfnGuestCall */
#define SVC_MSG_HOSTCALL   (5)  /* pfnHostCall */
#define SVC_MSG_LOADSTATE  (6)  /* pfnLoadState. */
#define SVC_MSG_SAVESTATE  (7)  /* pfnSaveState. */
#define SVC_MSG_QUIT       (8)  /* Terminate the thread. */
#define SVC_MSG_REGEXT     (9)  /* pfnRegisterExtension */
#define SVC_MSG_UNREGEXT   (10) /* pfnRegisterExtension */
#ifdef VBOX_WITH_CRHGSMI
# define SVC_MSG_HOSTFASTCALLASYNC (21) /* pfnHostCall */
#endif

class HGCMMsgSvcLoad: public HGCMMsgCore
{
};

class HGCMMsgSvcUnload: public HGCMMsgCore
{
};

class HGCMMsgSvcConnect: public HGCMMsgCore
{
    public:
        /* client identifier */
        uint32_t u32ClientId;
};

class HGCMMsgSvcDisconnect: public HGCMMsgCore
{
    public:
        /* client identifier */
        uint32_t u32ClientId;
};

class HGCMMsgHeader: public HGCMMsgCore
{
    public:
        HGCMMsgHeader() : pCmd(NULL), pHGCMPort(NULL) {};

        /* Command pointer/identifier. */
        PVBOXHGCMCMD pCmd;

        /* Port to be informed on message completion. */
        PPDMIHGCMPORT pHGCMPort;
};


class HGCMMsgCall: public HGCMMsgHeader
{
    public:
        /* client identifier */
        uint32_t u32ClientId;

        /* function number */
        uint32_t u32Function;

        /* number of parameters */
        uint32_t cParms;

        VBOXHGCMSVCPARM *paParms;
};

class HGCMMsgLoadSaveStateClient: public HGCMMsgCore
{
    public:
        uint32_t    u32ClientId;
        PSSMHANDLE  pSSM;
};

class HGCMMsgHostCallSvc: public HGCMMsgCore
{
    public:
        /* function number */
        uint32_t u32Function;

        /* number of parameters */
        uint32_t cParms;

        VBOXHGCMSVCPARM *paParms;
};

class HGCMMsgSvcRegisterExtension: public HGCMMsgCore
{
    public:
        /* Handle of the extension to be registered. */
        HGCMSVCEXTHANDLE handle;
        /* The extension entry point. */
        PFNHGCMSVCEXT pfnExtension;
        /* The extension pointer. */
        void *pvExtension;
};

class HGCMMsgSvcUnregisterExtension: public HGCMMsgCore
{
    public:
        /* Handle of the registered extension. */
        HGCMSVCEXTHANDLE handle;
};

#ifdef VBOX_WITH_CRHGSMI
class HGCMMsgHostFastCallAsyncSvc: public HGCMMsgCore
{
    public:
        /* function number */
        uint32_t u32Function;
        /* parameter */
        VBOXHGCMSVCPARM Param;
        /* completion info */
        PHGCMHOSTFASTCALLCB pfnCompletion;
        void *pvCompletion;
};
#endif

static HGCMMsgCore *hgcmMessageAllocSvc(uint32_t u32MsgId)
{
    switch (u32MsgId)
    {
#ifdef VBOX_WITH_CRHGSMI
        case SVC_MSG_HOSTFASTCALLASYNC: return new HGCMMsgHostFastCallAsyncSvc();
#endif
        case SVC_MSG_LOAD:        return new HGCMMsgSvcLoad();
        case SVC_MSG_UNLOAD:      return new HGCMMsgSvcUnload();
        case SVC_MSG_CONNECT:     return new HGCMMsgSvcConnect();
        case SVC_MSG_DISCONNECT:  return new HGCMMsgSvcDisconnect();
        case SVC_MSG_HOSTCALL:    return new HGCMMsgHostCallSvc();
        case SVC_MSG_GUESTCALL:   return new HGCMMsgCall();
        case SVC_MSG_LOADSTATE:
        case SVC_MSG_SAVESTATE:   return new HGCMMsgLoadSaveStateClient();
        case SVC_MSG_REGEXT:      return new HGCMMsgSvcRegisterExtension();
        case SVC_MSG_UNREGEXT:    return new HGCMMsgSvcUnregisterExtension();
        default:
            AssertReleaseMsgFailed(("Msg id = %08X\n", u32MsgId));
    }

    return NULL;
}

/*
 * The service thread. Loads the service library and calls the service entry points.
 */
DECLCALLBACK(void) hgcmServiceThread(HGCMTHREADHANDLE ThreadHandle, void *pvUser)
{
    HGCMService *pSvc = (HGCMService *)pvUser;
    AssertRelease(pSvc != NULL);

    bool fQuit = false;

    while (!fQuit)
    {
        HGCMMsgCore *pMsgCore;
        int rc = hgcmMsgGet(ThreadHandle, &pMsgCore);

        if (RT_FAILURE(rc))
        {
            /* The error means some serious unrecoverable problem in the hgcmMsg/hgcmThread layer. */
            AssertMsgFailed(("%Rrc\n", rc));
            break;
        }

        /* Cache required information to avoid unnecessary pMsgCore access. */
        uint32_t u32MsgId = pMsgCore->MsgId();

        switch (u32MsgId)
        {
#ifdef VBOX_WITH_CRHGSMI
            case SVC_MSG_HOSTFASTCALLASYNC:
            {
                HGCMMsgHostFastCallAsyncSvc *pMsg = (HGCMMsgHostFastCallAsyncSvc *)pMsgCore;

                LogFlowFunc(("SVC_MSG_HOSTFASTCALLASYNC u32Function = %d, pParm = %p\n", pMsg->u32Function, &pMsg->Param));

                rc = pSvc->m_fntable.pfnHostCall(pSvc->m_fntable.pvService, pMsg->u32Function, 1, &pMsg->Param);
            } break;
#endif
            case SVC_MSG_LOAD:
            {
                LogFlowFunc(("SVC_MSG_LOAD\n"));
                rc = pSvc->loadServiceDLL();
            } break;

            case SVC_MSG_UNLOAD:
            {
                LogFlowFunc(("SVC_MSG_UNLOAD\n"));
                if (pSvc->m_fntable.pfnUnload)
                {
                    pSvc->m_fntable.pfnUnload(pSvc->m_fntable.pvService);
                }

                pSvc->unloadServiceDLL();
                fQuit = true;
            } break;

            case SVC_MSG_CONNECT:
            {
                HGCMMsgSvcConnect *pMsg = (HGCMMsgSvcConnect *)pMsgCore;

                LogFlowFunc(("SVC_MSG_CONNECT u32ClientId = %d\n", pMsg->u32ClientId));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                if (pClient)
                {
                    rc = pSvc->m_fntable.pfnConnect(pSvc->m_fntable.pvService, pMsg->u32ClientId,
                                                    HGCM_CLIENT_DATA(pSvc, pClient));

                    hgcmObjDereference(pClient);
                }
                else
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                }
            } break;

            case SVC_MSG_DISCONNECT:
            {
                HGCMMsgSvcDisconnect *pMsg = (HGCMMsgSvcDisconnect *)pMsgCore;

                LogFlowFunc(("SVC_MSG_DISCONNECT u32ClientId = %d\n", pMsg->u32ClientId));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                if (pClient)
                {
                    rc = pSvc->m_fntable.pfnDisconnect(pSvc->m_fntable.pvService, pMsg->u32ClientId,
                                                       HGCM_CLIENT_DATA(pSvc, pClient));

                    hgcmObjDereference(pClient);
                }
                else
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                }
            } break;

            case SVC_MSG_GUESTCALL:
            {
                HGCMMsgCall *pMsg = (HGCMMsgCall *)pMsgCore;

                LogFlowFunc(("SVC_MSG_GUESTCALL u32ClientId = %d, u32Function = %d, cParms = %d, paParms = %p\n",
                             pMsg->u32ClientId, pMsg->u32Function, pMsg->cParms, pMsg->paParms));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                if (pClient)
                {
                    pSvc->m_fntable.pfnCall(pSvc->m_fntable.pvService, (VBOXHGCMCALLHANDLE)pMsg, pMsg->u32ClientId,
                                            HGCM_CLIENT_DATA(pSvc, pClient), pMsg->u32Function,
                                            pMsg->cParms, pMsg->paParms);

                    hgcmObjDereference(pClient);
                }
                else
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                }
            } break;

            case SVC_MSG_HOSTCALL:
            {
                HGCMMsgHostCallSvc *pMsg = (HGCMMsgHostCallSvc *)pMsgCore;

                LogFlowFunc(("SVC_MSG_HOSTCALL u32Function = %d, cParms = %d, paParms = %p\n",
                             pMsg->u32Function, pMsg->cParms, pMsg->paParms));

                rc = pSvc->m_fntable.pfnHostCall(pSvc->m_fntable.pvService, pMsg->u32Function, pMsg->cParms, pMsg->paParms);
            } break;

            case SVC_MSG_LOADSTATE:
            {
                HGCMMsgLoadSaveStateClient *pMsg = (HGCMMsgLoadSaveStateClient *)pMsgCore;

                LogFlowFunc(("SVC_MSG_LOADSTATE\n"));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                if (pClient)
                {
                    if (pSvc->m_fntable.pfnLoadState)
                    {
                        rc = pSvc->m_fntable.pfnLoadState(pSvc->m_fntable.pvService, pMsg->u32ClientId,
                                                          HGCM_CLIENT_DATA(pSvc, pClient), pMsg->pSSM);
                    }

                    hgcmObjDereference(pClient);
                }
                else
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                }
            } break;

            case SVC_MSG_SAVESTATE:
            {
                HGCMMsgLoadSaveStateClient *pMsg = (HGCMMsgLoadSaveStateClient *)pMsgCore;

                LogFlowFunc(("SVC_MSG_SAVESTATE\n"));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                rc = VINF_SUCCESS;

                if (pClient)
                {
                    if (pSvc->m_fntable.pfnSaveState)
                    {
                        g_fSaveState = true;
                        rc = pSvc->m_fntable.pfnSaveState(pSvc->m_fntable.pvService, pMsg->u32ClientId,
                                                          HGCM_CLIENT_DATA(pSvc, pClient), pMsg->pSSM);
                        g_fSaveState = false;
                    }

                    hgcmObjDereference(pClient);
                }
                else
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                }
            } break;

            case SVC_MSG_REGEXT:
            {
                HGCMMsgSvcRegisterExtension *pMsg = (HGCMMsgSvcRegisterExtension *)pMsgCore;

                LogFlowFunc(("SVC_MSG_REGEXT handle = %p, pfn = %p\n", pMsg->handle, pMsg->pfnExtension));

                if (pSvc->m_hExtension)
                {
                    rc = VERR_NOT_SUPPORTED;
                }
                else
                {
                    if (pSvc->m_fntable.pfnRegisterExtension)
                    {
                        rc = pSvc->m_fntable.pfnRegisterExtension(pSvc->m_fntable.pvService, pMsg->pfnExtension,
                                                                  pMsg->pvExtension);
                    }
                    else
                    {
                        rc = VERR_NOT_SUPPORTED;
                    }

                    if (RT_SUCCESS(rc))
                    {
                        pSvc->m_hExtension = pMsg->handle;
                    }
                }
            } break;

            case SVC_MSG_UNREGEXT:
            {
                HGCMMsgSvcUnregisterExtension *pMsg = (HGCMMsgSvcUnregisterExtension *)pMsgCore;

                LogFlowFunc(("SVC_MSG_UNREGEXT handle = %p\n", pMsg->handle));

                if (pSvc->m_hExtension != pMsg->handle)
                {
                    rc = VERR_NOT_SUPPORTED;
                }
                else
                {
                    if (pSvc->m_fntable.pfnRegisterExtension)
                    {
                        rc = pSvc->m_fntable.pfnRegisterExtension(pSvc->m_fntable.pvService, NULL, NULL);
                    }
                    else
                    {
                        rc = VERR_NOT_SUPPORTED;
                    }

                    pSvc->m_hExtension = NULL;
                }
            } break;

            default:
            {
                AssertMsgFailed(("hgcmServiceThread::Unsupported message number %08X\n", u32MsgId));
                rc = VERR_NOT_SUPPORTED;
            } break;
        }

        if (u32MsgId != SVC_MSG_GUESTCALL)
        {
            /* For SVC_MSG_GUESTCALL the service calls the completion helper.
             * Other messages have to be completed here.
             */
            hgcmMsgComplete (pMsgCore, rc);
        }
    }
}

/* static */ DECLCALLBACK(void) HGCMService::svcHlpCallComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
   HGCMMsgCore *pMsgCore = (HGCMMsgCore *)callHandle;

   if (pMsgCore->MsgId () == SVC_MSG_GUESTCALL)
   {
       /* Only call the completion for these messages. The helper
        * is called by the service, and the service does not get
        * any other messages.
        */
       hgcmMsgComplete(pMsgCore, rc);
   }
   else
   {
       AssertFailed();
   }
}

/* static */ DECLCALLBACK(void) HGCMService::svcHlpDisconnectClient(void *pvInstance, uint32_t u32ClientId)
{
     HGCMService *pService = static_cast <HGCMService *> (pvInstance);

     if (pService)
     {
         pService->DisconnectClient(u32ClientId, true);
     }
}

static DECLCALLBACK(void) hgcmMsgCompletionCallback(int32_t result, HGCMMsgCore *pMsgCore)
{
    /* Call the VMMDev port interface to issue IRQ notification. */
    HGCMMsgHeader *pMsgHdr = (HGCMMsgHeader *)pMsgCore;

    LogFlow(("MAIN::hgcmMsgCompletionCallback: message %p\n", pMsgCore));

    if (pMsgHdr->pHGCMPort && !g_fResetting)
    {
        pMsgHdr->pHGCMPort->pfnCompleted(pMsgHdr->pHGCMPort, g_fSaveState? VINF_HGCM_SAVE_STATE: result, pMsgHdr->pCmd);
    }
}

/*
 * The main HGCM methods of the service.
 */

int HGCMService::instanceCreate(const char *pszServiceLibrary, const char *pszServiceName)
{
    LogFlowFunc(("name %s, lib %s\n", pszServiceName, pszServiceLibrary));

    /* The maximum length of the thread name, allowed by the RT is 15. */
    char szThreadName[16];
    if (!strncmp(pszServiceName, RT_STR_TUPLE("VBoxShared")))
        RTStrPrintf(szThreadName, sizeof(szThreadName), "Sh%s", pszServiceName + 10);
    else if (!strncmp(pszServiceName, RT_STR_TUPLE("VBox")))
        RTStrCopy(szThreadName, sizeof(szThreadName), pszServiceName + 4);
    else
        RTStrCopy(szThreadName, sizeof(szThreadName), pszServiceName);

    int rc = hgcmThreadCreate(&m_thread, szThreadName, hgcmServiceThread, this);

    if (RT_SUCCESS(rc))
    {
        m_pszSvcName    = RTStrDup(pszServiceName);
        m_pszSvcLibrary = RTStrDup(pszServiceLibrary);

        if (!m_pszSvcName || !m_pszSvcLibrary)
        {
            RTStrFree(m_pszSvcLibrary);
            m_pszSvcLibrary = NULL;

            RTStrFree(m_pszSvcName);
            m_pszSvcName = NULL;

            rc = VERR_NO_MEMORY;
        }
        else
        {
            /* Initialize service helpers table. */
            m_svcHelpers.pfnCallComplete     = svcHlpCallComplete;
            m_svcHelpers.pvInstance          = this;
            m_svcHelpers.pfnDisconnectClient = svcHlpDisconnectClient;

            /* Execute the load request on the service thread. */
            HGCMMSGHANDLE hMsg;
            rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_LOAD, hgcmMessageAllocSvc);

            if (RT_SUCCESS(rc))
            {
                rc = hgcmMsgSend(hMsg);
            }
        }
    }

    if (RT_FAILURE(rc))
    {
        instanceDestroy();
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

void HGCMService::instanceDestroy(void)
{
    LogFlowFunc(("%s\n", m_pszSvcName));

    HGCMMSGHANDLE hMsg;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_UNLOAD, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        rc = hgcmMsgSend(hMsg);

        if (RT_SUCCESS(rc))
        {
            hgcmThreadWait(m_thread);
        }
    }

    RTStrFree(m_pszSvcLibrary);
    m_pszSvcLibrary = NULL;

    RTStrFree(m_pszSvcName);
    m_pszSvcName = NULL;
}

int HGCMService::saveClientState(uint32_t u32ClientId, PSSMHANDLE pSSM)
{
    LogFlowFunc(("%s\n", m_pszSvcName));

    HGCMMSGHANDLE hMsg;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_SAVESTATE, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgLoadSaveStateClient *pMsg = (HGCMMsgLoadSaveStateClient *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->u32ClientId = u32ClientId;
        pMsg->pSSM        = pSSM;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMService::loadClientState(uint32_t u32ClientId, PSSMHANDLE pSSM)
{
    LogFlowFunc(("%s\n", m_pszSvcName));

    HGCMMSGHANDLE hMsg;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_LOADSTATE, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgLoadSaveStateClient *pMsg = (HGCMMsgLoadSaveStateClient *)hgcmObjReference(hMsg, HGCMOBJ_MSG);

        AssertRelease(pMsg);

        pMsg->u32ClientId = u32ClientId;
        pMsg->pSSM        = pSSM;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}


/** The method creates a service and references it.
 *
 * @param pszServiceLibrary  The library to be loaded.
 * @param pszServiceName     The name of the service.
 * @return VBox rc.
 * @thread main HGCM
 */
/* static */ int HGCMService::LoadService(const char *pszServiceLibrary, const char *pszServiceName)
{
    LogFlowFunc(("lib %s, name = %s\n", pszServiceLibrary, pszServiceName));

    /* Look at already loaded services to avoid double loading. */

    HGCMService *pSvc;
    int rc = HGCMService::ResolveService(&pSvc, pszServiceName);

    if (RT_SUCCESS(rc))
    {
        /* The service is already loaded. */
        pSvc->ReleaseService();
        rc = VERR_HGCM_SERVICE_EXISTS;
    }
    else
    {
        /* Create the new service. */
        pSvc = new HGCMService();

        if (!pSvc)
        {
            rc = VERR_NO_MEMORY;
        }
        else
        {
            /* Load the library and call the initialization entry point. */
            rc = pSvc->instanceCreate(pszServiceLibrary, pszServiceName);

            if (RT_SUCCESS(rc))
            {
                /* Insert the just created service to list for future references. */
                pSvc->m_pSvcNext = sm_pSvcListHead;
                pSvc->m_pSvcPrev = NULL;

                if (sm_pSvcListHead)
                {
                    sm_pSvcListHead->m_pSvcPrev = pSvc;
                }
                else
                {
                    sm_pSvcListTail = pSvc;
                }

                sm_pSvcListHead = pSvc;

                sm_cServices++;

                /* Reference the service (for first time) until it is unloaded on HGCM termination. */
                AssertRelease(pSvc->m_u32RefCnt == 0);
                pSvc->ReferenceService();

                LogFlowFunc(("service %p\n", pSvc));
            }
        }
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/** The method unloads a service.
 *
 * @thread main HGCM
 */
void HGCMService::UnloadService(void)
{
    LogFlowFunc(("name = %s\n", m_pszSvcName));

    /* Remove the service from the list. */
    if (m_pSvcNext)
    {
        m_pSvcNext->m_pSvcPrev = m_pSvcPrev;
    }
    else
    {
        sm_pSvcListTail = m_pSvcPrev;
    }

    if (m_pSvcPrev)
    {
        m_pSvcPrev->m_pSvcNext = m_pSvcNext;
    }
    else
    {
        sm_pSvcListHead = m_pSvcNext;
    }

    sm_cServices--;

    /* The service must be unloaded only if all clients were disconnected. */
    LogFlowFunc(("m_u32RefCnt = %d\n", m_u32RefCnt));
    AssertRelease(m_u32RefCnt == 1);

    /* Now the service can be released. */
    ReleaseService();
}

/** The method unloads all services.
 *
 * @thread main HGCM
 */
/* static */ void HGCMService::UnloadAll(void)
{
    while (sm_pSvcListHead)
    {
        sm_pSvcListHead->UnloadService();
    }
}

/** The method obtains a referenced pointer to the service with
 *  specified name. The caller must call ReleaseService when
 *  the pointer is no longer needed.
 *
 * @param ppSvc          Where to store the pointer to the service.
 * @param pszServiceName The name of the service.
 * @return VBox rc.
 * @thread main HGCM
 */
/* static */ int HGCMService::ResolveService(HGCMService **ppSvc, const char *pszServiceName)
{
    LogFlowFunc(("ppSvc = %p name = %s\n",
                 ppSvc, pszServiceName));

    if (!ppSvc || !pszServiceName)
    {
        return VERR_INVALID_PARAMETER;
    }

    HGCMService *pSvc = sm_pSvcListHead;

    while (pSvc)
    {
        if (strcmp(pSvc->m_pszSvcName, pszServiceName) == 0)
        {
            break;
        }

        pSvc = pSvc->m_pSvcNext;
    }

    LogFlowFunc(("lookup in the list is %p\n", pSvc));

    if (pSvc == NULL)
    {
        *ppSvc = NULL;
        return VERR_HGCM_SERVICE_NOT_FOUND;
    }

    pSvc->ReferenceService();

    *ppSvc = pSvc;

    return VINF_SUCCESS;
}

/** The method increases reference counter.
 *
 * @thread main HGCM
 */
void HGCMService::ReferenceService(void)
{
    ASMAtomicIncU32(&m_u32RefCnt);
    LogFlowFunc(("[%s] m_u32RefCnt = %d\n", m_pszSvcName, m_u32RefCnt));
}

/** The method dereferences a service and deletes it when no more refs.
 *
 * @thread main HGCM
 */
void HGCMService::ReleaseService(void)
{
    LogFlowFunc(("m_u32RefCnt = %d\n", m_u32RefCnt));
    uint32_t u32RefCnt = ASMAtomicDecU32(&m_u32RefCnt);
    AssertRelease(u32RefCnt != ~0U);

    LogFlowFunc(("u32RefCnt = %d, name %s\n", u32RefCnt, m_pszSvcName));

    if (u32RefCnt == 0)
    {
        instanceDestroy();
        delete this;
    }
}

/** The method is called when the VM is being reset or terminated
 *  and disconnects all clients from all services.
 *
 * @thread main HGCM
 */
/* static */ void HGCMService::Reset(void)
{
    g_fResetting = true;

    HGCMService *pSvc = sm_pSvcListHead;

    while (pSvc)
    {
        while (pSvc->m_cClients && pSvc->m_paClientIds)
        {
            LogFlowFunc(("handle %d\n", pSvc->m_paClientIds[0]));
            pSvc->DisconnectClient(pSvc->m_paClientIds[0], false);
        }

#ifdef VBOX_WITH_CRHGSMI
        /** @todo could this actually happen that the service is destroyed on ReleaseService? */
        HGCMService *pNextSvc = pSvc->m_pSvcNext;
        while (pSvc->m_cHandleAcquires)
        {
            pSvc->HandleReleased();
            pSvc->ReleaseService();
        }
        pSvc = pNextSvc;
#else
        pSvc = pSvc->m_pSvcNext;
#endif
    }

    g_fResetting = false;
}

/** The method saves the HGCM state.
 *
 * @param pSSM  The saved state context.
 * @return VBox rc.
 * @thread main HGCM
 */
/* static */ int HGCMService::SaveState(PSSMHANDLE pSSM)
{
    /* Save the current handle count and restore afterwards to avoid client id conflicts. */
    int rc = SSMR3PutU32(pSSM, hgcmObjQueryHandleCount());
    AssertRCReturn(rc, rc);

    LogFlowFunc(("%d services to be saved:\n", sm_cServices));

    /* Save number of services. */
    rc = SSMR3PutU32(pSSM, sm_cServices);
    AssertRCReturn(rc, rc);

    /* Save every service. */
    HGCMService *pSvc = sm_pSvcListHead;

    while (pSvc)
    {
        LogFlowFunc(("Saving service [%s]\n", pSvc->m_pszSvcName));

        /* Save the length of the service name. */
        rc = SSMR3PutU32(pSSM, (uint32_t) strlen(pSvc->m_pszSvcName) + 1);
        AssertRCReturn(rc, rc);

        /* Save the name of the service. */
        rc = SSMR3PutStrZ(pSSM, pSvc->m_pszSvcName);
        AssertRCReturn(rc, rc);

        /* Save the number of clients. */
        rc = SSMR3PutU32(pSSM, pSvc->m_cClients);
        AssertRCReturn(rc, rc);

        /* Call the service for every client. Normally a service must not have
         * a global state to be saved: only per client info is relevant.
         * The global state of a service is configured during VM startup.
         */
        uint32_t i;

        for (i = 0; i < pSvc->m_cClients; i++)
        {
            uint32_t u32ClientId = pSvc->m_paClientIds[i];

            Log(("client id 0x%08X\n", u32ClientId));

            /* Save the client id. */
            rc = SSMR3PutU32(pSSM, u32ClientId);
            AssertRCReturn(rc, rc);

            /* Call the service, so the operation is executed by the service thread. */
            rc = pSvc->saveClientState(u32ClientId, pSSM);
            AssertRCReturn(rc, rc);
        }

        pSvc = pSvc->m_pSvcNext;
    }

    return VINF_SUCCESS;
}

/** The method loads saved HGCM state.
 *
 * @param pSSM  The saved state context.
 * @return VBox rc.
 * @thread main HGCM
 */
/* static */ int HGCMService::LoadState(PSSMHANDLE pSSM)
{
    /* Restore handle count to avoid client id conflicts. */
    uint32_t u32;

    int rc = SSMR3GetU32(pSSM, &u32);
    AssertRCReturn(rc, rc);

    hgcmObjSetHandleCount(u32);

    /* Get the number of services. */
    uint32_t cServices;

    rc = SSMR3GetU32(pSSM, &cServices);
    AssertRCReturn(rc, rc);

    LogFlowFunc(("%d services to be restored:\n", cServices));

    while (cServices--)
    {
        /* Get the length of the service name. */
        rc = SSMR3GetU32(pSSM, &u32);
        AssertRCReturn(rc, rc);
        AssertReturn(u32 <= VBOX_HGCM_SVC_NAME_MAX_BYTES, VERR_SSM_UNEXPECTED_DATA);

        char *pszServiceName = (char *)alloca(u32);

        /* Get the service name. */
        rc = SSMR3GetStrZ(pSSM, pszServiceName, u32);
        AssertRCReturn(rc, rc);

        LogRel(("HGCM: restoring [%s]\n", pszServiceName));

        /* Resolve the service instance. */
        HGCMService *pSvc;
        rc = ResolveService(&pSvc, pszServiceName);
        AssertLogRelMsgReturn(pSvc, ("rc=%Rrc, %s\n", rc, pszServiceName), VERR_SSM_UNEXPECTED_DATA);

        /* Get the number of clients. */
        uint32_t cClients;
        rc = SSMR3GetU32(pSSM, &cClients);
        if (RT_FAILURE(rc))
        {
            pSvc->ReleaseService();
            AssertFailed();
            return rc;
        }

        while (cClients--)
        {
            /* Get the client id. */
            uint32_t u32ClientId;
            rc = SSMR3GetU32(pSSM, &u32ClientId);
            if (RT_FAILURE(rc))
            {
                pSvc->ReleaseService();
                AssertFailed();
                return rc;
            }

            /* Connect the client. */
            rc = pSvc->CreateAndConnectClient(NULL, u32ClientId);
            if (RT_FAILURE(rc))
            {
                pSvc->ReleaseService();
                AssertLogRelMsgFailed(("rc=%Rrc %s\n", rc, pszServiceName));
                return rc;
            }

            /* Call the service, so the operation is executed by the service thread. */
            rc = pSvc->loadClientState(u32ClientId, pSSM);
            if (RT_FAILURE(rc))
            {
                pSvc->ReleaseService();
                AssertLogRelMsgFailed(("rc=%Rrc %s\n", rc, pszServiceName));
                return rc;
            }
        }

        pSvc->ReleaseService();
    }

    return VINF_SUCCESS;
}

/* Create a new client instance and connect it to the service.
 *
 * @param pu32ClientIdOut If not NULL, then the method must generate a new handle for the client.
 *                        If NULL, use the given 'u32ClientIdIn' handle.
 * @param u32ClientIdIn   The handle for the client, when 'pu32ClientIdOut' is NULL.
 * @return VBox rc.
 */
int HGCMService::CreateAndConnectClient(uint32_t *pu32ClientIdOut, uint32_t u32ClientIdIn)
{
    LogFlowFunc(("pu32ClientIdOut = %p, u32ClientIdIn = %d\n", pu32ClientIdOut, u32ClientIdIn));

    /* Allocate a client information structure. */
    HGCMClient *pClient = new HGCMClient();

    if (!pClient)
    {
        Log1WarningFunc(("Could not allocate HGCMClient!!!\n"));
        return VERR_NO_MEMORY;
    }

    uint32_t handle;

    if (pu32ClientIdOut != NULL)
    {
        handle = hgcmObjGenerateHandle(pClient);
    }
    else
    {
        handle = hgcmObjAssignHandle(pClient, u32ClientIdIn);
    }

    LogFlowFunc(("client id = %d\n", handle));

    AssertRelease(handle);

    /* Initialize the HGCM part of the client. */
    int rc = pClient->Init(this);

    if (RT_SUCCESS(rc))
    {
        /* Call the service. */
        HGCMMSGHANDLE hMsg;

        rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_CONNECT, hgcmMessageAllocSvc);

        if (RT_SUCCESS(rc))
        {
            HGCMMsgSvcConnect *pMsg = (HGCMMsgSvcConnect *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
            AssertRelease(pMsg);

            pMsg->u32ClientId = handle;

            hgcmObjDereference(pMsg);

            rc = hgcmMsgSend(hMsg);

            if (RT_SUCCESS(rc))
            {
                /* Add the client Id to the array. */
                if (m_cClients == m_cClientsAllocated)
                {
                    const uint32_t cDelta = 64;

                    /* Guards against integer overflow on 32bit arch and also limits size of m_paClientIds array to 4GB*/
                    if (m_cClientsAllocated < UINT32_MAX / sizeof(m_paClientIds[0]) - cDelta)
                    {
                        uint32_t *paClientIdsNew;

                        paClientIdsNew = (uint32_t *)RTMemRealloc(m_paClientIds, (m_cClientsAllocated + cDelta) *
                            sizeof(m_paClientIds[0]));
                        Assert(paClientIdsNew);

                        if (paClientIdsNew)
                        {
                            m_paClientIds = paClientIdsNew;
                            m_cClientsAllocated += cDelta;
                        }
                        else
                        {
                            rc = VERR_NO_MEMORY;
                        }
                    }
                    else
                    {
                        rc = VERR_NO_MEMORY;
                    }
                }

                m_paClientIds[m_cClients] = handle;
                m_cClients++;
            }
        }
    }

    if (RT_FAILURE(rc))
    {
        hgcmObjDeleteHandle(handle);
    }
    else
    {
        if (pu32ClientIdOut != NULL)
        {
            *pu32ClientIdOut = handle;
        }

        ReferenceService();
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Disconnect the client from the service and delete the client handle.
 *
 * @param u32ClientId  The handle of the client.
 * @return VBox rc.
 */
int HGCMService::DisconnectClient(uint32_t u32ClientId, bool fFromService)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("client id = %d, fFromService = %d\n", u32ClientId, fFromService));

    if (!fFromService)
    {
        /* Call the service. */
        HGCMMSGHANDLE hMsg;

        rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_DISCONNECT, hgcmMessageAllocSvc);

        if (RT_SUCCESS(rc))
        {
            HGCMMsgSvcDisconnect *pMsg = (HGCMMsgSvcDisconnect *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
            AssertRelease(pMsg);

            pMsg->u32ClientId = u32ClientId;

            hgcmObjDereference(pMsg);

            rc = hgcmMsgSend(hMsg);
        }
        else
        {
            LogRel(("(%d, %d) [%s] hgcmMsgAlloc(%p, SVC_MSG_DISCONNECT) failed %Rrc\n",
                    u32ClientId, fFromService, RT_VALID_PTR(m_pszSvcName)? m_pszSvcName: "", m_thread, rc));
        }
    }

    /* Remove the client id from the array in any case, rc does not matter. */
    uint32_t i;

    for (i = 0; i < m_cClients; i++)
    {
        if (m_paClientIds[i] == u32ClientId)
        {
            m_cClients--;

            if (m_cClients > i)
            {
                memmove (&m_paClientIds[i], &m_paClientIds[i + 1], sizeof(m_paClientIds[0]) * (m_cClients - i));
            }

            /* Delete the client handle. */
            hgcmObjDeleteHandle(u32ClientId);

            /* The service must be released. */
            ReleaseService();

            break;
        }
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMService::RegisterExtension(HGCMSVCEXTHANDLE handle,
                                   PFNHGCMSVCEXT pfnExtension,
                                   void *pvExtension)
{
    LogFlowFunc(("%s\n", handle->pszServiceName));

    /* Forward the message to the service thread. */
    HGCMMSGHANDLE hMsg = 0;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_REGEXT, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgSvcRegisterExtension *pMsg = (HGCMMsgSvcRegisterExtension *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->handle       = handle;
        pMsg->pfnExtension = pfnExtension;
        pMsg->pvExtension  = pvExtension;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

void HGCMService::UnregisterExtension(HGCMSVCEXTHANDLE handle)
{
    /* Forward the message to the service thread. */
    HGCMMSGHANDLE hMsg = 0;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_UNREGEXT, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgSvcUnregisterExtension *pMsg = (HGCMMsgSvcUnregisterExtension *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->handle = handle;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
}

/* Perform a guest call to the service.
 *
 * @param pHGCMPort      The port to be used for completion confirmation.
 * @param pCmd           The VBox HGCM context.
 * @param u32ClientId    The client handle to be disconnected and deleted.
 * @param u32Function    The function number.
 * @param cParms         Number of parameters.
 * @param paParms        Pointer to array of parameters.
 * @return VBox rc.
 */
int HGCMService::GuestCall(PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmd, uint32_t u32ClientId, uint32_t u32Function,
                           uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    HGCMMSGHANDLE hMsg = 0;

    LogFlow(("MAIN::HGCMService::Call\n"));

    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_GUESTCALL, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgCall *pMsg = (HGCMMsgCall *)hgcmObjReference(hMsg, HGCMOBJ_MSG);

        AssertRelease(pMsg);

        pMsg->pCmd      = pCmd;
        pMsg->pHGCMPort = pHGCMPort;

        pMsg->u32ClientId = u32ClientId;
        pMsg->u32Function = u32Function;
        pMsg->cParms      = cParms;
        pMsg->paParms     = paParms;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgPost(hMsg, hgcmMsgCompletionCallback);
    }
    else
    {
        Log(("MAIN::HGCMService::Call: Message allocation failed: %Rrc\n", rc));
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Perform a host call the service.
 *
 * @param u32Function    The function number.
 * @param cParms         Number of parameters.
 * @param paParms        Pointer to array of parameters.
 * @return VBox rc.
 */
int HGCMService::HostCall(uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM *paParms)
{
    LogFlowFunc(("%s u32Function = %d, cParms = %d, paParms = %p\n",
                 m_pszSvcName, u32Function, cParms, paParms));

    HGCMMSGHANDLE hMsg = 0;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_HOSTCALL, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgHostCallSvc *pMsg = (HGCMMsgHostCallSvc *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->u32Function      = u32Function;
        pMsg->cParms           = cParms;
        pMsg->paParms          = paParms;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

#ifdef VBOX_WITH_CRHGSMI
static DECLCALLBACK(void) hgcmMsgFastCallCompletionCallback(int32_t result, HGCMMsgCore *pMsgCore)
{
    /* Call the VMMDev port interface to issue IRQ notification. */
    LogFlow(("MAIN::hgcmMsgFastCallCompletionCallback: message %p\n", pMsgCore));

    HGCMMsgHostFastCallAsyncSvc *pMsg = (HGCMMsgHostFastCallAsyncSvc *)pMsgCore;
    if (pMsg->pfnCompletion)
    {
        pMsg->pfnCompletion(result, pMsg->u32Function, &pMsg->Param, pMsg->pvCompletion);
    }
}

int HGCMService::HandleAcquired()
{
    ++m_cHandleAcquires;
    return VINF_SUCCESS;
}

int HGCMService::HandleReleased()
{
    Assert(m_cHandleAcquires);
    if (m_cHandleAcquires)
    {
        --m_cHandleAcquires;
        return VINF_SUCCESS;
    }
    return VERR_INVALID_STATE;
}

int HGCMService::HostFastCallAsync(uint32_t u32Function, VBOXHGCMSVCPARM *pParm, PHGCMHOSTFASTCALLCB pfnCompletion,
                                   void *pvCompletion)
{
    LogFlowFunc(("%s u32Function = %d, pParm = %p\n",
                 m_pszSvcName, u32Function, pParm));

    HGCMMSGHANDLE hMsg = 0;
    int rc = hgcmMsgAlloc(m_thread, &hMsg, SVC_MSG_HOSTFASTCALLASYNC, hgcmMessageAllocSvc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgHostFastCallAsyncSvc *pMsg = (HGCMMsgHostFastCallAsyncSvc *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->u32Function      = u32Function;
        pMsg->Param = *pParm;
        pMsg->pfnCompletion = pfnCompletion;
        pMsg->pvCompletion = pvCompletion;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgPost(hMsg, hgcmMsgFastCallCompletionCallback);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}
#endif

/*
 * Main HGCM thread that manages services.
 */

/* Messages processed by the main HGCM thread. */
#define HGCM_MSG_CONNECT    (10)  /* Connect a client to a service. */
#define HGCM_MSG_DISCONNECT (11)  /* Disconnect the specified client id. */
#define HGCM_MSG_LOAD       (12)  /* Load the service. */
#define HGCM_MSG_HOSTCALL   (13)  /* Call the service. */
#define HGCM_MSG_LOADSTATE  (14)  /* Load saved state for the specified service. */
#define HGCM_MSG_SAVESTATE  (15)  /* Save state for the specified service. */
#define HGCM_MSG_RESET      (16)  /* Disconnect all clients from the specified service. */
#define HGCM_MSG_QUIT       (17)  /* Unload all services and terminate the thread. */
#define HGCM_MSG_REGEXT     (18)  /* Register a service extension. */
#define HGCM_MSG_UNREGEXT   (19)  /* Unregister a service extension. */
#ifdef VBOX_WITH_CRHGSMI
# define HGCM_MSG_SVCAQUIRE  (30)  /* Acquire a service handle (for fast host calls) */
# define HGCM_MSG_SVCRELEASE (31)  /* Release a service */
#endif

class HGCMMsgMainConnect: public HGCMMsgHeader
{
    public:
        /* Service name. */
        const char *pszServiceName;
        /* Where to store the client handle. */
        uint32_t *pu32ClientId;
};

class HGCMMsgMainDisconnect: public HGCMMsgHeader
{
    public:
        /* Handle of the client to be disconnected. */
        uint32_t u32ClientId;
};

class HGCMMsgMainLoad: public HGCMMsgCore
{
    public:
        /* Name of the library to be loaded. */
        const char *pszServiceLibrary;
        /* Name to be assigned to the service. */
        const char *pszServiceName;
};

class HGCMMsgMainHostCall: public HGCMMsgCore
{
    public:
        /* Which service to call. */
        const char *pszServiceName;
        /* Function number. */
        uint32_t u32Function;
        /* Number of the function parameters. */
        uint32_t cParms;
        /* Pointer to array of the function parameters. */
        VBOXHGCMSVCPARM *paParms;
};

class HGCMMsgMainLoadSaveState: public HGCMMsgCore
{
    public:
        /* SSM context. */
        PSSMHANDLE pSSM;
};

class HGCMMsgMainReset: public HGCMMsgCore
{
};

class HGCMMsgMainQuit: public HGCMMsgCore
{
};

class HGCMMsgMainRegisterExtension: public HGCMMsgCore
{
    public:
        /* Returned handle to be used in HGCMMsgMainUnregisterExtension. */
        HGCMSVCEXTHANDLE *pHandle;
        /* Name of the service. */
        const char *pszServiceName;
        /* The extension entry point. */
        PFNHGCMSVCEXT pfnExtension;
        /* The extension pointer. */
        void *pvExtension;
};

class HGCMMsgMainUnregisterExtension: public HGCMMsgCore
{
    public:
        /* Handle of the registered extension. */
        HGCMSVCEXTHANDLE handle;
};

#ifdef VBOX_WITH_CRHGSMI
class HGCMMsgMainSvcAcquire: public HGCMMsgCore
{
    public:
        /* Which service to call. */
        const char *pszServiceName;
        /* Returned service. */
        HGCMService *pService;
};

class HGCMMsgMainSvcRelease: public HGCMMsgCore
{
    public:
        /* Svc . */
        HGCMService *pService;
};
#endif


static HGCMMsgCore *hgcmMainMessageAlloc (uint32_t u32MsgId)
{
    switch (u32MsgId)
    {
        case HGCM_MSG_CONNECT:    return new HGCMMsgMainConnect();
        case HGCM_MSG_DISCONNECT: return new HGCMMsgMainDisconnect();
        case HGCM_MSG_LOAD:       return new HGCMMsgMainLoad();
        case HGCM_MSG_HOSTCALL:   return new HGCMMsgMainHostCall();
        case HGCM_MSG_LOADSTATE:
        case HGCM_MSG_SAVESTATE:  return new HGCMMsgMainLoadSaveState();
        case HGCM_MSG_RESET:      return new HGCMMsgMainReset();
        case HGCM_MSG_QUIT:       return new HGCMMsgMainQuit();
        case HGCM_MSG_REGEXT:     return new HGCMMsgMainRegisterExtension();
        case HGCM_MSG_UNREGEXT:   return new HGCMMsgMainUnregisterExtension();
#ifdef VBOX_WITH_CRHGSMI
        case HGCM_MSG_SVCAQUIRE: return new HGCMMsgMainSvcAcquire();
        case HGCM_MSG_SVCRELEASE: return new HGCMMsgMainSvcRelease();
#endif

        default:
            AssertReleaseMsgFailed(("Msg id = %08X\n", u32MsgId));
    }

    return NULL;
}


/* The main HGCM thread handler. */
static DECLCALLBACK(void) hgcmThread(HGCMTHREADHANDLE ThreadHandle, void *pvUser)
{
    LogFlowFunc(("ThreadHandle = %p, pvUser = %p\n",
                 ThreadHandle, pvUser));

    NOREF(pvUser);

    bool fQuit = false;

    while (!fQuit)
    {
        HGCMMsgCore *pMsgCore;
        int rc = hgcmMsgGet(ThreadHandle, &pMsgCore);

        if (RT_FAILURE(rc))
        {
            /* The error means some serious unrecoverable problem in the hgcmMsg/hgcmThread layer. */
            AssertMsgFailed(("%Rrc\n", rc));
            break;
        }

        uint32_t u32MsgId = pMsgCore->MsgId();

        switch (u32MsgId)
        {
            case HGCM_MSG_CONNECT:
            {
                HGCMMsgMainConnect *pMsg = (HGCMMsgMainConnect *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_CONNECT pszServiceName %s, pu32ClientId %p\n",
                             pMsg->pszServiceName, pMsg->pu32ClientId));

                /* Resolve the service name to the pointer to service instance.
                 */
                HGCMService *pService;
                rc = HGCMService::ResolveService(&pService, pMsg->pszServiceName);

                if (RT_SUCCESS(rc))
                {
                    /* Call the service instance method. */
                    rc = pService->CreateAndConnectClient(pMsg->pu32ClientId, 0);

                    /* Release the service after resolve. */
                    pService->ReleaseService();
                }
            } break;

            case HGCM_MSG_DISCONNECT:
            {
                HGCMMsgMainDisconnect *pMsg = (HGCMMsgMainDisconnect *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_DISCONNECT u32ClientId = %d\n",
                             pMsg->u32ClientId));

                HGCMClient *pClient = (HGCMClient *)hgcmObjReference(pMsg->u32ClientId, HGCMOBJ_CLIENT);

                if (!pClient)
                {
                    rc = VERR_HGCM_INVALID_CLIENT_ID;
                    break;
                }

                /* The service the client belongs to. */
                HGCMService *pService = pClient->pService;

                /* Call the service instance to disconnect the client. */
                rc = pService->DisconnectClient(pMsg->u32ClientId, false);

                hgcmObjDereference(pClient);
            } break;

            case HGCM_MSG_LOAD:
            {
                HGCMMsgMainLoad *pMsg = (HGCMMsgMainLoad *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_LOAD pszServiceName = %s, pMsg->pszServiceLibrary = %s\n",
                             pMsg->pszServiceName, pMsg->pszServiceLibrary));

                rc = HGCMService::LoadService(pMsg->pszServiceLibrary, pMsg->pszServiceName);
            } break;

            case HGCM_MSG_HOSTCALL:
            {
                HGCMMsgMainHostCall *pMsg = (HGCMMsgMainHostCall *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_HOSTCALL pszServiceName %s, u32Function %d, cParms %d, paParms %p\n",
                             pMsg->pszServiceName, pMsg->u32Function, pMsg->cParms, pMsg->paParms));

                /* Resolve the service name to the pointer to service instance. */
                HGCMService *pService;
                rc = HGCMService::ResolveService(&pService, pMsg->pszServiceName);

                if (RT_SUCCESS(rc))
                {
                    rc = pService->HostCall(pMsg->u32Function, pMsg->cParms, pMsg->paParms);

                    pService->ReleaseService();
                }
            } break;

#ifdef VBOX_WITH_CRHGSMI
            case HGCM_MSG_SVCAQUIRE:
            {
                HGCMMsgMainSvcAcquire *pMsg = (HGCMMsgMainSvcAcquire *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_SVCAQUIRE pszServiceName %s\n", pMsg->pszServiceName));

                /* Resolve the service name to the pointer to service instance. */
                HGCMService *pService;
                rc = HGCMService::ResolveService(&pService, pMsg->pszServiceName);
                if (RT_SUCCESS(rc))
                {
                    rc = pService->HandleAcquired();
                    if (RT_SUCCESS(rc))
                    {
                        pMsg->pService = pService;
                    }
                    else
                    {
                        pService->ReleaseService();
                    }
                }
            } break;

            case HGCM_MSG_SVCRELEASE:
            {
                HGCMMsgMainSvcRelease *pMsg = (HGCMMsgMainSvcRelease *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_SVCARELEASE pService %p\n", pMsg->pService));

                /* Resolve the service name to the pointer to service instance. */

                rc = pMsg->pService->HandleReleased();
                if (RT_SUCCESS(rc))
                {
                    pMsg->pService->ReleaseService();
                }
            } break;
#endif

            case HGCM_MSG_RESET:
            {
                LogFlowFunc(("HGCM_MSG_RESET\n"));

                HGCMService::Reset();
            } break;

            case HGCM_MSG_LOADSTATE:
            {
                HGCMMsgMainLoadSaveState *pMsg = (HGCMMsgMainLoadSaveState *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_LOADSTATE\n"));

                rc = HGCMService::LoadState(pMsg->pSSM);
            } break;

            case HGCM_MSG_SAVESTATE:
            {
                HGCMMsgMainLoadSaveState *pMsg = (HGCMMsgMainLoadSaveState *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_SAVESTATE\n"));

                rc = HGCMService::SaveState(pMsg->pSSM);
            } break;

            case HGCM_MSG_QUIT:
            {
                LogFlowFunc(("HGCM_MSG_QUIT\n"));

                HGCMService::UnloadAll();

                fQuit = true;
            } break;

            case HGCM_MSG_REGEXT:
            {
                HGCMMsgMainRegisterExtension *pMsg = (HGCMMsgMainRegisterExtension *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_REGEXT\n"));

                /* Allocate the handle data. */
                HGCMSVCEXTHANDLE handle = (HGCMSVCEXTHANDLE)RTMemAllocZ(sizeof(struct _HGCMSVCEXTHANDLEDATA)
                                                                        + strlen(pMsg->pszServiceName)
                                                                        + sizeof(char));

                if (handle == NULL)
                {
                    rc = VERR_NO_MEMORY;
                }
                else
                {
                    handle->pszServiceName = (char *)((uint8_t *)handle + sizeof(struct _HGCMSVCEXTHANDLEDATA));
                    strcpy(handle->pszServiceName, pMsg->pszServiceName);

                    HGCMService *pService;
                    rc = HGCMService::ResolveService(&pService, handle->pszServiceName);

                    if (RT_SUCCESS(rc))
                    {
                        pService->RegisterExtension(handle, pMsg->pfnExtension, pMsg->pvExtension);

                        pService->ReleaseService();
                    }

                    if (RT_FAILURE(rc))
                    {
                        RTMemFree(handle);
                    }
                    else
                    {
                        *pMsg->pHandle = handle;
                    }
                }
            } break;

            case HGCM_MSG_UNREGEXT:
            {
                HGCMMsgMainUnregisterExtension *pMsg = (HGCMMsgMainUnregisterExtension *)pMsgCore;

                LogFlowFunc(("HGCM_MSG_UNREGEXT\n"));

                HGCMService *pService;
                rc = HGCMService::ResolveService(&pService, pMsg->handle->pszServiceName);

                if (RT_SUCCESS(rc))
                {
                    pService->UnregisterExtension(pMsg->handle);

                    pService->ReleaseService();
                }

                RTMemFree(pMsg->handle);
            } break;

            default:
            {
                AssertMsgFailed(("hgcmThread: Unsupported message number %08X!!!\n", u32MsgId));
                rc = VERR_NOT_SUPPORTED;
            } break;
        }

        /* Complete the message processing. */
        hgcmMsgComplete(pMsgCore, rc);

        LogFlowFunc(("message processed %Rrc\n", rc));
    }
}


/*
 * The HGCM API.
 */

/* The main hgcm thread. */
static HGCMTHREADHANDLE g_hgcmThread = 0;

/*
 * Public HGCM functions.
 *
 * hgcmGuest* - called as a result of the guest HGCM requests.
 * hgcmHost*  - called by the host.
 */

/* Load a HGCM service from the specified library.
 * Assign the specified name to the service.
 *
 * @param pszServiceLibrary  The library to be loaded.
 * @param pszServiceName     The name to be assigned to the service.
 * @return VBox rc.
 */
int HGCMHostLoad(const char *pszServiceLibrary,
                 const char *pszServiceName)
{
    LogFlowFunc(("lib = %s, name = %s\n", pszServiceLibrary, pszServiceName));

    if (!pszServiceLibrary || !pszServiceName)
    {
        return VERR_INVALID_PARAMETER;
    }

    /* Forward the request to the main hgcm thread. */
    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_LOAD, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the message. Since the message is synchronous, use the supplied pointers. */
        HGCMMsgMainLoad *pMsg = (HGCMMsgMainLoad *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pszServiceLibrary = pszServiceLibrary;
        pMsg->pszServiceName    = pszServiceName;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Register a HGCM service extension.
 *
 * @param pHandle            Returned handle for the registered extension.
 * @param pszServiceName     The name of the service.
 * @param pfnExtension       The extension entry point (callback).
 * @param pvExtension        The extension pointer.
 * @return VBox rc.
 */
int HGCMHostRegisterServiceExtension(HGCMSVCEXTHANDLE *pHandle,
                                     const char *pszServiceName,
                                     PFNHGCMSVCEXT pfnExtension,
                                     void *pvExtension)
{
    LogFlowFunc(("pHandle = %p, name = %s, pfn = %p, rv = %p\n", pHandle, pszServiceName, pfnExtension, pvExtension));

    if (!pHandle || !pszServiceName || !pfnExtension)
    {
        return VERR_INVALID_PARAMETER;
    }

    /* Forward the request to the main hgcm thread. */
    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_REGEXT, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the message. Since the message is synchronous, use the supplied pointers. */
        HGCMMsgMainRegisterExtension *pMsg = (HGCMMsgMainRegisterExtension *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pHandle        = pHandle;
        pMsg->pszServiceName = pszServiceName;
        pMsg->pfnExtension   = pfnExtension;
        pMsg->pvExtension    = pvExtension;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("*pHandle = %p, rc = %Rrc\n", *pHandle, rc));
    return rc;
}

void HGCMHostUnregisterServiceExtension(HGCMSVCEXTHANDLE handle)
{
    LogFlowFunc(("handle = %p\n", handle));

    /* Forward the request to the main hgcm thread. */
    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_UNREGEXT, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the message. */
        HGCMMsgMainUnregisterExtension *pMsg = (HGCMMsgMainUnregisterExtension *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->handle = handle;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return;
}

/* Find a service and inform it about a client connection, create a client handle.
 *
 * @param pHGCMPort      The port to be used for completion confirmation.
 * @param pCmd           The VBox HGCM context.
 * @param pszServiceName The name of the service to be connected to.
 * @param pu32ClientId   Where the store the created client handle.
 * @return VBox rc.
 */
int HGCMGuestConnect(PPDMIHGCMPORT pHGCMPort,
                     PVBOXHGCMCMD pCmd,
                     const char *pszServiceName,
                     uint32_t *pu32ClientId)
{
    LogFlowFunc(("pHGCMPort = %p, pCmd = %p, name = %s, pu32ClientId = %p\n",
                 pHGCMPort, pCmd, pszServiceName, pu32ClientId));

    if (pHGCMPort == NULL || pCmd == NULL || pszServiceName == NULL || pu32ClientId == NULL)
    {
        return VERR_INVALID_PARAMETER;
    }

    /* Forward the request to the main hgcm thread. */
    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_CONNECT, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the message. Since 'pszServiceName' and 'pu32ClientId'
         * will not be deallocated by the caller until the message is completed,
         * use the supplied pointers.
         */
        HGCMMsgMainConnect *pMsg = (HGCMMsgMainConnect *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pHGCMPort      = pHGCMPort;
        pMsg->pCmd           = pCmd;
        pMsg->pszServiceName = pszServiceName;
        pMsg->pu32ClientId   = pu32ClientId;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgPost(hMsg, hgcmMsgCompletionCallback);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Tell a service that the client is disconnecting, destroy the client handle.
 *
 * @param pHGCMPort      The port to be used for completion confirmation.
 * @param pCmd           The VBox HGCM context.
 * @param u32ClientId    The client handle to be disconnected and deleted.
 * @return VBox rc.
 */
int HGCMGuestDisconnect(PPDMIHGCMPORT pHGCMPort,
                        PVBOXHGCMCMD pCmd,
                        uint32_t u32ClientId)
{
    LogFlowFunc(("pHGCMPort = %p, pCmd = %p, u32ClientId = %d\n",
                  pHGCMPort, pCmd, u32ClientId));

    if (!pHGCMPort || !pCmd || !u32ClientId)
    {
        return VERR_INVALID_PARAMETER;
    }

    /* Forward the request to the main hgcm thread. */
    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_DISCONNECT, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the message. */
        HGCMMsgMainDisconnect *pMsg = (HGCMMsgMainDisconnect *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pCmd        = pCmd;
        pMsg->pHGCMPort   = pHGCMPort;
        pMsg->u32ClientId = u32ClientId;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgPost(hMsg, hgcmMsgCompletionCallback);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Helper to send either HGCM_MSG_SAVESTATE or HGCM_MSG_LOADSTATE messages to the main HGCM thread.
 *
 * @param pSSM     The SSM handle.
 * @param u32MsgId The message to be sent: HGCM_MSG_SAVESTATE or HGCM_MSG_LOADSTATE.
 * @return VBox rc.
 */
static int hgcmHostLoadSaveState(PSSMHANDLE pSSM,
                                 uint32_t u32MsgId)
{
    LogFlowFunc(("pSSM = %p, u32MsgId = %d\n", pSSM, u32MsgId));

    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, u32MsgId, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgMainLoadSaveState *pMsg = (HGCMMsgMainLoadSaveState *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pSSM = pSSM;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* Save the state of services.
 *
 * @param pSSM     The SSM handle.
 * @return VBox rc.
 */
int HGCMHostSaveState(PSSMHANDLE pSSM)
{
    return hgcmHostLoadSaveState(pSSM, HGCM_MSG_SAVESTATE);
}

/* Load the state of services.
 *
 * @param pSSM     The SSM handle.
 * @return VBox rc.
 */
int HGCMHostLoadState(PSSMHANDLE pSSM)
{
    return hgcmHostLoadSaveState(pSSM, HGCM_MSG_LOADSTATE);
}

/* The guest calls the service.
 *
 * @param pHGCMPort      The port to be used for completion confirmation.
 * @param pCmd           The VBox HGCM context.
 * @param u32ClientId    The client handle to be disconnected and deleted.
 * @param u32Function    The function number.
 * @param cParms         Number of parameters.
 * @param paParms        Pointer to array of parameters.
 * @return VBox rc.
 */
int HGCMGuestCall(PPDMIHGCMPORT pHGCMPort,
                  PVBOXHGCMCMD pCmd,
                  uint32_t u32ClientId,
                  uint32_t u32Function,
                  uint32_t cParms,
                  VBOXHGCMSVCPARM *paParms)
{
    LogFlowFunc(("pHGCMPort = %p, pCmd = %p, u32ClientId = %d, u32Function = %d, cParms = %d, paParms = %p\n",
                  pHGCMPort, pCmd, u32ClientId, u32Function, cParms, paParms));

    if (!pHGCMPort || !pCmd || u32ClientId == 0)
    {
        return VERR_INVALID_PARAMETER;
    }

    int rc = VERR_HGCM_INVALID_CLIENT_ID;

    /* Resolve the client handle to the client instance pointer. */
    HGCMClient *pClient = (HGCMClient *)hgcmObjReference(u32ClientId, HGCMOBJ_CLIENT);

    if (pClient)
    {
        AssertRelease(pClient->pService);

        /* Forward the message to the service thread. */
        rc = pClient->pService->GuestCall(pHGCMPort, pCmd, u32ClientId, u32Function, cParms, paParms);

        hgcmObjDereference(pClient);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/* The host calls the service.
 *
 * @param pszServiceName The service name to be called.
 * @param u32Function    The function number.
 * @param cParms         Number of parameters.
 * @param paParms        Pointer to array of parameters.
 * @return VBox rc.
 */
int HGCMHostCall(const char *pszServiceName,
                 uint32_t u32Function,
                 uint32_t cParms,
                 VBOXHGCMSVCPARM *paParms)
{
    LogFlowFunc(("name = %s, u32Function = %d, cParms = %d, paParms = %p\n",
                 pszServiceName, u32Function, cParms, paParms));

    if (!pszServiceName)
    {
        return VERR_INVALID_PARAMETER;
    }

    HGCMMSGHANDLE hMsg = 0;

    /* Host calls go to main HGCM thread that resolves the service name to the
     * service instance pointer and then, using the service pointer, forwards
     * the message to the service thread.
     * So it is slow but host calls are intended mostly for configuration and
     * other non-time-critical functions.
     */
    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_HOSTCALL, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgMainHostCall *pMsg = (HGCMMsgMainHostCall *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pszServiceName = (char *)pszServiceName;
        pMsg->u32Function    = u32Function;
        pMsg->cParms         = cParms;
        pMsg->paParms        = paParms;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

#ifdef VBOX_WITH_CRHGSMI
int HGCMHostSvcHandleCreate(const char *pszServiceName, HGCMCVSHANDLE * phSvc)
{
    LogFlowFunc(("name = %s\n", pszServiceName));

    if (!pszServiceName)
    {
        return VERR_INVALID_PARAMETER;
    }

    if (!phSvc)
    {
        return VERR_INVALID_PARAMETER;
    }

    HGCMMSGHANDLE hMsg = 0;

    /* Host calls go to main HGCM thread that resolves the service name to the
     * service instance pointer and then, using the service pointer, forwards
     * the message to the service thread.
     * So it is slow but host calls are intended mostly for configuration and
     * other non-time-critical functions.
     */
    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_SVCAQUIRE, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgMainSvcAcquire *pMsg = (HGCMMsgMainSvcAcquire *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pszServiceName = (char *)pszServiceName;
        pMsg->pService = NULL;

        rc = hgcmMsgSend(hMsg);
        if (RT_SUCCESS(rc))
        {
            /* for simplicity just use a svc ptr as handle for now */
            *phSvc = (HGCMCVSHANDLE)pMsg->pService;
        }

        hgcmObjDereference(pMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMHostSvcHandleDestroy(HGCMCVSHANDLE hSvc)
{
    LogFlowFunc(("hSvc = %p\n", hSvc));

    if (!hSvc)
    {
        return VERR_INVALID_PARAMETER;
    }

    HGCMMSGHANDLE hMsg = 0;

    /* Host calls go to main HGCM thread that resolves the service name to the
     * service instance pointer and then, using the service pointer, forwards
     * the message to the service thread.
     * So it is slow but host calls are intended mostly for configuration and
     * other non-time-critical functions.
     */
    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_SVCRELEASE, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        HGCMMsgMainSvcRelease *pMsg = (HGCMMsgMainSvcRelease *)hgcmObjReference(hMsg, HGCMOBJ_MSG);
        AssertRelease(pMsg);

        pMsg->pService = (HGCMService *)hSvc;

        hgcmObjDereference(pMsg);

        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMHostFastCallAsync(HGCMCVSHANDLE hSvc, uint32_t function, VBOXHGCMSVCPARM *pParm, PHGCMHOSTFASTCALLCB pfnCompletion,
                          void *pvCompletion)
{
    LogFlowFunc(("hSvc = %p, u32Function = %d, pParm = %p\n",
            hSvc, function, pParm));

    if (!hSvc)
    {
        return VERR_INVALID_PARAMETER;
    }

    HGCMService *pService = (HGCMService *)hSvc;
    int rc = pService->HostFastCallAsync(function, pParm, pfnCompletion, pvCompletion);

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}
#endif

int HGCMHostReset(void)
{
    LogFlowFunc(("\n"));

    /* Disconnect all clients.
     */

    HGCMMSGHANDLE hMsg = 0;

    int rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_RESET, hgcmMainMessageAlloc);

    if (RT_SUCCESS(rc))
    {
        rc = hgcmMsgSend(hMsg);
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMHostInit(void)
{
    LogFlowFunc(("\n"));

    int rc = hgcmThreadInit();

    if (RT_SUCCESS(rc))
    {
        /*
         * Start main HGCM thread.
         */

        rc = hgcmThreadCreate(&g_hgcmThread, "MainHGCMthread", hgcmThread, NULL);

        if (RT_FAILURE(rc))
        {
            LogRel(("Failed to start HGCM thread. HGCM services will be unavailable!!! rc = %Rrc\n", rc));
        }
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int HGCMHostShutdown(void)
{
    LogFlowFunc(("\n"));

    /*
     * Do HGCMReset and then unload all services.
     */

    int rc = HGCMHostReset();

    if (RT_SUCCESS(rc))
    {
        /* Send the quit message to the main hgcmThread. */
        HGCMMSGHANDLE hMsg = 0;

        rc = hgcmMsgAlloc(g_hgcmThread, &hMsg, HGCM_MSG_QUIT, hgcmMainMessageAlloc);

        if (RT_SUCCESS(rc))
        {
            rc = hgcmMsgSend(hMsg);

            if (RT_SUCCESS(rc))
            {
                /* Wait for the thread termination. */
                hgcmThreadWait(g_hgcmThread);
                g_hgcmThread = 0;

                hgcmThreadUninit();
            }
        }
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}
