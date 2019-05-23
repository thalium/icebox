/* $Id: kLdrInternal.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, internal header.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___kLdrInternal_h___
#define ___kLdrInternal_h___

#include <k/kHlp.h>
#include <k/kRdr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__X86__) && !defined(__AMD64__)
# if defined(__i386__) || defined(_M_IX86)
#  define __X86__
# elif defined(__x86_64__) || defined(_M_X64) || defined(__AMD64__) || defined(_M_AMD64)
#  define __AMD64__
# else
#  error "can't figure out the target arch."
# endif
#endif

/* ignore definitions in winnt.h */
#undef IMAGE_DOS_SIGNATURE
#undef IMAGE_NT_SIGNATURE

/** @name Signatures we know
 * @{ */
/** ELF signature ("\x7fELF"). */
#define IMAGE_ELF_SIGNATURE         K_LE2H_U32(0x7f | ('E' << 8) | ((KU32)'L' << 16) | ((KU32)'F' << 24))
/** PE signature ("PE\0\0"). */
#define IMAGE_NT_SIGNATURE          K_LE2H_U32('P' | ('E' << 8))
/** LX signature ("LX") */
#define IMAGE_LX_SIGNATURE          K_LE2H_U16('L' | ('X' << 8))
/** LE signature ("LE") */
#define IMAGE_LE_SIGNATURE          K_LE2H_U16('L' | ('E' << 8))
/** NE signature ("NE") */
#define IMAGE_NE_SIGNATURE          K_LE2H_U16('N' | ('E' << 8))
/** MZ signature ("MZ"). */
#define IMAGE_DOS_SIGNATURE         K_LE2H_U16('M' | ('Z' << 8))
/** The FAT signature (universal binaries). */
#define IMAGE_FAT_SIGNATURE         KU32_C(0xcafebabe)
/** The FAT signature (universal binaries), other endian. */
#define IMAGE_FAT_SIGNATURE_OE      KU32_C(0xbebafeca)
/** The 32-bit Mach-O signature. */
#define IMAGE_MACHO32_SIGNATURE     KU32_C(0xfeedface)
/** The 32-bit Mach-O signature, other endian. */
#define IMAGE_MACHO32_SIGNATURE_OE  KU32_C(0xcefaedfe)
/** The 64-bit Mach-O signature. */
#define IMAGE_MACHO64_SIGNATURE     KU32_C(0xfeedfacf)
/** The 64-bit Mach-O signature, other endian. */
#define IMAGE_MACHO64_SIGNATURE_OE  KU32_C(0xfefaedfe)
/** @} */

/** @defgroup grp_kLdrInternal  Internals
 * @internal
 * @{
 */


/**
 * The state of a dynamic loader module.
 * @image html KLDRSTATE.gif "The state diagram"
 */
typedef enum KLDRSTATE
{
    /** The usual invalid 0 enum. */
    KLDRSTATE_INVALID = 0,

    /** The module has just been opened and linked into the load list.
     *
     * Prev state: -
     * Next state: MAPPED, PENDING_DESTROY
     */
    KLDRSTATE_OPEN,

    /** The module segments has been mapped into the process memory.
     *
     * Prev state: OPEN
     * Next state: LOADED_PREREQUISITES, PENDING_DESTROY
     */
    KLDRSTATE_MAPPED,
    /** The module has been reloaded and needs to be fixed up again.
     * This can occure when the loader is called recursivly.
     *
     * The reason RELOADED modules must go back to the PENDING_GC state is
     * because we want to guard against uninit order issues, and therefore
     * doesn't unmap modules untill all pending termintation callbacks has
     * been executed.
     *
     * Prev state: PENDING_GC
     * Next state: RELOADED_LOADED_PREREQUISITES, PENDING_GC
     */
    KLDRSTATE_RELOADED,

    /** The immediate prerequisites have been loaded.
     *
     * Prev state: MAPPED
     * Next state: FIXED_UP, PENDING_DESTROY
     */
    KLDRSTATE_LOADED_PREREQUISITES,
    /** The immediate prerequisites have been loaded for a reloaded module.
     *
     * Prev state: RELOADED
     * Next state: RELOADED_FIXED_UP, PENDING_GC
     */
    KLDRSTATE_RELOADED_LOADED_PREREQUISITES,

    /** Fixups has been applied.
     *
     * Prev state: LOADED_PREREQUISITES
     * Next state: PENDING_INITIALIZATION, PENDING_DESTROY
     */
    KLDRSTATE_FIXED_UP,
    /** Fixups has been applied.
     *
     * Prev state: RELOADED_LOADED_PREREQUISITES
     * Next state: PENDING_INITIALIZATION, PENDING_GC
     */
    KLDRSTATE_RELOADED_FIXED_UP,

    /** Pending initialization.
     * While the module is in this state the loader is in reentrant mode.
     *
     * Prev state: FIXED_UP, RELOADED_FIXED_UP
     * Next state: INITIALIZATION, PENDING_GC
     */
    KLDRSTATE_PENDING_INITIALIZATION,

    /** Initializing.
     * While the module is in this state the loader is in reentrant mode.
     *
     * Prev state: PENDING_INITIALIZATION
     * Next state: GOOD, PENDING_GC
     */
    KLDRSTATE_INITIALIZING,

    /** Initialization failed.
     *
     * This is somewhat similar to PENDING_GC except that, a module
     * in this state cannot be reloaded untill we've done GC. This ensures
     * that a init failure during recursive loading is propagated up.
     *
     * While the module is in this state the loader is in reentrant mode.
     *
     * Prev state: INITIALIZING
     * Next state: GC
     */
    KLDRSTATE_INITIALIZATION_FAILED,

    /** The module has been successfully loaded and initialized.
     * While the module is in this state the loader can be in reentrant
     * or 'unused' mode.
     *
     * Prev state: INITIALIZING
     * Next state: PENDING_TERMINATION
     */
    KLDRSTATE_GOOD,

    /** Pending termination, reference count is 0.
     * While the module is in this state the loader is in reentrant mode.
     * Prerequisite modules are dropped when a module enters this state.
     *
     * Prev state: GOOD
     * Next state: TERMINATING, GOOD
     */
    KLDRSTATE_PENDING_TERMINATION,

    /** Terminating, reference count is still 0.
     * While the module is in this state the loader is in reentrant mode.
     *
     * Prev state: PENDING_TERMINATION
     * Next state: PENDING_GC
     */
    KLDRSTATE_TERMINATING,

    /** Pending garbage collection.
     * Prerequisite modules are dropped when a module enters this state (if not done already).
     *
     * Prev state: TERMINATING, PENDING_INITIALIZATION, INITIALIZATION_FAILED
     * Next state: GC, RELOADED
     */
    KLDRSTATE_PENDING_GC,

    /** Being garbage collected.
     *
     * Prev state: PENDING_GC, INITIALIZATION_FAILED
     * Next state: PENDING_DESTROY, DESTROYED
     */
    KLDRSTATE_GC,

    /** The module has be unlinked, but there are still stack references to it.
     *
     * Prev state: GC, FIXED_UP, LOADED_PREREQUISITES, MAPPED, OPEN
     * Next state: DESTROYED
     */
    KLDRSTATE_PENDING_DESTROY,

    /** The module has been destroyed but not freed yet.
     *
     * This happens when a module ends up being destroyed when cRefs > 0. The
     * module structure will be freed when cRefs reaches 0.
     *
     * Prev state: GC, PENDING_DESTROY
     */
    KLDRSTATE_DESTROYED,

    /** The end of valid states (exclusive) */
    KLDRSTATE_END = KLDRSTATE_DESTROYED,
    /** The usual 32-bit blowup. */
    KLDRSTATE_32BIT_HACK = 0x7fffffff
} KLDRSTATE;


/**
 * Dynamic loader module.
 */
typedef struct KLDRDYLDMOD
{
    /** Magic number. */
    KU32                u32MagicHead;
    /** The module state. */
    KLDRSTATE           enmState;
    /** The module. */
    PKLDRMOD            pMod;
    /** The module handle. */
    HKLDRMOD            hMod;
    /** The total number of references. */
    KU32                cRefs;
    /** The number of dependency references. */
    KU32                cDepRefs;
    /** The number of dynamic load references. */
    KU32                cDynRefs;
    /** Set if this is the executable module.
     * When clear, the module is a shared object or relocatable object. */
    KU32                fExecutable : 1;
    /** Global DLL (set) or specific DLL (clear). */
    KU32                fGlobalOrSpecific : 1;
    /** Whether the module contains bindable symbols in the global unix namespace. */
    KU32                fBindable : 1;
    /** Set if linked into the global init list. */
    KU32                fInitList : 1;
    /** Already loaded or checked prerequisites.
     * This flag is used when loading prerequisites, when set it means that
     * this module is already seen and shouldn't be processed again. */
    KU32                fAlreadySeen : 1;
    /** Set if the module is currently mapped.
     * This is used to avoid unnecessary calls to kLdrModUnmap during cleanup. */
    KU32                fMapped : 1;
    /** Set if TLS allocation has been done. (part of the mapping). */
    KU32                fAllocatedTLS : 1;
    /** Reserved for future use. */
    KU32                f25Reserved : 25;
    /** The load list linkage. */
    struct
    {
        /** The next module in the list. */
        struct KLDRDYLDMOD *pNext;
        /** The prev module in the list. */
        struct KLDRDYLDMOD *pPrev;
    } Load;
    /** The initialization and termination list linkage.
     * If non-recursive initialization is used, the module will be pushed on
     * the initialization list.
     * A module will be linked into the termination list upon a successful
     * return from module initialization. */
    struct
    {
        /** The next module in the list. */
        struct KLDRDYLDMOD *pNext;
        /** The prev module in the list. */
        struct KLDRDYLDMOD *pPrev;
    } InitTerm;
    /** The bind order list linkage.
     * The module is not in this list when fBindable is clear. */
    struct
    {
        /** The next module in the list. */
        struct KLDRDYLDMOD *pNext;
        /** The prev module in the list. */
        struct KLDRDYLDMOD *pPrev;
    } Bind;

    /** The number of prerequisite modules in the prereq array. */
    KU32                cPrereqs;
    /** Pointer to an array of prerequisite module pointers.
     * This array is only filled when in the states starting with
     * KLDRSTATE_LOADED_PREREQUISITES thru KLDRSTATE_GOOD.
     */
    struct KLDRDYLDMOD **papPrereqs;

    /** Magic number. */
    KU32                u32MagicTail;
} KLDRDYLDMOD, *PKLDRDYLDMOD, **PPKLDRDYLDMOD;

/** KLDRDYLDMOD magic value. (Fuyumi Soryo) */
#define KLDRDYMOD_MAGIC     0x19590106

/** Return / crash validation of a module handle argument. */
#define KLDRDYLD_VALIDATE_HKLDRMOD(hMod) \
    do  { \
        if (    (hMod) == NIL_HKLDRMOD \
            ||  (hMod)->u32MagicHead != KLDRDYMOD_MAGIC \
            ||  (hMod)->u32MagicTail != KLDRDYMOD_MAGIC) \
        { \
            return KERR_INVALID_HANDLE; \
        } \
    } while (0)


int kldrInit(void);
void kldrTerm(void);

int kldrDyldInit(void);
void kldrDyldTerm(void);

void kldrDyldDoLoadExe(PKLDRDYLDMOD pExe);
int kldrDyldFailure(int rc, const char *pszFormat, ...);

int kldrDyldOSStartExe(KUPTR uMainEntrypoint, void *pvStack, KSIZE cbStack);
void *kldrDyldOSAllocStack(KSIZE cb);

int kldrDyldFindInit(void);
int kldrDyldFindNewModule(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                          KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKLDRDYLDMOD ppMod);
int kldrDyldFindExistingModule(const char *pszName, const char *pszPrefix, const char *pszSuffix,
                               KLDRDYLDSEARCH enmSearch, unsigned fFlags, PPKLDRDYLDMOD ppMod);

int kldrDyldGetPrerequisite(const char *pszDll, const char *pszPrefix, const char *pszSuffix, KLDRDYLDSEARCH enmSearch,
                            unsigned fFlags, PKLDRDYLDMOD pDep, PPKLDRDYLDMOD ppMod);


int kldrDyldModCreate(PKRDR pRdr, KU32 fFlags, PPKLDRDYLDMOD ppMod);
void kldrDyldModDestroy(PKLDRDYLDMOD pMod);
void kldrDyldModAddRef(PKLDRDYLDMOD pMod);
void kldrDyldModDeref(PKLDRDYLDMOD pMod);
void kldrDyldModAddDep(PKLDRDYLDMOD pMod, PKLDRDYLDMOD pDep);
void kldrDyldModRemoveDep(PKLDRDYLDMOD pMod, PKLDRDYLDMOD pDep);
int kldrDyldModDynamicLoad(PKLDRDYLDMOD pMod);
int kldrDyldModDynamicUnload(PKLDRDYLDMOD pMod);
void kldrDyldModMarkGlobal(PKLDRDYLDMOD pMod);
void kldrDyldModMarkSpecific(PKLDRDYLDMOD pMod);
void kldrDyldModSetBindable(PKLDRDYLDMOD pMod, unsigned fDeep);
void kldrDyldModClearBindable(PKLDRDYLDMOD pMod);
int kldrDyldModMap(PKLDRDYLDMOD pMod);
int kldrDyldModUnmap(PKLDRDYLDMOD pMod);
int kldrDyldModLoadPrerequisites(PKLDRDYLDMOD pMod, const char *pszPrefix, const char *pszSuffix,
                                 KLDRDYLDSEARCH enmSearch, unsigned fFlags);
int kldrDyldModCheckPrerequisites(PKLDRDYLDMOD pMod);
void kldrDyldModUnloadPrerequisites(PKLDRDYLDMOD pMod);
int kldrDyldModFixup(PKLDRDYLDMOD pMod);
int kldrDyldModCallInit(PKLDRDYLDMOD pMod);
void kldrDyldModCallTerm(PKLDRDYLDMOD pMod);
int kldrDyldModReload(PKLDRDYLDMOD pMod);
int kldrDyldModAttachThread(PKLDRDYLDMOD pMod);
void kldrDyldModDetachThread(PKLDRDYLDMOD pMod);
int kldrDyldModGetMainStack(PKLDRDYLDMOD pMod, void **ppvStack, KSIZE *pcbStack);
int kldrDyldModStartExe(PKLDRDYLDMOD pMod);

int kldrDyldModGetName(PKLDRDYLDMOD pMod, char *pszName, KSIZE cchName);
int kldrDyldModGetFilename(PKLDRDYLDMOD pMod, char *pszFilename, KSIZE cchFilename);
int kldrDyldModQuerySymbol(PKLDRDYLDMOD pMod, KU32 uSymbolOrdinal, const char *pszSymbolName, KUPTR *puValue, KU32 *pfKind);


/** Pointer to the head module (the executable).
 * (This is exported, so no prefix.) */
extern PKLDRDYLDMOD     kLdrDyldHead;
/** Pointer to the tail module.
 * (This is exported, so no prefix.) */
extern PKLDRDYLDMOD     kLdrDyldTail;
/** Pointer to the head module of the initialization list.
 * The outermost load call will pop elements from this list in LIFO order (i.e.
 * from the tail). The list is only used during non-recursive initialization
 * and may therefore share the pNext/pPrev members with the termination list
 * since we don't push a module onto the termination list untill it has been
 * successfully initialized. */
extern PKLDRDYLDMOD     g_pkLdrDyldInitHead;
/** Pointer to the tail module of the initalization list. */
extern PKLDRDYLDMOD     g_pkLdrDyldInitTail;
/** Pointer to the head module of the termination order list. */
extern PKLDRDYLDMOD     g_pkLdrDyldTermHead;
/** Pointer to the tail module of the termination order list. */
extern PKLDRDYLDMOD     g_pkLdrDyldTermTail;
/** Pointer to the head module of the bind order list.
 * The modules in this list makes up the global namespace used when binding symbol unix fashion. */
extern PKLDRDYLDMOD     g_pkLdrDyldBindHead;
/** Pointer to the tail module of the bind order list. */
extern PKLDRDYLDMOD     g_pkLdrDyldBindTail;

/** Indicates that the other MainStack globals have been filled in. */
extern unsigned         g_fkLdrDyldDoneMainStack;
/** Whether the stack was allocated seperatly or was part of the executable. */
extern unsigned         g_fkLdrDyldMainStackAllocated;
/** Pointer to the main stack object. */
extern void            *g_pvkLdrDyldMainStack;
/** The size of the main stack object. */
extern KSIZE            g_cbkLdrDyldMainStack;

/** The global error buffer. */
extern char             g_szkLdrDyldError[1024];

extern char             kLdrDyldExePath[8192];
extern char             kLdrDyldLibraryPath[8192];
extern char             kLdrDyldDefPrefix[16];
extern char             kLdrDyldDefSuffix[16];

extern int              g_fBootstrapping;


/** @name The Loader semaphore
 * @{ */
int     kLdrDyldSemInit(void);
void    kLdrDyldSemTerm(void);
int     kLdrDyldSemRequest(void);
void    kLdrDyldSemRelease(void);
/** @} */


/** @name Module interpreter method tables
 * @{ */
extern KLDRMODOPS       g_kLdrModLXOps;
extern KLDRMODOPS       g_kLdrModMachOOps;
extern KLDRMODOPS       g_kLdrModNativeOps;
extern KLDRMODOPS       g_kLdrModPEOps;
/** @} */


/** @} */
#ifdef __cplusplus
}
#endif

#endif
