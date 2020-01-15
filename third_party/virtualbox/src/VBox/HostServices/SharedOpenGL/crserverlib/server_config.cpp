    /* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <string.h>
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_error.h"
#include "cr_glstate.h"
#include "server.h"

#include <iprt/env.h>

#ifdef WINDOWS
#pragma warning( disable: 4706 )
#endif

static void
setDefaults(void)
{
    cr_server.run_queue = NULL;
    cr_server.optimizeBucket = 1;
    cr_server.useL2 = 0;
    cr_server.maxBarrierCount = 0;
    cr_server.ignore_papi = 0;
    cr_server.only_swap_once = 0;
    cr_server.overlapBlending = 0;
    cr_server.debug_barriers = 0;
    cr_server.sharedDisplayLists = 0;
    cr_server.sharedTextureObjects = 0;
    cr_server.sharedPrograms = 0;
    cr_server.sharedWindows = 0;
    cr_server.useDMX = 0;
    cr_server.vpProjectionMatrixParameter = -1;
    cr_server.vpProjectionMatrixVariable = NULL;
    cr_server.currentProgram = 0;

    cr_server.num_overlap_intens = 0;
    cr_server.overlap_intens = 0;
    crMemset(&cr_server.MainContextInfo, 0, sizeof (cr_server.MainContextInfo));

    crMatrixInit(&cr_server.viewMatrix[0]);
    crMatrixInit(&cr_server.viewMatrix[1]);
    crMatrixInit(&cr_server.projectionMatrix[0]);
    crMatrixInit(&cr_server.projectionMatrix[1]);
    cr_server.currentEye = -1;

    cr_server.uniqueWindows = 0;

    cr_server.screenCount = 0;
    cr_server.bUsePBOForReadback = GL_FALSE;
    cr_server.bWindowsInitiallyHidden = GL_FALSE;

    cr_server.pfnNotifyEventCB = NULL;
}

/* Check if host reports minimal OpenGL capabilities.
 *
 * Require OpenGL 2.1 or later.
 *
 * For example, on Windows host this may happen if host has no graphics
 * card drivers installed or drivers were not properly signed or VBox
 * is running via remote desktop session etc. Currently, we take care
 * about Windows host only when specific RENDERER and VERSION strings
 * returned in this case. Later this check should be expanded to the
 * rest of hosts. */
static bool crServerHasInsufficientCaps()
{
    const char *pszRealVersion;
    int rc;
    uint32_t u32VerMajor = 0;
    uint32_t u32VerMinor = 0;
    char *pszNext = NULL;

    if (!cr_server.head_spu)
        return true;

    pszRealVersion = (const char *)cr_server.head_spu->dispatch_table.GetString(GL_REAL_VERSION);
    if (!pszRealVersion)
        return true; /* No version == insufficient. */

    rc = RTStrToUInt32Ex(pszRealVersion, &pszNext, 10, &u32VerMajor);
    if (   RT_SUCCESS(rc)
        && *pszNext == '.')
            RTStrToUInt32Ex(pszNext + 1, NULL, 10, &u32VerMinor);

    crInfo("Host supports version %d.%d [%s]", u32VerMajor, u32VerMinor, pszRealVersion);

    if (   u32VerMajor > 2
        || (u32VerMajor == 2 && u32VerMinor >= 1))
        return false; /* >= 2.1, i.e. good enough. */

    return true; /* Insufficient. */
}

void crServerSetVBoxConfigurationHGCM()
{
    CRMuralInfo *defaultMural;
    int i;
    GLint dims[4];
    const char * env;

    defaultMural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, 0);
    CRASSERT(defaultMural);

    /// @todo should be moved to addclient so we have a chain for each client

    setDefaults();
    
    /* Load the SPUs */
    PCSPUREG aSpuRegs[] = { &g_RenderSpuReg, &g_ErrorSpuReg, NULL};
    cr_server.head_spu = crSPUInitFromReg(NULL, 0, "render", &cr_server, &aSpuRegs[0]);
    if (!cr_server.head_spu)
        return;


    env = RTEnvGet( "CR_SERVER_DEFAULT_VISUAL_BITS" );
    if (env != NULL && env[0] != '\0')
    {
        unsigned int bits = (unsigned int)crStrParseI32(env, 0);
        if (bits <= CR_ALL_BITS)
            cr_server.fVisualBitsDefault = bits;
        else
            crWarning("invalid bits option %c", bits);
    }
    else
        cr_server.fVisualBitsDefault = CR_RGB_BIT | CR_ALPHA_BIT | CR_DOUBLE_BIT;


    env = RTEnvGet("CR_SERVER_CAPS");
    if (env && env[0] != '\0')
    {
        cr_server.u32Caps = crStrParseI32(env, 0);
        cr_server.u32Caps &= CR_VBOX_CAPS_ALL;
    }
    else
    {
        cr_server.u32Caps = CR_VBOX_CAP_TEX_PRESENT
                | CR_VBOX_CAP_CMDVBVA
                | CR_VBOX_CAP_CMDBLOCKS
                | CR_VBOX_CAP_GETATTRIBSLOCATIONS
                | CR_VBOX_CAP_CMDBLOCKS_FLUSH
                ;
    }

    if (crServerHasInsufficientCaps())
    {
        crDebug("Cfg: report minimal OpenGL capabilities");
        cr_server.u32Caps |= CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT;
    }

    crInfo("Cfg: u32Caps(%#x), fVisualBitsDefault(%#x)",
            cr_server.u32Caps,
            cr_server.fVisualBitsDefault);

    cr_server.head_spu->dispatch_table.GetChromiumParametervCR(GL_WINDOW_POSITION_CR, 0, GL_INT, 2, &dims[0]);
    cr_server.head_spu->dispatch_table.GetChromiumParametervCR(GL_WINDOW_SIZE_CR, 0, GL_INT, 2, &dims[2]);
    
    defaultMural->gX = dims[0];
    defaultMural->gY = dims[1];
    defaultMural->width = dims[2];
    defaultMural->height = dims[3];

    cr_server.mtu = 1024 * 250;

    cr_server.numClients = 0;
    strcpy(cr_server.protocol, "vboxhgcm");

    for (i = 0; i < cr_server.numClients; i++)
    {
        CRClient *newClient = (CRClient *) crCalloc(sizeof(CRClient));
        newClient->spu_id = 0;
        newClient->conn = crNetAcceptClient(cr_server.protocol, NULL,
                                            cr_server.tcpip_port,
                                            cr_server.mtu, 0);
        newClient->currentCtxInfo = &cr_server.MainContextInfo;
        crServerAddToRunQueue(newClient);

        cr_server.clients[i] = newClient;
    }

    /* set default client and mural */
    if (cr_server.numClients > 0) {
         cr_server.curClient = cr_server.clients[0];
         cr_server.curClient->currentMural = defaultMural;
         cr_server.client_spu_id =cr_server.clients[0]->spu_id;
    }
}
