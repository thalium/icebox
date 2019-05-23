/** @file
 * Base class for an host-guest service.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


#ifndef ___VBox_HostService_Service_h
#define ___VBox_HostService_Service_h

#include <VBox/log.h>
#include <VBox/hgcmsvc.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/cpp/utils.h>

#include <memory>  /* for auto_ptr */

/** @todo  document the poor classes.   */
namespace HGCM
{

class Message
{
    /* Contains a copy of HGCM parameters. */
public:
    Message(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[])
        : m_uMsg(0)
        , m_cParms(0)
        , m_paParms(0)
    {
        initData(uMsg, cParms, aParms);
    }

    virtual ~Message(void)
    {
        cleanup();
    }

    uint32_t message() const { return m_uMsg; }
    uint32_t paramsCount() const { return m_cParms; }

    int getData(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[]) const
    {
        if (m_uMsg != uMsg)
        {
            LogFlowFunc(("Message type does not match (%RU32 (buffer), %RU32 (guest))\n", m_uMsg, uMsg));
            return VERR_INVALID_PARAMETER;
        }
        if (m_cParms > cParms)
        {
            LogFlowFunc(("Parameter count does not match (%RU32 (buffer), %RU32 (guest))\n", m_cParms, cParms));
            return VERR_INVALID_PARAMETER;
        }

        return copyParmsInternal(&aParms[0], cParms, m_paParms, m_cParms, false /* fDeepCopy */);
    }

    int getParmU32Info(uint32_t iParm, uint32_t *pu32Info) const
    {
        AssertPtrNullReturn(pu32Info, VERR_INVALID_PARAMETER);
        AssertReturn(iParm < m_cParms, VERR_INVALID_PARAMETER);
        AssertReturn(m_paParms[iParm].type == VBOX_HGCM_SVC_PARM_32BIT, VERR_INVALID_PARAMETER);

        *pu32Info = m_paParms[iParm].u.uint32;

        return VINF_SUCCESS;
    }

    int getParmU64Info(uint32_t iParm, uint64_t *pu64Info) const
    {
        AssertPtrNullReturn(pu64Info, VERR_INVALID_PARAMETER);
        AssertReturn(iParm < m_cParms, VERR_INVALID_PARAMETER);
        AssertReturn(m_paParms[iParm].type == VBOX_HGCM_SVC_PARM_64BIT, VERR_INVALID_PARAMETER);

        *pu64Info = m_paParms[iParm].u.uint64;

        return VINF_SUCCESS;
    }

    int getParmPtrInfo(uint32_t iParm, void **ppvAddr, uint32_t *pcSize) const
    {
        AssertPtrNullReturn(ppvAddr, VERR_INVALID_PARAMETER);
        AssertPtrNullReturn(pcSize, VERR_INVALID_PARAMETER);
        AssertReturn(iParm < m_cParms, VERR_INVALID_PARAMETER);
        AssertReturn(m_paParms[iParm].type == VBOX_HGCM_SVC_PARM_PTR, VERR_INVALID_PARAMETER);

        *ppvAddr = m_paParms[iParm].u.pointer.addr;
        *pcSize = m_paParms[iParm].u.pointer.size;

        return VINF_SUCCESS;
    }

    static int copyParms(PVBOXHGCMSVCPARM paParmsDst, uint32_t cParmsDst, PVBOXHGCMSVCPARM paParmsSrc, uint32_t cParmsSrc)
    {
        return copyParmsInternal(paParmsDst, cParmsDst, paParmsSrc, cParmsSrc, false /* fDeepCopy */);
    }

private:

    /** Stored message type. */
    uint32_t         m_uMsg;
    /** Number of stored HGCM parameters. */
    uint32_t         m_cParms;
    /** Stored HGCM parameters. */
    PVBOXHGCMSVCPARM m_paParms;

    int initData(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[])
    {
        AssertReturn(cParms < 256, VERR_INVALID_PARAMETER);
        AssertPtrNullReturn(aParms, VERR_INVALID_PARAMETER);

        /* Cleanup old messages. */
        cleanup();

        m_uMsg   = uMsg;
        m_cParms = cParms;

        int rc = VINF_SUCCESS;

        if (cParms)
        {
            m_paParms = (VBOXHGCMSVCPARM*)RTMemAllocZ(sizeof(VBOXHGCMSVCPARM) * m_cParms);
            if (m_paParms)
            {
                rc = copyParmsInternal(m_paParms, m_cParms, &aParms[0], cParms, true /* fDeepCopy */);
                if (RT_FAILURE(rc))
                    cleanup();
            }
            else
                rc = VERR_NO_MEMORY;
        }

        return rc;
    }

    static int copyParmsInternal(PVBOXHGCMSVCPARM paParmsDst, uint32_t cParmsDst,
                                 PVBOXHGCMSVCPARM paParmsSrc, uint32_t cParmsSrc,
                                 bool fDeepCopy)
    {
        AssertPtrReturn(paParmsSrc, VERR_INVALID_POINTER);
        AssertPtrReturn(paParmsDst, VERR_INVALID_POINTER);

        if (cParmsSrc > cParmsDst)
            return VERR_BUFFER_OVERFLOW;

        int rc = VINF_SUCCESS;
        for (uint32_t i = 0; i < cParmsSrc; i++)
        {
            paParmsDst[i].type = paParmsSrc[i].type;
            switch (paParmsSrc[i].type)
            {
                case VBOX_HGCM_SVC_PARM_32BIT:
                {
                    paParmsDst[i].u.uint32 = paParmsSrc[i].u.uint32;
                    break;
                }
                case VBOX_HGCM_SVC_PARM_64BIT:
                {
                    paParmsDst[i].u.uint64 = paParmsSrc[i].u.uint64;
                    break;
                }
                case VBOX_HGCM_SVC_PARM_PTR:
                {
                    /* Do we have to perform a deep copy? */
                    if (fDeepCopy)
                    {
                        /* Yes, do so. */
                        paParmsDst[i].u.pointer.size = paParmsSrc[i].u.pointer.size;
                        if (paParmsDst[i].u.pointer.size > 0)
                        {
                            paParmsDst[i].u.pointer.addr = RTMemAlloc(paParmsDst[i].u.pointer.size);
                            if (!paParmsDst[i].u.pointer.addr)
                            {
                                rc = VERR_NO_MEMORY;
                                break;
                            }
                        }
                    }
                    else
                    {
                        /* No, but we have to check if there is enough room. */
                        if (paParmsDst[i].u.pointer.size < paParmsSrc[i].u.pointer.size)
                        {
                            rc = VERR_BUFFER_OVERFLOW;
                            break;
                        }
                    }

                    if (paParmsSrc[i].u.pointer.size)
                    {
                        if (   paParmsDst[i].u.pointer.addr
                            && paParmsDst[i].u.pointer.size)
                        {
                            memcpy(paParmsDst[i].u.pointer.addr,
                                   paParmsSrc[i].u.pointer.addr,
                                   RT_MIN(paParmsDst[i].u.pointer.size, paParmsSrc[i].u.pointer.size));
                        }
                        else
                            rc = VERR_INVALID_POINTER;
                    }
                    break;
                }
                default:
                {
                    AssertMsgFailed(("Unknown HGCM type %u\n", paParmsSrc[i].type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }
            if (RT_FAILURE(rc))
                break;
        }
        return rc;
    }

    void cleanup()
    {
        if (m_paParms)
        {
            for (uint32_t i = 0; i < m_cParms; ++i)
            {
                switch (m_paParms[i].type)
                {
                    case VBOX_HGCM_SVC_PARM_PTR:
                        if (m_paParms[i].u.pointer.size)
                            RTMemFree(m_paParms[i].u.pointer.addr);
                        break;
                }
            }
            RTMemFree(m_paParms);
            m_paParms = 0;
        }
        m_cParms = 0;
        m_uMsg = 0;
    }
};

class Client
{
public:
    Client(uint32_t uClientId, VBOXHGCMCALLHANDLE hHandle = NULL,
           uint32_t uMsg = 0, uint32_t cParms = 0, VBOXHGCMSVCPARM aParms[] = NULL)
      : m_uClientId(uClientId)
      , m_uProtocol(0)
      , m_hHandle(hHandle)
      , m_uMsg(uMsg)
      , m_cParms(cParms)
      , m_paParms(aParms) {}

public:

    VBOXHGCMCALLHANDLE handle(void) const { return m_hHandle; }
    uint32_t message(void) const { return m_uMsg; }
    uint32_t clientId(void) const { return m_uClientId; }
    uint32_t protocol(void) const { return m_uProtocol; }

public:

    int setProtocol(uint32_t uProtocol) { m_uProtocol = uProtocol; return VINF_SUCCESS; }

public:

    int addMessageInfo(uint32_t uMsg, uint32_t cParms)
    {
        if (m_cParms != 3)
            return VERR_INVALID_PARAMETER;

        m_paParms[0].setUInt32(uMsg);
        m_paParms[1].setUInt32(cParms);

        return VINF_SUCCESS;
    }
    int addMessageInfo(const Message *pMessage)
    {
        AssertPtrReturn(pMessage, VERR_INVALID_POINTER);
        if (m_cParms != 3)
            return VERR_INVALID_PARAMETER;

        m_paParms[0].setUInt32(pMessage->message());
        m_paParms[1].setUInt32(pMessage->paramsCount());

        return VINF_SUCCESS;
    }
    int addMessage(const Message *pMessage)
    {
        AssertPtrReturn(pMessage, VERR_INVALID_POINTER);
        return pMessage->getData(m_uMsg, m_cParms, m_paParms);
    }

protected:

    uint32_t m_uClientId;
    /** Optional protocol version the client uses. */
    uint32_t m_uProtocol;
    VBOXHGCMCALLHANDLE m_hHandle;
    uint32_t m_uMsg;
    uint32_t m_cParms;
    PVBOXHGCMSVCPARM m_paParms;
};

/**
 * Structure for keeping a HGCM service context.
 */
typedef struct VBOXHGCMSVCTX
{
    /** HGCM helper functions. */
    PVBOXHGCMSVCHELPERS pHelpers;
    /*
     * Callback function supplied by the host for notification of updates
     * to properties.
     */
    PFNHGCMSVCEXT       pfnHostCallback;
    /** User data pointer to be supplied to the host callback function. */
    void               *pvHostData;
} VBOXHGCMSVCTX, *PVBOXHGCMSVCTX;

template <class T>
class AbstractService: public RTCNonCopyable
{
public:
    /**
     * @copydoc VBOXHGCMSVCLOAD
     */
    static DECLCALLBACK(int) svcLoad(VBOXHGCMSVCFNTABLE *pTable)
    {
        LogFlowFunc(("ptable = %p\n", pTable));
        int rc = VINF_SUCCESS;

        if (!VALID_PTR(pTable))
            rc = VERR_INVALID_PARAMETER;
        else
        {
            LogFlowFunc(("ptable->cbSize = %d, ptable->u32Version = 0x%08X\n", pTable->cbSize, pTable->u32Version));

            if (   pTable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
                || pTable->u32Version != VBOX_HGCM_SVC_VERSION)
                rc = VERR_VERSION_MISMATCH;
            else
            {
                RT_GCC_NO_WARN_DEPRECATED_BEGIN
                std::auto_ptr<AbstractService> apService;
                /* No exceptions may propagate outside. */
                try
                {
                    apService = std::auto_ptr<AbstractService>(new T(pTable->pHelpers));
                } catch (int rcThrown)
                {
                    rc = rcThrown;
                } catch (...)
                {
                    rc = VERR_UNRESOLVED_ERROR;
                }
                RT_GCC_NO_WARN_DEPRECATED_END
                if (RT_SUCCESS(rc))
                {
                    /*
                     * We don't need an additional client data area on the host,
                     * because we're a class which can have members for that :-).
                     */
                    pTable->cbClient = 0;

                    /* These functions are mandatory */
                    pTable->pfnUnload             = svcUnload;
                    pTable->pfnConnect            = svcConnect;
                    pTable->pfnDisconnect         = svcDisconnect;
                    pTable->pfnCall               = svcCall;
                    /* Clear obligatory functions. */
                    pTable->pfnHostCall           = NULL;
                    pTable->pfnSaveState          = NULL;
                    pTable->pfnLoadState          = NULL;
                    pTable->pfnRegisterExtension  = NULL;

                    /* Let the service itself initialize. */
                    rc = apService->init(pTable);

                    /* Only on success stop the auto release of the auto_ptr. */
                    if (RT_SUCCESS(rc))
                        pTable->pvService = apService.release();
                }
            }
        }

        LogFlowFunc(("returning %Rrc\n", rc));
        return rc;
    }
    virtual ~AbstractService() {};

protected:
    explicit AbstractService(PVBOXHGCMSVCHELPERS pHelpers)
    {
        RT_ZERO(m_SvcCtx);
        m_SvcCtx.pHelpers = pHelpers;
    }
    virtual int  init(VBOXHGCMSVCFNTABLE *ptable) { RT_NOREF1(ptable); return VINF_SUCCESS; }
    virtual int  uninit() { return VINF_SUCCESS; }
    virtual int  clientConnect(uint32_t u32ClientID, void *pvClient) = 0;
    virtual int  clientDisconnect(uint32_t u32ClientID, void *pvClient) = 0;
    virtual void guestCall(VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID, void *pvClient, uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]) = 0;
    virtual int  hostCall(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
    { RT_NOREF3(eFunction, cParms, paParms); return VINF_SUCCESS; }

    /** Type definition for use in callback functions. */
    typedef AbstractService SELF;
    /** The HGCM service context this service is bound to. */
    VBOXHGCMSVCTX m_SvcCtx;

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnUnload
     * Simply deletes the service object
     */
    static DECLCALLBACK(int) svcUnload(void *pvService)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->uninit();
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            delete pSelf;
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcConnect(void *pvService,
                                        uint32_t u32ClientID,
                                        void *pvClient)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, u32ClientID=%u, pvClient=%p\n", pvService, u32ClientID, pvClient));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->clientConnect(u32ClientID, pvClient);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcDisconnect(void *pvService,
                                           uint32_t u32ClientID,
                                           void *pvClient)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, u32ClientID=%u, pvClient=%p\n", pvService, u32ClientID, pvClient));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->clientDisconnect(u32ClientID, pvClient);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnCall
     * Wraps to the call member function
     */
    static DECLCALLBACK(void) svcCall(void * pvService,
                                      VBOXHGCMCALLHANDLE callHandle,
                                      uint32_t u32ClientID,
                                      void *pvClient,
                                      uint32_t u32Function,
                                      uint32_t cParms,
                                      VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturnVoid(VALID_PTR(pvService));
        LogFlowFunc(("pvService=%p, callHandle=%p, u32ClientID=%u, pvClient=%p, u32Function=%u, cParms=%u, paParms=%p\n", pvService, callHandle, u32ClientID, pvClient, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->guestCall(callHandle, u32ClientID, pvClient, u32Function, cParms, paParms);
        LogFlowFunc(("returning\n"));
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnHostCall
     * Wraps to the hostCall member function
     */
    static DECLCALLBACK(int) svcHostCall(void *pvService,
                                         uint32_t u32Function,
                                         uint32_t cParms,
                                         VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, u32Function=%u, cParms=%u, paParms=%p\n", pvService, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->hostCall(u32Function, cParms, paParms);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnRegisterExtension
     * Installs a host callback for notifications of property changes.
     */
    static DECLCALLBACK(int) svcRegisterExtension(void *pvService,
                                                  PFNHGCMSVCEXT pfnExtension,
                                                  void *pvExtension)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, pfnExtension=%p, pvExtention=%p\n", pvService, pfnExtension, pvExtension));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->m_SvcCtx.pfnHostCallback = pfnExtension;
        pSelf->m_SvcCtx.pvHostData      = pvExtension;
        return VINF_SUCCESS;
    }

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AbstractService);
};

}

#endif /* !___VBox_HostService_Service_h */

