/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server.h"
#include "cr_net.h"
#include "cr_unpack.h"
#include "cr_error.h"
#include "cr_glstate.h"
#include "cr_string.h"
#include "cr_mem.h"
#include "cr_hash.h"
#include "cr_vreg.h"
#include "cr_environment.h"
#include "cr_pixeldata.h"

#ifdef VBOX_WITH_CR_DISPLAY_LISTS
# include "cr_dlm.h"
#endif

#include "server_dispatch.h"
#include "state/cr_texture.h"
#include "render/renderspu.h"
#include <signal.h>
#include <stdlib.h>
#define DEBUG_FP_EXCEPTIONS 0
#if DEBUG_FP_EXCEPTIONS
#include <fpu_control.h>
#include <math.h>
#endif
#include <iprt/assert.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/AssertGuest.h>

#ifdef VBOXCR_LOGFPS
#include <iprt/timer.h>
#endif

#ifdef VBOX_WITH_CRHGSMI
# include <VBox/HostServices/VBoxCrOpenGLSvc.h>
uint8_t* g_pvVRamBase = NULL;
uint32_t g_cbVRam = 0;
PPDMLED g_pLed = NULL;

HCRHGSMICMDCOMPLETION g_hCrHgsmiCompletion = NULL;
PFNCRHGSMICMDCOMPLETION g_pfnCrHgsmiCompletion = NULL;
#endif

/**
 * \mainpage CrServerLib
 *
 * \section CrServerLibIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The core module basically takes care of API dispatch,
 * and OpenGL state management.
 */


/**
 * CRServer global data
 */
CRServer cr_server;

int tearingdown = 0; /* can't be static */

static DECLCALLBACK(int8_t) crVBoxCrCmdCmd(HVBOXCRCMDSVR hSvr,
                                           const VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd);

DECLINLINE(CRClient*) crVBoxServerClientById(uint32_t u32ClientID)
{
    int32_t i;

    if (cr_server.fCrCmdEnabled)
        return CrHTableGet(&cr_server.clientTable, u32ClientID);

    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn
            && cr_server.clients[i]->conn->u32ClientID==u32ClientID)
        {
            return cr_server.clients[i];
        }
    }

    return NULL;
}

int32_t crVBoxServerClientGet(uint32_t u32ClientID, CRClient **ppClient)
{
    CRClient *pClient = NULL;

    pClient = crVBoxServerClientById(u32ClientID);

    if (!pClient)
    {
        WARN(("client not found!"));
        *ppClient = NULL;
        return VERR_INVALID_PARAMETER;
    }

    if (!pClient->conn->vMajor)
    {
        WARN(("no major version specified for client!"));
        *ppClient = NULL;
        return VERR_NOT_SUPPORTED;
    }

    *ppClient = pClient;

    return VINF_SUCCESS;
}


/**
 * Return pointer to server's first SPU.
 */
SPU*
crServerHeadSPU(void)
{
     return cr_server.head_spu;
}



static void DeleteBarrierCallback( void *data )
{
    CRServerBarrier *barrier = (CRServerBarrier *) data;
    crFree(barrier->waiting);
    crFree(barrier);
}


static void deleteContextInfoCallback( void *data )
{
    CRContextInfo *c = (CRContextInfo *) data;
    crStateDestroyContext(c->pContext);
    if (c->CreateInfo.pszDpyName)
        crFree(c->CreateInfo.pszDpyName);
    crFree(c);
}

static void deleteMuralInfoCallback( void *data )
{
    CRMuralInfo *m = (CRMuralInfo *) data;
    if (m->spuWindow != CR_RENDER_DEFAULT_WINDOW_ID) /* <- do not do term for default mural as it does not contain any info to be freed,
                                                      * and renderspu will destroy it up itself*/
    {
        crServerMuralTerm(m);
    }
    crFree(m);
}

static int crVBoxServerCrCmdDisablePostProcess(VBOXCRCMDCTL_HGCMENABLE_DATA *pData);

static void crServerTearDown( void )
{
    GLint i;
    CRClientNode *pNode, *pNext;
    GLboolean fOldEnableDiff;
    GLboolean fContextsDeleted = GL_FALSE;

    /* avoid a race condition */
    if (tearingdown)
        return;

    tearingdown = 1;

    if (cr_server.fCrCmdEnabled)
    {
        VBOXCRCMDCTL_HGCMENABLE_DATA EnableData;
        /* crVBoxServerHgcmEnable will erase the DisableData, preserve it here */
        VBOXCRCMDCTL_HGCMDISABLE_DATA DisableData = cr_server.DisableData;
        int rc;

        CRASSERT(DisableData.pfnNotifyTerm);
        rc = DisableData.pfnNotifyTerm(DisableData.hNotifyTerm, &EnableData);
        if (!RT_SUCCESS(rc))
        {
            WARN(("pfnNotifyTerm failed %d", rc));
            return;
        }

        crVBoxServerCrCmdDisablePostProcess(&EnableData);
        fContextsDeleted = GL_TRUE;

        CRASSERT(DisableData.pfnNotifyTermDone);
        DisableData.pfnNotifyTermDone(DisableData.hNotifyTerm);

        Assert(!cr_server.fCrCmdEnabled);
    }

    crStateSetCurrent( NULL );

    cr_server.curClient = NULL;
    cr_server.run_queue = NULL;

    crFree( cr_server.overlap_intens );
    cr_server.overlap_intens = NULL;

    /* needed to make sure window dummy mural not get created on mural destruction
     * and generally this should be zeroed up */
    cr_server.currentCtxInfo = NULL;
    cr_server.currentWindow = -1;
    cr_server.currentNativeWindow = 0;
    cr_server.currentMural = NULL;

    if (!fContextsDeleted)
    {
#ifndef VBOX_WITH_CR_DISPLAY_LISTS
        /* sync our state with renderspu,
         * do it before mural & context deletion to avoid deleting currently set murals/contexts*/
        cr_server.head_spu->dispatch_table.MakeCurrent(CR_RENDER_DEFAULT_WINDOW_ID, 0, CR_RENDER_DEFAULT_CONTEXT_ID);
#endif
    }

    /* Deallocate all semaphores */
    crFreeHashtable(cr_server.semaphores, crFree);
    cr_server.semaphores = NULL;

    /* Deallocate all barriers */
    crFreeHashtable(cr_server.barriers, DeleteBarrierCallback);
    cr_server.barriers = NULL;

#if 0 /** @todo @bugref{8662} -- can trigger SEGFAULTs during savestate */
    /* Free all context info */
    crFreeHashtable(cr_server.contextTable, deleteContextInfoCallback);
#endif

    /* synchronize with reality */
    if (!fContextsDeleted)
    {
        fOldEnableDiff = crStateEnableDiffOnMakeCurrent(GL_FALSE);
        if(cr_server.MainContextInfo.pContext)
            crStateMakeCurrent(cr_server.MainContextInfo.pContext);
        crStateEnableDiffOnMakeCurrent(fOldEnableDiff);
    }

    /* Free vertex programs */
    crFreeHashtable(cr_server.programTable, crFree);

    /* Free murals */
    crFreeHashtable(cr_server.muralTable, deleteMuralInfoCallback);

    CrPMgrTerm();

    if (CrBltIsInitialized(&cr_server.Blitter))
    {
        CrBltTerm(&cr_server.Blitter);
    }

    /* Free dummy murals */
    crFreeHashtable(cr_server.dummyMuralTable, deleteMuralInfoCallback);

    for (i = 0; i < cr_server.numClients; i++) {
        if (cr_server.clients[i]) {
            CRConnection *conn = cr_server.clients[i]->conn;
            crNetFreeConnection(conn);
            crFree(cr_server.clients[i]);
        }
    }
    cr_server.numClients = 0;

    pNode = cr_server.pCleanupClient;
    while (pNode)
    {
        pNext=pNode->next;
        crFree(pNode->pClient);
        crFree(pNode);
        pNode=pNext;
    }
    cr_server.pCleanupClient = NULL;

    if (crServerRpwIsInitialized(&cr_server.RpwWorker))
    {
        crServerRpwTerm(&cr_server.RpwWorker);
    }

#if 1
    /* disable these two lines if trying to get stack traces with valgrind */
    crSPUUnloadChain(cr_server.head_spu);
    cr_server.head_spu = NULL;
#endif

    crStateDestroy();

    crNetTearDown();

    VBoxVrListClear(&cr_server.RootVr);

    VBoxVrTerm();

    RTSemEventDestroy(cr_server.hCalloutCompletionEvent);
}

static void crServerClose( unsigned int id )
{
    crError( "Client disconnected!" );
    (void) id;
}

static void crServerCleanup( int sigio )
{
    crServerTearDown();

    tearingdown = 0;
}


void
crServerSetPort(int port)
{
    cr_server.tcpip_port = port;
}



static void
crPrintHelp(void)
{
    printf("Usage: crserver [OPTIONS]\n");
    printf("Options:\n");
    printf("  -mothership URL  Specifies URL for contacting the mothership.\n");
    printf("                   URL is of the form [protocol://]hostname[:port]\n");
    printf("  -port N          Specifies the port number this server will listen to.\n");
    printf("  -help            Prints this information.\n");
}


/**
 * Do CRServer initializations.  After this, we can begin servicing clients.
 */
void
crServerInit(int argc, char *argv[])
{
    int i;
    const char*env;
    char *mothership = NULL;
    CRMuralInfo *defaultMural;
    int rc = VBoxVrInit();
    if (!RT_SUCCESS(rc))
    {
        crWarning("VBoxVrInit failed, rc %d", rc);
        return;
    }

    for (i = 1 ; i < argc ; i++)
    {
        if (!crStrcmp( argv[i], "-mothership" ))
        {
            if (i == argc - 1)
            {
                crError( "-mothership requires an argument" );
            }
            mothership = argv[i+1];
            i++;
        }
        else if (!crStrcmp( argv[i], "-port" ))
        {
            /* This is the port on which we'll accept client connections */
            if (i == argc - 1)
            {
                crError( "-port requires an argument" );
            }
            cr_server.tcpip_port = crStrToInt(argv[i+1]);
            i++;
        }
        else if (!crStrcmp( argv[i], "-vncmode" ))
        {
            cr_server.vncMode = 1;
        }
        else if (!crStrcmp( argv[i], "-help" ))
        {
            crPrintHelp();
            exit(0);
        }
    }

    signal( SIGTERM, crServerCleanup );
    signal( SIGINT, crServerCleanup );
#ifndef WINDOWS
    signal( SIGPIPE, SIG_IGN );
#endif

#if DEBUG_FP_EXCEPTIONS
    {
        fpu_control_t mask;
        _FPU_GETCW(mask);
        mask &= ~(_FPU_MASK_IM | _FPU_MASK_DM | _FPU_MASK_ZM
                            | _FPU_MASK_OM | _FPU_MASK_UM);
        _FPU_SETCW(mask);
    }
#endif

    cr_server.fCrCmdEnabled = GL_FALSE;
    cr_server.fProcessingPendedCommands = GL_FALSE;
    CrHTableCreate(&cr_server.clientTable, CR_MAX_CLIENTS);

    cr_server.bUseMultipleContexts = (crGetenv( "CR_SERVER_ENABLE_MULTIPLE_CONTEXTS" ) != NULL);

    if (cr_server.bUseMultipleContexts)
    {
        crInfo("Info: using multiple contexts!");
        crDebug("Debug: using multiple contexts!");
    }

    cr_server.firstCallCreateContext = GL_TRUE;
    cr_server.firstCallMakeCurrent = GL_TRUE;
    cr_server.bForceMakeCurrentOnClientSwitch = GL_FALSE;

    /*
     * Create default mural info and hash table.
     */
    cr_server.muralTable = crAllocHashtable();
    defaultMural = (CRMuralInfo *) crCalloc(sizeof(CRMuralInfo));
    defaultMural->spuWindow = CR_RENDER_DEFAULT_WINDOW_ID;
    crHashtableAdd(cr_server.muralTable, 0, defaultMural);

    cr_server.programTable = crAllocHashtable();

    crNetInit(crServerRecv, crServerClose);
    crStateInit();

    crServerSetVBoxConfiguration();

    crStateLimitsInit( &(cr_server.limits) );

    /*
     * Default context
     */
    cr_server.contextTable = crAllocHashtable();
    cr_server.curClient->currentCtxInfo = &cr_server.MainContextInfo;

    cr_server.dummyMuralTable = crAllocHashtable();

    CrPMgrInit();

    cr_server.fRootVrOn = GL_FALSE;
    VBoxVrListInit(&cr_server.RootVr);
    crMemset(&cr_server.RootVrCurPoint, 0, sizeof (cr_server.RootVrCurPoint));

    crMemset(&cr_server.RpwWorker, 0, sizeof (cr_server.RpwWorker));

    env = crGetenv("CR_SERVER_BFB");
    if (env)
    {
        cr_server.fBlitterMode = env[0] - '0';
    }
    else
    {
        cr_server.fBlitterMode = CR_SERVER_BFB_DISABLED;
    }
    crMemset(&cr_server.Blitter, 0, sizeof (cr_server.Blitter));

    crServerInitDispatch();
    crServerInitTmpCtxDispatch();
    crStateDiffAPI( &(cr_server.head_spu->dispatch_table) );

#ifdef VBOX_WITH_CRSERVER_DUMPER
    crMemset(&cr_server.Recorder, 0, sizeof (cr_server.Recorder));
    crMemset(&cr_server.RecorderBlitter, 0, sizeof (cr_server.RecorderBlitter));
    crMemset(&cr_server.DbgPrintDumper, 0, sizeof (cr_server.DbgPrintDumper));
    crMemset(&cr_server.HtmlDumper, 0, sizeof (cr_server.HtmlDumper));
    cr_server.pDumper = NULL;
#endif

    crUnpackSetReturnPointer( &(cr_server.return_ptr) );
    crUnpackSetWritebackPointer( &(cr_server.writeback_ptr) );

    cr_server.barriers = crAllocHashtable();
    cr_server.semaphores = crAllocHashtable();
}

void crVBoxServerTearDown(void)
{
    crServerTearDown();
}

/**
 * Do CRServer initializations.  After this, we can begin servicing clients.
 */
GLboolean crVBoxServerInit(void)
{
    CRMuralInfo *defaultMural;
    const char*env;
    int rc = VBoxVrInit();
    if (!RT_SUCCESS(rc))
    {
        crWarning("VBoxVrInit failed, rc %d", rc);
        return GL_FALSE;
    }

#if DEBUG_FP_EXCEPTIONS
    {
        fpu_control_t mask;
        _FPU_GETCW(mask);
        mask &= ~(_FPU_MASK_IM | _FPU_MASK_DM | _FPU_MASK_ZM
                            | _FPU_MASK_OM | _FPU_MASK_UM);
        _FPU_SETCW(mask);
    }
#endif

    cr_server.fCrCmdEnabled = GL_FALSE;
    cr_server.fProcessingPendedCommands = GL_FALSE;
    CrHTableCreate(&cr_server.clientTable, CR_MAX_CLIENTS);

    cr_server.bUseMultipleContexts = (crGetenv( "CR_SERVER_ENABLE_MULTIPLE_CONTEXTS" ) != NULL);

    if (cr_server.bUseMultipleContexts)
    {
        crInfo("Info: using multiple contexts!");
        crDebug("Debug: using multiple contexts!");
    }

    crNetInit(crServerRecv, crServerClose);

    cr_server.firstCallCreateContext = GL_TRUE;
    cr_server.firstCallMakeCurrent = GL_TRUE;

    cr_server.bIsInLoadingState = GL_FALSE;
    cr_server.bIsInSavingState  = GL_FALSE;
    cr_server.bForceMakeCurrentOnClientSwitch = GL_FALSE;

    cr_server.pCleanupClient = NULL;

    rc = RTSemEventCreate(&cr_server.hCalloutCompletionEvent);
    if (!RT_SUCCESS(rc))
    {
        WARN(("RTSemEventCreate failed %d", rc));
        return GL_FALSE;
    }

    /*
     * Create default mural info and hash table.
     */
    cr_server.muralTable = crAllocHashtable();
    defaultMural = (CRMuralInfo *) crCalloc(sizeof(CRMuralInfo));
    defaultMural->spuWindow = CR_RENDER_DEFAULT_WINDOW_ID;
    crHashtableAdd(cr_server.muralTable, 0, defaultMural);

    cr_server.programTable = crAllocHashtable();

    crStateInit();

    crStateLimitsInit( &(cr_server.limits) );

    cr_server.barriers = crAllocHashtable();
    cr_server.semaphores = crAllocHashtable();

    crUnpackSetReturnPointer( &(cr_server.return_ptr) );
    crUnpackSetWritebackPointer( &(cr_server.writeback_ptr) );

    /*
     * Default context
     */
    cr_server.contextTable = crAllocHashtable();

    cr_server.dummyMuralTable = crAllocHashtable();

    CrPMgrInit();

    cr_server.fRootVrOn = GL_FALSE;
    VBoxVrListInit(&cr_server.RootVr);
    crMemset(&cr_server.RootVrCurPoint, 0, sizeof (cr_server.RootVrCurPoint));

    crMemset(&cr_server.RpwWorker, 0, sizeof (cr_server.RpwWorker));

    env = crGetenv("CR_SERVER_BFB");
    if (env)
    {
        cr_server.fBlitterMode = env[0] - '0';
    }
    else
    {
        cr_server.fBlitterMode = CR_SERVER_BFB_DISABLED;
    }
    crMemset(&cr_server.Blitter, 0, sizeof (cr_server.Blitter));

    crServerSetVBoxConfigurationHGCM();

    if (!cr_server.head_spu)
    {
        crStateDestroy();
        return GL_FALSE;
    }

    crServerInitDispatch();
    crServerInitTmpCtxDispatch();
    crStateDiffAPI( &(cr_server.head_spu->dispatch_table) );

#ifdef VBOX_WITH_CRSERVER_DUMPER
    crMemset(&cr_server.Recorder, 0, sizeof (cr_server.Recorder));
    crMemset(&cr_server.RecorderBlitter, 0, sizeof (cr_server.RecorderBlitter));
    crMemset(&cr_server.DbgPrintDumper, 0, sizeof (cr_server.DbgPrintDumper));
    crMemset(&cr_server.HtmlDumper, 0, sizeof (cr_server.HtmlDumper));
    cr_server.pDumper = NULL;
#endif

    /*Check for PBO support*/
    if (crStateGetCurrent()->extensions.ARB_pixel_buffer_object)
    {
        cr_server.bUsePBOForReadback=GL_TRUE;
    }

    return GL_TRUE;
}

static int32_t crVBoxServerAddClientObj(uint32_t u32ClientID, CRClient **ppNewClient)
{
    CRClient *newClient;

    if (cr_server.numClients>=CR_MAX_CLIENTS)
    {
        if (ppNewClient)
            *ppNewClient = NULL;
        return VERR_MAX_THRDS_REACHED;
    }

    newClient = (CRClient *) crCalloc(sizeof(CRClient));
    crDebug("crServer: AddClient u32ClientID=%d", u32ClientID);

    newClient->spu_id = 0;
    newClient->currentCtxInfo = &cr_server.MainContextInfo;
    newClient->currentContextNumber = -1;
    newClient->conn = crNetAcceptClient(cr_server.protocol, NULL,
                                        cr_server.tcpip_port,
                                        cr_server.mtu, 0);
    newClient->conn->u32ClientID = u32ClientID;

    cr_server.clients[cr_server.numClients++] = newClient;

    crServerAddToRunQueue(newClient);

    if (ppNewClient)
        *ppNewClient = newClient;

    return VINF_SUCCESS;
}

int32_t crVBoxServerAddClient(uint32_t u32ClientID)
{
    CRClient *newClient;

    if (cr_server.numClients>=CR_MAX_CLIENTS)
    {
        return VERR_MAX_THRDS_REACHED;
    }

    newClient = (CRClient *) crCalloc(sizeof(CRClient));
    crDebug("crServer: AddClient u32ClientID=%d", u32ClientID);

    newClient->spu_id = 0;
    newClient->currentCtxInfo = &cr_server.MainContextInfo;
    newClient->currentContextNumber = -1;
    newClient->conn = crNetAcceptClient(cr_server.protocol, NULL,
                                        cr_server.tcpip_port,
                                        cr_server.mtu, 0);
    newClient->conn->u32ClientID = u32ClientID;

    cr_server.clients[cr_server.numClients++] = newClient;

    crServerAddToRunQueue(newClient);

    return VINF_SUCCESS;
}

static void crVBoxServerRemoveClientObj(CRClient *pClient)
{
#ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
#endif

    /* Disconnect the client */
    pClient->conn->Disconnect(pClient->conn);

    /* Let server clear client from the queue */
    crServerDeleteClient(pClient);
}

static void crVBoxServerRemoveAllClients()
{
    int32_t i;
    for (i = cr_server.numClients - 1; i >= 0; --i)
    {
        Assert(cr_server.clients[i]);
        crVBoxServerRemoveClientObj(cr_server.clients[i]);
    }
}

void crVBoxServerRemoveClient(uint32_t u32ClientID)
{
    CRClient *pClient=NULL;
    int32_t i;

    crDebug("crServer: RemoveClient u32ClientID=%d", u32ClientID);

    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn
            && cr_server.clients[i]->conn->u32ClientID==u32ClientID)
        {
            pClient = cr_server.clients[i];
            break;
        }
    }
    //if (!pClient) return VERR_INVALID_PARAMETER;
    if (!pClient)
    {
        WARN(("Invalid client id %u passed to crVBoxServerRemoveClient", u32ClientID));
        return;
    }

    crVBoxServerRemoveClientObj(pClient);
}

static void crVBoxServerInternalClientWriteRead(CRClient *pClient)
{
#ifdef VBOXCR_LOGFPS
    uint64_t tstart, tend;
#endif

    /*crDebug("=>crServer: ClientWrite u32ClientID=%d", u32ClientID);*/


#ifdef VBOXCR_LOGFPS
    tstart = RTTimeNanoTS();
#endif

    /* This should be setup already */
    CRASSERT(pClient->conn->pBuffer);
    CRASSERT(pClient->conn->cbBuffer);
#ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(&pClient->conn->CmdData);
#endif

    if (
#ifdef VBOX_WITH_CRHGSMI
         !CRVBOXHGSMI_CMDDATA_IS_SET(&pClient->conn->CmdData) &&
#endif
         cr_server.run_queue->client != pClient
         && crServerClientInBeginEnd(cr_server.run_queue->client))
    {
        crDebug("crServer: client %d blocked, allow_redir_ptr = 0", pClient->conn->u32ClientID);
        pClient->conn->allow_redir_ptr = 0;
    }
    else
    {
        pClient->conn->allow_redir_ptr = 1;
    }

    crNetRecv();
    CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
    CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

    crServerServiceClients();
    crStateResetCurrentPointers(&cr_server.current);

#ifndef VBOX_WITH_CRHGSMI
    CRASSERT(!pClient->conn->allow_redir_ptr || crNetNumMessages(pClient->conn)==0);
#endif

#ifdef VBOXCR_LOGFPS
    tend = RTTimeNanoTS();
    pClient->timeUsed += tend-tstart;
#endif
    /*crDebug("<=crServer: ClientWrite u32ClientID=%d", u32ClientID);*/
}


int32_t crVBoxServerClientWrite(uint32_t u32ClientID, uint8_t *pBuffer, uint32_t cbBuffer)
{
    CRClient *pClient=NULL;
    int32_t rc = crVBoxServerClientGet(u32ClientID, &pClient);

    if (RT_FAILURE(rc))
        return rc;

    CRASSERT(pBuffer);

    /* This should never fire unless we start to multithread */
    CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);

    pClient->conn->pBuffer = pBuffer;
    pClient->conn->cbBuffer = cbBuffer;
#ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
#endif

    crVBoxServerInternalClientWriteRead(pClient);

    return VINF_SUCCESS;
}

int32_t crVBoxServerInternalClientRead(CRClient *pClient, uint8_t *pBuffer, uint32_t *pcbBuffer)
{
    if (pClient->conn->cbHostBuffer > *pcbBuffer)
    {
        crDebug("crServer: [%lx] ClientRead u32ClientID=%d FAIL, host buffer too small %d of %d",
                  crThreadID(), pClient->conn->u32ClientID, *pcbBuffer, pClient->conn->cbHostBuffer);

        /* Return the size of needed buffer */
        *pcbBuffer = pClient->conn->cbHostBuffer;

        return VERR_BUFFER_OVERFLOW;
    }

    *pcbBuffer = pClient->conn->cbHostBuffer;

    if (*pcbBuffer)
    {
        CRASSERT(pClient->conn->pHostBuffer);

        crMemcpy(pBuffer, pClient->conn->pHostBuffer, *pcbBuffer);
        pClient->conn->cbHostBuffer = 0;
    }

    return VINF_SUCCESS;
}

int32_t crVBoxServerClientRead(uint32_t u32ClientID, uint8_t *pBuffer, uint32_t *pcbBuffer)
{
    CRClient *pClient=NULL;
    int32_t rc = crVBoxServerClientGet(u32ClientID, &pClient);

    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_CRHGSMI
    CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
#endif

    return crVBoxServerInternalClientRead(pClient, pBuffer, pcbBuffer);
}

extern DECLEXPORT(int32_t) crVBoxServerClientGetCapsLegacy(uint32_t u32ClientID, uint32_t *pu32Caps)
{
    uint32_t u32Caps = cr_server.u32Caps;
    u32Caps &= ~CR_VBOX_CAP_CMDVBVA;
    *pu32Caps = u32Caps;
    return VINF_SUCCESS;
}

extern DECLEXPORT(int32_t) crVBoxServerClientGetCapsNew(uint32_t u32ClientID, CR_CAPS_INFO *pInfo)
{
    pInfo->u32Caps = cr_server.u32Caps;
    pInfo->u32CmdVbvaVersion = CR_CMDVBVA_VERSION;
    return VINF_SUCCESS;
}

static int32_t crVBoxServerClientObjSetVersion(CRClient *pClient, uint32_t vMajor, uint32_t vMinor)
{
    pClient->conn->vMajor = vMajor;
    pClient->conn->vMinor = vMinor;

    if (vMajor != CR_PROTOCOL_VERSION_MAJOR
        || vMinor != CR_PROTOCOL_VERSION_MINOR)
        return VERR_NOT_SUPPORTED;
    return VINF_SUCCESS;
}

int32_t crVBoxServerClientSetVersion(uint32_t u32ClientID, uint32_t vMajor, uint32_t vMinor)
{
    CRClient *pClient=NULL;
    int32_t i;

    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn
            && cr_server.clients[i]->conn->u32ClientID==u32ClientID)
        {
            pClient = cr_server.clients[i];
            break;
        }
    }
    if (!pClient) return VERR_INVALID_PARAMETER;

    return crVBoxServerClientObjSetVersion(pClient, vMajor, vMinor);
}

static int32_t crVBoxServerClientObjSetPID(CRClient *pClient, uint64_t pid)
{
    pClient->pid = pid;

    return VINF_SUCCESS;
}

int32_t crVBoxServerClientSetPID(uint32_t u32ClientID, uint64_t pid)
{
    CRClient *pClient=NULL;
    int32_t i;

    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn
            && cr_server.clients[i]->conn->u32ClientID==u32ClientID)
        {
            pClient = cr_server.clients[i];
            break;
        }
    }
    if (!pClient) return VERR_INVALID_PARAMETER;

    return crVBoxServerClientObjSetPID(pClient, pid);
}

int
CRServerMain(int argc, char *argv[])
{
    crServerInit(argc, argv);

    crServerSerializeRemoteStreams();

    crServerTearDown();

    tearingdown = 0;

    return 0;
}

static void crVBoxServerSaveMuralCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMI = (CRMuralInfo*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    CRASSERT(pMI && pSSM);

    /* Don't store default mural */
    if (!key) return;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pMI, RT_UOFFSETOF(CRMuralInfo, CreateInfo));
    CRASSERT(rc == VINF_SUCCESS);

    if (pMI->pVisibleRects)
    {
        rc = SSMR3PutMem(pSSM, pMI->pVisibleRects, 4*sizeof(GLint)*pMI->cVisibleRects);
    }

    rc = SSMR3PutMem(pSSM, pMI->ctxUsage, sizeof (pMI->ctxUsage));
    CRASSERT(rc == VINF_SUCCESS);
}

/** @todo add hashtable walker with result info and intermediate abort */
static void crVBoxServerSaveCreateInfoCB(unsigned long key, void *data1, void *data2)
{
    CRCreateInfo_t *pCreateInfo = (CRCreateInfo_t *)data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    CRASSERT(pCreateInfo && pSSM);

    /* Don't store default mural create info */
    if (!key) return;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pCreateInfo, sizeof(*pCreateInfo));
    CRASSERT(rc == VINF_SUCCESS);

    if (pCreateInfo->pszDpyName)
    {
        rc = SSMR3PutStrZ(pSSM, pCreateInfo->pszDpyName);
        CRASSERT(rc == VINF_SUCCESS);
    }
}

static void crVBoxServerSaveCreateInfoFromMuralInfoCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMural = (CRMuralInfo *)data1;
    CRCreateInfo_t CreateInfo;
    CreateInfo.pszDpyName = pMural->CreateInfo.pszDpyName;
    CreateInfo.visualBits = pMural->CreateInfo.requestedVisualBits;
    CreateInfo.externalID = pMural->CreateInfo.externalID;
    crVBoxServerSaveCreateInfoCB(key, &CreateInfo, data2);
}

static void crVBoxServerSaveCreateInfoFromCtxInfoCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *pContextInfo = (CRContextInfo *)data1;
    CRCreateInfo_t CreateInfo;
    CreateInfo.pszDpyName = pContextInfo->CreateInfo.pszDpyName;
    CreateInfo.visualBits = pContextInfo->CreateInfo.requestedVisualBits;
    /* saved state contains internal id */
    CreateInfo.externalID = pContextInfo->pContext->id;
    crVBoxServerSaveCreateInfoCB(key, &CreateInfo, data2);
}

static void crVBoxServerSyncTextureCB(unsigned long key, void *data1, void *data2)
{
    CRTextureObj *pTexture = (CRTextureObj *) data1;
    CRContext *pContext = (CRContext *) data2;

    CRASSERT(pTexture && pContext);
    crStateTextureObjectDiff(pContext, NULL, NULL, pTexture, GL_TRUE);
}

typedef struct CRVBOX_SAVE_STATE_GLOBAL
{
    /* context id -> mural association
     * on context data save, each context will be made current with the corresponding mural from this table
     * thus saving the mural front & back buffer data */
    CRHashTable *contextMuralTable;
    /* mural id -> context info
     * for murals that do not have associated context in contextMuralTable
     * we still need to save*/
    CRHashTable *additionalMuralContextTable;

    PSSMHANDLE pSSM;

    int rc;
} CRVBOX_SAVE_STATE_GLOBAL, *PCRVBOX_SAVE_STATE_GLOBAL;


typedef struct CRVBOX_CTXWND_CTXWALKER_CB
{
    PCRVBOX_SAVE_STATE_GLOBAL pGlobal;
    CRHashTable *usedMuralTable;
    GLuint cAdditionalMurals;
} CRVBOX_CTXWND_CTXWALKER_CB, *PCRVBOX_CTXWND_CTXWALKER_CB;

static void crVBoxServerBuildAdditionalWindowContextMapCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo * pMural = (CRMuralInfo *) data1;
    PCRVBOX_CTXWND_CTXWALKER_CB pData = (PCRVBOX_CTXWND_CTXWALKER_CB)data2;
    CRContextInfo *pContextInfo = NULL;

    if (!pMural->CreateInfo.externalID)
    {
        CRASSERT(!key);
        return;
    }

    if (crHashtableSearch(pData->usedMuralTable, pMural->CreateInfo.externalID))
    {
        Assert(crHashtableGetDataKey(pData->pGlobal->contextMuralTable, pMural, NULL));
        return;
    }

    Assert(!crHashtableGetDataKey(pData->pGlobal->contextMuralTable, pMural, NULL));

    if (cr_server.MainContextInfo.CreateInfo.realVisualBits == pMural->CreateInfo.realVisualBits)
    {
        pContextInfo = &cr_server.MainContextInfo;
    }
    else
    {
        crWarning("different visual bits not implemented!");
        pContextInfo = &cr_server.MainContextInfo;
    }

    crHashtableAdd(pData->pGlobal->additionalMuralContextTable, pMural->CreateInfo.externalID, pContextInfo);
}


typedef struct CRVBOX_CTXWND_WNDWALKER_CB
{
    PCRVBOX_SAVE_STATE_GLOBAL pGlobal;
    CRHashTable *usedMuralTable;
    CRContextInfo *pContextInfo;
    CRMuralInfo * pMural;
} CRVBOX_CTXWND_WNDWALKER_CB, *PCRVBOX_CTXWND_WNDWALKER_CB;

static void crVBoxServerBuildContextWindowMapWindowWalkerCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo * pMural = (CRMuralInfo *) data1;
    PCRVBOX_CTXWND_WNDWALKER_CB pData = (PCRVBOX_CTXWND_WNDWALKER_CB)data2;

    Assert(pData->pMural != pMural);
    Assert(pData->pContextInfo);

    if (pData->pMural)
        return;

    if (!pMural->CreateInfo.externalID)
    {
        CRASSERT(!key);
        return;
    }

    if (!CR_STATE_SHAREDOBJ_USAGE_IS_SET(pMural, pData->pContextInfo->pContext))
        return;

    if (crHashtableSearch(pData->usedMuralTable, pMural->CreateInfo.externalID))
        return;

    CRASSERT(pMural->CreateInfo.realVisualBits == pData->pContextInfo->CreateInfo.realVisualBits);
    pData->pMural = pMural;
}

static void crVBoxServerBuildContextUsedWindowMapCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *pContextInfo = (CRContextInfo *)data1;
    PCRVBOX_CTXWND_CTXWALKER_CB pData = (PCRVBOX_CTXWND_CTXWALKER_CB)data2;

    if (!pContextInfo->currentMural)
        return;

    crHashtableAdd(pData->pGlobal->contextMuralTable, pContextInfo->CreateInfo.externalID, pContextInfo->currentMural);
    crHashtableAdd(pData->usedMuralTable, pContextInfo->currentMural->CreateInfo.externalID, pContextInfo->currentMural);
}

CRMuralInfo * crServerGetDummyMural(GLint visualBits)
{
    CRMuralInfo * pMural = (CRMuralInfo *)crHashtableSearch(cr_server.dummyMuralTable, visualBits);
    if (!pMural)
    {
        GLint id;
        pMural = (CRMuralInfo *) crCalloc(sizeof(CRMuralInfo));
        if (!pMural)
        {
            crWarning("crCalloc failed!");
            return NULL;
        }
        id = crServerMuralInit(pMural, GL_FALSE, visualBits, 0);
        if (id < 0)
        {
            crWarning("crServerMuralInit failed!");
            crFree(pMural);
            return NULL;
        }

        crHashtableAdd(cr_server.dummyMuralTable, visualBits, pMural);
    }

    return pMural;
}

static void crVBoxServerBuildContextUnusedWindowMapCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *pContextInfo = (CRContextInfo *)data1;
    PCRVBOX_CTXWND_CTXWALKER_CB pData = (PCRVBOX_CTXWND_CTXWALKER_CB)data2;
    CRMuralInfo * pMural = NULL;

    if (pContextInfo->currentMural)
        return;

    Assert(crHashtableNumElements(pData->pGlobal->contextMuralTable) <= crHashtableNumElements(cr_server.muralTable) - 1);
    if (crHashtableNumElements(pData->pGlobal->contextMuralTable) < crHashtableNumElements(cr_server.muralTable) - 1)
    {
        CRVBOX_CTXWND_WNDWALKER_CB MuralData;
        MuralData.pGlobal = pData->pGlobal;
        MuralData.usedMuralTable = pData->usedMuralTable;
        MuralData.pContextInfo = pContextInfo;
        MuralData.pMural = NULL;

        crHashtableWalk(cr_server.muralTable, crVBoxServerBuildContextWindowMapWindowWalkerCB, &MuralData);

        pMural = MuralData.pMural;

    }

    if (!pMural)
    {
        pMural = crServerGetDummyMural(pContextInfo->CreateInfo.realVisualBits);
        if (!pMural)
        {
            crWarning("crServerGetDummyMural failed");
            return;
        }
    }
    else
    {
        crHashtableAdd(pData->usedMuralTable, pMural->CreateInfo.externalID, pMural);
        ++pData->cAdditionalMurals;
    }

    crHashtableAdd(pData->pGlobal->contextMuralTable, pContextInfo->CreateInfo.externalID, pMural);
}

static void crVBoxServerBuildSaveStateGlobal(PCRVBOX_SAVE_STATE_GLOBAL pGlobal)
{
    CRVBOX_CTXWND_CTXWALKER_CB Data;
    GLuint cMurals;
    pGlobal->contextMuralTable = crAllocHashtable();
    pGlobal->additionalMuralContextTable = crAllocHashtable();
    /* 1. go through all contexts and match all having currentMural set */
    Data.pGlobal = pGlobal;
    Data.usedMuralTable = crAllocHashtable();
    Data.cAdditionalMurals = 0;
    crHashtableWalk(cr_server.contextTable, crVBoxServerBuildContextUsedWindowMapCB, &Data);

    cMurals = crHashtableNumElements(pGlobal->contextMuralTable);
    CRASSERT(cMurals <= crHashtableNumElements(cr_server.contextTable));
    CRASSERT(cMurals <= crHashtableNumElements(cr_server.muralTable) - 1);
    CRASSERT(cMurals == crHashtableNumElements(Data.usedMuralTable));
    if (cMurals < crHashtableNumElements(cr_server.contextTable))
    {
        Data.cAdditionalMurals = 0;
        crHashtableWalk(cr_server.contextTable, crVBoxServerBuildContextUnusedWindowMapCB, &Data);
    }

    CRASSERT(crHashtableNumElements(pGlobal->contextMuralTable) == crHashtableNumElements(cr_server.contextTable));
    CRASSERT(cMurals + Data.cAdditionalMurals <= crHashtableNumElements(cr_server.muralTable) - 1);
    if (cMurals + Data.cAdditionalMurals < crHashtableNumElements(cr_server.muralTable) - 1)
    {
        crHashtableWalk(cr_server.muralTable, crVBoxServerBuildAdditionalWindowContextMapCB, &Data);
        CRASSERT(cMurals + Data.cAdditionalMurals + crHashtableNumElements(pGlobal->additionalMuralContextTable) == crHashtableNumElements(cr_server.muralTable) - 1);
    }

    crFreeHashtable(Data.usedMuralTable, NULL);
}

static void crVBoxServerFBImageDataTerm(CRFBData *pData)
{
    GLuint i;
    for (i = 0; i < pData->cElements; ++i)
    {
        CRFBDataElement * pEl = &pData->aElements[i];
        if (pEl->pvData)
        {
            crFree(pEl->pvData);
            /* sanity */
            pEl->pvData = NULL;
        }
    }
    pData->cElements = 0;
}

static int crVBoxAddFBDataElement(CRFBData *pData, GLint idFBO, GLenum enmBuffer, GLint width, GLint height, GLenum enmFormat, GLenum enmType)
{
    CRFBDataElement *pEl;

    AssertCompile(sizeof (GLfloat) == 4);
    AssertCompile(sizeof (GLuint) == 4);

    pEl = &pData->aElements[pData->cElements];
    pEl->idFBO = idFBO;
    pEl->enmBuffer = enmBuffer;
    pEl->posX = 0;
    pEl->posY = 0;
    pEl->width = width;
    pEl->height = height;
    pEl->enmFormat = enmFormat;
    pEl->enmType = enmType;
    pEl->cbData = width * height * 4;

    pEl->pvData = crCalloc(pEl->cbData);
    if (!pEl->pvData)
    {
        crVBoxServerFBImageDataTerm(pData);
        crWarning(": crCalloc failed");
        return VERR_NO_MEMORY;
    }

    ++pData->cElements;

    return VINF_SUCCESS;
}

/* Add framebuffer image elements arrording to SSM version. Please refer to cr_version.h
 * in order to distinguish between versions. */
static int crVBoxServerFBImageDataInitEx(CRFBData *pData, CRContextInfo *pCtxInfo, CRMuralInfo *pMural, GLboolean fWrite, uint32_t version, GLuint overrideWidth, GLuint overrideHeight)
{
    CRContext *pContext;
    GLuint i;
    GLfloat *pF;
    GLuint width;
    GLuint height;
    int rc;

    crMemset(pData, 0, sizeof (*pData));

    pContext = pCtxInfo->pContext;

    /* the version should be always actual when we do reads,
     * i.e. it could differ on writes when snapshot is getting loaded */
    CRASSERT(fWrite || version == SHCROGL_SSM_VERSION);

    width = overrideWidth ? overrideWidth : pMural->width;
    height = overrideHeight ? overrideHeight : pMural->height;

    if (!width || !height)
        return VINF_SUCCESS;

    if (pMural)
    {
        if (fWrite)
        {
            if (!pContext->framebufferobject.drawFB)
                pData->idOverrrideFBO = CR_SERVER_FBO_FOR_IDX(pMural, pMural->iCurDrawBuffer);
        }
        else
        {
            if (!pContext->framebufferobject.readFB)
                pData->idOverrrideFBO = CR_SERVER_FBO_FOR_IDX(pMural, pMural->iCurReadBuffer);
        }
    }

    pData->u32Version = version;

    pData->cElements = 0;

    rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0,
        pData->aElements[1].idFBO ? GL_COLOR_ATTACHMENT0 : GL_FRONT, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    AssertReturn(rc == VINF_SUCCESS, rc);

    /* There is a lot of code that assumes we have double buffering, just assert here to print a warning in the log
     * so that we know that something irregular is going on. */
    CRASSERT(pCtxInfo->CreateInfo.requestedVisualBits & CR_DOUBLE_BIT);

    if ((   pCtxInfo->CreateInfo.requestedVisualBits & CR_DOUBLE_BIT)
         || version < SHCROGL_SSM_VERSION_WITH_SINGLE_DEPTH_STENCIL) /* <- Older version had a typo which lead to back always being used,
                                                                      *    no matter what the visual bits are. */
    {
        rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_BB_IDX(pMural)] : 0,
            pData->aElements[1].idFBO ? GL_COLOR_ATTACHMENT0 : GL_BACK, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
        AssertReturn(rc == VINF_SUCCESS, rc);
    }

    if (version < SHCROGL_SSM_VERSION_WITH_SAVED_DEPTH_STENCIL_BUFFER)
        return VINF_SUCCESS;

    if (version < SHCROGL_SSM_VERSION_WITH_SINGLE_DEPTH_STENCIL)
    {
        rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0,
            pMural ? pMural->idDepthStencilRB : 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT);
        AssertReturn(rc == VINF_SUCCESS, rc);

        /* Init to default depth value, just in case. "pData->cElements - 1" because we incremented counter in crVBoxAddFBDataElement(). */
        pF = (GLfloat*)pData->aElements[pData->cElements - 1].pvData;
        for (i = 0; i < width * height; ++i)
            pF[i] = 1.;

        rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0,
            pMural ? pMural->idDepthStencilRB : 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_INT);
        AssertReturn(rc == VINF_SUCCESS, rc);

        return VINF_SUCCESS;
    }

    if (version < SHCROGL_SSM_VERSION_WITH_SEPARATE_DEPTH_STENCIL_BUFFERS)
    {
        /* Use GL_DEPTH_STENCIL only in case if both CR_STENCIL_BIT and CR_DEPTH_BIT specified. */
        if (   (pCtxInfo->CreateInfo.requestedVisualBits & CR_STENCIL_BIT)
            && (pCtxInfo->CreateInfo.requestedVisualBits & CR_DEPTH_BIT))
        {
            rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0, 0,
                width, height, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
            AssertReturn(rc == VINF_SUCCESS, rc);
        }

        return VINF_SUCCESS;
    }

    /* Current SSM verion (SHCROGL_SSM_VERSION_WITH_SEPARATE_DEPTH_STENCIL_BUFFERS). */

    if (pCtxInfo->CreateInfo.requestedVisualBits & CR_DEPTH_BIT)
    {
        rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0,
            pMural ? pMural->idDepthStencilRB : 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT);
        AssertReturn(rc == VINF_SUCCESS, rc);

        /* Init to default depth value, just in case. "pData->cElements - 1" because we incremented counter in crVBoxAddFBDataElement(). */
        pF = (GLfloat*)pData->aElements[pData->cElements - 1].pvData;
        for (i = 0; i < width * height; ++i)
            pF[i] = 1.;
    }

    if (pCtxInfo->CreateInfo.requestedVisualBits & CR_STENCIL_BIT)
    {
        rc = crVBoxAddFBDataElement(pData, pMural && pMural->fRedirected ? pMural->aidFBOs[CR_SERVER_FBO_FB_IDX(pMural)] : 0,
            pMural ? pMural->idDepthStencilRB : 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_INT);
        AssertReturn(rc == VINF_SUCCESS, rc);
    }

    return VINF_SUCCESS;
}

static int crVBoxServerSaveFBImage(PSSMHANDLE pSSM)
{
    CRContextInfo *pCtxInfo;
    CRContext *pContext;
    CRMuralInfo *pMural;
    int32_t rc;
    GLuint i;
    struct
    {
        CRFBData data;
        CRFBDataElement buffer[3]; /* CRFBData::aElements[1] + buffer[3] gives 4: back, front, depth and stencil  */
    } Data;

    Assert(sizeof (Data) >= RT_UOFFSETOF(CRFBData, aElements[4]));

    pCtxInfo = cr_server.currentCtxInfo;
    pContext = pCtxInfo->pContext;
    pMural = pCtxInfo->currentMural;

    rc = crVBoxServerFBImageDataInitEx(&Data.data, pCtxInfo, pMural, GL_FALSE, SHCROGL_SSM_VERSION, 0, 0);
    if (!RT_SUCCESS(rc))
    {
        crWarning("crVBoxServerFBImageDataInit failed rc %d", rc);
        return rc;
    }

    rc = crStateAcquireFBImage(pContext, &Data.data);
    AssertRCReturn(rc, rc);

    for (i = 0; i < Data.data.cElements; ++i)
    {
        CRFBDataElement * pEl = &Data.data.aElements[i];
        rc = SSMR3PutMem(pSSM, pEl->pvData, pEl->cbData);
        AssertRCReturn(rc, rc);
    }

    crVBoxServerFBImageDataTerm(&Data.data);

    return VINF_SUCCESS;
}

#define CRSERVER_ASSERTRC_RETURN_VOID(_rc) do { \
        if(!RT_SUCCESS((_rc))) { \
            AssertFailed(); \
            return; \
        } \
    } while (0)

static void crVBoxServerSaveAdditionalMuralsCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *pContextInfo = (CRContextInfo *) data1;
    PCRVBOX_SAVE_STATE_GLOBAL pData = (PCRVBOX_SAVE_STATE_GLOBAL)data2;
    CRMuralInfo *pMural = (CRMuralInfo*)crHashtableSearch(cr_server.muralTable, key);
    PSSMHANDLE pSSM = pData->pSSM;
    CRbitvalue initialCtxUsage[CR_MAX_BITARRAY];
    CRMuralInfo *pInitialCurMural = pContextInfo->currentMural;

    crMemcpy(initialCtxUsage, pMural->ctxUsage, sizeof (initialCtxUsage));

    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    pData->rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    pData->rc = SSMR3PutMem(pSSM, &pContextInfo->CreateInfo.externalID, sizeof(pContextInfo->CreateInfo.externalID));
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    crServerPerformMakeCurrent(pMural, pContextInfo);

    pData->rc = crVBoxServerSaveFBImage(pSSM);

    /* restore the reference data, we synchronize it with the HW state in a later crServerPerformMakeCurrent call */
    crMemcpy(pMural->ctxUsage, initialCtxUsage, sizeof (initialCtxUsage));
    pContextInfo->currentMural = pInitialCurMural;

    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);
}

static void crVBoxServerSaveContextStateCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *pContextInfo = (CRContextInfo *) data1;
    CRContext *pContext = pContextInfo->pContext;
    PCRVBOX_SAVE_STATE_GLOBAL pData = (PCRVBOX_SAVE_STATE_GLOBAL)data2;
    PSSMHANDLE pSSM = pData->pSSM;
    CRMuralInfo *pMural = (CRMuralInfo*)crHashtableSearch(pData->contextMuralTable, key);
    CRMuralInfo *pContextCurrentMural = pContextInfo->currentMural;
    const int32_t i32Dummy = 0;

    AssertCompile(sizeof (i32Dummy) == sizeof (pMural->CreateInfo.externalID));
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    CRASSERT(pContext && pSSM);
    CRASSERT(pMural);
    CRASSERT(pMural->CreateInfo.externalID);

    /* We could have skipped saving the key and use similar callback to load context states back,
     * but there's no guarantee we'd traverse hashtable in same order after loading.
     */
    pData->rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

#ifdef DEBUG_misha
    {
            unsigned long id;
            if (!crHashtableGetDataKey(cr_server.contextTable, pContextInfo, &id))
                crWarning("No client id for server ctx %d", pContextInfo->CreateInfo.externalID);
            else
                CRASSERT(id == key);
    }
#endif

#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
    if (pContextInfo->currentMural
            || crHashtableSearch(cr_server.muralTable, pMural->CreateInfo.externalID) /* <- this is not a dummy mural */
            )
    {
        CRASSERT(pMural->CreateInfo.externalID);
        CRASSERT(!crHashtableSearch(cr_server.dummyMuralTable, pMural->CreateInfo.externalID));
        pData->rc = SSMR3PutMem(pSSM, &pMural->CreateInfo.externalID, sizeof(pMural->CreateInfo.externalID));
    }
    else
    {
        /* this is a dummy mural */
        CRASSERT(!pMural->width);
        CRASSERT(!pMural->height);
        CRASSERT(crHashtableSearch(cr_server.dummyMuralTable, pMural->CreateInfo.externalID));
        pData->rc = SSMR3PutMem(pSSM, &i32Dummy, sizeof(pMural->CreateInfo.externalID));
    }
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    CRASSERT(CR_STATE_SHAREDOBJ_USAGE_IS_SET(pMural, pContext));
    CRASSERT(pContextInfo->currentMural == pMural || !pContextInfo->currentMural);
    CRASSERT(cr_server.curClient);

    crServerPerformMakeCurrent(pMural, pContextInfo);
#endif

    pData->rc = crStateSaveContext(pContext, pSSM);
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    pData->rc = crVBoxServerSaveFBImage(pSSM);
    CRSERVER_ASSERTRC_RETURN_VOID(pData->rc);

    /* restore the initial current mural */
    pContextInfo->currentMural = pContextCurrentMural;
}

static uint32_t g_hackVBoxServerSaveLoadCallsLeft = 0;

static int32_t crVBoxServerSaveStatePerform(PSSMHANDLE pSSM)
{
    int32_t  rc, i;
    uint32_t ui32;
    GLboolean b;
    unsigned long key;
    GLenum err;
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
    CRClient *curClient;
    CRMuralInfo *curMural = NULL;
    CRContextInfo *curCtxInfo = NULL;
#endif
    CRVBOX_SAVE_STATE_GLOBAL Data;

    crMemset(&Data, 0, sizeof (Data));

#if 0
    crVBoxServerCheckConsistency();
#endif

    /* We shouldn't be called if there's no clients at all*/
    CRASSERT(cr_server.numClients > 0);

    /** @todo it's hack atm */
    /* We want to be called only once to save server state but atm we're being called from svcSaveState
     * for every connected client (e.g. guest opengl application)
     */
    if (!cr_server.bIsInSavingState) /* It's first call */
    {
        cr_server.bIsInSavingState = GL_TRUE;

        /* Store number of clients */
        rc = SSMR3PutU32(pSSM, (uint32_t) cr_server.numClients);
        AssertRCReturn(rc, rc);

        /* we get called only once for CrCmd case, so disable the hack */
        g_hackVBoxServerSaveLoadCallsLeft = cr_server.fCrCmdEnabled ? 1 : cr_server.numClients;
    }

    g_hackVBoxServerSaveLoadCallsLeft--;

    /* Do nothing until we're being called last time */
    if (g_hackVBoxServerSaveLoadCallsLeft>0)
    {
        return VINF_SUCCESS;
    }

#ifdef DEBUG_misha
#define CR_DBG_STR_STATE_SAVE_START "VBox.Cr.StateSaveStart"
#define CR_DBG_STR_STATE_SAVE_STOP "VBox.Cr.StateSaveStop"

    if (cr_server.head_spu->dispatch_table.StringMarkerGREMEDY)
        cr_server.head_spu->dispatch_table.StringMarkerGREMEDY(sizeof (CR_DBG_STR_STATE_SAVE_START), CR_DBG_STR_STATE_SAVE_START);
#endif

    /* Save rendering contexts creation info */
    ui32 = crHashtableNumElements(cr_server.contextTable);
    rc = SSMR3PutU32(pSSM, (uint32_t) ui32);
    AssertRCReturn(rc, rc);
    crHashtableWalk(cr_server.contextTable, crVBoxServerSaveCreateInfoFromCtxInfoCB, pSSM);

#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
    curClient = cr_server.curClient;
    /* Save current win and ctx IDs, as we'd rebind contexts when saving textures */
    if (curClient)
    {
        curCtxInfo = cr_server.curClient->currentCtxInfo;
        curMural = cr_server.curClient->currentMural;
    }
    else if (cr_server.numClients)
    {
        cr_server.curClient = cr_server.clients[0];
    }
#endif

    /* first save windows info */
    /* Save windows creation info */
    ui32 = crHashtableNumElements(cr_server.muralTable);
    /* There should be default mural always */
    CRASSERT(ui32>=1);
    rc = SSMR3PutU32(pSSM, (uint32_t) ui32-1);
    AssertRCReturn(rc, rc);
    crHashtableWalk(cr_server.muralTable, crVBoxServerSaveCreateInfoFromMuralInfoCB, pSSM);

    /* Save cr_server.muralTable
     * @todo we don't need it all, just geometry info actually
     */
    rc = SSMR3PutU32(pSSM, (uint32_t) ui32-1);
    AssertRCReturn(rc, rc);
    crHashtableWalk(cr_server.muralTable, crVBoxServerSaveMuralCB, pSSM);

    /* we need to save front & backbuffer data for each mural first create a context -> mural association */
    crVBoxServerBuildSaveStateGlobal(&Data);

    rc = crStateSaveGlobals(pSSM);
    AssertRCReturn(rc, rc);

    Data.pSSM = pSSM;
    /* Save contexts state tracker data */
    /** @todo For now just some blind data dumps,
     * but I've a feeling those should be saved/restored in a very strict sequence to
     * allow diff_api to work correctly.
     * Should be tested more with multiply guest opengl apps working when saving VM snapshot.
     */
    crHashtableWalk(cr_server.contextTable, crVBoxServerSaveContextStateCB, &Data);
    AssertRCReturn(Data.rc, Data.rc);

    ui32 = crHashtableNumElements(Data.additionalMuralContextTable);
    rc = SSMR3PutU32(pSSM, (uint32_t) ui32);
    AssertRCReturn(rc, rc);

    crHashtableWalk(Data.additionalMuralContextTable, crVBoxServerSaveAdditionalMuralsCB, &Data);
    AssertRCReturn(Data.rc, Data.rc);

#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
    cr_server.curClient = curClient;
    /* Restore original win and ctx IDs*/
    if (curClient && curMural && curCtxInfo)
    {
        crServerPerformMakeCurrent(curMural, curCtxInfo);
    }
    else
    {
        cr_server.bForceMakeCurrentOnClientSwitch = GL_TRUE;
    }
#endif

    /* Save clients info */
    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn)
        {
            CRClient *pClient = cr_server.clients[i];

            rc = SSMR3PutU32(pSSM, pClient->conn->u32ClientID);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, pClient->conn->vMajor);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, pClient->conn->vMinor);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutMem(pSSM, pClient, sizeof(*pClient));
            AssertRCReturn(rc, rc);

            if (pClient->currentCtxInfo && pClient->currentCtxInfo->pContext && pClient->currentContextNumber > 0)
            {
                b = crHashtableGetDataKey(cr_server.contextTable, pClient->currentCtxInfo, &key);
                CRASSERT(b);
                rc = SSMR3PutMem(pSSM, &key, sizeof(key));
                AssertRCReturn(rc, rc);
            }

            if (pClient->currentMural && pClient->currentWindow > 0)
            {
                b = crHashtableGetDataKey(cr_server.muralTable, pClient->currentMural, &key);
                CRASSERT(b);
                rc = SSMR3PutMem(pSSM, &key, sizeof(key));
                AssertRCReturn(rc, rc);
            }
        }
    }

    rc = crServerPendSaveState(pSSM);
    AssertRCReturn(rc, rc);

    rc = CrPMgrSaveState(pSSM);
    AssertRCReturn(rc, rc);

#ifdef VBOX_WITH_CR_DISPLAY_LISTS
    if (cr_server.head_spu->dispatch_table.spu_save_state)
    {
        rc = cr_server.head_spu->dispatch_table.spu_save_state((void *)pSSM);
        AssertRCReturn(rc, rc);
    }
    else
        crDebug("Do not save %s SPU state: no interface exported.", cr_server.head_spu->name);
#endif

    /* all context gl error states should have now be synced with chromium erro states,
     * reset the error if any */
    while ((err = cr_server.head_spu->dispatch_table.GetError()) != GL_NO_ERROR)
        crWarning("crServer: glGetError %d after saving snapshot", err);

    cr_server.bIsInSavingState = GL_FALSE;

#ifdef DEBUG_misha
    if (cr_server.head_spu->dispatch_table.StringMarkerGREMEDY)
        cr_server.head_spu->dispatch_table.StringMarkerGREMEDY(sizeof (CR_DBG_STR_STATE_SAVE_STOP), CR_DBG_STR_STATE_SAVE_STOP);
#endif

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerSaveState(PSSMHANDLE pSSM)
{
    if (cr_server.fCrCmdEnabled)
    {
        WARN(("we should not be called with cmd enabled!"));
        return VERR_INTERNAL_ERROR;
    }

    return crVBoxServerSaveStatePerform(pSSM);
}

static DECLCALLBACK(CRContext*) crVBoxServerGetContextCB(void* pvData)
{
    CRContextInfo* pContextInfo = (CRContextInfo*)pvData;
    CRASSERT(pContextInfo);
    CRASSERT(pContextInfo->pContext);
    return pContextInfo->pContext;
}

typedef struct CR_SERVER_LOADSTATE_READER
{
    PSSMHANDLE pSSM;
    uint32_t cbBuffer;
    uint32_t cbData;
    uint32_t offData;
    uint8_t *pu8Buffer;
} CR_SERVER_LOADSTATE_READER;

static void crServerLsrInit(CR_SERVER_LOADSTATE_READER *pReader, PSSMHANDLE pSSM)
{
    memset(pReader, 0, sizeof (*pReader));
    pReader->pSSM = pSSM;
}

static void crServerLsrTerm(CR_SERVER_LOADSTATE_READER *pReader)
{
    if (pReader->pu8Buffer)
        RTMemFree(pReader->pu8Buffer);

    /* sanity */
    memset(pReader, 0, sizeof (*pReader));
}

static int crServerLsrDataGetMem(CR_SERVER_LOADSTATE_READER *pReader, void *pvBuffer, uint32_t cbBuffer)
{
    int rc = VINF_SUCCESS;
    uint32_t cbRemaining = cbBuffer;
    if (pReader->cbData)
    {
        uint8_t cbData = RT_MIN(pReader->cbData, cbBuffer);
        memcpy(pvBuffer, pReader->pu8Buffer + pReader->offData, cbData);
        pReader->cbData -= cbData;
        pReader->offData += cbData;

        cbRemaining -= cbData;
        pvBuffer = ((uint8_t*)pvBuffer) + cbData;
    }

    if (cbRemaining)
    {
        rc = SSMR3GetMem(pReader->pSSM, pvBuffer, cbRemaining);
        AssertRC(rc);
    }

    return rc;
}

static int crServerLsrDataGetU32(CR_SERVER_LOADSTATE_READER *pReader, uint32_t *pu32)
{
    return crServerLsrDataGetMem(pReader, pu32, sizeof (*pu32));
}

static int crServerLsrDataPutMem(CR_SERVER_LOADSTATE_READER *pReader, void *pvBuffer, uint32_t cbBuffer)
{
    if (!pReader->cbData && pReader->cbBuffer >= cbBuffer)
    {
        pReader->offData = 0;
        pReader->cbData = cbBuffer;
        memcpy(pReader->pu8Buffer, pvBuffer, cbBuffer);
    }
    else if (pReader->offData >= cbBuffer)
    {
        pReader->offData -= cbBuffer;
        pReader->cbData += cbBuffer;
        memcpy(pReader->pu8Buffer + pReader->offData, pvBuffer, cbBuffer);
    }
    else
    {
        uint8_t *pu8Buffer = pReader->pu8Buffer;

        pReader->pu8Buffer = (uint8_t*)RTMemAlloc(cbBuffer + pReader->cbData);
        if (!pReader->pu8Buffer)
        {
            crWarning("failed to allocate mem %d", cbBuffer + pReader->cbData);
            return VERR_NO_MEMORY;
        }

        memcpy(pReader->pu8Buffer, pvBuffer, cbBuffer);
        if (pu8Buffer)
        {
            memcpy(pReader->pu8Buffer + cbBuffer, pu8Buffer + pReader->offData, pReader->cbData);
            RTMemFree(pu8Buffer);
        }
        else
        {
            Assert(!pReader->cbData);
        }
        pReader->offData = 0;
        pReader->cbData += cbBuffer;
    }

    return VINF_SUCCESS;
}

/* data to be skipped */

typedef struct CR_SERVER_BUGGY_MURAL_DATA_2
{
    void*ListHead_pNext;
    void*ListHead_pPrev;
    uint32_t cEntries;
} CR_SERVER_BUGGY_MURAL_DATA_2;
typedef struct CR_SERVER_BUGGY_MURAL_DATA_1
{
    /* VBOXVR_COMPOSITOR_ENTRY Ce; */
    void*Ce_Node_pNext;
    void*Ce_Node_pPrev;
    CR_SERVER_BUGGY_MURAL_DATA_2 Vr;
    /* VBOXVR_TEXTURE Tex; */
    uint32_t Tex_width;
    uint32_t Tex_height;
    uint32_t Tex_target;
    uint32_t Tex_hwid;
    /* RTPOINT Pos; */
    uint32_t Pos_x;
    uint32_t Pos_y;
    uint32_t fChanged;
    uint32_t cRects;
    void* paSrcRects;
    void* paDstRects;
} CR_SERVER_BUGGY_MURAL_DATA_1;

typedef struct CR_SERVER_BUGGY_MURAL_DATA_4
{
    uint32_t                   u32Magic;
    int32_t                    cLockers;
    RTNATIVETHREAD             NativeThreadOwner;
    int32_t                    cNestings;
    uint32_t                            fFlags;
    void*                          EventSem;
    R3R0PTRTYPE(PRTLOCKVALRECEXCL)      pValidatorRec;
    RTHCPTR                             Alignment;
} CR_SERVER_BUGGY_MURAL_DATA_4;

typedef struct CR_SERVER_BUGGY_MURAL_DATA_3
{
    void*Compositor_List_pNext;
    void*Compositor_List_pPrev;
    void*Compositor_pfnEntryRemoved;
    float StretchX;
    float StretchY;
    uint32_t cRects;
    uint32_t cRectsBuffer;
    void*paSrcRects;
    void*paDstRects;
    CR_SERVER_BUGGY_MURAL_DATA_4 CritSect;
} CR_SERVER_BUGGY_MURAL_DATA_3;

typedef struct CR_SERVER_BUGGY_MURAL_DATA
{
    uint8_t fRootVrOn;
    CR_SERVER_BUGGY_MURAL_DATA_1 RootVrCEntry;
    CR_SERVER_BUGGY_MURAL_DATA_3 RootVrCompositor;
} CR_SERVER_BUGGY_MURAL_DATA;

AssertCompile(sizeof (CR_SERVER_BUGGY_MURAL_DATA) < sizeof (CRClient));

static int32_t crVBoxServerLoadMurals(CR_SERVER_LOADSTATE_READER *pReader, uint32_t version)
{
    unsigned long key;
    uint32_t ui, uiNumElems;
    bool fBuggyMuralData = false;
    /* Load windows */
    int32_t rc = crServerLsrDataGetU32(pReader, &uiNumElems);
    AssertLogRelRCReturn(rc, rc);
    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRCreateInfo_t createInfo;
        char psz[200];
        GLint winID;
        unsigned long key;

        rc = crServerLsrDataGetMem(pReader, &key, sizeof(key));
        AssertLogRelRCReturn(rc, rc);
        rc = crServerLsrDataGetMem(pReader, &createInfo, sizeof(createInfo));
        AssertLogRelRCReturn(rc, rc);

        CRASSERT(!pReader->cbData);

        if (createInfo.pszDpyName)
        {
            rc = SSMR3GetStrZEx(pReader->pSSM, psz, 200, NULL);
            AssertLogRelRCReturn(rc, rc);
            createInfo.pszDpyName = psz;
        }

        winID = crServerDispatchWindowCreateEx(createInfo.pszDpyName, createInfo.visualBits, key);
        CRASSERT((int64_t)winID == (int64_t)key);
    }

    /* Load cr_server.muralTable */
    rc = SSMR3GetU32(pReader->pSSM, &uiNumElems);
    AssertLogRelRCReturn(rc, rc);
    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRMuralInfo muralInfo;
        CRMuralInfo *pActualMural = NULL;

        rc = crServerLsrDataGetMem(pReader, &key, sizeof(key));
        AssertLogRelRCReturn(rc, rc);
        rc = crServerLsrDataGetMem(pReader, &muralInfo, RT_UOFFSETOF(CRMuralInfo, CreateInfo));
        AssertLogRelRCReturn(rc, rc);

        if (version <= SHCROGL_SSM_VERSION_BEFORE_FRONT_DRAW_TRACKING)
            muralInfo.bFbDraw = GL_TRUE;

        if (!ui && version == SHCROGL_SSM_VERSION_WITH_BUGGY_MURAL_INFO)
        {
            /* Lookahead buffer used to determine whether the data erroneously stored root visible regions data */
            union
            {
                void * apv[1];
                CR_SERVER_BUGGY_MURAL_DATA Data;
                /* need to chak spuWindow, so taking the offset of filed following it*/
                uint8_t au8[RT_UOFFSETOF(CRMuralInfo, screenId)];
                RTRECT aVisRects[sizeof (CR_SERVER_BUGGY_MURAL_DATA) / sizeof (RTRECT)];
            } LaBuf;

            do {
                /* first value is bool (uint8_t) value followed by pointer-size-based alignment.
                 * the mural memory is zero-initialized initially, so we can be sure the padding is zeroed */
                rc = crServerLsrDataGetMem(pReader, &LaBuf, sizeof (LaBuf));
                AssertLogRelRCReturn(rc, rc);
                if (LaBuf.apv[0] != NULL && LaBuf.apv[0] != ((void *)(uintptr_t)1))
                    break;

                /* check that the pointers are either valid or NULL */
                if(LaBuf.Data.RootVrCEntry.Ce_Node_pNext && !RT_VALID_PTR(LaBuf.Data.RootVrCEntry.Ce_Node_pNext))
                    break;
                if(LaBuf.Data.RootVrCEntry.Ce_Node_pPrev && !RT_VALID_PTR(LaBuf.Data.RootVrCEntry.Ce_Node_pPrev))
                    break;
                if(LaBuf.Data.RootVrCEntry.Vr.ListHead_pNext && !RT_VALID_PTR(LaBuf.Data.RootVrCEntry.Vr.ListHead_pNext))
                    break;
                if(LaBuf.Data.RootVrCEntry.Vr.ListHead_pPrev && !RT_VALID_PTR(LaBuf.Data.RootVrCEntry.Vr.ListHead_pPrev))
                    break;

                /* the entry can can be the only one within the (mural) compositor,
                 * so its compositor entry node can either contain NULL pNext and pPrev,
                 * or both of them pointing to compositor's list head */
                if (LaBuf.Data.RootVrCEntry.Ce_Node_pNext != LaBuf.Data.RootVrCEntry.Ce_Node_pPrev)
                    break;

                /* can either both or none be NULL */
                if (!LaBuf.Data.RootVrCEntry.Ce_Node_pNext != !LaBuf.Data.RootVrCEntry.Ce_Node_pPrev)
                    break;

                if (!LaBuf.Data.fRootVrOn)
                {
                    if (LaBuf.Data.RootVrCEntry.Ce_Node_pNext || LaBuf.Data.RootVrCEntry.Ce_Node_pPrev)
                        break;

                    /* either non-initialized (zeroed) or empty list */
                    if (LaBuf.Data.RootVrCEntry.Vr.ListHead_pNext != LaBuf.Data.RootVrCEntry.Vr.ListHead_pPrev)
                        break;

                    if (LaBuf.Data.RootVrCEntry.Vr.cEntries)
                        break;
                }
                else
                {
                    /* the entry should be initialized */
                    if (!LaBuf.Data.RootVrCEntry.Vr.ListHead_pNext)
                        break;
                    if (!LaBuf.Data.RootVrCEntry.Vr.ListHead_pPrev)
                        break;

                    if (LaBuf.Data.RootVrCEntry.Vr.cEntries)
                    {
                        /* entry should be in compositor list*/
                        if (LaBuf.Data.RootVrCEntry.Ce_Node_pPrev == NULL)
                            break;
                        CRASSERT(LaBuf.Data.RootVrCEntry.Ce_Node_pNext);
                    }
                    else
                    {
                        /* entry should NOT be in compositor list*/
                        if (LaBuf.Data.RootVrCEntry.Ce_Node_pPrev != NULL)
                            break;
                        CRASSERT(!LaBuf.Data.RootVrCEntry.Ce_Node_pNext);
                    }
                }

                /* fExpectPtr == true, the valid pointer values should not match possible mural width/height/position */
                fBuggyMuralData = true;
                break;

            } while (0);

            rc = crServerLsrDataPutMem(pReader, &LaBuf, sizeof (LaBuf));
            AssertLogRelRCReturn(rc, rc);
        }

        if (fBuggyMuralData)
        {
            CR_SERVER_BUGGY_MURAL_DATA Tmp;
            rc = crServerLsrDataGetMem(pReader, &Tmp, sizeof (Tmp));
            AssertLogRelRCReturn(rc, rc);
        }

        if (muralInfo.pVisibleRects)
        {
            muralInfo.pVisibleRects = crAlloc(4*sizeof(GLint)*muralInfo.cVisibleRects);
            if (!muralInfo.pVisibleRects)
            {
                return VERR_NO_MEMORY;
            }

            rc = crServerLsrDataGetMem(pReader, muralInfo.pVisibleRects, 4*sizeof(GLint)*muralInfo.cVisibleRects);
            AssertLogRelRCReturn(rc, rc);
        }

        pActualMural = (CRMuralInfo *)crHashtableSearch(cr_server.muralTable, key);
        CRASSERT(pActualMural);

        if (version >= SHCROGL_SSM_VERSION_WITH_WINDOW_CTX_USAGE)
        {
            rc = crServerLsrDataGetMem(pReader, pActualMural->ctxUsage, sizeof (pActualMural->ctxUsage));
            CRASSERT(rc == VINF_SUCCESS);
        }

        /* Restore windows geometry info */
        crServerDispatchWindowSize(key, muralInfo.width, muralInfo.height);
        crServerDispatchWindowPosition(key, muralInfo.gX, muralInfo.gY);
        /* Same workaround as described in stub.c:stubUpdateWindowVisibileRegions for compiz on a freshly booted VM*/
        if (muralInfo.bReceivedRects)
        {
            crServerDispatchWindowVisibleRegion(key, muralInfo.cVisibleRects, muralInfo.pVisibleRects);
        }
        crServerDispatchWindowShow(key, muralInfo.bVisible);

        if (muralInfo.pVisibleRects)
        {
            crFree(muralInfo.pVisibleRects);
        }
    }

    CRASSERT(RT_SUCCESS(rc));
    return VINF_SUCCESS;
}

static int crVBoxServerLoadFBImage(PSSMHANDLE pSSM, uint32_t version,
        CRContextInfo* pContextInfo, CRMuralInfo *pMural)
{
    CRContext *pContext = pContextInfo->pContext;
    int32_t rc = VINF_SUCCESS;
    GLuint i;
    /* can apply the data right away */
    struct
    {
        CRFBData data;
        CRFBDataElement buffer[3]; /* CRFBData::aElements[1] + buffer[3] gives 4: back, front, depth and stencil  */
    } Data;

    Assert(sizeof (Data) >= RT_UOFFSETOF(CRFBData, aElements[4]));

    if (version >= SHCROGL_SSM_VERSION_WITH_SAVED_DEPTH_STENCIL_BUFFER)
    {
        if (!pMural->width || !pMural->height)
            return VINF_SUCCESS;

        rc = crVBoxServerFBImageDataInitEx(&Data.data, pContextInfo, pMural, GL_TRUE, version, 0, 0);
        if (!RT_SUCCESS(rc))
        {
            crWarning("crVBoxServerFBImageDataInit failed rc %d", rc);
            return rc;
        }
    }
    else
    {
        GLint storedWidth, storedHeight;

        if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA)
        {
            CRASSERT(cr_server.currentCtxInfo == pContextInfo);
            CRASSERT(cr_server.currentMural == pMural);
            storedWidth = pMural->width;
            storedHeight = pMural->height;
        }
        else
        {
            storedWidth = pContext->buffer.storedWidth;
            storedHeight = pContext->buffer.storedHeight;
        }

        if (!storedWidth || !storedHeight)
            return VINF_SUCCESS;

        rc = crVBoxServerFBImageDataInitEx(&Data.data, pContextInfo, pMural, GL_TRUE, version, storedWidth, storedHeight);
        if (!RT_SUCCESS(rc))
        {
            crWarning("crVBoxServerFBImageDataInit failed rc %d", rc);
            return rc;
        }
    }

    CRASSERT(Data.data.cElements);

    for (i = 0; i < Data.data.cElements; ++i)
    {
        CRFBDataElement * pEl = &Data.data.aElements[i];
        rc = SSMR3GetMem(pSSM, pEl->pvData, pEl->cbData);
        AssertLogRelRCReturn(rc, rc);
    }

    if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA)
    {
        CRBufferState *pBuf = &pContext->buffer;
        /* can apply the data right away */
        CRASSERT(cr_server.currentCtxInfo == &cr_server.MainContextInfo);
        CRASSERT(cr_server.currentMural);

        cr_server.head_spu->dispatch_table.MakeCurrent( pMural->spuWindow,
                                                                0,
                                                                pContextInfo->SpuContext >= 0
                                                                    ? pContextInfo->SpuContext
                                                                      : cr_server.MainContextInfo.SpuContext);
        crStateApplyFBImage(pContext, &Data.data);
        CRASSERT(!pBuf->pFrontImg);
        CRASSERT(!pBuf->pBackImg);
        crVBoxServerFBImageDataTerm(&Data.data);

        crServerPresentFBO(pMural);

        CRASSERT(cr_server.currentMural);
        cr_server.head_spu->dispatch_table.MakeCurrent( cr_server.currentMural->spuWindow,
                                                                    0,
                                                                    cr_server.currentCtxInfo->SpuContext >= 0
                                                                        ? cr_server.currentCtxInfo->SpuContext
                                                                          : cr_server.MainContextInfo.SpuContext);
    }
    else
    {
        CRBufferState *pBuf = &pContext->buffer;
        CRASSERT(!pBuf->pFrontImg);
        CRASSERT(!pBuf->pBackImg);
        CRASSERT(Data.data.cElements); /* <- older versions always saved front and back, and we filtered out the null-sized buffers above */

        if (Data.data.cElements)
        {
            CRFBData *pLazyData = crAlloc(RT_UOFFSETOF_DYN(CRFBData, aElements[Data.data.cElements]));
            if (!RT_SUCCESS(rc))
            {
                crVBoxServerFBImageDataTerm(&Data.data);
                crWarning("crAlloc failed");
                return VERR_NO_MEMORY;
            }

            crMemcpy(pLazyData, &Data.data, RT_UOFFSETOF_DYN(CRFBData, aElements[Data.data.cElements]));
            pBuf->pFrontImg = pLazyData;
        }
    }

    CRASSERT(RT_SUCCESS(rc));
    return VINF_SUCCESS;
}

static int32_t crVBoxServerLoadStatePerform(PSSMHANDLE pSSM, uint32_t version)
{
    int32_t  rc, i;
    uint32_t ui, uiNumElems;
    unsigned long key;
    GLenum err;
    CR_SERVER_LOADSTATE_READER Reader;

    if (!cr_server.bIsInLoadingState)
    {
        /* AssertRCReturn(...) will leave us in loading state, but it doesn't matter as we'd be failing anyway */
        cr_server.bIsInLoadingState = GL_TRUE;

        /* Read number of clients */
        rc = SSMR3GetU32(pSSM, &g_hackVBoxServerSaveLoadCallsLeft);
        AssertLogRelRCReturn(rc, rc);

        Assert(g_hackVBoxServerSaveLoadCallsLeft);
        /* we get called only once for CrCmd */
        if (cr_server.fCrCmdEnabled)
            g_hackVBoxServerSaveLoadCallsLeft = 1;
    }

    g_hackVBoxServerSaveLoadCallsLeft--;

    /* Do nothing until we're being called last time */
    if (g_hackVBoxServerSaveLoadCallsLeft>0)
    {
        return VINF_SUCCESS;
    }

    if (version < SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
    {
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }

    crServerLsrInit(&Reader, pSSM);

#ifdef DEBUG_misha
#define CR_DBG_STR_STATE_LOAD_START "VBox.Cr.StateLoadStart"
#define CR_DBG_STR_STATE_LOAD_STOP "VBox.Cr.StateLoadStop"

    if (cr_server.head_spu->dispatch_table.StringMarkerGREMEDY)
        cr_server.head_spu->dispatch_table.StringMarkerGREMEDY(sizeof (CR_DBG_STR_STATE_LOAD_START), CR_DBG_STR_STATE_LOAD_START);
#endif

    /* Load and recreate rendering contexts */
    rc = SSMR3GetU32(pSSM, &uiNumElems);
    AssertLogRelRCReturn(rc, rc);
    for (ui = 0; ui < uiNumElems; ++ui)
    {
        CRCreateInfo_t createInfo;
        char psz[200];
        GLint ctxID;
        CRContextInfo* pContextInfo;
        CRContext* pContext;

        rc = SSMR3GetMem(pSSM, &key, sizeof(key));
        AssertLogRelRCReturn(rc, rc);
        rc = SSMR3GetMem(pSSM, &createInfo, sizeof(createInfo));
        AssertLogRelRCReturn(rc, rc);

        if (createInfo.pszDpyName)
        {
            rc = SSMR3GetStrZEx(pSSM, psz, 200, NULL);
            AssertLogRelRCReturn(rc, rc);
            createInfo.pszDpyName = psz;
        }

        ctxID = crServerDispatchCreateContextEx(createInfo.pszDpyName, createInfo.visualBits, 0, key, createInfo.externalID /* <-saved state stores internal id here*/);
        CRASSERT((int64_t)ctxID == (int64_t)key);

        pContextInfo = (CRContextInfo*) crHashtableSearch(cr_server.contextTable, key);
        CRASSERT(pContextInfo);
        CRASSERT(pContextInfo->pContext);
        pContext = pContextInfo->pContext;
        pContext->shared->id=-1;
    }

    if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA)
    {
        CRASSERT(!Reader.pu8Buffer);
        /* we have a mural data here */
        rc = crVBoxServerLoadMurals(&Reader, version);
        AssertLogRelRCReturn(rc, rc);
        CRASSERT(!Reader.pu8Buffer);
    }

    if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA && uiNumElems)
    {
        /* set the current client to allow doing crServerPerformMakeCurrent later */
        CRASSERT(cr_server.numClients);
        cr_server.curClient = cr_server.clients[0];
    }

    rc = crStateLoadGlobals(pSSM, version);
    AssertLogRelRCReturn(rc, rc);

    if (uiNumElems)
    {
        /* ensure we have main context set up as current */
        CRMuralInfo *pMural;
        CRASSERT(cr_server.MainContextInfo.SpuContext > 0);
        CRASSERT(!cr_server.currentCtxInfo);
        CRASSERT(!cr_server.currentMural);
        pMural = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        CRASSERT(pMural);
        crServerPerformMakeCurrent(pMural, &cr_server.MainContextInfo);
    }

    /* Restore context state data */
    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRContextInfo* pContextInfo;
        CRContext *pContext;
        CRMuralInfo *pMural = NULL;
        int32_t winId = 0;

        rc = SSMR3GetMem(pSSM, &key, sizeof(key));
        AssertLogRelRCReturn(rc, rc);

        pContextInfo = (CRContextInfo*) crHashtableSearch(cr_server.contextTable, key);
        CRASSERT(pContextInfo);
        CRASSERT(pContextInfo->pContext);
        pContext = pContextInfo->pContext;

        if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA)
        {
            rc = SSMR3GetMem(pSSM, &winId, sizeof(winId));
            AssertLogRelRCReturn(rc, rc);

            if (winId)
            {
                pMural = (CRMuralInfo*)crHashtableSearch(cr_server.muralTable, winId);
                CRASSERT(pMural);
            }
            else
            {
                /* null winId means a dummy mural, get it */
                pMural = crServerGetDummyMural(pContextInfo->CreateInfo.realVisualBits);
                CRASSERT(pMural);
            }
        }

        rc = crStateLoadContext(pContext, cr_server.contextTable, crVBoxServerGetContextCB, pSSM, version);
        AssertLogRelRCReturn(rc, rc);

        /*Restore front/back buffer images*/
        rc = crVBoxServerLoadFBImage(pSSM, version, pContextInfo, pMural);
        AssertLogRelRCReturn(rc, rc);
    }

    if (version > SHCROGL_SSM_VERSION_WITH_BUGGY_FB_IMAGE_DATA)
    {
        CRContextInfo *pContextInfo;
        CRMuralInfo *pMural;
        GLint ctxId;

        rc = SSMR3GetU32(pSSM, &uiNumElems);
        AssertLogRelRCReturn(rc, rc);
        for (ui=0; ui<uiNumElems; ++ui)
        {
            CRbitvalue initialCtxUsage[CR_MAX_BITARRAY];
            CRMuralInfo *pInitialCurMural;

            rc = SSMR3GetMem(pSSM, &key, sizeof(key));
            AssertLogRelRCReturn(rc, rc);

            rc = SSMR3GetMem(pSSM, &ctxId, sizeof(ctxId));
            AssertLogRelRCReturn(rc, rc);

            pMural = (CRMuralInfo*)crHashtableSearch(cr_server.muralTable, key);
            CRASSERT(pMural);
            if (ctxId)
            {
                pContextInfo = (CRContextInfo *)crHashtableSearch(cr_server.contextTable, ctxId);
                CRASSERT(pContextInfo);
            }
            else
                pContextInfo =  &cr_server.MainContextInfo;

            crMemcpy(initialCtxUsage, pMural->ctxUsage, sizeof (initialCtxUsage));
            pInitialCurMural = pContextInfo->currentMural;

            rc = crVBoxServerLoadFBImage(pSSM, version, pContextInfo, pMural);
            AssertLogRelRCReturn(rc, rc);

            /* restore the reference data, we synchronize it with the HW state in a later crServerPerformMakeCurrent call */
            crMemcpy(pMural->ctxUsage, initialCtxUsage, sizeof (initialCtxUsage));
            pContextInfo->currentMural = pInitialCurMural;
        }

        CRASSERT(!uiNumElems || cr_server.currentCtxInfo == &cr_server.MainContextInfo);

        cr_server.curClient = NULL;
        cr_server.bForceMakeCurrentOnClientSwitch = GL_TRUE;
    }
    else
    {
        CRServerFreeIDsPool_t dummyIdsPool;

        CRASSERT(!Reader.pu8Buffer);

        /* we have a mural data here */
        rc = crVBoxServerLoadMurals(&Reader, version);
        AssertLogRelRCReturn(rc, rc);

        /* not used any more, just read it out and ignore */
        rc = crServerLsrDataGetMem(&Reader, &dummyIdsPool, sizeof(dummyIdsPool));
        CRASSERT(rc == VINF_SUCCESS);
    }

    /* Load clients info */
    for (i = 0; i < cr_server.numClients; i++)
    {
        if (cr_server.clients[i] && cr_server.clients[i]->conn)
        {
            CRClient *pClient = cr_server.clients[i];
            CRClient client;
            unsigned long ctxID=-1, winID=-1;

            rc = crServerLsrDataGetU32(&Reader, &ui);
            AssertLogRelRCReturn(rc, rc);
            /* If this assert fires, then we should search correct client in the list first*/
            CRASSERT(ui == pClient->conn->u32ClientID);

            if (version>=4)
            {
                rc = crServerLsrDataGetU32(&Reader, &pClient->conn->vMajor);
                AssertLogRelRCReturn(rc, rc);

                rc = crServerLsrDataGetU32(&Reader, &pClient->conn->vMinor);
                AssertLogRelRCReturn(rc, rc);
            }

            rc = crServerLsrDataGetMem(&Reader, &client, sizeof(client));
            CRASSERT(rc == VINF_SUCCESS);

            client.conn = pClient->conn;
            /* We can't reassign client number, as we'd get wrong results in TranslateTextureID
             * and fail to bind old textures.
             */
            /*client.number = pClient->number;*/
            *pClient = client;

            pClient->currentContextNumber = -1;
            pClient->currentCtxInfo = &cr_server.MainContextInfo;
            pClient->currentMural = NULL;
            pClient->currentWindow = -1;

            cr_server.curClient = pClient;

            if (client.currentCtxInfo && client.currentContextNumber > 0)
            {
                rc = crServerLsrDataGetMem(&Reader, &ctxID, sizeof(ctxID));
                AssertLogRelRCReturn(rc, rc);
                client.currentCtxInfo = (CRContextInfo*) crHashtableSearch(cr_server.contextTable, ctxID);
                CRASSERT(client.currentCtxInfo);
                CRASSERT(client.currentCtxInfo->pContext);
                //pClient->currentCtx = client.currentCtx;
                //pClient->currentContextNumber = ctxID;
            }

            if (client.currentMural && client.currentWindow > 0)
            {
                rc = crServerLsrDataGetMem(&Reader, &winID, sizeof(winID));
                AssertLogRelRCReturn(rc, rc);
                client.currentMural = (CRMuralInfo*) crHashtableSearch(cr_server.muralTable, winID);
                CRASSERT(client.currentMural);
                //pClient->currentMural = client.currentMural;
                //pClient->currentWindow = winID;
            }

            CRASSERT(!Reader.cbData);

            /* Restore client active context and window */
            crServerDispatchMakeCurrent(winID, 0, ctxID);
        }
    }

    cr_server.curClient = NULL;

    rc = crServerPendLoadState(pSSM, version);
    AssertLogRelRCReturn(rc, rc);

    if (version >= SHCROGL_SSM_VERSION_WITH_SCREEN_INFO)
    {
        rc = CrPMgrLoadState(pSSM, version);
        AssertLogRelRCReturn(rc, rc);
    }

#ifdef VBOX_WITH_CR_DISPLAY_LISTS
    if (version >= SHCROGL_SSM_VERSION_WITH_DISPLAY_LISTS)
    {
        if (cr_server.head_spu->dispatch_table.spu_load_state)
        {
            rc = cr_server.head_spu->dispatch_table.spu_load_state((void *)pSSM);
            AssertLogRelRCReturn(rc, rc);
        }
        else
            crDebug("Do not load %s SPU state: no interface exported.", cr_server.head_spu->name);
    }
#endif

    while ((err = cr_server.head_spu->dispatch_table.GetError()) != GL_NO_ERROR)
        crWarning("crServer: glGetError %d after loading snapshot", err);

    cr_server.bIsInLoadingState = GL_FALSE;

#ifdef DEBUG_misha
    if (cr_server.head_spu->dispatch_table.StringMarkerGREMEDY)
        cr_server.head_spu->dispatch_table.StringMarkerGREMEDY(sizeof (CR_DBG_STR_STATE_LOAD_STOP), CR_DBG_STR_STATE_LOAD_STOP);
#endif

    CRASSERT(!Reader.cbData);
    crServerLsrTerm(&Reader);

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerLoadState(PSSMHANDLE pSSM, uint32_t version)
{
    if (cr_server.fCrCmdEnabled)
    {
        WARN(("CrCmd enabled"));
        return VERR_INTERNAL_ERROR;
    }

    return crVBoxServerLoadStatePerform(pSSM, version);
}

#define SCREEN(i) (cr_server.screen[i])
#define MAPPED(screen) ((screen).winID != 0)

extern DECLEXPORT(void) crServerVBoxSetNotifyEventCB(PFNCRSERVERNOTIFYEVENT pfnCb)
{
    cr_server.pfnNotifyEventCB = pfnCb;
}

void crVBoxServerNotifyEvent(int32_t idScreen, uint32_t uEvent, void* pvData, uint32_t cbData)
{
    /* this is something unexpected, but just in case */
    if (idScreen >= cr_server.screenCount)
    {
        crWarning("invalid screen id %d", idScreen);
        return;
    }

    cr_server.pfnNotifyEventCB(idScreen, uEvent, pvData, cbData);
}

void crServerWindowReparent(CRMuralInfo *pMural)
{
    pMural->fHasParentWindow = !!cr_server.screen[pMural->screenId].winID;

    renderspuReparentWindow(pMural->spuWindow);
}

DECLEXPORT(void) crServerSetUnscaledHiDPI(bool fEnable)
{
    renderspuSetUnscaledHiDPI(fEnable);
}

static void crVBoxServerReparentMuralCB(unsigned long key, void *data1, void *data2)
{
    CRMuralInfo *pMI = (CRMuralInfo*) data1;
    int *sIndex = (int*) data2;

    if (pMI->screenId == *sIndex)
    {
        crServerWindowReparent(pMI);
    }
}

DECLEXPORT(int32_t) crVBoxServerSetScreenCount(int sCount)
{
    int i;

    if (sCount > CR_MAX_GUEST_MONITORS)
        return VERR_INVALID_PARAMETER;

    /*Shouldn't happen yet, but to be safe in future*/
    for (i = 0; i < cr_server.screenCount; /*++i - unreachable code*/)
    {
        if (MAPPED(SCREEN(i)))
            WARN(("Screen count is changing, but screen[%i] is still mapped", i));
        return VERR_NOT_IMPLEMENTED;
    }

    cr_server.screenCount = sCount;

    for (i=0; i<sCount; ++i)
    {
        SCREEN(i).winID = 0;
    }

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerUnmapScreen(int sIndex)
{
    crDebug("crVBoxServerUnmapScreen(%i)", sIndex);

    if (sIndex<0 || sIndex>=cr_server.screenCount)
        return VERR_INVALID_PARAMETER;

    if (MAPPED(SCREEN(sIndex)))
    {
        SCREEN(sIndex).winID = 0;
        renderspuSetWindowId(0);

        crHashtableWalk(cr_server.muralTable, crVBoxServerReparentMuralCB, &sIndex);

        crHashtableWalk(cr_server.dummyMuralTable, crVBoxServerReparentMuralCB, &sIndex);

        CrPMgrScreenChanged((uint32_t)sIndex);
    }

    renderspuSetWindowId(SCREEN(0).winID);

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerMapScreen(int sIndex, int32_t x, int32_t y, uint32_t w, uint32_t h, uint64_t winID)
{
    crDebug("crVBoxServerMapScreen(%i) [%i,%i:%u,%u %x]", sIndex, x, y, w, h, winID);

    if (sIndex<0 || sIndex>=cr_server.screenCount)
        return VERR_INVALID_PARAMETER;

    if (MAPPED(SCREEN(sIndex)) && SCREEN(sIndex).winID!=winID)
    {
        crDebug("Mapped screen[%i] is being remapped.", sIndex);
        crVBoxServerUnmapScreen(sIndex);
    }

    SCREEN(sIndex).winID = winID;
    SCREEN(sIndex).x = x;
    SCREEN(sIndex).y = y;
    SCREEN(sIndex).w = w;
    SCREEN(sIndex).h = h;

    renderspuSetWindowId(SCREEN(sIndex).winID);
    crHashtableWalk(cr_server.muralTable, crVBoxServerReparentMuralCB, &sIndex);

    crHashtableWalk(cr_server.dummyMuralTable, crVBoxServerReparentMuralCB, &sIndex);
    renderspuSetWindowId(SCREEN(0).winID);

#ifndef WINDOWS
    /*Restore FB content for clients, which have current window on a screen being remapped*/
    {
        GLint i;

        for (i = 0; i < cr_server.numClients; i++)
        {
            cr_server.curClient = cr_server.clients[i];
            if (cr_server.curClient->currentCtxInfo
                && cr_server.curClient->currentCtxInfo->pContext
                && (cr_server.curClient->currentCtxInfo->pContext->buffer.pFrontImg)
                && cr_server.curClient->currentMural
                && cr_server.curClient->currentMural->screenId == sIndex
                && cr_server.curClient->currentCtxInfo->pContext->buffer.storedHeight == h
                && cr_server.curClient->currentCtxInfo->pContext->buffer.storedWidth == w)
            {
                int clientWindow = cr_server.curClient->currentWindow;
                int clientContext = cr_server.curClient->currentContextNumber;
                CRFBData *pLazyData = (CRFBData *)cr_server.curClient->currentCtxInfo->pContext->buffer.pFrontImg;

                if (clientWindow && clientWindow != cr_server.currentWindow)
                {
                    crServerDispatchMakeCurrent(clientWindow, 0, clientContext);
                }

                crStateApplyFBImage(cr_server.curClient->currentCtxInfo->pContext, pLazyData);
                crStateFreeFBImageLegacy(cr_server.curClient->currentCtxInfo->pContext);
            }
        }
        cr_server.curClient = NULL;
    }
#endif

    CrPMgrScreenChanged((uint32_t)sIndex);

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerSetRootVisibleRegion(GLint cRects, const RTRECT *pRects)
{
    int32_t rc = VINF_SUCCESS;
    GLboolean fOldRootVrOn = cr_server.fRootVrOn;

    /* non-zero rects pointer indicate rects are present and switched on
     * i.e. cRects==0 and pRects!=NULL means root visible regioning is ON and there are no visible regions,
     * while pRects==NULL means root visible regioning is OFF, i.e. everything is visible */
    if (pRects)
    {
        crMemset(&cr_server.RootVrCurPoint, 0, sizeof (cr_server.RootVrCurPoint));
        rc = VBoxVrListRectsSet(&cr_server.RootVr, cRects, pRects, NULL);
        if (!RT_SUCCESS(rc))
        {
            crWarning("VBoxVrListRectsSet failed! rc %d", rc);
            return rc;
        }

        cr_server.fRootVrOn = GL_TRUE;
    }
    else
    {
        if (!cr_server.fRootVrOn)
            return VINF_SUCCESS;

        VBoxVrListClear(&cr_server.RootVr);

        cr_server.fRootVrOn = GL_FALSE;
    }

    if (!fOldRootVrOn != !cr_server.fRootVrOn)
    {
        rc = CrPMgrModeRootVr(cr_server.fRootVrOn);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrPMgrModeRootVr failed rc %d", rc);
            return rc;
        }
    }
    else if (cr_server.fRootVrOn)
    {
        rc = CrPMgrRootVrUpdate();
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrPMgrRootVrUpdate failed rc %d", rc);
            return rc;
        }
    }

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerSetOffscreenRendering(GLboolean value)
{
    return CrPMgrModeVrdp(value);
}

DECLEXPORT(int32_t) crVBoxServerOutputRedirectSet(const CROutputRedirect *pCallbacks)
{
    /* No need for a synchronization as this is single threaded. */
    if (pCallbacks)
    {
        cr_server.outputRedirect = *pCallbacks;
    }
    else
    {
        memset (&cr_server.outputRedirect, 0, sizeof (cr_server.outputRedirect));
    }

    return VINF_SUCCESS;
}

DECLEXPORT(int32_t) crVBoxServerSetScreenViewport(int sIndex, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    CRScreenViewportInfo *pViewport;
    RTRECT NewRect;
    int rc;

    crDebug("crVBoxServerSetScreenViewport(%i)", sIndex);

    if (sIndex<0 || sIndex>=cr_server.screenCount)
    {
        crWarning("crVBoxServerSetScreenViewport: invalid screen id %d", sIndex);
        return VERR_INVALID_PARAMETER;
    }

    NewRect.xLeft = x;
    NewRect.yTop = y;
    NewRect.xRight = x + w;
    NewRect.yBottom = y + h;

    pViewport = &cr_server.screenVieport[sIndex];
    /*always do viewport updates no matter whether the rectangle actually changes,
     * this is needed to ensure window is adjusted properly on OSX */
    pViewport->Rect = NewRect;
    rc = CrPMgrViewportUpdate((uint32_t)sIndex);
    if (!RT_SUCCESS(rc))
    {
        crWarning("CrPMgrViewportUpdate failed %d", rc);
        return rc;
    }

    return VINF_SUCCESS;
}

static void crVBoxServerDeleteMuralCb(unsigned long key, void *data1, void *data2)
{
    CRHashTable *h = (CRHashTable*)data2;
    CRMuralInfo *m = (CRMuralInfo *) data1;
    if (m->spuWindow == CR_RENDER_DEFAULT_WINDOW_ID)
        return;

    crHashtableDelete(h, key, NULL);
    crServerMuralTerm(m);
    crFree(m);
}

static void crVBoxServerDefaultContextClear()
{
    HCR_FRAMEBUFFER hFb;
    int rc = CrPMgrDisable();
    if (RT_FAILURE(rc))
    {
        WARN(("CrPMgrDisable failed %d", rc));
        return;
    }

    for (hFb = CrPMgrFbGetFirstEnabled(); hFb; hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        int rc = CrFbUpdateBegin(hFb);
        if (RT_SUCCESS(rc))
        {
            CrFbRegionsClear(hFb);
            CrFbUpdateEnd(hFb);
        }
        else
            WARN(("CrFbUpdateBegin failed %d", rc));
    }

    cr_server.head_spu->dispatch_table.MakeCurrent(0, 0, 0);
    crStateCleanupCurrent();

    /* note: we need to clean all contexts, since otherwise renderspu leanup won't work,
     * i.e. renderspu would need to clean up its own internal windows, it won't be able to do that if
     * some those windows is associated with any context.  */
    if (cr_server.MainContextInfo.SpuContext)
    {
        cr_server.head_spu->dispatch_table.DestroyContext(cr_server.MainContextInfo.SpuContext);
        crStateDestroyContext(cr_server.MainContextInfo.pContext);
        if (cr_server.MainContextInfo.CreateInfo.pszDpyName)
            crFree(cr_server.MainContextInfo.CreateInfo.pszDpyName);

        memset(&cr_server.MainContextInfo, 0, sizeof (cr_server.MainContextInfo));
    }

    cr_server.firstCallCreateContext = GL_TRUE;
    cr_server.firstCallMakeCurrent = GL_TRUE;
    cr_server.bForceMakeCurrentOnClientSwitch = GL_FALSE;

    CRASSERT(!cr_server.curClient);

    cr_server.currentCtxInfo = NULL;
    cr_server.currentWindow = 0;
    cr_server.currentNativeWindow = 0;
    cr_server.currentMural = NULL;

    crStateDestroy();
//    crStateCleanupCurrent();

    if (CrBltIsInitialized(&cr_server.Blitter))
    {
        CrBltTerm(&cr_server.Blitter);
        Assert(!CrBltIsInitialized(&cr_server.Blitter));
    }

    crHashtableWalk(cr_server.dummyMuralTable, crVBoxServerDeleteMuralCb, cr_server.dummyMuralTable);

    cr_server.head_spu->dispatch_table.ChromiumParameteriCR(GL_HH_RENDERTHREAD_INFORM, 0);
}

static void crVBoxServerDefaultContextSet()
{
    cr_server.head_spu->dispatch_table.ChromiumParameteriCR(GL_HH_RENDERTHREAD_INFORM, 1);

    CRASSERT(!cr_server.MainContextInfo.SpuContext);

//    crStateSetCurrent(NULL);
    crStateInit();
    crStateDiffAPI( &(cr_server.head_spu->dispatch_table) );

    CrPMgrEnable();
}

#ifdef VBOX_WITH_CRHGSMI

/** @todo RT_UNTRUSTED_VOLATILE_GUEST   */
static int32_t crVBoxServerCmdVbvaCrCmdProcess(VBOXCMDVBVA_CRCMD_CMD const RT_UNTRUSTED_VOLATILE_GUEST *pCmdTodo, uint32_t cbCmd)
{
    VBOXCMDVBVA_CRCMD_CMD const *pCmd = (VBOXCMDVBVA_CRCMD_CMD const *)pCmdTodo;
    int32_t rc;
    uint32_t cBuffers = pCmd->cBuffers;
    uint32_t cParams;
    uint32_t cbHdr;
    CRVBOXHGSMIHDR *pHdr;
    uint32_t u32Function;
    uint32_t u32ClientID;
    CRClient *pClient;

    if (!g_pvVRamBase)
    {
        WARN(("g_pvVRamBase is not initialized"));
        return VERR_INVALID_STATE;
    }

    if (!cBuffers)
    {
        WARN(("zero buffers passed in!"));
        return VERR_INVALID_PARAMETER;
    }

    cParams = cBuffers-1;

    if (cbCmd < RT_UOFFSETOF_DYN(VBOXCMDVBVA_CRCMD_CMD, aBuffers[cBuffers]))
    {
        WARN(("invalid buffer size"));
        return VERR_INVALID_PARAMETER;
    }

    cbHdr = pCmd->aBuffers[0].cbBuffer;
    pHdr = VBOXCRHGSMI_PTR_SAFE(pCmd->aBuffers[0].offBuffer, cbHdr, CRVBOXHGSMIHDR);
    if (!pHdr)
    {
        WARN(("invalid header buffer!"));
        return VERR_INVALID_PARAMETER;
    }

    if (cbHdr < sizeof (*pHdr))
    {
        WARN(("invalid header buffer size!"));
        return VERR_INVALID_PARAMETER;
    }

    u32Function = pHdr->u32Function;
    u32ClientID = pHdr->u32ClientID;

    switch (u32Function)
    {
        case SHCRGL_GUEST_FN_WRITE:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIWRITE* pFnCmd = (CRVBOXHGSMIWRITE*)pHdr;
                const VBOXCMDVBVA_CRCMD_BUFFER *pBuf = &pCmd->aBuffers[1];
                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    WARN(("invalid write cmd buffer size!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    WARN(("invalid buffer data received from guest!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    WARN(("crVBoxServerClientGet failed %d", rc));
                    break;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SET(&pClient->conn->CmdData, pCmd, pHdr, false);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }

            WARN(("invalid number of args"));
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_INJECT:
        {
            WARN(("svcCall: SHCRGL_GUEST_FN_INJECT\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIINJECT *pFnCmd = (CRVBOXHGSMIINJECT*)pHdr;
                /* Fetch parameters. */
                uint32_t u32InjectClientID = pFnCmd->u32ClientID;
                const VBOXCMDVBVA_CRCMD_BUFFER *pBuf = &pCmd->aBuffers[1];
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    WARN(("invalid inject cmd buffer size!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    WARN(("invalid buffer data received from guest!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32InjectClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    WARN(("crVBoxServerClientGet failed %d", rc));
                    break;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SET(&pClient->conn->CmdData, pCmd, pHdr, false);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }

            WARN(("invalid number of args"));
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_READ\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIREAD *pFnCmd = (CRVBOXHGSMIREAD*)pHdr;
                const VBOXCMDVBVA_CRCMD_BUFFER *pBuf = &pCmd->aBuffers[1];
                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    WARN(("invalid read cmd buffer size!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                if (!pBuffer)
                {
                    WARN(("invalid buffer data received from guest!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    WARN(("crVBoxServerClientGet failed %d", rc));
                    break;
                }

                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                rc = crVBoxServerInternalClientRead(pClient, pBuffer, &cbBuffer);

                /* Return the required buffer size always */
                pFnCmd->cbBuffer = cbBuffer;

                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                /* the read command is never pended, complete it right away */
                if (RT_FAILURE(rc))
                {
                    WARN(("crVBoxServerInternalClientRead failed %d", rc));
                    break;
                }

                break;
            }

            crWarning("invalid number of args");
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_WRITE_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE_READ\n"));

            /** @todo Verify  */
            if (cParams == 2)
            {
                CRVBOXHGSMIWRITEREAD *pFnCmd = (CRVBOXHGSMIWRITEREAD*)pHdr;
                const VBOXCMDVBVA_CRCMD_BUFFER *pBuf = &pCmd->aBuffers[1];
                const VBOXCMDVBVA_CRCMD_BUFFER *pWbBuf = &pCmd->aBuffers[2];

                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                uint32_t cbWriteback = pWbBuf->cbBuffer;
                char *pWriteback  = VBOXCRHGSMI_PTR_SAFE(pWbBuf->offBuffer, cbWriteback, char);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    WARN(("invalid write_read cmd buffer size!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    WARN(("invalid write buffer data received from guest!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbWriteback);
                if (!pWriteback)
                {
                    WARN(("invalid writeback buffer data received from guest!"));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    WARN(("crVBoxServerClientGet failed %d", rc));
                    break;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SETWB(&pClient->conn->CmdData, pCmd, pHdr, pWriteback, cbWriteback, &pFnCmd->cbWriteback, false);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }

            crWarning("invalid number of args");
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_SET_VERSION:
        {
            WARN(("SHCRGL_GUEST_FN_SET_VERSION: invalid function"));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

        case SHCRGL_GUEST_FN_SET_PID:
        {
            WARN(("SHCRGL_GUEST_FN_SET_PID: invalid function"));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

        default:
        {
            WARN(("invalid function, %d", u32Function));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

    }

    pHdr->result = rc;

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) crVBoxCrCmdEnable(HVBOXCRCMDSVR hSvr, VBOXCRCMD_SVRENABLE_INFO *pInfo)
{
    Assert(!cr_server.fCrCmdEnabled);
    Assert(!cr_server.numClients);

    cr_server.CrCmdClientInfo = *pInfo;

    crVBoxServerDefaultContextSet();

    cr_server.fCrCmdEnabled = GL_TRUE;

    crInfo("crCmd ENABLED");

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) crVBoxCrCmdDisable(HVBOXCRCMDSVR hSvr)
{
    Assert(cr_server.fCrCmdEnabled);

    crVBoxServerRemoveAllClients();

    CrHTableEmpty(&cr_server.clientTable);

    crVBoxServerDefaultContextClear();

    memset(&cr_server.CrCmdClientInfo, 0, sizeof (cr_server.CrCmdClientInfo));

    cr_server.fCrCmdEnabled = GL_FALSE;

    crInfo("crCmd DISABLED");

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) crVBoxCrCmdHostCtl(HVBOXCRCMDSVR hSvr, uint8_t* pCmd, uint32_t cbCmd)
{
    return crVBoxServerHostCtl((VBOXCRCMDCTL*)pCmd, cbCmd);
}

static int crVBoxCrDisconnect(uint32_t u32Client)
{
    CRClient *pClient = (CRClient*)CrHTableRemove(&cr_server.clientTable, u32Client);
    if (!pClient)
    {
        WARN(("invalid client id"));
        return VERR_INVALID_PARAMETER;
    }

    crVBoxServerRemoveClientObj(pClient);

    return VINF_SUCCESS;
}

static int crVBoxCrConnectEx(VBOXCMDVBVA_3DCTL_CONNECT RT_UNTRUSTED_VOLATILE_GUEST *pConnect, uint32_t u32ClientId)
{
    CRClient *pClient;
    int rc;
    uint32_t const uMajorVersion = pConnect->u32MajorVersion;
    uint32_t const uMinorVersion = pConnect->u32MinorVersion;
    uint64_t const uPid          = pConnect->u64Pid;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    if (u32ClientId == CRHTABLE_HANDLE_INVALID)
    {
        /* allocate client id */
        u32ClientId =  CrHTablePut(&cr_server.clientTable, (void *)(uintptr_t)1);
        if (u32ClientId == CRHTABLE_HANDLE_INVALID)
        {
            WARN(("CrHTablePut failed"));
            return VERR_NO_MEMORY;
        }
    }

    rc = crVBoxServerAddClientObj(u32ClientId, &pClient);
    if (RT_SUCCESS(rc))
    {
        rc = crVBoxServerClientObjSetVersion(pClient, uMajorVersion, uMinorVersion);
        if (RT_SUCCESS(rc))
        {
            rc = crVBoxServerClientObjSetPID(pClient, uPid);
            if (RT_SUCCESS(rc))
            {
                rc = CrHTablePutToSlot(&cr_server.clientTable, u32ClientId, pClient);
                if (RT_SUCCESS(rc))
                {
                    pConnect->Hdr.u32CmdClientId = u32ClientId;
                    return VINF_SUCCESS;
                }
                WARN(("CrHTablePutToSlot failed %d", rc));
            }
            else
                WARN(("crVBoxServerClientObjSetPID failed %d", rc));
        }
        else
            WARN(("crVBoxServerClientObjSetVersion failed %d", rc));

        crVBoxServerRemoveClientObj(pClient);
    }
    else
        WARN(("crVBoxServerAddClientObj failed %d", rc));

    CrHTableRemove(&cr_server.clientTable, u32ClientId);

    return rc;
}

static int crVBoxCrConnect(VBOXCMDVBVA_3DCTL_CONNECT RT_UNTRUSTED_VOLATILE_GUEST *pConnect)
{
    return crVBoxCrConnectEx(pConnect, CRHTABLE_HANDLE_INVALID);
}

/**
 * @interface_method_impl{VBOXCRCMD_SVRINFO,pfnGuestCtl}
 */
static DECLCALLBACK(int) crVBoxCrCmdGuestCtl(HVBOXCRCMDSVR hSvr, uint8_t RT_UNTRUSTED_VOLATILE_GUEST *pbCmd, uint32_t cbCmd)
{
    /*
     * Toplevel input validation.
     */
    ASSERT_GUEST_LOGREL_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_3DCTL), VERR_INVALID_PARAMETER);
    {
        VBOXCMDVBVA_3DCTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl = (VBOXCMDVBVA_3DCTL RT_UNTRUSTED_VOLATILE_GUEST*)pbCmd;
        const uint32_t uType = pCtl->u32Type;
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

        ASSERT_GUEST_LOGREL_RETURN(   uType == VBOXCMDVBVA3DCTL_TYPE_CMD
                                   || uType == VBOXCMDVBVA3DCTL_TYPE_CONNECT
                                   || uType == VBOXCMDVBVA3DCTL_TYPE_DISCONNECT
                                   , VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();

        /*
         * Call worker abd process the request.
         */
        switch (uType)
        {
            case VBOXCMDVBVA3DCTL_TYPE_CMD:
                ASSERT_GUEST_LOGREL_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_3DCTL_CMD), VERR_INVALID_PARAMETER);
                {
                    VBOXCMDVBVA_3DCTL_CMD RT_UNTRUSTED_VOLATILE_GUEST *p3DCmd
                        = (VBOXCMDVBVA_3DCTL_CMD RT_UNTRUSTED_VOLATILE_GUEST *)pbCmd;
                    return crVBoxCrCmdCmd(NULL, &p3DCmd->Cmd, cbCmd - RT_UOFFSETOF(VBOXCMDVBVA_3DCTL_CMD, Cmd));
                }

            case VBOXCMDVBVA3DCTL_TYPE_CONNECT:
                ASSERT_GUEST_LOGREL_RETURN(cbCmd == sizeof(VBOXCMDVBVA_3DCTL_CONNECT), VERR_INVALID_PARAMETER);
                return crVBoxCrConnect((VBOXCMDVBVA_3DCTL_CONNECT RT_UNTRUSTED_VOLATILE_GUEST *)pCtl);

            case VBOXCMDVBVA3DCTL_TYPE_DISCONNECT:
                ASSERT_GUEST_LOGREL_RETURN(cbCmd == sizeof(VBOXCMDVBVA_3DCTL), VERR_INVALID_PARAMETER);
                {
                    uint32_t idClient = pCtl->u32CmdClientId;
                    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
                    return crVBoxCrDisconnect(idClient);
                }

            default:
                AssertFailedReturn(VERR_IPE_NOT_REACHED_DEFAULT_CASE);
        }
    }
}

static DECLCALLBACK(int) crVBoxCrCmdResize(HVBOXCRCMDSVR hSvr, const struct VBVAINFOSCREEN *pScreen, const uint32_t *pTargetMap)
{
    CRASSERT(cr_server.fCrCmdEnabled);
    return CrPMgrResize(pScreen, NULL, pTargetMap);
}

static const char* gszVBoxOGLSSMMagic = "***OpenGL state data***";

static int crVBoxCrCmdSaveClients(PSSMHANDLE pSSM)
{
    int i;
    int rc = SSMR3PutU32(pSSM, cr_server.numClients);
    AssertRCReturn(rc, rc);

    for (i = 0; i < cr_server.numClients; i++)
    {
        CRClient * pClient = cr_server.clients[i];
        Assert(pClient);

        rc = SSMR3PutU32(pSSM, pClient->conn->u32ClientID);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU32(pSSM, pClient->conn->vMajor);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU32(pSSM, pClient->conn->vMinor);
        AssertRCReturn(rc, rc);
        rc = SSMR3PutU64(pSSM, pClient->pid);
        AssertRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}

static int crVBoxCrCmdLoadClients(PSSMHANDLE pSSM, uint32_t u32Version)
{
    uint32_t i;
    uint32_t u32;
    VBOXCMDVBVA_3DCTL_CONNECT Connect;
    int rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);

    for (i = 0; i < u32; i++)
    {
        uint32_t u32ClientID;
        Connect.Hdr.u32Type = VBOXCMDVBVA3DCTL_TYPE_CONNECT;
        Connect.Hdr.u32CmdClientId = 0;

        rc = SSMR3GetU32(pSSM, &u32ClientID);
        AssertLogRelRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &Connect.u32MajorVersion);
        AssertLogRelRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &Connect.u32MinorVersion);
        AssertLogRelRCReturn(rc, rc);
        rc = SSMR3GetU64(pSSM, &Connect.u64Pid);
        AssertLogRelRCReturn(rc, rc);

        rc = crVBoxCrConnectEx(&Connect, u32ClientID);
        AssertLogRelRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) crVBoxCrCmdSaveState(HVBOXCRCMDSVR hSvr, PSSMHANDLE pSSM)
{
    int rc = VINF_SUCCESS;

    Assert(cr_server.fCrCmdEnabled);

    /* Start*/
    rc = SSMR3PutStrZ(pSSM, gszVBoxOGLSSMMagic);
    AssertRCReturn(rc, rc);

    if (!cr_server.numClients)
    {
        rc = SSMR3PutU32(pSSM, 0);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutStrZ(pSSM, gszVBoxOGLSSMMagic);
        AssertRCReturn(rc, rc);

        return VINF_SUCCESS;
    }

    rc = SSMR3PutU32(pSSM, 1);
    AssertRCReturn(rc, rc);

    /* Version */
    rc = SSMR3PutU32(pSSM, (uint32_t) SHCROGL_SSM_VERSION);
    AssertRCReturn(rc, rc);

    rc = crVBoxCrCmdSaveClients(pSSM);
    AssertRCReturn(rc, rc);

    /* The state itself */
    rc = crVBoxServerSaveStatePerform(pSSM);
    AssertRCReturn(rc, rc);

    /* Save svc buffers info */
    {
        rc = SSMR3PutU32(pSSM, 0);
        AssertRCReturn(rc, rc);

        rc = SSMR3PutU32(pSSM, 0);
        AssertRCReturn(rc, rc);
    }

    /* End */
    rc = SSMR3PutStrZ(pSSM, gszVBoxOGLSSMMagic);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) crVBoxCrCmdLoadState(HVBOXCRCMDSVR hSvr, PSSMHANDLE pSSM, uint32_t u32Version)
{
    int rc = VINF_SUCCESS;

    char szBuf[2000];
    uint32_t ui32;

    Assert(cr_server.fCrCmdEnabled);

    /* Start of data */
    rc = SSMR3GetStrZEx(pSSM, szBuf, sizeof(szBuf), NULL);
    AssertLogRelRCReturn(rc, rc);
    AssertLogRelMsgReturn(!strcmp(gszVBoxOGLSSMMagic, szBuf), ("Unexpected data1: '%s'\n", szBuf), VERR_SSM_UNEXPECTED_DATA);

    /* num clients */
    rc = SSMR3GetU32(pSSM, &ui32);
    AssertLogRelRCReturn(rc, rc);

    if (!ui32)
    {
        /* no clients, dummy stub */
        rc = SSMR3GetStrZEx(pSSM, szBuf, sizeof(szBuf), NULL);
        AssertLogRelRCReturn(rc, rc);
        AssertLogRelMsgReturn(!strcmp(gszVBoxOGLSSMMagic, szBuf), ("Unexpected data2: '%s'\n", szBuf), VERR_SSM_UNEXPECTED_DATA);

        return VINF_SUCCESS;
    }
    AssertLogRelMsgReturn(ui32 == 1, ("Invalid client count: %#x\n", ui32), VERR_SSM_UNEXPECTED_DATA);

    /* Version */
    rc = SSMR3GetU32(pSSM, &ui32);
    AssertLogRelRCReturn(rc, rc);
    AssertLogRelMsgReturn(ui32 >= SHCROGL_SSM_VERSION_CRCMD, ("Unexpected version: %#x\n", ui32), VERR_SSM_UNEXPECTED_DATA);

    rc = crVBoxCrCmdLoadClients(pSSM, u32Version);
    AssertLogRelRCReturn(rc, rc);

    /* The state itself */
    rc = crVBoxServerLoadStatePerform(pSSM, ui32);
    AssertLogRelRCReturn(rc, rc);

    /* Save svc buffers info */
    {
        rc = SSMR3GetU32(pSSM, &ui32);
        AssertLogRelRCReturn(rc, rc);
        AssertLogRelMsgReturn(ui32 == 0, ("Unexpected data3: %#x\n", ui32), VERR_SSM_UNEXPECTED_DATA);

        rc = SSMR3GetU32(pSSM, &ui32);
        AssertLogRelRCReturn(rc, rc);
        AssertLogRelMsgReturn(ui32 == 0, ("Unexpected data4: %#x\n", ui32), VERR_SSM_UNEXPECTED_DATA);
    }

    /* End */
    rc = SSMR3GetStrZEx(pSSM, szBuf, sizeof(szBuf), NULL);
    AssertLogRelRCReturn(rc, rc);
    AssertLogRelMsgReturn(!strcmp(gszVBoxOGLSSMMagic, szBuf), ("Unexpected data5: '%s'\n", szBuf), VERR_SSM_UNEXPECTED_DATA);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{VBOXCRCMD_SVRINFO,pfnCmd}
 */
static DECLCALLBACK(int8_t) crVBoxCrCmdCmd(HVBOXCRCMDSVR hSvr,
                                           const VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd)
{
    uint8_t bOpcode = pCmd->u8OpCode;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    ASSERT_GUEST_LOGREL_MSG_RETURN(   bOpcode == VBOXCMDVBVA_OPTYPE_CRCMD
                                   || bOpcode == VBOXCMDVBVA_OPTYPE_FLIP
                                   || bOpcode == VBOXCMDVBVA_OPTYPE_BLT
                                   || bOpcode == VBOXCMDVBVA_OPTYPE_CLRFILL,
                                   ("%#x\n", bOpcode), -1);
    RT_UNTRUSTED_VALIDATED_FENCE();

    switch (bOpcode)
    {
        case VBOXCMDVBVA_OPTYPE_CRCMD:
            ASSERT_GUEST_LOGREL_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_CRCMD), ("cbCmd=%u\n", cbCmd), -1);
            {
                VBOXCMDVBVA_CRCMD const RT_UNTRUSTED_VOLATILE_GUEST *pCrCmdDr
                    = (VBOXCMDVBVA_CRCMD const RT_UNTRUSTED_VOLATILE_GUEST *)pCmd;
                VBOXCMDVBVA_CRCMD_CMD const RT_UNTRUSTED_VOLATILE_GUEST *pCrCmd = &pCrCmdDr->Cmd;
                int rc = crVBoxServerCmdVbvaCrCmdProcess(pCrCmd, cbCmd - RT_UOFFSETOF(VBOXCMDVBVA_CRCMD, Cmd));
                ASSERT_GUEST_LOGREL_RC_RETURN(rc, -1);
                return 0;
            }

        case VBOXCMDVBVA_OPTYPE_FLIP:
            ASSERT_GUEST_LOGREL_MSG_RETURN(cbCmd >= VBOXCMDVBVA_SIZEOF_FLIPSTRUCT_MIN, ("cbCmd=%u\n", cbCmd), -1);
            {
                VBOXCMDVBVA_FLIP const RT_UNTRUSTED_VOLATILE_GUEST *pFlip
                    = (VBOXCMDVBVA_FLIP const RT_UNTRUSTED_VOLATILE_GUEST *)pCmd;
                return crVBoxServerCrCmdFlipProcess(pFlip, cbCmd);
            }

        case VBOXCMDVBVA_OPTYPE_BLT:
            ASSERT_GUEST_LOGREL_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_BLT_HDR), ("cbCmd=%u\n", cbCmd), -1);
            return crVBoxServerCrCmdBltProcess((VBOXCMDVBVA_BLT_HDR const RT_UNTRUSTED_VOLATILE_GUEST *)pCmd, cbCmd);

        case VBOXCMDVBVA_OPTYPE_CLRFILL:
            ASSERT_GUEST_LOGREL_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_CLRFILL_HDR), ("cbCmd=%u\n", cbCmd), -1);
            return crVBoxServerCrCmdClrFillProcess((VBOXCMDVBVA_CLRFILL_HDR const RT_UNTRUSTED_VOLATILE_GUEST *)pCmd, cbCmd);

        default:
            AssertFailedReturn(-1);
    }
    /* not reached */
}

/* We moved all CrHgsmi command processing to crserverlib to keep the logic of dealing with CrHgsmi commands in one place.
 *
 * For now we need the notion of CrHgdmi commands in the crserver_lib to be able to complete it asynchronously once it is really processed.
 * This help avoiding the "blocked-client" issues. The client is blocked if another client is doing begin-end stuff.
 * For now we eliminated polling that could occur on block, which caused a higher-priority thread (in guest) polling for the blocked command complition
 * to block the lower-priority thread trying to complete the blocking command.
 * And removed extra memcpy done on blocked command arrival.
 *
 * In the future we will extend CrHgsmi functionality to maintain texture data directly in CrHgsmi allocation to avoid extra memcpy-ing with PBO,
 * implement command completion and stuff necessary for GPU scheduling to work properly for WDDM Windows guests, etc.
 *
 * NOTE: it is ALWAYS responsibility of the crVBoxServerCrHgsmiCmd to complete the command!
 * */


int32_t crVBoxServerCrHgsmiCmd(struct VBOXVDMACMD_CHROMIUM_CMD *pCmd, uint32_t cbCmd)
{

    int32_t rc;
    uint32_t cBuffers = pCmd->cBuffers;
    uint32_t cParams;
    uint32_t cbHdr;
    CRVBOXHGSMIHDR *pHdr;
    uint32_t u32Function;
    uint32_t u32ClientID;
    CRClient *pClient;

    if (!g_pvVRamBase)
    {
        WARN(("g_pvVRamBase is not initialized"));

        crServerCrHgsmiCmdComplete(pCmd, VERR_INVALID_STATE);
        return VINF_SUCCESS;
    }

    if (!cBuffers)
    {
        WARN(("zero buffers passed in!"));

        crServerCrHgsmiCmdComplete(pCmd, VERR_INVALID_PARAMETER);
        return VINF_SUCCESS;
    }

    cParams = cBuffers-1;

    cbHdr = pCmd->aBuffers[0].cbBuffer;
    pHdr = VBOXCRHGSMI_PTR_SAFE(pCmd->aBuffers[0].offBuffer, cbHdr, CRVBOXHGSMIHDR);
    if (!pHdr)
    {
        WARN(("invalid header buffer!"));

        crServerCrHgsmiCmdComplete(pCmd, VERR_INVALID_PARAMETER);
        return VINF_SUCCESS;
    }

    if (cbHdr < sizeof (*pHdr))
    {
        WARN(("invalid header buffer size!"));

        crServerCrHgsmiCmdComplete(pCmd, VERR_INVALID_PARAMETER);
        return VINF_SUCCESS;
    }

    u32Function = pHdr->u32Function;
    u32ClientID = pHdr->u32ClientID;

    switch (u32Function)
    {
        case SHCRGL_GUEST_FN_WRITE:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIWRITE* pFnCmd = (CRVBOXHGSMIWRITE*)pHdr;
                VBOXVDMACMD_CHROMIUM_BUFFER *pBuf = &pCmd->aBuffers[1];
                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    crWarning("invalid write cmd buffer size!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    crWarning("invalid buffer data received from guest!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    break;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SET(&pClient->conn->CmdData, pCmd, pHdr, true);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }
            else
            {
                crWarning("invalid number of args");
                rc = VERR_INVALID_PARAMETER;
                break;
            }
            break;
        }

        case SHCRGL_GUEST_FN_INJECT:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_INJECT\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIINJECT *pFnCmd = (CRVBOXHGSMIINJECT*)pHdr;
                /* Fetch parameters. */
                uint32_t u32InjectClientID = pFnCmd->u32ClientID;
                VBOXVDMACMD_CHROMIUM_BUFFER *pBuf = &pCmd->aBuffers[1];
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    crWarning("invalid inject cmd buffer size!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    crWarning("invalid buffer data received from guest!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32InjectClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    break;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SET(&pClient->conn->CmdData, pCmd, pHdr, true);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }

            crWarning("invalid number of args");
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_READ\n"));

            /** @todo Verify  */
            if (cParams == 1)
            {
                CRVBOXHGSMIREAD *pFnCmd = (CRVBOXHGSMIREAD*)pHdr;
                VBOXVDMACMD_CHROMIUM_BUFFER *pBuf = &pCmd->aBuffers[1];
                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    crWarning("invalid read cmd buffer size!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }


                if (!pBuffer)
                {
                    crWarning("invalid buffer data received from guest!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    break;
                }

                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                rc = crVBoxServerInternalClientRead(pClient, pBuffer, &cbBuffer);

                /* Return the required buffer size always */
                pFnCmd->cbBuffer = cbBuffer;

                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                /* the read command is never pended, complete it right away */
                pHdr->result = rc;

                crServerCrHgsmiCmdComplete(pCmd, VINF_SUCCESS);
                return VINF_SUCCESS;
            }

            crWarning("invalid number of args");
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_WRITE_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE_READ\n"));

            /** @todo Verify  */
            if (cParams == 2)
            {
                CRVBOXHGSMIWRITEREAD *pFnCmd = (CRVBOXHGSMIWRITEREAD*)pHdr;
                VBOXVDMACMD_CHROMIUM_BUFFER *pBuf = &pCmd->aBuffers[1];
                VBOXVDMACMD_CHROMIUM_BUFFER *pWbBuf = &pCmd->aBuffers[2];

                /* Fetch parameters. */
                uint32_t cbBuffer = pBuf->cbBuffer;
                uint8_t *pBuffer  = VBOXCRHGSMI_PTR_SAFE(pBuf->offBuffer, cbBuffer, uint8_t);

                uint32_t cbWriteback = pWbBuf->cbBuffer;
                char *pWriteback  = VBOXCRHGSMI_PTR_SAFE(pWbBuf->offBuffer, cbWriteback, char);

                if (cbHdr < sizeof (*pFnCmd))
                {
                    crWarning("invalid write_read cmd buffer size!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }


                CRASSERT(cbBuffer);
                if (!pBuffer)
                {
                    crWarning("invalid write buffer data received from guest!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                CRASSERT(cbWriteback);
                if (!pWriteback)
                {
                    crWarning("invalid writeback buffer data received from guest!");
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
                rc = crVBoxServerClientGet(u32ClientID, &pClient);
                if (RT_FAILURE(rc))
                {
                    pHdr->result = rc;
                    crServerCrHgsmiCmdComplete(pCmd, VINF_SUCCESS);
                    return rc;
                }

                /* This should never fire unless we start to multithread */
                CRASSERT(pClient->conn->pBuffer==NULL && pClient->conn->cbBuffer==0);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);

                pClient->conn->pBuffer = pBuffer;
                pClient->conn->cbBuffer = cbBuffer;
                CRVBOXHGSMI_CMDDATA_SETWB(&pClient->conn->CmdData, pCmd, pHdr, pWriteback, cbWriteback, &pFnCmd->cbWriteback, true);
                crVBoxServerInternalClientWriteRead(pClient);
                CRVBOXHGSMI_CMDDATA_ASSERT_CLEANED(&pClient->conn->CmdData);
                return VINF_SUCCESS;
            }

            crWarning("invalid number of args");
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        case SHCRGL_GUEST_FN_SET_VERSION:
        {
            WARN(("crVBoxServerCrHgsmiCmd, SHCRGL_GUEST_FN_SET_VERSION: invalid function"));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

        case SHCRGL_GUEST_FN_SET_PID:
        {
            WARN(("crVBoxServerCrHgsmiCmd, SHCRGL_GUEST_FN_SET_PID: invalid function"));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

        default:
        {
            WARN(("crVBoxServerCrHgsmiCmd: invalid functionm %d", u32Function));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }

    }

    /* we can be on fail only here */
    CRASSERT(RT_FAILURE(rc));
    pHdr->result = rc;

    crServerCrHgsmiCmdComplete(pCmd, VINF_SUCCESS);
    return rc;

}


static DECLCALLBACK(bool) crVBoxServerHasDataForScreen(uint32_t u32ScreenID)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetEnabledForScreen(u32ScreenID);
    if (hFb)
        return CrFbHas3DData(hFb);

    return false;
}


static DECLCALLBACK(bool) crVBoxServerHasData(void)
{
    HCR_FRAMEBUFFER hFb = CrPMgrFbGetFirstEnabled();
    for (;
            hFb;
            hFb = CrPMgrFbGetNextEnabled(hFb))
    {
        if (CrFbHas3DData(hFb))
            return true;
    }

    return false;
}

int32_t crVBoxServerCrHgsmiCtl(struct VBOXVDMACMD_CHROMIUM_CTL *pCtl, uint32_t cbCtl)
{
    int rc = VINF_SUCCESS;

    switch (pCtl->enmType)
    {
        case VBOXVDMACMD_CHROMIUM_CTL_TYPE_CRHGSMI_SETUP:
        {
            PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP pSetup = (PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP)pCtl;
            g_pvVRamBase = (uint8_t*)pSetup->pvVRamBase;
            g_cbVRam = pSetup->cbVRam;

            g_pLed = pSetup->pLed;

            cr_server.ClientInfo = pSetup->CrClientInfo;

            pSetup->CrCmdServerInfo.hSvr = NULL;
            pSetup->CrCmdServerInfo.pfnEnable = crVBoxCrCmdEnable;
            pSetup->CrCmdServerInfo.pfnDisable = crVBoxCrCmdDisable;
            pSetup->CrCmdServerInfo.pfnCmd = crVBoxCrCmdCmd;
            pSetup->CrCmdServerInfo.pfnHostCtl = crVBoxCrCmdHostCtl;
            pSetup->CrCmdServerInfo.pfnGuestCtl = crVBoxCrCmdGuestCtl;
            pSetup->CrCmdServerInfo.pfnResize = crVBoxCrCmdResize;
            pSetup->CrCmdServerInfo.pfnSaveState = crVBoxCrCmdSaveState;
            pSetup->CrCmdServerInfo.pfnLoadState = crVBoxCrCmdLoadState;
            rc = VINF_SUCCESS;
            break;
        }
        case VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_BEGIN:
        case VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_END:
            rc = VINF_SUCCESS;
            break;
        case VBOXVDMACMD_CHROMIUM_CTL_TYPE_CRHGSMI_SETUP_MAINCB:
        {
            PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP_MAINCB pSetup = (PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP_MAINCB)pCtl;
            g_hCrHgsmiCompletion = pSetup->hCompletion;
            g_pfnCrHgsmiCompletion = pSetup->pfnCompletion;

            pSetup->MainInterface.pfnHasData = crVBoxServerHasData;
            pSetup->MainInterface.pfnHasDataForScreen = crVBoxServerHasDataForScreen;

            rc = VINF_SUCCESS;
            break;
        }
        default:
            AssertMsgFailed(("invalid param %d", pCtl->enmType));
            rc = VERR_INVALID_PARAMETER;
    }

    /* NOTE: Control commands can NEVER be pended here, this is why its a task of a caller (Main)
     * to complete them accordingly.
     * This approach allows using host->host and host->guest commands in the same way here
     * making the command completion to be the responsibility of the command originator.
     * E.g. ctl commands can be both Hgcm Host synchronous commands that do not require completion at all,
     * or Hgcm Host Fast Call commands that do require completion. All this details are hidden here */
    return rc;
}

static int crVBoxServerCrCmdDisablePostProcess(VBOXCRCMDCTL_HGCMENABLE_DATA *pData)
{
    int rc = VINF_SUCCESS;
    uint8_t* pCtl;
    uint32_t cbCtl;
    HVBOXCRCMDCTL_REMAINING_HOST_COMMAND hRHCmd = pData->hRHCmd;
    PFNVBOXCRCMDCTL_REMAINING_HOST_COMMAND pfnRHCmd = pData->pfnRHCmd;

    Assert(!cr_server.fCrCmdEnabled);

    if (cr_server.numClients)
    {
        WARN(("cr_server.numClients(%d) is not NULL", cr_server.numClients));
        return VERR_INVALID_STATE;
    }

    for (pCtl = pfnRHCmd(hRHCmd, &cbCtl, rc); pCtl; pCtl = pfnRHCmd(hRHCmd, &cbCtl, rc))
    {
        rc = crVBoxCrCmdHostCtl(NULL, pCtl, cbCtl);
    }

    memset(&cr_server.DisableData, 0, sizeof (cr_server.DisableData));

    return VINF_SUCCESS;
}

int32_t crVBoxServerHgcmEnable(VBOXCRCMDCTL_HGCMENABLE_DATA *pData)
{
    int rc = crVBoxServerCrCmdDisablePostProcess(pData);
    if (RT_FAILURE(rc))
    {
        WARN(("crVBoxServerCrCmdDisablePostProcess failed %d", rc));
        return rc;
    }

    crVBoxServerDefaultContextSet();

    return VINF_SUCCESS;
}

int32_t crVBoxServerHgcmDisable(VBOXCRCMDCTL_HGCMDISABLE_DATA *pData)
{
    Assert(!cr_server.fCrCmdEnabled);

    Assert(!cr_server.numClients);

    crVBoxServerRemoveAllClients();

    CRASSERT(!cr_server.numClients);

    crVBoxServerDefaultContextClear();

    cr_server.DisableData = *pData;

    return VINF_SUCCESS;
}

#endif
