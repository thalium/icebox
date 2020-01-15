/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef INCLUDE_CR_SERVER_H
#define INCLUDE_CR_SERVER_H

#include "cr_spu.h"
#include "cr_net.h"
#include "cr_hash.h"
#include "cr_protocol.h"
#include "cr_glstate.h"
#include "cr_vreg.h"
#include "cr_blitter.h"
#include "cr_htable.h"
#include "cr_unpack.h"
#include "spu_dispatch_table.h"
#include "cr_dump.h"

#include "state/cr_currentpointers.h"

#include <iprt/types.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/list.h>
#include <iprt/thread.h>
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/memcache.h>

#include <VBox/vmm/ssm.h>

#include <VBoxVideo.h>
#include <VBoxVideoVBE.h>
#include <VBoxVideo3D.h>
#include <VBoxVideoHost3D.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CR_MAX_WINDOWS 100
#define CR_MAX_CLIENTS 64

/** @todo must match MaxGuestMonitors from SchemaDefs.h*/
#define CR_MAX_GUEST_MONITORS VBOX_VIDEO_MAX_SCREENS

typedef DECLCALLBACKPTR(void, PFNCRSERVERPRESENTFBO) (void *data, int32_t screenId, int32_t x, int32_t y, uint32_t w, uint32_t h);

/* Callbacks for output of the rendered frames.
 *
 * This allows to pass rendered frames to an external component rather than draw them on screen.
 *
 * An external component registers the redirection callbacks using crVBoxServerOutputRedirectSet.
 *
 * The list of formats supported by the caller is obtained using CRORContextProperty.
 * The actual format choosed by the service is passed as a CRORBegin parameter.
 */
typedef struct {
    const void *pvContext; /* Supplied by crVBoxServerOutputRedirectSet. */
    DECLR3CALLBACKMEMBER(void, CRORBegin,           (const void *pvContext, void **ppvInstance,
                                                     const char *pszFormat));
    DECLR3CALLBACKMEMBER(void, CRORGeometry,        (void *pvInstance,
                                                     int32_t x, int32_t y, uint32_t w, uint32_t h));
    DECLR3CALLBACKMEMBER(void, CRORVisibleRegion,   (void *pvInstance,
                                                     uint32_t cRects, const RTRECT *paRects));
    DECLR3CALLBACKMEMBER(void, CRORFrame,           (void *pvInstance,
                                                     void *pvData, uint32_t cbData));
    DECLR3CALLBACKMEMBER(void, CROREnd,             (void *pvInstance));
    DECLR3CALLBACKMEMBER(int,  CRORContextProperty, (const void *pvContext, uint32_t index,
                                                      void *pvBuffer, uint32_t cbBuffer, uint32_t *pcbOut));
} CROutputRedirect;

typedef struct {
    CRrecti imagewindow;    /**< coordinates in mural space */
    CRrectf bounds;         /**< normalized coordinates in [-1,-1] x [1,1] */
    CRrecti outputwindow;   /**< coordinates in server's rendering window */
    CRrecti clippedImagewindow;  /**< imagewindow clipped to current viewport */
    CRmatrix baseProjection;  /**< pre-multiplied onto projection matrix */
    CRrecti scissorBox;     /**< passed to back-end OpenGL */
    CRrecti viewport;       /**< passed to back-end OpenGL */
    GLuint serialNo;        /**< an optimization */
} CRExtent;

struct BucketingInfo;

typedef struct {
    char   *pszDpyName;
    GLint   visualBits;
    int32_t externalID;
} CRCreateInfo_t;

typedef struct {
    char   *pszDpyName;
    int32_t externalID;
    GLint   requestedVisualBits;
    GLint   realVisualBits;
} CRCreateInfoEx_t;

/* VRAM->RAM worker thread */

typedef enum
{
    CR_SERVER_RPW_STATE_UNINITIALIZED = 0,
    CR_SERVER_RPW_STATE_INITIALIZING,
    CR_SERVER_RPW_STATE_INITIALIZED,
    CR_SERVER_RPW_STATE_UNINITIALIZING,
} CR_SERVER_RPW_STATE;

/* worker control command */
typedef enum
{
    CR_SERVER_RPW_CTL_TYPE_UNDEFINED = 0,
    CR_SERVER_RPW_CTL_TYPE_WAIT_COMPLETE,
    CR_SERVER_RPW_CTL_TYPE_TERM
} CR_SERVER_RPW_CTL_TYPE;

struct CR_SERVER_RPW_ENTRY;

typedef struct CR_SERVER_RPW_CTL {
    CR_SERVER_RPW_CTL_TYPE enmType;
    int rc;
    RTSEMEVENT hCompleteEvent;
    /* valid for *_WAIT_COMPLETE and *_CANCEL */
    struct CR_SERVER_RPW_ENTRY *pEntry;
} CR_SERVER_RPW_CTL;


struct CR_SERVER_RPW_ENTRY;

typedef DECLCALLBACKPTR(void, PFNCR_SERVER_RPW_DATA) (const struct CR_SERVER_RPW_ENTRY* pEntry, void *pvEntryTexData);

typedef DECLCALLBACKPTR(void, PFNCRSERVERNOTIFYEVENT) (int32_t screenId, uint32_t uEvent, void* pvData, uint32_t cbData);

typedef struct CR_SERVER_RPW_ENTRY
{
    RTRECTSIZE Size;
    /*  We have to use 4 textures here.
     *
     *  1. iDrawTex - the texture clients can draw to and then submit it for contents acquisition via crServerRpwEntrySubmit
     *  2. iSubmittedTex - the texture submitted to the worker for processing and, whose processing has not start yet,
     *     i.e. it is being in the queue and can be safely removed/replaced [from] there
     *  3. iWorkerTex - the texture being prepared & passed by the worker to the GPU (stage 1 of a worker contents acquisition process)
     *  4. iGpuTex - the texture passed/processed to/by the GPU, whose data is then acquired by the server (stage 2 of a worker contents acquisition process)
     *
     *  - There can be valid distinct iGpuTex, iWorkerTex, iSubmittedTex and iDrawTex present simultaneously.
     *  - Either or both of iSubmittedTex and iFreeTex are always valid
     *
     *  Detail:
     *
     *  - iSubmittedTex and iFreeTex modifications are performed under CR_SERVER_RPW::CritSect lock.
     *
     *  - iDrawTex can only be changed by client side (i.e. the crServerRpwEntrySubmit caller), this is why client thread can access it w/o a lock
     *  - iSubmittedTex and iFreeTex can be modified by both client and worker, so lock is always required
     *
     *  - iDrawTex can be accessed by client code only
     *  - iWorkerTex and iGpuTex can be accessed by worker code only
     *  - iSubmittedTex and iFreeTex can be accessed under CR_SERVER_RPW::CritSect lock only
     *  - either or both of iSubmittedTex and iFreeTex are always valid (see below for more explanation),
     *    this is why client can easily determine the new iDrawTex value on Submit, i.e. :
     *
     *          (if initial iSubmittedTex was valid)
     *                ---------------
     *                |              ^
     *                >              |
     *   Submit-> iDrawTex -> iSubmittedTex
     *                ^
     *                | (if initial iSubmittedTex was NOT valid)
     *              iFreeTex
     *
     *  - The worker can invalidate the iSubmittedTex (i.e. do iSubmittedTex -> iWorkerTex) only after it is done
     *    with the last iWorkerTex -> iGpuTex transformation freeing the previously used iGpuTex to iFreeTex.
     *
     *  - A simplified worker iXxxTex transformation logic is:
     *    1. iFreeTex is initially valid
     *    2. iSubmittedTex -> iWorkerTex;
     *    3. submit iWorkerTex acquire request to the GPU
     *    4. complete current iGpuTex
     *    5. iGpuTex -> iFreeTex
     *    6. iWorkerTex -> iGpuTex
     *    7. goto 1
     *
     *  */
    int8_t iTexDraw;
    int8_t iTexSubmitted;
    int8_t iTexWorker;
    int8_t iTexGpu;
    int8_t iCurPBO;
    GLuint aidWorkerTexs[4];
    GLuint aidPBOs[2];
    RTLISTNODE WorkEntry;
    RTLISTNODE WorkerWorkEntry;
    RTLISTNODE GpuSubmittedEntry;
    PFNCR_SERVER_RPW_DATA pfnData;
} CR_SERVER_RPW_ENTRY;

typedef struct CR_SERVER_RPW {
    RTLISTNODE WorkList;
    RTCRITSECT CritSect;
    RTSEMEVENT hSubmitEvent;
    /* only one outstanding command is supported,
     * and ctl requests must be cynchronized, hold it right here */
    CR_SERVER_RPW_CTL Ctl;
    int ctxId;
    GLint ctxVisBits;
    RTTHREAD hThread;
} CR_SERVER_RPW;
/* */

/* FRAMEBUFFER */
typedef struct CR_FRAMEBUFFER *HCR_FRAMEBUFFER;
typedef struct CR_FRAMEBUFFER_ENTRY *HCR_FRAMEBUFFER_ENTRY;
/* */

typedef struct CR_FBDATA
{
    HCR_FRAMEBUFFER hFb;
    HCR_FRAMEBUFFER_ENTRY hFbEntry;
    CR_TEXDATA* apTexDatas[2];
} CR_FBDATA;
/**
 * Mural info
 */
typedef struct {
    GLuint width, height;
    GLint gX, gY;            /*guest coordinates*/
    GLint hX, hY;            /*host coordinates, screenID related*/

    int spuWindow;           /*the SPU's corresponding window ID */

    int screenId;

    GLboolean bVisible;      /*guest window is visible*/
    GLubyte   u8Unused;       /*redirect to FBO instead of real host window*/
    GLboolean bFbDraw;       /*GL_FRONT buffer is drawn to directly*/
    GLboolean fIsDummyRefference;

    GLint       cVisibleRects;    /*count of visible rects*/
    GLint      *pVisibleRects;    /*visible rects left, top, right, bottom*/
    GLboolean   bReceivedRects;   /*indicates if guest did any updates for visible regions*/

    GLuint cBuffers;
    GLuint iBbBuffer;
    GLuint aidFBOs[2];
    GLuint aidColorTexs[2];

    void *pvReserved;

    CRCreateInfoEx_t CreateInfo;

    /* to avoid saved state breakage we need to keep RT_OFFSETOF(CRMuralInfo, CreateInfo) intact
     * this is why we place some FBO stuff to the tail
     * @todo: once we need to increment a saved state version, we could refactor this structure */
    GLint iCurDrawBuffer;
    GLint iCurReadBuffer;

    GLuint idDepthStencilRB;
    GLuint fboWidth, fboHeight;

    GLboolean fHasParentWindow;

    GLboolean fRedirected;
    GLboolean fForcePresentState;
    GLboolean fOrPresentOnReenable;

    GLboolean fIsVisible;

    CR_TEXDATA aTexs[2];
    uint32_t cUsedFBDatas;
    CR_FBDATA *apUsedFBDatas[CR_MAX_GUEST_MONITORS];
    CR_FBDATA aFBDatas[CR_MAX_GUEST_MONITORS];

    /* bitfield representing contexts the mural has been ever current with
     * we just reuse CR_STATE_SHAREDOBJ_USAGE_XXX API here for simplicity */
    CRbitvalue             ctxUsage[CR_MAX_BITARRAY];
} CRMuralInfo;

typedef struct {
    CRContext *pContext;
    int SpuContext;
    CRCreateInfoEx_t CreateInfo;
    CRMuralInfo * currentMural;
} CRContextInfo;

/**
 * A client is basically an upstream Cr Node (connected via mothership)
 */
typedef struct _crclient {
    int spu_id;        /**< id of the last SPU in the client's SPU chain */
    CRConnection *conn;       /**< network connection from the client */
    int number;        /**< a unique number for each client */
    uint64_t pid;      /*guest pid*/
    GLint currentContextNumber;
    CRContextInfo *currentCtxInfo;
    GLint currentWindow;
    CRMuralInfo *currentMural;
    GLint windowList[CR_MAX_WINDOWS];
    GLint contextList[CR_MAX_CONTEXTS];
#ifdef VBOXCR_LOGFPS
    uint64_t timeUsed;
#endif
} CRClient;

typedef struct _crclientnode {
    CRClient *pClient;
    struct _crclientnode *prev, *next;
} CRClientNode;

typedef struct CRPoly_t {
    int npoints;
    double *points;
    struct CRPoly_t *next;
} CRPoly;

/**
 * There's one of these run queue entries per client
 * The run queue is a circular, doubly-linked list of these objects.
 */
typedef struct RunQueue_t {
    CRClient *client;
    int blocked;
    struct RunQueue_t *next;
    struct RunQueue_t *prev;
} RunQueue;

typedef struct {
    GLint freeWindowID;
    GLint freeContextID;
    GLint freeClientID;
} CRServerFreeIDsPool_t;

typedef struct {
    int32_t    x, y;
    uint32_t   w, h;
    uint64_t   winID;
} CRScreenInfo;

typedef struct {
    RTRECT Rect;
} CRScreenViewportInfo;

/* BFB (BlitFramebuffer Blitter) flags
 * so far only CR_SERVER_BFB_ON_ALWAIS is supported and is alwais used if any flag is set */
#define CR_SERVER_BFB_DISABLED 0
#define CR_SERVER_BFB_ON_INVERTED_BLIT 1
#define CR_SERVER_BFB_ON_STRAIGHT_BLIT 2
#define CR_SERVER_BFB_ON_ALWAIS (CR_SERVER_BFB_ON_INVERTED_BLIT | CR_SERVER_BFB_ON_STRAIGHT_BLIT)

typedef struct {
    unsigned short tcpip_port;

    CRScreenInfo screen[CR_MAX_GUEST_MONITORS];
    CRScreenViewportInfo screenVieport[CR_MAX_GUEST_MONITORS];
    int          screenCount;

    GLboolean fCrCmdEnabled;

    GLboolean fProcessingPendedCommands;

    int numClients;
    CRClient *clients[CR_MAX_CLIENTS];  /**< array [numClients] */
    CRClient *curClient;
    CRClientNode *pCleanupClient;  /*list of clients with pending clean up*/
    CRHTABLE clientTable;
    CRCurrentStatePointers current;

    GLboolean firstCallCreateContext;
    GLboolean firstCallMakeCurrent;
    GLboolean bIsInLoadingState; /* Indicates if we're in process of loading VM snapshot */
    GLboolean bIsInSavingState; /* Indicates if we're in process of saving VM snapshot */
    GLboolean bForceMakeCurrentOnClientSwitch;
    CRContextInfo *currentCtxInfo;
    GLint currentWindow;
    GLint currentNativeWindow;
    CRMuralInfo *currentMural;

    CRHashTable *muralTable;  /**< hash table where all murals are stored */

    int client_spu_id;

    int mtu;
    int buffer_size;
    char protocol[1024];

    SPU *head_spu;
    SPUDispatchTable dispatch;

    CRNetworkPointer return_ptr;
    CRNetworkPointer writeback_ptr;
    /*
     * The current active unpacker state, this hack is required for the server dispatch routines
     * (see crServerDispatchReadPixels for example) as it needs to read data from the buffer
     * but it is not possible to supply the state as a parameter unfortunately.
     */
    PCrUnpackerState pUnpackerState;

    CRLimitsState limits; /**< GL limits for any contexts we create */

    CRContextInfo MainContextInfo;

    CRHashTable *contextTable;  /**< hash table for rendering contexts */

    CRHashTable *programTable;  /**< for vertex programs */
    GLuint currentProgram;

    /* visBits -> dummy mural association */
    CRHashTable *dummyMuralTable;

    /** State tracker state. */
    CRStateTracker StateTracker;

    GLboolean fRootVrOn;
    VBOXVR_LIST RootVr;
    /* we need to translate Root Vr to each window coords, this one cpecifies the current translation point
     * note that since window attributes modifications is performed in HGCM thread only and thus is serialized,
     * we deal with the global RootVr data directly */
    RTPOINT RootVrCurPoint;

    /* blitter so far used for working around host drivers BlitFramebuffer bugs
     * by implementing */
    uint32_t fBlitterMode;
    CR_BLITTER Blitter;

    CR_SERVER_RPW RpwWorker;

    VBOXCRCMDCTL_HGCMDISABLE_DATA DisableData;

    RTSEMEVENT hCalloutCompletionEvent;
    VBOXCRCMDCTL *pCurrentCalloutCtl;
    VBOXCRCLIENT_INFO ClientInfo;

    /** configuration options */
    /*@{*/
    int useL2;
    int ignore_papi;
    unsigned int maxBarrierCount;
    unsigned int clearCount;
    int optimizeBucket;
    int only_swap_once;
    int debug_barriers;
    int sharedDisplayLists;
    int sharedTextureObjects;
    int sharedPrograms;
    int sharedWindows;
    int uniqueWindows;
    int localTileSpec;
    int useDMX;
    int overlapBlending;
    int vpProjectionMatrixParameter;
    const char *vpProjectionMatrixVariable;
    int stereoView;
    int vncMode;   /* cmd line option */
    /*@}*/
    /** view_matrix config */
    /*@{*/
    GLboolean viewOverride;
    CRmatrix viewMatrix[2];  /**< left and right eye */
    /*@}*/
    /** projection_matrix config */
    /*@{*/
    GLboolean projectionOverride;
    CRmatrix projectionMatrix[2];  /**< left and right eye */
    int currentEye;
    /*@}*/

    /** for warped tiles */
    /*@{*/
    GLfloat alignment_matrix[16], unnormalized_alignment_matrix[16];
    /*@}*/

    /** tile overlap/blending info - this should probably be per-mural */
    /*@{*/
    CRPoly **overlap_geom;
    CRPoly *overlap_knockout;
    float *overlap_intens;
    int num_overlap_intens;
    int num_overlap_levels;
    /*@}*/

    CRHashTable *barriers, *semaphores;

    RunQueue *run_queue;

    GLuint currentSerialNo;

    GLuint                fVisualBitsDefault;
    GLboolean             bUsePBOForReadback;       /*Use PBO's for data readback*/

    CROutputRedirect      outputRedirect;

    GLboolean             bUseMultipleContexts;

    GLboolean             bWindowsInitiallyHidden;

    /* OR-ed CR_VBOX_CAP_XXX cap values
     * describing VBox Chromium functionality caps visible to guest
     * Currently can have only CR_VBOX_CAP_TEX_PRESENT cap to notify
     * that the TexPresent mechanism is available and enabled */
    uint32_t              u32Caps;

    PFNCRSERVERNOTIFYEVENT pfnNotifyEventCB;

    SPUDispatchTable TmpCtxDispatch;

    VBOXCRCMD_SVRENABLE_INFO CrCmdClientInfo;

#ifdef VBOX_WITH_CRSERVER_DUMPER
    CR_RECORDER Recorder;
    CR_BLITTER RecorderBlitter;
    CR_DBGPRINT_DUMPER DbgPrintDumper;
    CR_HTML_DUMPER HtmlDumper;
    CR_DUMPER *pDumper;
#endif

    int RcToGuest;
    int RcToGuestOnce;
} CRServer;


extern DECLEXPORT(void) crServerInit( int argc, char *argv[] );
extern DECLEXPORT(int)  CRServerMain( int argc, char *argv[] );
extern DECLEXPORT(void) crServerServiceClients(void);
extern DECLEXPORT(void) crServerAddNewClient(void);
extern DECLEXPORT(SPU*) crServerHeadSPU(void);
extern DECLEXPORT(void) crServerSetPort(int port);

extern DECLEXPORT(GLboolean) crVBoxServerInit(void);
extern DECLEXPORT(void) crVBoxServerTearDown(void);
extern DECLEXPORT(int32_t) crVBoxServerAddClient(uint32_t u32ClientID);
extern DECLEXPORT(void) crVBoxServerRemoveClient(uint32_t u32ClientID);
extern DECLEXPORT(int32_t) crVBoxServerClientWrite(uint32_t u32ClientID, uint8_t *pBuffer, uint32_t cbBuffer);
extern DECLEXPORT(int32_t) crVBoxServerClientRead(uint32_t u32ClientID, uint8_t *pBuffer, uint32_t *pcbBuffer);
extern DECLEXPORT(int32_t) crVBoxServerClientSetVersion(uint32_t u32ClientID, uint32_t vMajor, uint32_t vMinor);
extern DECLEXPORT(int32_t) crVBoxServerClientGetCapsLegacy(uint32_t u32ClientID, uint32_t *pu32Caps);
extern DECLEXPORT(int32_t) crVBoxServerClientGetCapsNew(uint32_t u32ClientID, CR_CAPS_INFO *pInfo);
extern DECLEXPORT(int32_t) crVBoxServerClientSetPID(uint32_t u32ClientID, uint64_t pid);

extern DECLEXPORT(int32_t) crVBoxServerSaveState(PSSMHANDLE pSSM);
extern DECLEXPORT(int32_t) crVBoxServerLoadState(PSSMHANDLE pSSM, uint32_t version);

typedef struct
{
    CR_BLITTER_IMG Img;
    uint32_t u32Screen;
    uint32_t fDataAllocated;
} CR_SCREENSHOT;

extern DECLEXPORT(int) crServerVBoxWindowsShow(bool fShow);
extern DECLEXPORT(int) crServerVBoxScreenshotGet(uint32_t u32Screen, uint32_t width, uint32_t height, uint32_t pitch, void *pvBuffer, CR_SCREENSHOT *pScreenshot);
extern DECLEXPORT(void) crServerVBoxScreenshotRelease(CR_SCREENSHOT *pScreenshot);

extern DECLEXPORT(void) crServerVBoxCompositionSetEnableStateGlobal(GLboolean fEnable);
extern DECLEXPORT(int32_t) crVBoxServerSetScreenCount(int sCount);
extern DECLEXPORT(int32_t) crVBoxServerUnmapScreen(int sIndex);
extern DECLEXPORT(int32_t) crVBoxServerMapScreen(int sIndex, int32_t x, int32_t y, uint32_t w, uint32_t h, uint64_t winID);
extern DECLEXPORT(void) crServerVBoxCompositionSetEnableStateGlobal(GLboolean fEnable);
struct VBVAINFOSCREEN;
extern DECLEXPORT(int) crVBoxServerNotifyResize(const struct VBVAINFOSCREEN *pScreen, void *pvVRAM);
extern DECLEXPORT(int32_t) crVBoxServerSetRootVisibleRegion(GLint cRects, const RTRECT *pRects);

extern DECLEXPORT(int32_t) crVBoxServerSetOffscreenRendering(GLboolean value);

extern DECLEXPORT(int32_t) crVBoxServerOutputRedirectSet(const CROutputRedirect *pCallbacks);

extern DECLEXPORT(int32_t) crVBoxServerSetScreenViewport(int sIndex, int32_t x, int32_t y, uint32_t w, uint32_t h);

extern DECLEXPORT(void) crServerVBoxSetNotifyEventCB(PFNCRSERVERNOTIFYEVENT pfnCb);

extern DECLEXPORT(void) crVBoxServerCalloutEnable(VBOXCRCMDCTL *pCtl);
extern DECLEXPORT(void) crVBoxServerCalloutDisable(void);
extern DECLEXPORT(void) crServerSetUnscaledHiDPI(bool fEnable);

#ifdef VBOX_WITH_CRHGSMI
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
extern DECLEXPORT(int32_t) crVBoxServerCrHgsmiCmd(struct VBOXVDMACMD_CHROMIUM_CMD *pCmd, uint32_t cbCmd);
extern DECLEXPORT(int32_t) crVBoxServerCrHgsmiCtl(struct VBOXVDMACMD_CHROMIUM_CTL *pCtl, uint32_t cbCtl);

#endif

extern DECLEXPORT(int32_t) crVBoxServerHgcmEnable(VBOXCRCMDCTL_HGCMENABLE_DATA *pData);
extern DECLEXPORT(int32_t) crVBoxServerHgcmDisable(VBOXCRCMDCTL_HGCMDISABLE_DATA *pData);

extern int crVBoxServerHostCtl(VBOXCRCMDCTL *pCtl, uint32_t cbCtl);

extern DECLEXPORT(void) crVBoxServerDetachThread(void);
extern DECLEXPORT(void) crVBoxServerAttachThread(void);

#ifdef __cplusplus
}
#endif

#endif

