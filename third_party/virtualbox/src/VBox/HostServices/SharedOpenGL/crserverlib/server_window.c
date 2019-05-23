/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server.h"
#include "server_dispatch.h"
#include "cr_mem.h"
#include "cr_rand.h"
#include "cr_string.h"

#include "render/renderspu.h"

GLint SERVER_DISPATCH_APIENTRY
crServerDispatchWindowCreate(const char *dpyName, GLint visBits)
{
    return crServerDispatchWindowCreateEx(dpyName, visBits, -1);
}

GLint crServerMuralInit(CRMuralInfo *mural, GLboolean fGuestWindow, GLint visBits, GLint preloadWinID)
{
    CRMuralInfo *defaultMural;
    GLint dims[2];
    GLint windowID = -1;
    GLint spuWindow = 0;
    GLint realVisBits = visBits;
    const char *dpyName = "";

    crMemset(mural, 0, sizeof (*mural));

    if (cr_server.fVisualBitsDefault)
        realVisBits = cr_server.fVisualBitsDefault;

#ifdef RT_OS_DARWIN
    if (fGuestWindow)
    {
        CRMuralInfo *dummy = crServerGetDummyMural(realVisBits);
        if (!dummy)
        {
            WARN(("crServerGetDummyMural failed"));
            return -1;
        }
        spuWindow = dummy->spuWindow;
        mural->fIsDummyRefference = GL_TRUE;

        dims[0] = dummy->width;
        dims[1] = dummy->height;
    }
    else
#endif
    {
        /*
         * Have first SPU make a new window.
         */
        spuWindow = cr_server.head_spu->dispatch_table.WindowCreate( dpyName, realVisBits );
        if (spuWindow < 0) {
            return spuWindow;
        }
        mural->fIsDummyRefference = GL_FALSE;

        /* get initial window size */
        cr_server.head_spu->dispatch_table.GetChromiumParametervCR(GL_WINDOW_SIZE_CR, spuWindow, GL_INT, 2, dims);
    }

    defaultMural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, 0);
    CRASSERT(defaultMural);
    mural->gX = 0;
    mural->gY = 0;
    mural->width = dims[0];
    mural->height = dims[1];

    mural->spuWindow = spuWindow;
    mural->screenId = 0;
    mural->fHasParentWindow = !!cr_server.screen[0].winID;
    mural->bVisible = !cr_server.bWindowsInitiallyHidden;

    mural->cVisibleRects = 0;
    mural->pVisibleRects = NULL;
    mural->bReceivedRects = GL_FALSE;

    /* generate ID for this new window/mural (special-case for file conns) */
    if (cr_server.curClient && cr_server.curClient->conn->type == CR_FILE)
        windowID = spuWindow;
    else
        windowID = preloadWinID<0 ? (GLint)crHashtableAllocKeys( cr_server.muralTable, 1 ) : preloadWinID;

    mural->CreateInfo.realVisualBits = realVisBits;
    mural->CreateInfo.requestedVisualBits = visBits;
    mural->CreateInfo.externalID = windowID;
    mural->CreateInfo.pszDpyName = dpyName ? crStrdup(dpyName) : NULL;

    CR_STATE_SHAREDOBJ_USAGE_INIT(mural);

    return windowID;
}

GLint crServerDispatchWindowCreateEx(const char *dpyName, GLint visBits, GLint preloadWinID)
{
    CRMuralInfo *mural;
    GLint windowID = -1;

    NOREF(dpyName);

    if (cr_server.sharedWindows) {
        int pos, j;

        /* find empty position in my (curclient) windowList */
        for (pos = 0; pos < CR_MAX_WINDOWS; pos++) {
            if (cr_server.curClient->windowList[pos] == 0) {
                break;
            }
        }
        if (pos == CR_MAX_WINDOWS) {
            crWarning("Too many windows in crserver!");
            return -1;
        }

        /* Look if any other client has a window for this slot */
        for (j = 0; j < cr_server.numClients; j++) {
            if (cr_server.clients[j]->windowList[pos] != 0) {
                /* use that client's window */
                windowID = cr_server.clients[j]->windowList[pos];
                cr_server.curClient->windowList[pos] = windowID;
                crServerReturnValue( &windowID, sizeof(windowID) ); /* real return value */
                crDebug("CRServer: client %p sharing window %d",
                                cr_server.curClient, windowID);
                return windowID;
            }
        }
    }


    /*
     * Create a new mural for the new window.
     */
    mural = (CRMuralInfo *) crCalloc(sizeof(CRMuralInfo));
    if (!mural)
    {
        crWarning("crCalloc failed!");
        return -1;
    }

    windowID = crServerMuralInit(mural, GL_TRUE, visBits, preloadWinID);
    if (windowID < 0)
    {
        crWarning("crServerMuralInit failed!");
        crServerReturnValue( &windowID, sizeof(windowID) );
        crFree(mural);
        return windowID;
    }

    crHashtableAdd(cr_server.muralTable, windowID, mural);

    crDebug("CRServer: client %p created new window %d (SPU window %d)",
                    cr_server.curClient, windowID, mural->spuWindow);

    if (windowID != -1 && !cr_server.bIsInLoadingState) {
        int pos;
        for (pos = 0; pos < CR_MAX_WINDOWS; pos++) {
            if (cr_server.curClient->windowList[pos] == 0) {
                cr_server.curClient->windowList[pos] = windowID;
                break;
            }
        }
    }

    /* ensure we have a dummy mural created right away to avoid potential deadlocks on VM shutdown */
    crServerGetDummyMural(mural->CreateInfo.realVisualBits);

    crServerReturnValue( &windowID, sizeof(windowID) );
    return windowID;
}

static int crServerRemoveClientWindow(CRClient *pClient, GLint window)
{
    int pos;

    for (pos = 0; pos < CR_MAX_WINDOWS; ++pos)
    {
        if (pClient->windowList[pos] == window)
        {
            pClient->windowList[pos] = 0;
            return true;
        }
    }

    return false;
}

void crServerMuralTerm(CRMuralInfo *mural)
{
	PCR_BLITTER pBlitter;
    crServerRedirMuralFBO(mural, false);
    crServerDeleteMuralFBO(mural);

    if (cr_server.currentMural == mural)
    {
        CRMuralInfo *dummyMural = crServerGetDummyMural(cr_server.MainContextInfo.CreateInfo.realVisualBits);
        /* reset the current context to some dummy values to ensure render spu does not switch to a default "0" context,
         * which might lead to muralFBO (offscreen rendering) gl entities being created in a scope of that context */
        cr_server.head_spu->dispatch_table.MakeCurrent(dummyMural->spuWindow, 0, cr_server.MainContextInfo.SpuContext);
        cr_server.currentWindow = -1;
        cr_server.currentMural = dummyMural;
    }
    else
    {
        CRASSERT(cr_server.currentMural != mural);
    }

    pBlitter = crServerVBoxBlitterGetInitialized();
    if (pBlitter)
    {
    	const CR_BLITTER_WINDOW * pWindow = CrBltMuralGetCurrentInfo(pBlitter);
    	if (pWindow && pWindow->Base.id == mural->spuWindow)
    	{
    		CRMuralInfo *dummy = crServerGetDummyMural(mural->CreateInfo.realVisualBits);
    		CR_BLITTER_WINDOW DummyInfo;
    		CRASSERT(dummy);
    		crServerVBoxBlitterWinInit(&DummyInfo, dummy);
    		CrBltMuralSetCurrentInfo(pBlitter, &DummyInfo);
    	}
    }

    if (!mural->fIsDummyRefference)
        cr_server.head_spu->dispatch_table.WindowDestroy( mural->spuWindow );

    mural->spuWindow = 0;

    if (mural->pVisibleRects)
    {
        crFree(mural->pVisibleRects);
    }

    if (mural->CreateInfo.pszDpyName)
        crFree(mural->CreateInfo.pszDpyName);

    crServerRedirMuralFbClear(mural);
}

static void crServerCleanupCtxMuralRefsCB(unsigned long key, void *data1, void *data2)
{
    CRContextInfo *ctxInfo = (CRContextInfo *) data1;
    CRMuralInfo *mural = (CRMuralInfo *) data2;

    if (ctxInfo->currentMural == mural)
        ctxInfo->currentMural = NULL;
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchWindowDestroy( GLint window )
{
    CRMuralInfo *mural;
    int32_t client;
    CRClientNode *pNode;
    int found=false;

    if (!window)
    {
        crWarning("Unexpected attempt to delete default mural, ignored!");
        return;
    }

    mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
    if (!mural) {
         crWarning("CRServer: invalid window %d passed to WindowDestroy()", window);
         return;
    }

    crDebug("CRServer: Destroying window %d (spu window %d)", window, mural->spuWindow);

    crHashtableWalk(cr_server.contextTable, crServerCleanupCtxMuralRefsCB, mural);

    crServerMuralTerm(mural);

    CRASSERT(cr_server.currentWindow != window);

    if (cr_server.curClient)
    {
        if (cr_server.curClient->currentMural == mural)
        {
            cr_server.curClient->currentMural = NULL;
            cr_server.curClient->currentWindow = -1;
        }

        found = crServerRemoveClientWindow(cr_server.curClient, window);

        /*Same as with contexts, some apps destroy it not in a thread where it was created*/
        if (!found)
        {
            for (client=0; client<cr_server.numClients; ++client)
            {
                if (cr_server.clients[client]==cr_server.curClient)
                    continue;

                found = crServerRemoveClientWindow(cr_server.clients[client], window);

                if (found) break;
            }
        }

        if (!found)
        {
            pNode=cr_server.pCleanupClient;

            while (pNode && !found)
            {
                found = crServerRemoveClientWindow(pNode->pClient, window);
                pNode = pNode->next;
            }
        }

        CRASSERT(found);
    }

    /*Make sure this window isn't active in other clients*/
    for (client=0; client<cr_server.numClients; ++client)
    {
        if (cr_server.clients[client]->currentMural == mural)
        {
            cr_server.clients[client]->currentMural = NULL;
            cr_server.clients[client]->currentWindow = -1;
        }
    }

    pNode=cr_server.pCleanupClient;
    while (pNode)
    {
        if (pNode->pClient->currentMural == mural)
        {
            pNode->pClient->currentMural = NULL;
            pNode->pClient->currentWindow = -1;
        }
        pNode = pNode->next;
    }

    crHashtableDelete(cr_server.muralTable, window, crFree);

    crServerCheckAllMuralGeometry(NULL);
}

GLboolean crServerMuralSize(CRMuralInfo *mural, GLint width, GLint height)
{
    if (mural->width == width && mural->height == height)
        return GL_FALSE;

    mural->width = width;
    mural->height = height;

    if (cr_server.curClient && cr_server.curClient->currentMural == mural
            && !mural->fRedirected)
    {
        crStateGetCurrent()->buffer.width = mural->width;
        crStateGetCurrent()->buffer.height = mural->height;
    }

    crServerCheckAllMuralGeometry(mural);

    return GL_TRUE;
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchWindowSize( GLint window, GLint width, GLint height )
{
    CRMuralInfo *mural;

    /*  crDebug("CRServer: Window %d size %d x %d", window, width, height);*/
    mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
    if (!mural) {
#if EXTRA_WARN
         crWarning("CRServer: invalid window %d passed to WindowSize()", window);
#endif
         return;
    }

    crServerMuralSize(mural, width, height);

    if (cr_server.currentMural == mural)
    {
        crServerPerformMakeCurrent( mural, cr_server.currentCtxInfo );
    }
}

void crServerMuralPosition(CRMuralInfo *mural, GLint x, GLint y)
{
    if (mural->gX == x && mural->gY == y)
        return;

    mural->gX = x;
    mural->gY = y;

    crServerCheckAllMuralGeometry(mural);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchWindowPosition( GLint window, GLint x, GLint y )
{
    CRMuralInfo *mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
    if (!mural) {
#if EXTRA_WARN
         crWarning("CRServer: invalid window %d passed to WindowPosition()", window);
#endif
         return;
    }
    crServerMuralPosition(mural, x, y);
}

void crServerMuralVisibleRegion( CRMuralInfo *mural, GLint cRects, const GLint *pRects )
{
    if (mural->pVisibleRects)
    {
        crFree(mural->pVisibleRects);
        mural->pVisibleRects = NULL;
    }

    mural->cVisibleRects = cRects;
    mural->bReceivedRects = GL_TRUE;
    if (cRects)
    {
        mural->pVisibleRects = (GLint*) crAlloc(4*sizeof(GLint)*cRects);
        if (!mural->pVisibleRects)
        {
            crError("Out of memory in crServerDispatchWindowVisibleRegion");
        }
        crMemcpy(mural->pVisibleRects, pRects, 4*sizeof(GLint)*cRects);
    }

    crServerCheckAllMuralGeometry(mural);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchWindowVisibleRegion( GLint window, GLint cRects, const GLint *pRects )
{
    CRMuralInfo *mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
    if (!mural) {
#if EXTRA_WARN
         crWarning("CRServer: invalid window %d passed to WindowVisibleRegion()", window);
#endif
         return;
    }

    crServerMuralVisibleRegion( mural, cRects, pRects );
}

void crServerMuralShow( CRMuralInfo *mural, GLint state )
{
    if (!mural->bVisible == !state)
        return;

    mural->bVisible = !!state;

    if (mural->bVisible)
        crServerCheckMuralGeometry(mural);
    else
        crServerCheckAllMuralGeometry(mural);
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchWindowShow( GLint window, GLint state )
{
    CRMuralInfo *mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, window);
    if (!mural) {
#if EXTRA_WARN
         crWarning("CRServer: invalid window %d passed to WindowShow()", window);
#endif
         return;
    }

    crServerMuralShow( mural, state );
}

GLint
crServerSPUWindowID(GLint serverWindow)
{
    CRMuralInfo *mural = (CRMuralInfo *) crHashtableSearch(cr_server.muralTable, serverWindow);
    if (!mural) {
#if EXTRA_WARN
         crWarning("CRServer: invalid window %d passed to crServerSPUWindowID()",
                             serverWindow);
#endif
         return -1;
    }
    return mural->spuWindow;
}
