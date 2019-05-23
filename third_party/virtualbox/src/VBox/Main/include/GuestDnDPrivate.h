/* $Id: GuestDnDPrivate.h $ */
/** @file
 * Private guest drag and drop code, used by GuestDnDTarget +
 * GuestDnDSource.
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
 */

#ifndef ____H_GUESTDNDPRIVATE
#define ____H_GUESTDNDPRIVATE

#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>

#include <VBox/hgcmsvc.h> /* For PVBOXHGCMSVCPARM. */
#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/DragAndDropSvc.h>

/**
 * Forward prototype declarations.
 */
class Guest;
class GuestDnDBase;
class GuestDnDResponse;
class GuestDnDSource;
class GuestDnDTarget;
class Progress;

/**
 * Type definitions.
 */

/** List (vector) of MIME types. */
typedef std::vector<com::Utf8Str> GuestDnDMIMEList;

/*
 ** @todo Put most of the implementations below in GuestDnDPrivate.cpp!
 */

class GuestDnDCallbackEvent
{
public:

    GuestDnDCallbackEvent(void)
        : mSemEvent(NIL_RTSEMEVENT)
        , mRc(VINF_SUCCESS) { }

    virtual ~GuestDnDCallbackEvent(void);

public:

    int Reset(void);

    int Notify(int rc = VINF_SUCCESS);

    int Result(void) const { return mRc; }

    int Wait(RTMSINTERVAL msTimeout);

protected:

    /** Event semaphore to notify on error/completion. */
    RTSEMEVENT mSemEvent;
    /** Callback result. */
    int        mRc;
};

/**
 * Class for handling the (raw) meta data.
 */
class GuestDnDMetaData
{
public:

    GuestDnDMetaData(void)
        : pvData(NULL)
        , cbData(0)
        , cbDataUsed(0) { }

    virtual ~GuestDnDMetaData(void)
    {
        reset();
    }

public:

    uint32_t add(const void *pvDataAdd, uint32_t cbDataAdd)
    {
        LogFlowThisFunc(("pvDataAdd=%p, cbDataAdd=%zu\n", pvDataAdd, cbDataAdd));

        if (!cbDataAdd)
            return 0;
        AssertPtrReturn(pvDataAdd, 0);

        int rc = resize(cbData + cbDataAdd);
        if (RT_FAILURE(rc))
            return 0;

        Assert(cbData >= cbDataUsed + cbDataAdd);
        memcpy((uint8_t *)pvData + cbDataUsed, pvDataAdd, cbDataAdd);

        cbDataUsed += cbDataAdd;

        return cbDataAdd;
    }

    uint32_t add(const std::vector<BYTE> &vecAdd)
    {
        if (!vecAdd.size())
            return 0;

        if (vecAdd.size() > UINT32_MAX) /* Paranoia. */
            return 0;

        return add(&vecAdd.front(), (uint32_t)vecAdd.size());
    }

    void reset(void)
    {
        if (pvData)
        {
            Assert(cbData);
            RTMemFree(pvData);
            pvData = NULL;
        }

        cbData     = 0;
        cbDataUsed = 0;
    }

    const void *getData(void) const { return pvData; }

    void *getDataMutable(void) { return pvData; }

    uint32_t getSize(void) const { return cbDataUsed; }

public:

    int fromString(const RTCString &strData)
    {
        int rc = VINF_SUCCESS;

        if (strData.isNotEmpty())
        {
            const uint32_t cbStrData = (uint32_t)strData.length() + 1; /* Include terminating zero. */
            rc = resize(cbStrData);
            if (RT_SUCCESS(rc))
                memcpy(pvData, strData.c_str(), cbStrData);
        }

        return rc;
    }

    int fromURIList(const DnDURIList &lstURI)
    {
        return fromString(lstURI.RootToString());
    }

protected:

    int resize(uint32_t cbSize)
    {
        if (!cbSize)
        {
            reset();
            return VINF_SUCCESS;
        }

        if (cbSize == cbData)
            return VINF_SUCCESS;

        void *pvTmp = NULL;
        if (!cbData)
        {
            Assert(cbDataUsed == 0);
            pvTmp = RTMemAllocZ(cbSize);
        }
        else
        {
            AssertPtr(pvData);
            pvTmp = RTMemRealloc(pvData, cbSize);
            RT_BZERO(pvTmp, cbSize);
        }

        if (pvTmp)
        {
            pvData = pvTmp;
            cbData = cbSize;
            return VINF_SUCCESS;
        }

        return VERR_NO_MEMORY;
    }

protected:

    void     *pvData;
    uint32_t  cbData;
    uint32_t  cbDataUsed;
};

/**
 * Class for keeping drag and drop (meta) data
 * to be sent/received.
 */
class GuestDnDData
{
public:

    GuestDnDData(void)
        : cbEstTotal(0)
        , cbEstMeta(0)
        , cbProcessed(0)
    {
        RT_ZERO(dataHdr);
    }

    virtual ~GuestDnDData(void)
    {
        reset();
    }

public:

    uint64_t addProcessed(uint32_t cbDataAdd)
    {
        const uint64_t cbTotal = getTotal(); NOREF(cbTotal);
        Assert(cbProcessed + cbDataAdd <= cbTotal);
        cbProcessed += cbDataAdd;
        return cbProcessed;
    }

    bool isComplete(void) const
    {
        const uint64_t cbTotal = getTotal();
        LogFlowFunc(("cbProcessed=%RU64, cbTotal=%RU64\n", cbProcessed, cbTotal));
        Assert(cbProcessed <= cbTotal);
        return (cbProcessed == cbTotal);
    }

    void *getChkSumMutable(void) { return dataHdr.pvChecksum; }

    uint32_t getChkSumSize(void) const { return dataHdr.cbChecksum; }

    void *getFmtMutable(void) { return dataHdr.pvMetaFmt; }

    uint32_t getFmtSize(void) const { return dataHdr.cbMetaFmt; }

    GuestDnDMetaData &getMeta(void) { return dataMeta; }

    uint8_t getPercentComplete(void) const
    {
        int64_t cbTotal = RT_MAX(getTotal(), 1);
        return (uint8_t)(cbProcessed * 100 / cbTotal);
    }

    uint64_t getProcessed(void) const { return cbProcessed; }

    uint64_t getRemaining(void) const
    {
        const uint64_t cbTotal = getTotal();
        Assert(cbProcessed <= cbTotal);
        return cbTotal - cbProcessed;
    }

    uint64_t getTotal(void) const { return cbEstTotal; }

    void reset(void)
    {
        clearFmt();
        clearChkSum();

        RT_ZERO(dataHdr);

        dataMeta.reset();

        cbEstTotal  = 0;
        cbEstMeta   = 0;
        cbProcessed = 0;
    }

    int setFmt(const void *pvFmt, uint32_t cbFmt)
    {
        if (cbFmt)
        {
            AssertPtrReturn(pvFmt, VERR_INVALID_POINTER);
            void *pvFmtTmp = RTMemAlloc(cbFmt);
            if (!pvFmtTmp)
                return VERR_NO_MEMORY;

            clearFmt();

            memcpy(pvFmtTmp, pvFmt, cbFmt);

            dataHdr.pvMetaFmt = pvFmtTmp;
            dataHdr.cbMetaFmt = cbFmt;
        }
        else
            clearFmt();

        return VINF_SUCCESS;
    }

    void setEstimatedSize(uint64_t cbTotal, uint32_t cbMeta)
    {
        Assert(cbMeta <= cbTotal);

        LogFlowFunc(("cbTotal=%RU64, cbMeta=%RU32\n", cbTotal, cbMeta));

        cbEstTotal = cbTotal;
        cbEstMeta  = cbMeta;
    }

protected:

    void clearChkSum(void)
    {
        if (dataHdr.pvChecksum)
        {
            Assert(dataHdr.cbChecksum);
            RTMemFree(dataHdr.pvChecksum);
            dataHdr.pvChecksum = NULL;
        }

        dataHdr.cbChecksum = 0;
    }

    void clearFmt(void)
    {
        if (dataHdr.pvMetaFmt)
        {
            Assert(dataHdr.cbMetaFmt);
            RTMemFree(dataHdr.pvMetaFmt);
            dataHdr.pvMetaFmt = NULL;
        }

        dataHdr.cbMetaFmt = 0;
    }

protected:

    /** The data header. */
    VBOXDNDDATAHDR    dataHdr;
    /** For storing the actual meta data.
     *  This might be an URI list or just plain raw data,
     *  according to the format being sent. */
    GuestDnDMetaData  dataMeta;
    /** Estimated total data size when receiving data. */
    uint64_t          cbEstTotal;
    /** Estimated meta data size when receiving data. */
    uint32_t          cbEstMeta;
    /** Overall size (in bytes) of processed data. */
    uint64_t          cbProcessed;
};

/** Initial state. */
#define DND_OBJCTX_STATE_NONE           0
/** The header was received/sent. */
#define DND_OBJCTX_STATE_HAS_HDR        RT_BIT(0)

/**
 * Structure for keeping a DnDURIObject context around.
 */
class GuestDnDURIObjCtx
{
public:

    GuestDnDURIObjCtx(void)
        : pObjURI(NULL)
        , fIntermediate(false)
        , fState(DND_OBJCTX_STATE_NONE) { }

    virtual ~GuestDnDURIObjCtx(void)
    {
        destroy();
    }

public:

    int createIntermediate(DnDURIObject::Type enmType = DnDURIObject::Unknown)
    {
        reset();

        int rc;

        try
        {
            pObjURI       = new DnDURIObject(enmType);
            fIntermediate = true;

            rc = VINF_SUCCESS;
        }
        catch (std::bad_alloc &)
        {
            rc = VERR_NO_MEMORY;
        }

        LogThisFunc(("Returning %Rrc\n", rc));
        return rc;
    }

    void destroy(void)
    {
        LogFlowThisFuncEnter();

        if (   pObjURI
            && fIntermediate)
        {
            delete pObjURI;
        }

        pObjURI       = NULL;
        fIntermediate = false;
    }

    DnDURIObject *getObj(void) const { return pObjURI; }

    bool isIntermediate(void) { return fIntermediate; }

    bool isValid(void) const { return (pObjURI != NULL); }

    uint32_t getState(void) const { return fState; }

    void reset(void)
    {
        LogFlowThisFuncEnter();

        destroy();

        fIntermediate = false;
        fState        = 0;
    }

    void setObj(DnDURIObject *pObj)
    {
        LogFlowThisFunc(("%p\n", pObj));

        destroy();

        pObjURI = pObj;
    }

    uint32_t setState(uint32_t fStateNew)
    {
        /** @todo Add validation. */
        fState = fStateNew;
        return fState;
    }

protected:

    /** Pointer to current object being handled. */
    DnDURIObject             *pObjURI;
    /** Flag whether pObjURI needs deletion after use. */
    bool                      fIntermediate;
    /** Internal context state, corresponding to DND_OBJCTX_STATE_XXX. */
    uint32_t                  fState;
    /** @todo Add more statistics / information here. */
};

/**
 * Structure for keeping around an URI (data) transfer.
 */
class GuestDnDURIData
{

public:

    GuestDnDURIData(void)
        : cObjToProcess(0)
        , cObjProcessed(0)
        , pvScratchBuf(NULL)
        , cbScratchBuf(0) { }

    virtual ~GuestDnDURIData(void)
    {
        reset();

        if (pvScratchBuf)
        {
            Assert(cbScratchBuf);
            RTMemFree(pvScratchBuf);
            pvScratchBuf = NULL;
        }
        cbScratchBuf = 0;
    }

    DnDDroppedFiles &getDroppedFiles(void) { return droppedFiles; }

    DnDURIList &getURIList(void) { return lstURI; }

    int init(size_t cbBuf = _64K)
    {
        reset();

        pvScratchBuf = RTMemAlloc(cbBuf);
        if (!pvScratchBuf)
            return VERR_NO_MEMORY;

        cbScratchBuf = cbBuf;
        return VINF_SUCCESS;
    }

    bool isComplete(void) const
    {
        LogFlowFunc(("cObjProcessed=%RU64, cObjToProcess=%RU64\n", cObjProcessed, cObjToProcess));

        if (!cObjToProcess) /* Always return true if we don't have an object count. */
            return true;

        Assert(cObjProcessed <= cObjToProcess);
        return (cObjProcessed == cObjToProcess);
    }

    const void *getBuffer(void) const { return pvScratchBuf; }

    void *getBufferMutable(void) { return pvScratchBuf; }

    size_t getBufferSize(void) const { return cbScratchBuf; }

    GuestDnDURIObjCtx &getObj(uint64_t uID = 0)
    {
        RT_NOREF(uID);
        AssertMsg(uID == 0, ("Other objects than object 0 is not supported yet\n"));
        return objCtx;
    }

    GuestDnDURIObjCtx &getObjCurrent(void)
    {
        DnDURIObject *pCurObj = lstURI.First();
        if (   !lstURI.IsEmpty()
            && pCurObj)
        {
            /* Point the context object to the current DnDURIObject to process. */
            objCtx.setObj(pCurObj);
        }
        else
            objCtx.reset();

        return objCtx;
    }

    uint64_t getObjToProcess(void) const { return cObjToProcess; }

    uint64_t getObjProcessed(void) const { return cObjProcessed; }

    int processObject(const DnDURIObject &Obj)
    {
        int rc;

        /** @todo Find objct in lstURI first! */
        switch (Obj.GetType())
        {
            case DnDURIObject::Directory:
            case DnDURIObject::File:
                rc = VINF_SUCCESS;
                break;

            default:
                rc = VERR_NOT_IMPLEMENTED;
                break;
        }

        if (RT_SUCCESS(rc))
        {
            if (cObjToProcess)
            {
                cObjProcessed++;
                Assert(cObjProcessed <= cObjToProcess);
            }
        }

        return rc;
    }

    void removeObjCurrent(void)
    {
        if (cObjToProcess)
        {
            cObjProcessed++;
            Assert(cObjProcessed <= cObjToProcess);
        }

        lstURI.RemoveFirst();
        objCtx.reset();
    }

    void reset(void)
    {
        LogFlowFuncEnter();

        cObjToProcess = 0;
        cObjProcessed = 0;

        droppedFiles.Close();

        lstURI.Clear();
        objCtx.reset();
    }

    void setEstimatedObjects(uint64_t cObjs)
    {
        Assert(cObjToProcess == 0);
        cObjToProcess = cObjs;
        LogFlowFunc(("cObjToProcess=%RU64\n", cObjs));
    }

public:

    int fromLocalMetaData(const GuestDnDMetaData &Data)
    {
        reset();

        if (!Data.getSize())
            return VINF_SUCCESS;

        char *pszList;
        int rc = RTStrCurrentCPToUtf8(&pszList, (const char *)Data.getData());
        if (RT_FAILURE(rc))
        {
            LogFlowThisFunc(("String conversion failed with rc=%Rrc\n", rc));
            return rc;
        }

        const size_t cbList = Data.getSize();
        LogFlowThisFunc(("metaData=%p, cbList=%zu\n", &Data, cbList));

        if (cbList)
        {
            RTCList<RTCString> lstURIOrg = RTCString(pszList, cbList).split("\r\n");
            if (!lstURIOrg.isEmpty())
            {
                /* Note: All files to be transferred will be kept open during the entire DnD
                 *       operation, also to keep the accounting right. */
                rc = lstURI.AppendURIPathsFromList(lstURIOrg, DNDURILIST_FLAGS_KEEP_OPEN);
                if (RT_SUCCESS(rc))
                    cObjToProcess = lstURI.TotalCount();
            }
        }

        RTStrFree(pszList);
        return rc;
    }

    int fromRemoteMetaData(const GuestDnDMetaData &Data)
    {
        LogFlowFuncEnter();

        int rc = lstURI.RootFromURIData(Data.getData(), Data.getSize(), 0 /* uFlags */);
        if (RT_SUCCESS(rc))
        {
            const size_t cRootCount = lstURI.RootCount();
            LogFlowFunc(("cRootCount=%zu, cObjToProcess=%RU64\n", cRootCount, cObjToProcess));
            if (cRootCount > cObjToProcess)
                rc = VERR_INVALID_PARAMETER;
        }

        return rc;
    }

    int toMetaData(std::vector<BYTE> &vecData)
    {
        const char *pszDroppedFilesDir = droppedFiles.GetDirAbs();

        Utf8Str strURIs = lstURI.RootToString(RTCString(pszDroppedFilesDir));
        size_t cbData = strURIs.length();

        LogFlowFunc(("%zu root URIs (%zu bytes)\n", lstURI.RootCount(), cbData));

        int rc;

        try
        {
            vecData.resize(cbData + 1 /* Include termination */);
            memcpy(&vecData.front(), strURIs.c_str(), cbData);

            rc = VINF_SUCCESS;
        }
        catch (std::bad_alloc &)
        {
            rc = VERR_NO_MEMORY;
        }

        return rc;
    }

protected:

    int processDirectory(const char *pszPath, uint32_t fMode)
    {
        /** @todo Find directory in lstURI first! */
        int rc;

        const char *pszDroppedFilesDir = droppedFiles.GetDirAbs();
        char *pszDir = RTPathJoinA(pszDroppedFilesDir, pszPath);
        if (pszDir)
        {
            rc = RTDirCreateFullPath(pszDir, fMode);
            if (cObjToProcess)
            {
                cObjProcessed++;
                Assert(cObjProcessed <= cObjToProcess);
            }

            RTStrFree(pszDir);
        }
        else
             rc = VERR_NO_MEMORY;

        return rc;
    }

protected:

    /** Number of objects to process. */
    uint64_t                        cObjToProcess;
    /** Number of objects already processed. */
    uint64_t                        cObjProcessed;
    /** Handles all drop files for this operation. */
    DnDDroppedFiles                 droppedFiles;
    /** (Non-recursive) List of URI objects to handle. */
    DnDURIList                      lstURI;
    /** Context to current object being handled.
     *  As we currently do all transfers one after another we
     *  only have one context at a time. */
    GuestDnDURIObjCtx               objCtx;
    /** Pointer to an optional scratch buffer to use for
     *  doing the actual chunk transfers. */
    void                           *pvScratchBuf;
    /** Size (in bytes) of scratch buffer. */
    size_t                          cbScratchBuf;
};

/**
 * Context structure for sending data to the guest.
 */
typedef struct SENDDATACTX
{
    /** Pointer to guest target class this context belongs to. */
    GuestDnDTarget                     *mpTarget;
    /** Pointer to guest response class this context belongs to. */
    GuestDnDResponse                   *mpResp;
    /** Flag indicating whether a file transfer is active and
     *  initiated by the host. */
    bool                                mIsActive;
    /** Target (VM) screen ID. */
    uint32_t                            mScreenID;
    /** Drag'n drop format requested by the guest. */
    com::Utf8Str                        mFmtReq;
    /** Drag'n drop data to send.
     *  This can be arbitrary data or an URI list. */
    GuestDnDData                        mData;
    /** URI data structure. */
    GuestDnDURIData                     mURI;
    /** Callback event to use. */
    GuestDnDCallbackEvent               mCBEvent;

} SENDDATACTX, *PSENDDATACTX;

/**
 * Context structure for receiving data from the guest.
 */
typedef struct RECVDATACTX
{
    /** Pointer to guest source class this context belongs to. */
    GuestDnDSource                     *mpSource;
    /** Pointer to guest response class this context belongs to. */
    GuestDnDResponse                   *mpResp;
    /** Flag indicating whether a file transfer is active and
     *  initiated by the host. */
    bool                                mIsActive;
    /** Formats offered by the guest (and supported by the host). */
    GuestDnDMIMEList                    mFmtOffered;
    /** Original drop format requested to receive from the guest. */
    com::Utf8Str                        mFmtReq;
    /** Intermediate drop format to be received from the guest.
     *  Some original drop formats require a different intermediate
     *  drop format:
     *
     *  Receiving a file link as "text/plain"  requires still to
     *  receive the file from the guest as "text/uri-list" first,
     *  then pointing to the file path on the host with the data
     *  in "text/plain" format returned. */
    com::Utf8Str                        mFmtRecv;
    /** Desired drop action to perform on the host.
     *  Needed to tell the guest if data has to be
     *  deleted e.g. when moving instead of copying. */
    uint32_t                            mAction;
    /** Drag'n drop received from the guest.
     *  This can be arbitrary data or an URI list. */
    GuestDnDData                        mData;
    /** URI data structure. */
    GuestDnDURIData                     mURI;
    /** Callback event to use. */
    GuestDnDCallbackEvent               mCBEvent;

} RECVDATACTX, *PRECVDATACTX;

/**
 * Simple structure for a buffered guest DnD message.
 */
class GuestDnDMsg
{
public:

    GuestDnDMsg(void)
        : uMsg(0)
        , cParms(0)
        , cParmsAlloc(0)
        , paParms(NULL) { }

    virtual ~GuestDnDMsg(void)
    {
        reset();
    }

public:

    PVBOXHGCMSVCPARM getNextParam(void)
    {
        if (cParms >= cParmsAlloc)
        {
            if (!paParms)
                paParms = (PVBOXHGCMSVCPARM)RTMemAlloc(4 * sizeof(VBOXHGCMSVCPARM));
            else
                paParms = (PVBOXHGCMSVCPARM)RTMemRealloc(paParms, (cParmsAlloc + 4) * sizeof(VBOXHGCMSVCPARM));
            if (!paParms)
                throw VERR_NO_MEMORY;
            RT_BZERO(&paParms[cParmsAlloc], 4 * sizeof(VBOXHGCMSVCPARM));
            cParmsAlloc += 4;
        }

        return &paParms[cParms++];
    }

    uint32_t getCount(void) const { return cParms; }
    PVBOXHGCMSVCPARM getParms(void) const { return paParms; }
    uint32_t getType(void) const { return uMsg; }

    void reset(void)
    {
        if (paParms)
        {
            /* Remove deep copies. */
            for (uint32_t i = 0; i < cParms; i++)
            {
                if (   paParms[i].type == VBOX_HGCM_SVC_PARM_PTR
                    && paParms[i].u.pointer.size)
                {
                    AssertPtr(paParms[i].u.pointer.addr);
                    RTMemFree(paParms[i].u.pointer.addr);
                }
            }

            RTMemFree(paParms);
            paParms = NULL;
        }

        uMsg = cParms = cParmsAlloc = 0;
    }

    int setNextPointer(void *pvBuf, uint32_t cbBuf)
    {
        PVBOXHGCMSVCPARM pParm = getNextParam();
        if (!pParm)
            return VERR_NO_MEMORY;

        void *pvTmp = NULL;
        if (pvBuf)
        {
            Assert(cbBuf);
            pvTmp = RTMemDup(pvBuf, cbBuf);
            if (!pvTmp)
                return VERR_NO_MEMORY;
        }

        pParm->setPointer(pvTmp, cbBuf);
        return VINF_SUCCESS;
    }

    int setNextString(const char *pszString)
    {
        PVBOXHGCMSVCPARM pParm = getNextParam();
        if (!pParm)
            return VERR_NO_MEMORY;

        char *pszTemp = RTStrDup(pszString);
        if (!pszTemp)
            return VERR_NO_MEMORY;

        pParm->setString(pszTemp);
        return VINF_SUCCESS;
    }

    int setNextUInt32(uint32_t u32Val)
    {
        PVBOXHGCMSVCPARM pParm = getNextParam();
        if (!pParm)
            return VERR_NO_MEMORY;

        pParm->setUInt32(u32Val);
        return VINF_SUCCESS;
    }

    int setNextUInt64(uint64_t u64Val)
    {
        PVBOXHGCMSVCPARM pParm = getNextParam();
        if (!pParm)
            return VERR_NO_MEMORY;

        pParm->setUInt64(u64Val);
        return VINF_SUCCESS;
    }

    void setType(uint32_t uMsgType) { uMsg = uMsgType; }

protected:

    /** Message type. */
    uint32_t                    uMsg;
    /** Message parameters. */
    uint32_t                    cParms;
    /** Size of array. */
    uint32_t                    cParmsAlloc;
    /** Array of HGCM parameters */
    PVBOXHGCMSVCPARM            paParms;
};

/** Guest DnD callback function definition. */
typedef DECLCALLBACKPTR(int, PFNGUESTDNDCALLBACK) (uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser);

/**
 * Structure for keeping a guest DnD callback.
 * Each callback can handle one HGCM message, however, multiple HGCM messages can be registered
 * to the same callback (function).
 */
typedef struct GuestDnDCallback
{
    GuestDnDCallback(void)
        : uMessgage(0)
        , pfnCallback(NULL)
        , pvUser(NULL) { }

    GuestDnDCallback(PFNGUESTDNDCALLBACK pvCB, uint32_t uMsg, void *pvUsr = NULL)
        : uMessgage(uMsg)
        , pfnCallback(pvCB)
        , pvUser(pvUsr) { }

    /** The HGCM message ID to handle. */
    uint32_t             uMessgage;
    /** Pointer to callback function. */
    PFNGUESTDNDCALLBACK  pfnCallback;
    /** Pointer to user-supplied data. */
    void                *pvUser;

} GuestDnDCallback;

/** Contains registered callback pointers for specific HGCM message types. */
typedef std::map<uint32_t, GuestDnDCallback> GuestDnDCallbackMap;

/** @todo r=andy This class needs to go, as this now is too inflexible when it comes to all
 *               the callback handling/dispatching. It's part of the initial code and only adds
 *               unnecessary complexity. */
class GuestDnDResponse
{

public:

    GuestDnDResponse(const ComObjPtr<Guest>& pGuest);
    virtual ~GuestDnDResponse(void);

public:

    int notifyAboutGuestResponse(void) const;
    int waitForGuestResponse(RTMSINTERVAL msTimeout = 500) const;

    void setAllActions(uint32_t a) { m_allActions = a; }
    uint32_t allActions(void) const { return m_allActions; }

    void setDefAction(uint32_t a) { m_defAction = a; }
    uint32_t defAction(void) const { return m_defAction; }

    void setFormats(const GuestDnDMIMEList &lstFormats) { m_lstFormats = lstFormats; }
    GuestDnDMIMEList formats(void) const { return m_lstFormats; }

    void reset(void);

    bool isProgressCanceled(void) const;
    int setCallback(uint32_t uMsg, PFNGUESTDNDCALLBACK pfnCallback, void *pvUser = NULL);
    int setProgress(unsigned uPercentage, uint32_t uState, int rcOp = VINF_SUCCESS, const Utf8Str &strMsg = "");
    HRESULT resetProgress(const ComObjPtr<Guest>& pParent);
    HRESULT queryProgressTo(IProgress **ppProgress);

public:

    /** @name HGCM callback handling.
       @{ */
    int onDispatch(uint32_t u32Function, void *pvParms, uint32_t cbParms);
    /** @}  */

protected:

    /** Pointer to context this class is tied to. */
    void                 *m_pvCtx;
    /** Event for waiting for response. */
    RTSEMEVENT            m_EventSem;
    /** Default action to perform in case of a
     *  successful drop. */
    uint32_t              m_defAction;
    /** Actions supported by the guest in case of
     *  a successful drop. */
    uint32_t              m_allActions;
    /** Format(s) requested/supported from the guest. */
    GuestDnDMIMEList      m_lstFormats;
    /** Pointer to IGuest parent object. */
    ComObjPtr<Guest>      m_pParent;
    /** Pointer to associated progress object. Optional. */
    ComObjPtr<Progress>   m_pProgress;
    /** Callback map. */
    GuestDnDCallbackMap   m_mapCallbacks;
};

/**
 * Private singleton class for the guest's DnD
 * implementation. Can't be instanciated directly, only via
 * the factory pattern.
 *
 ** @todo Move this into GuestDnDBase.
 */
class GuestDnD
{
public:

    static GuestDnD *createInstance(const ComObjPtr<Guest>& pGuest)
    {
        Assert(NULL == GuestDnD::s_pInstance);
        GuestDnD::s_pInstance = new GuestDnD(pGuest);
        return GuestDnD::s_pInstance;
    }

    static void destroyInstance(void)
    {
        if (GuestDnD::s_pInstance)
        {
            delete GuestDnD::s_pInstance;
            GuestDnD::s_pInstance = NULL;
        }
    }

    static inline GuestDnD *getInstance(void)
    {
        AssertPtr(GuestDnD::s_pInstance);
        return GuestDnD::s_pInstance;
    }

protected:

    GuestDnD(const ComObjPtr<Guest>& pGuest);
    virtual ~GuestDnD(void);

public:

    /** @name Public helper functions.
     * @{ */
    HRESULT           adjustScreenCoordinates(ULONG uScreenId, ULONG *puX, ULONG *puY) const;
    int               hostCall(uint32_t u32Function, uint32_t cParms, PVBOXHGCMSVCPARM paParms) const;
    GuestDnDResponse *response(void) { return m_pResponse; }
    GuestDnDMIMEList  defaultFormats(void) const { return m_strDefaultFormats; }
    /** @}  */

public:

    /** @name Static low-level HGCM callback handler.
     * @{ */
    static DECLCALLBACK(int)   notifyDnDDispatcher(void *pvExtension, uint32_t u32Function, void *pvParms, uint32_t cbParms);
    /** @}  */

    /** @name Static helper methods.
     * @{ */
    static bool                     isFormatInFormatList(const com::Utf8Str &strFormat, const GuestDnDMIMEList &lstFormats);
    static GuestDnDMIMEList         toFormatList(const com::Utf8Str &strFormats);
    static com::Utf8Str             toFormatString(const GuestDnDMIMEList &lstFormats);
    static GuestDnDMIMEList         toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const GuestDnDMIMEList &lstFormatsWanted);
    static GuestDnDMIMEList         toFilteredFormatList(const GuestDnDMIMEList &lstFormatsSupported, const com::Utf8Str &strFormatsWanted);
    static DnDAction_T              toMainAction(uint32_t uAction);
    static std::vector<DnDAction_T> toMainActions(uint32_t uActions);
    static uint32_t                 toHGCMAction(DnDAction_T enmAction);
    static void                     toHGCMActions(DnDAction_T enmDefAction, uint32_t *puDefAction, const std::vector<DnDAction_T> vecAllowedActions, uint32_t *puAllowedActions);
    /** @}  */

protected:

    /** @name Singleton properties.
     * @{ */
    /** List of supported default MIME/Content-type formats. */
    GuestDnDMIMEList           m_strDefaultFormats;
    /** Pointer to guest implementation. */
    const ComObjPtr<Guest>     m_pGuest;
    /** The current (last) response from the guest. At the
     *  moment we only support only response a time (ARQ-style). */
    GuestDnDResponse          *m_pResponse;
    /** @}  */

private:

    /** Staic pointer to singleton instance. */
    static GuestDnD           *s_pInstance;
};

/** Access to the GuestDnD's singleton instance.
 * @todo r=bird: Please add a 'Get' or something to this as it currently looks
 *       like a class instantiation rather than a getter.  Alternatively, use
 *       UPPER_CASE like the coding guideline suggest for macros. */
#define GuestDnDInst() GuestDnD::getInstance()

/** List of pointers to guest DnD Messages. */
typedef std::list<GuestDnDMsg *> GuestDnDMsgList;

/**
 * IDnDBase class implementation for sharing code between
 * IGuestDnDSource and IGuestDnDTarget implementation.
 */
class GuestDnDBase
{
protected:

    GuestDnDBase(void);

protected:

    /** Shared (internal) IDnDBase method implementations.
     * @{ */
    HRESULT i_isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported);
    HRESULT i_getFormats(GuestDnDMIMEList &aFormats);
    HRESULT i_addFormats(const GuestDnDMIMEList &aFormats);
    HRESULT i_removeFormats(const GuestDnDMIMEList &aFormats);

    HRESULT i_getProtocolVersion(ULONG *puVersion);
    /** @}  */

protected:

    int getProtocolVersion(uint32_t *puVersion);

    /** @name Functions for handling a simple host HGCM message queue.
     * @{ */
    int msgQueueAdd(GuestDnDMsg *pMsg);
    GuestDnDMsg *msgQueueGetNext(void);
    void msgQueueRemoveNext(void);
    void msgQueueClear(void);
    /** @}  */

    int sendCancel(void);
    int updateProgress(GuestDnDData *pData, GuestDnDResponse *pResp, uint32_t cbDataAdd = 0);
    int waitForEvent(GuestDnDCallbackEvent *pEvent, GuestDnDResponse *pResp, RTMSINTERVAL msTimeout);

protected:

    /** @name Public attributes (through getters/setters).
     * @{ */
    /** Pointer to guest implementation. */
    const ComObjPtr<Guest>          m_pGuest;
    /** List of supported MIME types by the source. */
    GuestDnDMIMEList                m_lstFmtSupported;
    /** List of offered MIME types to the counterpart. */
    GuestDnDMIMEList                m_lstFmtOffered;
    /** @}  */

    /**
     * Internal stuff.
     */
    struct
    {
        /** Number of active transfers (guest->host or host->guest). */
        uint32_t                    m_cTransfersPending;
        /** The DnD protocol version to use, depending on the
         *  installed Guest Additions. See DragAndDropSvc.h for
         *  a protocol changelog. */
        uint32_t                    m_uProtocolVersion;
        /** Outgoing message queue (FIFO). */
        GuestDnDMsgList             m_lstMsgOut;
    } mDataBase;
};
#endif /* ____H_GUESTDNDPRIVATE */

