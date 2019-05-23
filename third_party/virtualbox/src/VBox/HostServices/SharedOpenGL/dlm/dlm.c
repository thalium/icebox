/* $Id: dlm.c $ */

#include <float.h>
#include "cr_dlm.h"
#include "cr_mem.h"
#include "dlm.h"

/**
 * \mainpage Dlm 
 *
 * \section DlmIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The dlm module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */

/**
 * Module globals: the current DLM state, bound either to each thread, or
 * to a global.
 */
#ifdef CHROMIUM_THREADSAFE
CRtsd CRDLMTSDKey;
#else
CRDLMContextState *CRDLMCurrentState = NULL;
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))


/*************************************************************************/

#ifdef CHROMIUM_THREADSAFE
/**
 * This is the thread-specific destructor function for the 
 * data used in the DLM.  It's very simple: if a thread exits
 * that has DLM-specific data, the data represents the listState
 * for the thread.  All data and buffers associated with the list
 * can be deleted, and the structure itself can be freed.
 *
 * Most Chromium threads don't have such things; but then,
 * if a thread dies elsewhere in Chromium, huge buffers
 * of information won't still be floating around in
 * unrecoverable allocated areas, either.
 */
static void threadDestructor(void *tsd)
{
    CRDLMContextState *listState = (CRDLMContextState *)tsd;

    if (listState)
    {
        //if (listState->currentListInfo)
        //    crdlm_free_list(listState->currentListInfo);

        crFree(listState);
    }
}
#endif

/**
 * This function creates and initializes a new display list
 * manager.  It returns a pointer to the manager, or NULL in
 * the case of insufficient memory.  The dispatch table pointer
 * is passed in to allow the utilities to muck with the table
 * to gain functional control when GL calls are made.
 */
CRDLM DLM_APIENTRY *crDLMNewDLM(unsigned int userConfigSize, const CRDLMConfig *userConfig)
{
    CRDLM *dlm;

    /* This is the default configuration.  We'll overwrite it later
     * with user-supplied configuration information.
     */
    CRDLMConfig config = {
	CRDLM_DEFAULT_BUFFERSIZE,
    };

    dlm = crAlloc(sizeof(*dlm));
    if (!dlm) {
	return NULL;
    }

    /* Start off by initializing all entries that require further
     * memory allocation, so we can free up all the memory if there's
     * a problem.
     */
    if (!(dlm->displayLists = crAllocHashtable())) {
	crFree(dlm);
	return NULL;
    }

    /* The creator counts as the first user. */
    dlm->userCount = 1;

#ifdef CHROMIUM_THREADSAFE
    /* This mutex ensures that only one thread is changing the displayLists
     * hash at a time.  Note that we may also need a mutex to guarantee that
     * the hash is not changed by one thread while another thread is
     * traversing it; this issue has not yet been resolved.
     */
    crInitMutex(&(dlm->dlMutex));

    /* Although the thread-specific data (TSD) functions will initialize
     * the thread key themselves when needed, those functions do not allow
     * us to specify a thread destructor.  Since a thread could potentially
     * exit with considerable memory allocated (e.g. if a thread exits 
     * after it has issued NewList but before EndList, and while there
     * are considerable content buffers allocated), I do the initialization
     * myself, in order to be able to reclaim those resources if a thread
     * exits.
     */
    crInitTSDF(&(dlm->tsdKey), threadDestructor);
    crInitTSD(&CRDLMTSDKey);
#endif

    /* Copy over any appropriate configuration values */
    if (userConfig != NULL) {
	/* Copy over as much configuration information as is provided.
	 * Note that if the CRDLMConfig structure strictly grows, this
	 * allows forward compatability - routines compiled with
	 * older versions of the structure will only initialize that
	 * section of the structure that they know about.
	 */
	crMemcpy((void *)&config, (void *) userConfig, 
		MIN(userConfigSize, sizeof(config)));
    }
    dlm->bufferSize = config.bufferSize;

    /* Return the pointer to the newly-allocated display list manager */
    return dlm;
}

void DLM_APIENTRY crDLMUseDLM(CRDLM *dlm)
{
    DLM_LOCK(dlm);
    dlm->userCount++;
    DLM_UNLOCK(dlm);
}

/**
 * This routine is called when a context or thread is done with a DLM.
 * It maintains an internal count of users, and will only actually destroy
 * itself when no one is still using the DLM.
 */
void DLM_APIENTRY crDLMFreeDLM(CRDLM *dlm, SPUDispatchTable *dispatchTable)
{
    /* We're about to change the displayLists hash; lock it first */
    DLM_LOCK(dlm)

    /* Decrement the user count.  If the user count has gone to
     * 0, then free the rest of the DLM.  Otherwise, other
     * contexts or threads are still using this DLM; keep
     * it around.
     */
    dlm->userCount--;
    if (dlm->userCount == 0) {

	crFreeHashtableEx(dlm->displayLists, crdlmFreeDisplayListResourcesCb, dispatchTable);
	dlm->displayLists = NULL;

	/* Must unlock before freeing the mutex */
	DLM_UNLOCK(dlm)

#ifdef CHROMIUM_THREADSAFE
	/* We release the mutex here; we really should delete the
	 * thread data key, but there's no utility in Chromium to
	 * do this.
	 *
	 * Note that, should one thread release the entire DLM
	 * while other threads still believe they are using it,
	 * any other threads that have current display lists (i.e.
	 * have issued glNewList more recently than glEndList)
	 * will be unable to reclaim their (likely very large)
	 * content buffers, as there will be no way to reclaim
	 * the thread-specific data.
	 *
	 * On the other hand, if one thread really does release
	 * the DLM while other threads still believe they are 
	 * using it, unreclaimed memory is the least of the
	 * application's problems...
	 */
	crFreeMutex(&(dlm->dlMutex));

	/* We free the TSD key here as well.  Note that this will
	 * strand any threads that still have thread-specific data
	 * tied to this key; but as stated above, if any threads
	 * still do have thread-specific data attached to this DLM,
	 * they're in big trouble anyway.
	 */
	crFreeTSD(&(dlm->tsdKey));
	crFreeTSD(&CRDLMTSDKey);
#endif

	/* Free the master record, and we're all done. */
	crFree(dlm);
    }
    else {
	/* We're keeping the DLM around for other users.  Unlock it,
	 * but retain its memory and display lists.
	 */
	DLM_UNLOCK(dlm)
    }
}

/**
 * The actual run-time state of a DLM is bound to a context
 * (because each context can be used by at most one thread at
 * a time, and a thread can only use one context at a time,
 * while multiple contexts can use the same DLM).
 * This creates the structure required to hold the state, and
 * returns it to the caller, who should store it with any other
 * context-specific information.
 */

CRDLMContextState DLM_APIENTRY *crDLMNewContext(CRDLM *dlm)
{
	CRDLMContextState *state;

	/* Get a record for our own internal state structure */
	state = (CRDLMContextState *)crAlloc(sizeof(CRDLMContextState));
	if (!state) {
		return NULL;
	}

	state->dlm = dlm;
	state->currentListIdentifier = 0;
	state->currentListInfo = NULL;
	state->currentListMode = GL_FALSE;
	state->listBase = 0;
	
	/* Increment the use count of the DLM provided.  This guarantees that
	 * the DLM won't be released until all the contexts have released it.
	 */
	crDLMUseDLM(dlm);

	return state;
}


/**
 * This routine should be called when a MakeCurrent changes the current
 * context.  It sets the thread data (or global data, in an unthreaded
 * environment) appropriately; this in turn changes the behavior of
 * the installed DLM API functions.
 */
void DLM_APIENTRY crDLMSetCurrentState(CRDLMContextState *state)
{
	CRDLMContextState *currentState = CURRENT_STATE();
	if (currentState != state) {
		SET_CURRENT_STATE(state);
	}
}

CRDLMContextState DLM_APIENTRY *crDLMGetCurrentState(void)
{
	return CURRENT_STATE();
}

/**
 * This routine, of course, is used to release a DLM context when it
 * is no longer going to be used.
 */

void DLM_APIENTRY crDLMFreeContext(CRDLMContextState *state, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();

    /* If we're currently using this context, release it first */
    if (listState == state)
        crDLMSetCurrentState(NULL);

    /* Try to free the DLM.  This will either decrement the use count,
     * or will actually free the DLM, if we were the last user.
     */
    crDLMFreeDLM(state->dlm, dispatchTable);
    state->dlm = NULL;

    /* If any buffers still remain (e.g. because there was an open
     * display list), remove those as well.
     */
    if (state->currentListInfo)
    {
        crdlmFreeDisplayListResourcesCb((void *)state->currentListInfo, (void *)dispatchTable);
        state->currentListInfo = NULL;
    }
    state->currentListIdentifier = 0;

    /* Free the state record itself */
    crFree(state);
}


/**
 * This function can be used if the caller wishes to free up the
 * potentially considerable resources used to store the display list
 * content, without losing the rest of the display list management.
 * For one example, consider an SPU that conditionally sends its 
 * input stream to multiple servers.  It could broadcast all display
 * lists to all servers, or it could only send display lists to servers
 * that need them.  After all servers have the display list, the SPU
 * may wish to release the resources used to manage the content.
 */
CRDLMError DLM_APIENTRY crDLMDeleteListContent(CRDLM *dlm, unsigned long listIdentifier)
{
    DLMListInfo *listInfo;
    DLMInstanceList *instance;

    listInfo = (DLMListInfo *) crHashtableSearch(dlm->displayLists, listIdentifier);
    if (listInfo && (instance = listInfo->first)) {
	while (instance) {
	    DLMInstanceList *nextInstance;
	    nextInstance = instance->next;
	    crFree(instance);
	    instance = nextInstance;
	}
	listInfo->first = listInfo->last = NULL;
    }
    return GL_NO_ERROR;
}

/**
 *
 * Playback/execute a list.
 * dlm - the display list manager context
 * listIdentifier - the display list ID (as specified by app) to playback
 * dispatchTable - the GL dispatch table to jump through as we execute commands
 */
void DLM_APIENTRY crDLMReplayDLMList(CRDLM *dlm, unsigned long listIdentifier, SPUDispatchTable *dispatchTable)
{
    DLMListInfo *listInfo;

    listInfo = (DLMListInfo *)crHashtableSearch(dlm->displayLists, listIdentifier);
    if (listInfo) {
	DLMInstanceList *instance = listInfo->first;
	while (instance) {
	    /* mutex, to make sure another thread doesn't change the list? */
	    /* For now, leave it alone. */
	    (*instance->execute)(instance, dispatchTable);
	    instance = instance->next;
	}
    }
}

/* Playback/execute a list in the current DLM */
void DLM_APIENTRY crDLMReplayList(unsigned long listIdentifier, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState)
        crDLMReplayDLMList(listState->dlm, listIdentifier, dispatchTable);
}

/*
 * Playback/execute the state changing portions of a list.
 * dlm - the display list manager context
 * listIdentifier - the display list ID (as specified by app) to playback
 * dispatchTable - the GL dispatch table to jump through as we execute commands
 */
void DLM_APIENTRY crDLMReplayDLMListState(CRDLM *dlm, unsigned long listIdentifier, SPUDispatchTable *dispatchTable)
{
    DLMListInfo *listInfo;

    listInfo = (DLMListInfo *)crHashtableSearch(dlm->displayLists, listIdentifier);
    if (listInfo) {
	DLMInstanceList *instance = listInfo->stateFirst;
	while (instance) {
	    /* mutex, to make sure another thread doesn't change the list? */
	    /* For now, leave it alone. */
	    (*instance->execute)(instance, dispatchTable);
	    instance = instance->stateNext;
	}
    }
}

void DLM_APIENTRY crDLMReplayListState(unsigned long listIdentifier, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState)
        crDLMReplayDLMListState(listState->dlm, listIdentifier, dispatchTable);
}

/* This is a switch statement that lists every "type" value valid for a
 * glCallLists() function call, with code for decoding the subsequent
 * values correctly.  It uses the current value of the EXPAND() macro,
 * which must expand into an appropriate action to be taken.
 * Its codification here allows for multiple uses.
 */
#define CALL_LISTS_SWITCH(type, defaultAction) \
    switch (type) {\
	EXPAND(GL_BYTE, GLbyte *, *p, p++)\
	EXPAND(GL_UNSIGNED_BYTE, GLubyte *, *p, p++)\
	EXPAND(GL_SHORT, GLshort *, *p, p++)\
	EXPAND(GL_UNSIGNED_SHORT, GLushort *, *p, p++)\
	EXPAND(GL_INT, GLint *, *p, p++)\
	EXPAND(GL_FLOAT, GLfloat *, *p, p++)\
	EXPAND(GL_2_BYTES, unsigned char *, 256*p[0] + p[1], p += 2)\
	EXPAND(GL_3_BYTES, unsigned char *, 65536*p[0] + 256*p[1] + p[2], p += 3)\
	EXPAND(GL_4_BYTES, unsigned char *, 16777216*p[0] + 65536*p[1] + 256*p[2] + p[3], p += 4)\
	default:\
	    defaultAction;\
    }

void DLM_APIENTRY crDLMReplayDLMLists(CRDLM *dlm, GLsizei n, GLenum type, const GLvoid * lists, SPUDispatchTable *dispatchTable)
{
    unsigned long listId;
    CRDLMContextState *listState = CURRENT_STATE();

#define EXPAND(TYPENAME, TYPE, REFERENCE, INCREMENT) \
    case TYPENAME: {\
	TYPE p = (TYPE)lists;\
	while (n--) {\
	    listId = listState->listBase + (unsigned long) (REFERENCE);\
	    crDLMReplayDLMList(dlm, listId, dispatchTable);\
	    INCREMENT;\
	}\
	break;\
    }

    CALL_LISTS_SWITCH(type, break)
#undef EXPAND

}

void DLM_APIENTRY crDLMReplayLists(GLsizei n, GLenum type, const GLvoid * lists, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState) {
	crDLMReplayDLMLists(listState->dlm, n, type, lists, dispatchTable);
    }
}

void DLM_APIENTRY crDLMReplayDLMListsState(CRDLM *dlm, GLsizei n, GLenum type, const GLvoid * lists, SPUDispatchTable *dispatchTable)
{
    unsigned long listId;
    CRDLMContextState *listState = CURRENT_STATE();

#define EXPAND(TYPENAME, TYPE, REFERENCE, INCREMENT) \
    case TYPENAME: {\
	TYPE p = (TYPE)lists;\
	while (n--) {\
	    listId = listState->listBase + (unsigned long) (REFERENCE);\
	    crDLMReplayDLMListState(dlm, listId, dispatchTable);\
	    INCREMENT;\
	}\
	break;\
    }

    CALL_LISTS_SWITCH(type, break)
#undef EXPAND

}

void DLM_APIENTRY crDLMReplayListsState(GLsizei n, GLenum type, const GLvoid * lists, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState) {
	crDLMReplayDLMListsState(listState->dlm, n, type, lists, dispatchTable);
    }
}

/* When we compiled the display list, we packed all pixel data
 * tightly.  When we execute the display list, we have to make
 * sure that the client state reflects that the pixel data is
 * tightly packed, or it will be interpreted incorrectly.
 */
void DLM_APIENTRY crDLMSetupClientState(SPUDispatchTable *dispatchTable)
{
    dispatchTable->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    dispatchTable->PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    dispatchTable->PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    dispatchTable->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void DLM_APIENTRY crDLMRestoreClientState(CRClientState *clientState, SPUDispatchTable *dispatchTable)
{
    if (clientState) {
	dispatchTable->PixelStorei(GL_UNPACK_ROW_LENGTH, clientState->unpack.rowLength);
	dispatchTable->PixelStorei(GL_UNPACK_SKIP_PIXELS, clientState->unpack.skipPixels);
	dispatchTable->PixelStorei(GL_UNPACK_SKIP_ROWS, clientState->unpack.skipRows);
	dispatchTable->PixelStorei(GL_UNPACK_ALIGNMENT, clientState->unpack.alignment);
    }
}

void DLM_APIENTRY crDLMSendDLMList(CRDLM *dlm, unsigned long listIdentifier,
	SPUDispatchTable *dispatchTable)
{
    dispatchTable->NewList(listIdentifier, GL_COMPILE);
    crDLMReplayDLMList(dlm, listIdentifier, dispatchTable);
    dispatchTable->EndList();
}

void DLM_APIENTRY crDLMSendList(unsigned long listIdentifier, SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState) {
	crDLMSendDLMList(listState->dlm, listIdentifier, dispatchTable);
    }
}

struct sendListsCallbackParms {
    CRDLM *dlm;
    SPUDispatchTable *dispatchTable;
};

static void sendListsCallback(unsigned long key, void *data, void *dataPtr2)
{
    struct sendListsCallbackParms *parms = (struct sendListsCallbackParms *)dataPtr2;

    crDLMSendDLMList(parms->dlm, key, parms->dispatchTable);
}

void DLM_APIENTRY crDLMSendAllDLMLists(CRDLM *dlm, SPUDispatchTable *dispatchTable)
{
    struct sendListsCallbackParms parms;

    /* This is how we pass our parameter information to the callback routine -
     * through a pointer to this local structure.
     */
    parms.dlm = dlm;
    parms.dispatchTable = dispatchTable;

    crHashtableWalk(dlm->displayLists, sendListsCallback, (void *)&parms);
}

void DLM_APIENTRY crDLMSendAllLists(SPUDispatchTable *dispatchTable)
{
    CRDLMContextState *listState = CURRENT_STATE();
    if (listState) {
	crDLMSendAllDLMLists(listState->dlm, dispatchTable);
    }
}

/** Another clever callback arrangement to get the desired data. */
struct getRefsCallbackParms {
    int remainingOffset;
    int remainingCount;
    unsigned int *buffer;
    int totalCount;
};

/*
 * Return id of list currently being compiled.  Returns 0 of there's no
 * current DLM state, or if no list is being compiled. 
 */
GLuint DLM_APIENTRY crDLMGetCurrentList(void)
{
	CRDLMContextState *listState = CURRENT_STATE();
	return listState ? listState->currentListIdentifier : 0;
}

/*
 * Return mode of list currently being compiled.  Should be 
 * GL_FALSE if no list is being compiled, or GL_COMPILE if a
 * list is being compiled but not executed, or GL_COMPILE_AND_EXECUTE
 * if a list is being compiled and executed.
 */
GLenum DLM_APIENTRY crDLMGetCurrentMode(void)
{
	CRDLMContextState *listState = CURRENT_STATE();
	return listState ? listState->currentListMode : 0;
}


static CRDLMErrorCallback ErrorCallback = NULL;

void DLM_APIENTRY crDLMErrorFunction(CRDLMErrorCallback callback)
{
	ErrorCallback = callback;
}

void crdlm_error(int line, const char *file, GLenum error, const char *info)
{
	if (ErrorCallback)
		(*ErrorCallback)(line, file, error, info);
}
