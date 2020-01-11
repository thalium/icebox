/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_string.h"
#include "cr_dll.h"
#include "cr_error.h"
#include "cr_spu.h"


#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/path.h>

#include <stdio.h>

#ifdef WINDOWS
#ifdef VBOX_WDDM_WOW64
#define DLL_SUFFIX "-x86.dll"
#else
#define DLL_SUFFIX ".dll"
#endif
#define DLL_PREFIX "VBoxOGL"
#define snprintf _snprintf
#elif defined(DARWIN)
#define DLL_SUFFIX ".dylib"
#define DLL_PREFIX "VBoxOGL"
/*
#define DLL_SUFFIX ".bundle"
#define DLL_PREFIX ""
*/
#else
#ifdef AIX
#define DLL_SUFFIX ".o"
#define DLL_PREFIX "VBoxOGL"
#else
#define DLL_SUFFIX ".so"
#define DLL_PREFIX "VBoxOGL"
#endif
#endif

extern void __buildDispatch( SPU *spu );

static char *__findDLL( char *name, char *dir )
{
        static char path[8092];

        if (!dir)
        {
#if defined(DARWIN)
            char szSharedLibPath[8092];
            int rc = RTPathAppPrivateArch (szSharedLibPath, sizeof(szSharedLibPath));
            if (RT_SUCCESS(rc))
                sprintf ( path, "%s/%s%sspu%s", szSharedLibPath, DLL_PREFIX, name, DLL_SUFFIX );
            else
#endif /* DARWIN */
#ifdef VBOX
                snprintf ( path, sizeof(path), "%s%sspu%s", DLL_PREFIX, name, DLL_SUFFIX );
#else
                sprintf ( path, "%s%sspu%s", DLL_PREFIX, name, DLL_SUFFIX );
#endif
        }
        else
        {
#ifdef VBOX
                snprintf ( path, sizeof(path), "%s/%s%sspu%s", dir, DLL_PREFIX, name, DLL_SUFFIX );
#else
                sprintf ( path, "%s/%s%sspu%s", dir, DLL_PREFIX, name, DLL_SUFFIX );
#endif
        }
        return path;
}

/**
 *  Load a single SPU from disk and initialize it.  Is there any reason
 * to export this from the SPU loader library? */

SPU * crSPULoad( SPU *child, int id, char *name, char *dir, void *server )
{
        SPU *the_spu;
        char *path;
        bool fNeedSuperSPU = false;

        CRASSERT( name != NULL );

        the_spu = (SPU*)crAlloc( sizeof( *the_spu ) );
        /* ensure all fields are initially zero,
         * NOTE: what actually MUST be zero at this point is the_spu->superSPU, otherwise
         * crSPUUnloadChain in the failure branches below will misbehave */
        crMemset(the_spu, 0, sizeof (*the_spu));
        the_spu->id = id;
        the_spu->privatePtr = NULL;
        path = __findDLL( name, dir );
        the_spu->dll = crDLLOpen( path, 0/*resolveGlobal*/ );
        if (the_spu->dll == NULL)
        {
                crError("Couldn't load the DLL \"%s\"!\n", path);
                crFree(the_spu);
                return NULL;
        }
#if defined(DEBUG_misha) && defined(RT_OS_WINDOWS)
        crDbgCmdSymLoadPrint(path, the_spu->dll->hinstLib);
#endif
        the_spu->entry_point =
                (SPULoadFunction) crDLLGetNoError( the_spu->dll, SPU_ENTRY_POINT_NAME );
        if (!the_spu->entry_point)
        {
                crError( "Couldn't load the SPU entry point \"%s\" from SPU \"%s\"!",
                                SPU_ENTRY_POINT_NAME, name );
                crSPUUnloadChain(the_spu);
                return NULL;
        }

        /* This basically calls the SPU's SPULoad() function */
        if (!the_spu->entry_point( &(the_spu->name), &(the_spu->super_name),
                                   &(the_spu->init), &(the_spu->self),
                                   &(the_spu->cleanup),
                                   &(the_spu->spu_flags)) )
        {
                crError( "I found the SPU \"%s\", but loading it failed!", name );
                crSPUUnloadChain(the_spu);
                return NULL;
        }
#ifdef IN_GUEST
        if (crStrcmp(the_spu->name,"error"))
        {
                /* the default super/base class for an SPU is the error SPU */
                if (the_spu->super_name == NULL)
                {
                        the_spu->super_name = "error";
                }
                the_spu->superSPU = crSPULoad( child, id, the_spu->super_name, dir, server );
                fNeedSuperSPU = true;
        }
#else
        if (crStrcmp(the_spu->name,"hosterror"))
        {
                /* the default super/base class for an SPU is the error SPU */
                if (the_spu->super_name == NULL)
                {
                        the_spu->super_name = "hosterror";
                }
                the_spu->superSPU = crSPULoad( child, id, the_spu->super_name, dir, server );
                fNeedSuperSPU = true;
        }
#endif
        else
        {
                the_spu->superSPU = NULL;
        }
        if (fNeedSuperSPU && !the_spu->superSPU)
        {
                crError( "Unable to load super SPU \"%s\" of \"%s\"!", the_spu->super_name, name );
                crSPUUnloadChain(the_spu);
                return NULL;
        }
        crDebug("Initializing %s SPU", name);
        the_spu->function_table = the_spu->init( id, child, the_spu, 0, 1 );
        if (!the_spu->function_table) {
                crDebug("Failed to init %s SPU", name);
                crSPUUnloadChain(the_spu);
                return NULL;
        }
        __buildDispatch( the_spu );
        /*crDebug( "initializing dispatch table %p (for SPU %s)", (void*)&(the_spu->dispatch_table), name );*/
        crSPUInitDispatchTable( &(the_spu->dispatch_table) );
        /*crDebug( "Done initializing the dispatch table for SPU %s, calling the self function", name );*/

        the_spu->dispatch_table.server = server;
        the_spu->self( &(the_spu->dispatch_table) );
        /*crDebug( "Done with the self function" );*/

        return the_spu;
}

/**
 *  Load the entire chain of SPUs and initialize all of them.
 * This function returns the first one in the chain.
 */
SPU *
crSPULoadChain( int count, int *ids, char **names, char *dir, void *server )
{
        int i;
        SPU *child_spu = NULL;
        CRASSERT( count > 0 );

        for (i = count-1 ; i >= 0 ; i--)
        {
                int spu_id = ids[i];
                char *spu_name = names[i];
                SPU *the_spu, *temp;

                /* This call passes the previous version of spu, which is the SPU's
                 * "child" in this chain. */

                the_spu = crSPULoad( child_spu, spu_id, spu_name, dir, server );
                if (!the_spu) {
                        return NULL;
                }

                if (child_spu != NULL)
                {
                        /* keep track of this so that people can pass functions through but
                         * still get updated when API's change on the fly. */
                        for (temp = the_spu ; temp ; temp = temp->superSPU )
                        {
                                struct _copy_list_node *node = (struct _copy_list_node *) crAlloc( sizeof( *node ) );
                                node->copy = &(temp->dispatch_table);
                                node->next = child_spu->dispatch_table.copyList;
                                child_spu->dispatch_table.copyList = node;
                        }
                }
                child_spu = the_spu;
        }
        return child_spu;
}


/**
 * Returns the SPU registration record for the given name.
 *
 * @returns Pointer to the SPU registration record on success or NULL if not found.
 * @param   pszName             The name to look for.
 * @param   papSpuReg           Pointer to the NULL terminated array of builtin SPU registration record pointers.
 */
static PCSPUREG crSPUGetRegFromName(const char *pszName, PCSPUREG *papSpuReg)
{
    while (*papSpuReg)
    {
        if (!RTStrCmp(pszName, (*papSpuReg)->pszName))
            return *papSpuReg;
        papSpuReg++;
    }

    return NULL;
}


/**
 * Creates a SPU chain from the given SPU registration structure (VBox only).
 *
 * @returns Pointer to the SPU head in the chain on success.
 * @param   pSpuChild           Pointer to the child SPU if any.
 * @param   iId                 ID to assign to the head SPU.
 * @param   pszName             Name of the SPU to initialize.
 * @param   pvServer            The server owning the SPU chain.
 * @param   papSpuReg           Pointer to the NULL terminated array of builtin SPU registration record pointers.
 */
SPU * crSPUInitFromReg(SPU *pSpuChild, int iId, const char *pszName, void *pvServer, PCSPUREG *papSpuReg)
{
    SPU *pSpu;
    bool fNeedSuperSPU = false;
    PCSPUREG pSpuReg = crSPUGetRegFromName(pszName, papSpuReg);
    AssertPtrReturn(pSpuReg, NULL);

    pSpu = (SPU*)crAlloc(sizeof(*pSpu));
    crMemset(pSpu, 0, sizeof (*pSpu));
    pSpu->id = iId;
    pSpu->privatePtr = NULL;

    /* Init the SPU structure from the SPU registration record. */
    pSpu->name       = (char *)pSpuReg->pszName;
    pSpu->super_name = (char *)pSpuReg->pszSuperName;
    pSpu->init       = pSpuReg->pfnInit;
    pSpu->self       = pSpuReg->pfnDispatch;
    pSpu->cleanup    = pSpuReg->pfnCleanup;
    pSpu->spu_flags  = pSpuReg->fFlags;

#ifdef IN_GUEST
    if (crStrcmp(pSpu->name,"error"))
    {
            /* the default super/base class for an SPU is the error SPU */
            if (pSpu->super_name == NULL)
                pSpu->super_name = "error";

            pSpu->superSPU = crSPUInitFromReg(pSpuChild, iId, pSpu->super_name, pvServer, papSpuReg);
            fNeedSuperSPU = true;
    }
#else
    if (crStrcmp(pSpu->name,"hosterror"))
    {
            /* the default super/base class for an SPU is the error SPU */
            if (pSpu->super_name == NULL)
                pSpu->super_name = "hosterror";

            pSpu->superSPU = crSPUInitFromReg(pSpuChild, iId, pSpu->super_name, pvServer, papSpuReg);
            fNeedSuperSPU = true;
    }
#endif
    else
        pSpu->superSPU = NULL;

    if (fNeedSuperSPU && !pSpu->superSPU)
    {
            crError( "Unable to load super SPU \"%s\" of \"%s\"!", pSpu->super_name, pSpuReg->pszName);
            crSPUUnloadChain(pSpu);
            return NULL;
    }
    crDebug("Initializing %s SPU", pSpuReg->pszName);
    pSpu->function_table = pSpu->init(iId, pSpuChild, pSpu, 0, 1);
    if (!pSpu->function_table) {
            crDebug("Failed to init %s SPU", pSpuReg->pszName);
            crSPUUnloadChain(pSpu);
            return NULL;
    }
    __buildDispatch( pSpu );
    /*crDebug( "initializing dispatch table %p (for SPU %s)", (void*)&(pSpu->dispatch_table), name );*/
    crSPUInitDispatchTable( &(pSpu->dispatch_table) );
    /*crDebug( "Done initializing the dispatch table for SPU %s, calling the self function", name );*/

    pSpu->dispatch_table.server = pvServer;
    pSpu->self( &(pSpu->dispatch_table) );
    /*crDebug( "Done with the self function" );*/

    return pSpu;
}


/**
 * Initializes a give nchain of SPUs from the builtin SPU registration descriptors (VBox only).
 *
 * @returns Pointer to the Head SPU on success or NULL on failure.
 * @param   cSpus               The number of SPUs to initialize.
 * @param   paIds               Pointer to the array of IDs.
 * @param   papszNames          Pointer to the array of SPU names to initalize.
 * @param   pvServer            The server owning the SPU chain.
 * @param   papSpuReg           Pointer to the NULL terminated array of builtin SPU registration record pointers.
 */
SPU *crSPUInitChainFromReg(int cSpus, int *paIds, const char * const *papszNames, void *pvServer, PCSPUREG *papSpuReg)
{
    int i = 0;
    SPU *pSpuChild = NULL;
    CRASSERT( cSpus > 0 );

    for (i = cSpus - 1; i >= 0; i--)
    {
        int idSpu = paIds[i];
        const char *pszSpuName = papszNames[i];

        /*
         * This call passes the previous version of spu, which is the SPU's
         * "child" in this chain.
         */
        SPU *pSpu = crSPUInitFromReg( pSpuChild, idSpu, pszSpuName, pvServer, papSpuReg);
        if (!pSpu)
            return NULL; /** @todo Proper rollback. */

        if (pSpuChild != NULL)
        {
            SPU *pTmp;

            /* keep track of this so that people can pass functions through but
             * still get updated when API's change on the fly. */
            for (pTmp = pSpu ; pTmp ; pTmp = pTmp->superSPU )
            {
                struct _copy_list_node *node = (struct _copy_list_node *) crAlloc( sizeof( *node ) );
                node->copy = &(pTmp->dispatch_table);
                node->next = pSpuChild->dispatch_table.copyList;
                pSpuChild->dispatch_table.copyList = node;
            }
        }
        pSpuChild = pSpu;
    }

    return pSpuChild;
}


#if 00
/* XXXX experimental code - not used at this time */
/**
 * Like crSPUChangeInterface(), but don't loop over all functions in
 * the table to search for 'old_func'.
 */
void
crSPUChangeFunction(SPUDispatchTable *table, unsigned int funcOffset,
                    void *newFunc)
{
        SPUGenericFunction *f = (SPUGenericFunction *) table + funcOffset;
        struct _copy_list_node *temp;

        CRASSERT(funcOffset < sizeof(*table) / sizeof(SPUGenericFunction));

        printf("%s\n", __FUNCTION__);
        if (table->mark == 1)
                return;
        table->mark = 1;
        *f = newFunc;

        /* update all copies of this table */
#if 1
        for (temp = table->copyList ; temp ; temp = temp->next)
        {
                crSPUChangeFunction( temp->copy, funcOffset, newFunc );
        }
#endif
        if (table->copy_of != NULL)
        {
                crSPUChangeFunction( table->copy_of, funcOffset, newFunc );
        }
#if 0
        for (temp = table->copyList ; temp ; temp = temp->next)
        {
                crSPUChangeFunction( temp->copy, funcOffset, newFunc );
        }
#endif
        table->mark = 0;
}
#endif



/**
 * Call the cleanup() function for each SPU in a chain, close the SPU
 * DLLs and free the SPU objects.
 * \param headSPU  pointer to the first SPU in the chain
 */
void
crSPUUnloadChain(SPU *headSPU)
{
        SPU *the_spu = headSPU, *next_spu;

        while (the_spu)
        {
                crDebug("Cleaning up SPU %s", the_spu->name);

                if (the_spu->cleanup)
                        the_spu->cleanup();

                next_spu = the_spu->superSPU;
                if (the_spu->dll != NULL)
                    crDLLClose(the_spu->dll);
                crFree(the_spu);
                the_spu = next_spu;
        }
}
