/* $Id: kRdrFile.cpp 81 2016-08-18 22:10:38Z bird $ */
/** @file
 * kRdrFile - The Native File Provider
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "kRdrInternal.h"
#include <k/kHlpAlloc.h>
#include <k/kHlpString.h>
#include <k/kErrors.h>

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
# include <k/kHlpSys.h>
# include <sys/fcntl.h>
# include <sys/mman.h>
# include <unistd.h>

#elif K_OS == K_OS_OS2
# define INCL_ERRORS
# define INCL_BASE
# include <os2.h>

#elif K_OS == K_OS_WINDOWS
# define WIN32_NO_STATUS
# include <Windows.h>
# include <ntsecapi.h>
# include <ntstatus.h>

# ifdef __cplusplus
  extern "C" {
# endif

  /** @todo find a non-conflicting header with NTSTATUS, NTAPI, ++ */
  typedef LONG NTSTATUS;
  #define NT_SUCCESS(x) ((x)>=0)

  typedef struct _OBJECT_ATTRIBUTES
  {
      ULONG   Length;
      HANDLE  RootDirectory;
      PUNICODE_STRING ObjectName;
      ULONG   Attributes;
      PVOID   SecurityDescriptor;
      PVOID   SecurityQualityOfService;
  } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

  typedef enum _SECTION_INHERIT
  {
      ViewShare = 1,
      ViewUnmap = 2
  } SECTION_INHERIT;

# define NTOSAPI __declspec(dllimport)
# define NtCurrentProcess() GetCurrentProcess()

# ifndef MEM_DOS_LIM
#  define MEM_DOS_LIM 0x40000000UL
# endif

  NTOSAPI
  NTSTATUS
  NTAPI
  NtCreateSection(
      OUT PHANDLE SectionHandle,
      IN ACCESS_MASK DesiredAccess,
      IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
      IN PLARGE_INTEGER SectionSize OPTIONAL,
      IN ULONG Protect,
      IN ULONG Attributes,
      IN HANDLE FileHandle OPTIONAL
  );

  NTOSAPI
  NTSTATUS
  NTAPI
  NtMapViewOfSection(
      IN HANDLE SectionHandle,
      IN HANDLE ProcessHandle,
      IN OUT PVOID *BaseAddress,
      IN ULONG ZeroBits,
      IN ULONG CommitSize,
      IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
      IN OUT PSIZE_T ViewSize,
      IN SECTION_INHERIT InheritDisposition,
      IN ULONG AllocationType,
      IN ULONG Protect
  );

  NTOSAPI
  NTSTATUS
  NTAPI
  NtUnmapViewOfSection(
      IN HANDLE ProcessHandle,
      IN PVOID BaseAddress
  );

  NTOSAPI
  NTSTATUS
  NTAPI
  NtClose(
      IN HANDLE Handle
  );

  NTOSAPI
  NTSTATUS
  NTAPI
  ZwProtectVirtualMemory(
      IN HANDLE ProcessHandle,
      IN OUT PVOID *BaseAddress,
      IN OUT PSIZE_T ProtectSize,
      IN ULONG NewProtect,
      OUT PULONG OldProtect
  );
# define NtProtectVirtualMemory ZwProtectVirtualMemory

  NTOSAPI
  NTSTATUS
  NTAPI
  NtAllocateVirtualMemory(
      IN HANDLE ProcessHandle,
      IN OUT PVOID *BaseAddress,
      IN ULONG ZeroBits,
      IN OUT PSIZE_T AllocationSize,
      IN ULONG AllocationType,
      IN ULONG Protect
  );

  NTOSAPI
  NTSTATUS
  NTAPI
  NtFreeVirtualMemory(
      IN HANDLE ProcessHandle,
      IN OUT PVOID *BaseAddress,
      IN OUT PSIZE_T FreeSize,
      IN ULONG FreeType
  );

# ifdef __cplusplus
  }
# endif

#else
# error "port me"
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Prepared stuff.
 */
typedef struct KRDRFILEPREP
{
    /** The address of the prepared region. */
    void           *pv;
    /** The size of the prepared region. */
    KSIZE           cb;
#if K_OS == K_OS_WINDOWS
    /** Handle to the section created to map the file. */
    HANDLE          hSection;
#endif
} KRDRFILEPREP, *PKRDRFILEPREP;

/**
 * The file provier instance for native files.
 */
typedef struct KRDRFILE
{
    /** The file reader vtable. */
    KRDR                Core;
    /** The file handle. */
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    int                 File;
#elif K_OS == K_OS_OS2
    HFILE               File;
#elif K_OS == K_OS_WINDOWS
    HANDLE              File;
#else
# error "Port me!"
#endif
    /** The current file offset. */
    KFOFF               off;
    /** The file size. */
    KFOFF               cb;
    /** Array where we stuff the mapping area data. */
    KRDRFILEPREP        aPreps[4];
    /** The number of current preps. */
    KU32                cPreps;
    /** Number of mapping references. */
    KI32                cMappings;
    /** The memory mapping. */
    void               *pvMapping;
    /** The filename. */
    char                szFilename[1];
} KRDRFILE, *PKRDRFILE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void     krdrFileDone(PKRDR pRdr);
static int      krdrFileUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrFileGenericUnmap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrFileProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
static int      krdrFileGenericProtect(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect);
static int      krdrFileRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrFileGenericRefresh(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments);
static int      krdrFileMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
static int      krdrFileGenericMap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed);
static KSIZE   krdrFilePageSize(PKRDR pRdr);
static const char *krdrFileName(PKRDR pRdr);
static KIPTR    krdrFileNativeFH(PKRDR pRdr);
static KFOFF    krdrFileTell(PKRDR pRdr);
static KFOFF    krdrFileSize(PKRDR pRdr);
static int      krdrFileAllUnmap(PKRDR pRdr, const void *pvBits);
static int      krdrFileAllMap(PKRDR pRdr, const void **ppvBits);
static int      krdrFileRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off);
static int      krdrFileDestroy(PKRDR pRdr);
static int      krdrFileCreate(PPKRDR ppRdr, const char *pszFilename);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Native file provider operations. */
const KRDROPS g_kRdrFileOps =
{
    "native file",
    NULL,
    krdrFileCreate,
    krdrFileDestroy,
    krdrFileRead,
    krdrFileAllMap,
    krdrFileAllUnmap,
    krdrFileSize,
    krdrFileTell,
    krdrFileName,
    krdrFileNativeFH,
    krdrFilePageSize,
    krdrFileMap,
    krdrFileRefresh,
    krdrFileProtect,
    krdrFileUnmap,
    krdrFileDone,
    42
};


#if K_OS == K_OS_WINDOWS
/**
 * Converts a kLdr segment protection to NT protection for a mapping.
 *
 * @returns Nt page protection.
 * @param   enmProt     kLdr protection.
 */
static ULONG krdrFileGetNtMapProt(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return PAGE_NOACCESS;
        case KPROT_READONLY:             return PAGE_READONLY;
        case KPROT_READWRITE:            return PAGE_READWRITE;
        case KPROT_WRITECOPY:            return PAGE_WRITECOPY;
        case KPROT_EXECUTE:              return PAGE_EXECUTE;
        case KPROT_EXECUTE_READ:         return PAGE_EXECUTE_READ;
        case KPROT_EXECUTE_READWRITE:    return PAGE_EXECUTE_READWRITE;
        case KPROT_EXECUTE_WRITECOPY:    return PAGE_EXECUTE_WRITECOPY;
        default:                            return ~(ULONG)0;
    }
}


/**
 * Converts a kLdr segment protection to NT protection for a allocation.
 *
 * @returns Nt page protection.
 * @param   enmProt     kLdr protection.
 */
static ULONG krdrFileGetNtAllocProt(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return PAGE_NOACCESS;
        case KPROT_READONLY:             return PAGE_READONLY;
        case KPROT_WRITECOPY:
        case KPROT_READWRITE:            return PAGE_READWRITE;
        case KPROT_EXECUTE:              return PAGE_EXECUTE;
        case KPROT_EXECUTE_READ:         return PAGE_EXECUTE_READ;
        case KPROT_EXECUTE_WRITECOPY:
        case KPROT_EXECUTE_READWRITE:    return PAGE_EXECUTE_READWRITE;
        default:                            return ~(ULONG)0;
    }
}
#endif


/** @copydoc KRDROPS::pfnDone */
static void     krdrFileDone(PKRDR pRdr)
{
}


/**
 * Finds a prepared mapping region.
 *
 * @returns Pointer to the aPrep entry.
 * @param   pFile   The instance data.
 * @param   pv      The base of the region.
 */
static PKRDRFILEPREP krdrFileFindPrepExact(PKRDRFILE pFile, void *pv)
{
    KI32 i = pFile->cPreps;
    while (i-- > 0)
        if (pFile->aPreps[i].pv == pv)
            return &pFile->aPreps[i];
    return NULL;
}


/** @copydoc KRDROPS::pfnUnmap */
static int      krdrFileUnmap(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP       pPrep = krdrFileFindPrepExact(pRdrFile, pvBase);
    int                 rc;
    if (!pPrep)
        return KERR_INVALID_PARAMETER;

#if K_OS == K_OS_WINDOWS
    if (pPrep->hSection != NULL)
    {
        /** @todo implement me. */
        return -1;
    }
#endif

    rc = krdrFileGenericUnmap(pRdr, pPrep, cSegments, paSegments);

    /* remove the mapping data on success. */
    if (!rc)
    {
        pRdrFile->cPreps--;
        if (pPrep != &pRdrFile->aPreps[pRdrFile->cPreps])
            *pPrep = pRdrFile->aPreps[pRdrFile->cPreps];
    }
    return rc;
}


/** Generic implementation of krdrFileUnmap. */
static int krdrFileGenericUnmap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments)
{
    krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    return kHlpPageFree(pPrep->pv, pPrep->cb);
}


/** @copydoc KRDROPS::pfnProtect */
static int krdrFileProtect(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP    pPrep = krdrFileFindPrepExact(pRdrFile, pvBase);
    if (!pPrep)
        return KERR_INVALID_PARAMETER;

#if K_OS == K_OS_WINDOWS
    if (pPrep->hSection != NULL)
    {
        /** @todo implement me. */
        return -1;
    }
#endif

    return krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, fUnprotectOrProtect);
}


/** Generic implementation of krdrFileProtect. */
static int krdrFileGenericProtect(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    KU32 i;

    /*
     * Iterate the segments and apply memory protection changes.
     */
    for (i = 0; i < cSegments; i++)
    {
        int rc;
        void *pv;
        KPROT enmProt;

        if (paSegments[i].RVA == NIL_KLDRADDR)
            continue;

        /* calc new protection. */
        enmProt = (KPROT)paSegments[i].enmProt; /** @todo drop cast */
        if (fUnprotectOrProtect)
        {
            switch (enmProt)
            {
                case KPROT_NOACCESS:
                case KPROT_READONLY:
                case KPROT_READWRITE:
                case KPROT_WRITECOPY:
                    enmProt = KPROT_READWRITE;
                    break;
                case KPROT_EXECUTE:
                case KPROT_EXECUTE_READ:
                case KPROT_EXECUTE_READWRITE:
                case KPROT_EXECUTE_WRITECOPY:
                    enmProt = KPROT_EXECUTE_READWRITE;
                    break;
                default:
                    kRdrAssert(!"bad enmProt");
                    return -1;
            }
        }
        else
        {
            /* copy on write -> normal write. */
            if (enmProt == KPROT_EXECUTE_WRITECOPY)
                enmProt = KPROT_EXECUTE_READWRITE;
            else if (enmProt == KPROT_WRITECOPY)
                enmProt = KPROT_READWRITE;
        }

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;

        rc = kHlpPageProtect(pv, paSegments[i].cbMapped, enmProt);
        if (rc)
            break;
    }

    return 0;
}


/** @copydoc KRDROPS::pfnRefresh */
static int      krdrFileRefresh(PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    PKRDRFILE        pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP    pPrep = krdrFileFindPrepExact(pRdrFile, pvBase);
    if (!pPrep)
        return KERR_INVALID_PARAMETER;

#if K_OS == K_OS_WINDOWS
    if (pPrep->hSection != NULL)
    {
        /** @todo implement me. */
        return -1;
    }
#endif

    return krdrFileGenericRefresh(pRdr, pPrep, cSegments, paSegments);
}


/** Generic implementation of krdrFileRefresh. */
static int      krdrFileGenericRefresh(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments)
{
    int rc;
    int rc2;
    KU32 i;

    /*
     * Make everything writable again.
     */
    rc = krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    if (rc)
    {
        krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
        return rc;
    }

    /*
     * Clear everything.
     */
    /** @todo only zero the areas not covered by raw file bits. */
    kHlpMemSet(pPrep->pv, 0, pPrep->cb);

    /*
     * Reload all the segments.
     * We could possibly skip some segments, but we currently have
     * no generic way of figuring out which at the moment.
     */
    for (i = 0; i < cSegments; i++)
    {
        void *pv;

        if (    paSegments[i].RVA == NIL_KLDRADDR
            ||  paSegments[i].cbFile <= 0)
            continue;

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
        rc = pRdr->pOps->pfnRead(pRdr, pv, paSegments[i].cbFile, paSegments[i].offFile);
        if (rc)
            break;
    }

    /*
     * Protect the bits again.
     */
    rc2 = krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
    if (rc2 && rc)
        rc = rc2;

    return rc;
}


/** @copydoc KRDROPS::pfnMap */
static int      krdrFileMap(PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    PKRDRFILE       pRdrFile = (PKRDRFILE)pRdr;
    PKRDRFILEPREP   pPrep = &pRdrFile->aPreps[pRdrFile->cPreps];
    KLDRSIZE        cbTotal;
    const KSIZE     cbPage = pRdr->pOps->pfnPageSize(pRdr);
    int             rc;
    KU32            i;

    if (pRdrFile->cPreps >= K_ELEMENTS(pRdrFile->aPreps))
        return KRDR_ERR_TOO_MANY_MAPPINGS;

    /*
     * Calc the total mapping space needed.
     */
    cbTotal = 0;
    for (i = 0; i < cSegments; i++)
    {
        KLDRSIZE uRVASegmentEnd;
        if (paSegments[i].RVA == NIL_KLDRADDR)
            continue;
        uRVASegmentEnd = paSegments[i].RVA + paSegments[i].cbMapped;
        if (cbTotal < uRVASegmentEnd)
            cbTotal = uRVASegmentEnd;
    }
    pPrep->cb = (KSIZE)cbTotal;
    if (pPrep->cb != cbTotal)
        return KLDR_ERR_ADDRESS_OVERFLOW;
    pPrep->cb = (pPrep->cb + (cbPage - 1)) & ~(cbPage- 1);

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    /** @todo */

#elif K_OS == K_OS_WINDOWS
    /*
     * The NT memory mapped file API sucks in a lot of ways. Unless you're actually
     * trying to map a PE image and the kernel can parse the file for it self, the
     * API just isn't up to scratch.
     *
     * Problems:
     *      1. Reserving memory for the views is risky because you can't reserve and
     *         map into the reserved space. So, other threads might grab the memory
     *         before we get to it.
     *      2. The page aligning of file offsets makes it impossible to map most
     *         executable images since these are commonly sector aligned.
     *      3. When mapping a read+execute file, its not possible to create section
     *         larger than the file since the section size is bound to the data file
     *         size. This wouldn't have been such a problem if it was possible to
     *         map views beyond the section restriction, i.e. have a file size and
     *         view size.
     *      4. Only x86 can map views at page granularity it seems, and that only
     *         using an undocument flag. The default granularity is 64KB.
     *      5. There is more crappyness here...
     *
     * So, first we'll have to check if we can the file using the crappy NT APIs.
     * Chances are we can't.
     */
    for (i = 0; i < cSegments; i++)
    {
        if (paSegments[i].RVA == NIL_KLDRADDR)
            continue;

        /* The file backing of the segments must be page aligned. */
        if (    paSegments[i].cbFile > 0
            &&  paSegments[i].offFile & (cbPage - 1))
            break;

        /* Only page alignment gaps between the file size and the mapping size. */
        if (    paSegments[i].cbFile > 0
            &&  (paSegments[i].cbFile & ~(cbPage - 1)) != (paSegments[i].cbMapped & ~(cbPage - 1)) )
            break;

        /* The mapping addresses of the segments must be page aligned.
         * Non-x86 will probably require 64KB alignment here. */
        if (paSegments[i].RVA & (cbPage - 1))
            break;

        /* If we do have to allocate the segment it's RVA must be 64KB aligned. */
        if (    paSegments[i].cbFile > 0
            &&  (paSegments[i].RVA & 0xffff))
            break;
    }
    /** @todo if this is a PE image, we might just try a SEC_IMAGE mapping. It'll work if the host and image machines matches. */
    if (i == cSegments)
    {
        /* WOW! it may work out! Incredible! */
        SIZE_T          ViewSize;
        LARGE_INTEGER   SectionOffset;
        LARGE_INTEGER   MaxiumSize;
        NTSTATUS        Status;
        PVOID           pv;

        MaxiumSize.QuadPart = pRdr->pOps->pfnSize(pRdr);
        if (MaxiumSize.QuadPart > (LONGLONG)cbTotal)
            MaxiumSize.QuadPart = cbTotal;

        Status = NtCreateSection(&pPrep->hSection,
                                 SECTION_MAP_EXECUTE | SECTION_MAP_READ,    /* desired access */
                                 NULL,                                      /* object attributes */
                                 &MaxiumSize,
                                 PAGE_EXECUTE_WRITECOPY,                    /* page attributes */
                                 SEC_COMMIT,                                /* section attributes */
                                 pRdrFile->File);
        if (!NT_SUCCESS(Status))
            return (int)Status;

        /*
         * Determin the base address.
         */
        if (fFixed)
            pPrep->pv = *ppvBase;
        else
        {
            pv = NULL;
            ViewSize = (KSIZE)cbTotal;

            Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                             &pv,
                                             0,                             /* ZeroBits */
                                             &ViewSize,
                                             MEM_RESERVE,
                                             PAGE_READONLY);
            if (NT_SUCCESS(Status))
            {
                pPrep->pv = *ppvBase = pv;
                ViewSize = 0;
                Status = NtFreeVirtualMemory(NtCurrentProcess(), &pv, &ViewSize, MEM_RELEASE);
            }
            if (!NT_SUCCESS(Status))
            {
                NtClose(pPrep->hSection);
                return Status;
            }
        }

        /*
         * Map the segments.
         */
        for (i = 0; i < cSegments; i++)
        {
            ULONG fPageProt;

            if (paSegments[i].RVA == NIL_KLDRADDR)
                continue;

            pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
            if (paSegments[i].cbFile > 0)
            {
                SectionOffset.QuadPart = paSegments[i].offFile;
                ViewSize = paSegments[i].cbFile;
                fPageProt = krdrFileGetNtMapProt(paSegments[i].enmProt);
                /* STATUS_MAPPED_ALIGNMENT
                   STATUS_CONFLICTING_ADDRESSES
                   STATUS_INVALID_VIEW_SIZE */
                Status = NtMapViewOfSection(pPrep->hSection, NtCurrentProcess(),
                                            &pv,
                                            0,                                  /* ZeroBits */
                                            0,                                  /* CommitSize */
                                            &SectionOffset,                     /* SectionOffset */
                                            &ViewSize,
                                            ViewUnmap,
                                            MEM_DOS_LIM,                        /* AllocationType */
                                            fPageProt);
                /* do we have to zero anything? */
                if (    NT_SUCCESS(Status)
                    &&  0/*later*/)
                {
                    /*ULONG OldPageProt = 0;
                      NtProtectVirtualMemory(NtCurrentProcess(), &pv, &ViewSize, , */
                }
            }
            else
            {
                ViewSize = paSegments[i].cbMapped;
                fPageProt = krdrFileGetNtAllocProt(paSegments[i].enmProt);
                Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                                 &pv,
                                                 0,                             /* ZeroBits */
                                                 &ViewSize,
                                                 MEM_COMMIT,
                                                 fPageProt);
            }
            if (!NT_SUCCESS(Status))
                break;
        }

        /*
         * On success, commit the mapping and return.
         */
        if (NT_SUCCESS(Status))
        {
            pRdrFile->cPreps++;
            return 0;
        }

        /* bail out and fall back on the generic code. */
        while (i-- > 0)
        {
            PVOID pv;

            if (paSegments[i].RVA == NIL_KLDRADDR)
                continue;

            pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
            if (paSegments[i].cbFile > 0)
                NtUnmapViewOfSection(NtCurrentProcess(), pv);
            else
                NtFreeVirtualMemory(NtCurrentProcess(), &pv, NULL, MEM_RELEASE);
        }
        NtClose(pPrep->hSection);
    }
    /* else: fall back to the generic code */
    pPrep->hSection = NULL;
#endif

    /*
     * Use the generic map emulation.
     */
    pPrep->pv = fFixed ? *ppvBase : NULL;
    rc = krdrFileGenericMap(pRdr, pPrep, cSegments, paSegments, fFixed);
    if (!rc)
    {
        *ppvBase = pPrep->pv;
        pRdrFile->cPreps++;
    }

    return rc;
}


/** Generic implementation of krdrFileMap. */
static int  krdrFileGenericMap(PKRDR pRdr, PKRDRFILEPREP pPrep, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    int rc;
    KU32 i;

    /*
     * Generic mapping code using kHlpPageAlloc(), kHlpPageFree() and kHlpPageProtect().
     */
    rc = kHlpPageAlloc(&pPrep->pv, pPrep->cb, KPROT_EXECUTE_READWRITE, fFixed);
    if (rc)
        return rc;

    /*
     * Load the data.
     */
    for (i = 0; i < cSegments; i++)
    {
        void *pv;

        if (    paSegments[i].RVA == NIL_KLDRADDR
            ||  paSegments[i].cbFile <= 0)
            continue;

        pv = (KU8 *)pPrep->pv + paSegments[i].RVA;
        rc = pRdr->pOps->pfnRead(pRdr, pv, paSegments[i].cbFile, paSegments[i].offFile);
        if (rc)
            break;
    }

    /*
     * Set segment protection.
     */
    if (!rc)
    {
        rc = krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 0 /* protect */);
        if (!rc)
            return 0;
        krdrFileGenericProtect(pRdr, pPrep, cSegments, paSegments, 1 /* unprotect */);
    }

    /* bailout */
    kHlpPageFree(pPrep->pv, pPrep->cb);
    return rc;
}


/** @copydoc KRDROPS::pfnPageSize */
static KSIZE   krdrFilePageSize(PKRDR pRdr)
{
#if K_OS == K_OS_DARWIN
    return 0x1000; /** @todo find some header somewhere... */

#elif K_OS == K_OS_LINUX
    return 0x1000; /** @todo find some header somewhere... */

#elif K_OS == K_OS_OS2
    /* The page size on OS/2 wont change anytime soon. :-) */
    return 0x1000;

#elif K_OS == K_OS_WINDOWS
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    return SysInfo.dwPageSize;
    /*return SysInfo.dwAllocationGranularity;*/
#else
# error "port me"
#endif
}


/** @copydoc KRDROPS::pfnName */
static const char *krdrFileName(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    return &pRdrFile->szFilename[0];
}


static KIPTR krdrFileNativeFH(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_OS2 \
 || K_OS == K_OS_SOLARIS \
 || K_OS == K_OS_WINDOWS
    return (KIPTR)pRdrFile->File;
#else
# error "port me"
#endif
}


/** @copydoc KRDROPS::pfnTell */
static KFOFF krdrFileTell(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /*
     * If the offset is undefined, try figure out what it is.
     */
    if (pRdrFile->off == -1)
    {
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
        pRdrFile->off = kHlpSys_lseek(pRdrFile->File, SEEK_CUR, 0);
        if (pRdrFile->off < 0)
            pRdrFile->off = -1;

#elif K_OS == K_OS_OS2
        ULONG ulNew;
        APIRET rc = DosSetFilePtr(pRdrFile->File, 0, FILE_CURRENT, &ulNew);
        if (rc)
            return -1;
        pRdrFile->off = ulNew;

#elif K_OS == K_OS_WINDOWS
        LONG offHigh = 0;
        LONG offLow;
        int rc;

        SetLastError(0);
        offLow = SetFilePointer(pRdrFile->File, 0, &offHigh, FILE_CURRENT);
        rc = GetLastError();
        if (rc)
            return -1;
        pRdrFile->off = ((KFOFF)offHigh << 32) | offLow;

#else
# error "port me."
#endif
    }
    return pRdrFile->off;
}


/** @copydoc KRDROPS::pfnSize */
static KFOFF krdrFileSize(PKRDR pRdr)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;
    return pRdrFile->cb;
}


/** @copydoc KRDROPS::pfnAllUnmap */
static int krdrFileAllUnmap(PKRDR pRdr, const void *pvBits)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /* check for underflow */
    if (pRdrFile->cMappings <= 0)
        return KERR_INVALID_PARAMETER;

    /* decrement usage counter, free mapping if no longer in use. */
    if (!--pRdrFile->cMappings)
    {
        kHlpFree(pRdrFile->pvMapping);
        pRdrFile->pvMapping = NULL;
    }

    return 0;
}


/** @copydoc KRDROPS::pfnAllMap */
static int krdrFileAllMap(PKRDR pRdr, const void **ppvBits)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /*
     * Do we need to map it?
     */
    if (!pRdrFile->pvMapping)
    {
        int rc;
        KFOFF cbFile = pRdrFile->Core.pOps->pfnSize(pRdr);
        KSIZE cb = (KSIZE)cbFile;
        if (cb != cbFile)
            return KERR_NO_MEMORY;

        pRdrFile->pvMapping = kHlpAlloc(cb);
        if (!pRdrFile->pvMapping)
            return KERR_NO_MEMORY;
        rc = pRdrFile->Core.pOps->pfnRead(pRdr, pRdrFile->pvMapping, cb, 0);
        if (rc)
        {
            kHlpFree(pRdrFile->pvMapping);
            pRdrFile->pvMapping = NULL;
            return rc;
        }
        pRdrFile->cMappings = 0;
    }

    *ppvBits = pRdrFile->pvMapping;
    pRdrFile->cMappings++;
    return 0;
}


/** @copydoc KRDROPS::pfnRead */
static int krdrFileRead(PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off)
{
    PKRDRFILE pRdrFile = (PKRDRFILE)pRdr;

    /*
     * Do a seek if needed.
     */
    if (pRdrFile->off != off)
    {
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
        pRdrFile->off = kHlpSys_lseek(pRdrFile->File, SEEK_SET, off);
        if (pRdrFile->off < 0)
        {
            int rc = (int)-pRdrFile->off;
            pRdrFile->off = -1;
            return -rc;
        }

#elif K_OS == K_OS_OS2
        ULONG ulNew;
        APIRET rc;

        rc = DosSetFilePtr(pRdrFile->File, off, FILE_BEGIN, &ulNew);
        if (rc)
        {
            pRdrFile->off = -1;
            return rc;
        }

#elif K_OS == K_OS_WINDOWS
        LONG offHigh;
        LONG offLow;

        offHigh = (LONG)(off >> 32);
        offLow = SetFilePointer(pRdrFile->File, (LONG)off, &offHigh, FILE_BEGIN);
        if (    offLow != (LONG)off
            ||  offHigh != (LONG)(off >> 32))
        {
            int rc = GetLastError();
            if (!rc)
                rc = KERR_GENERAL_FAILURE;
            pRdrFile->off = -1;
            return rc;
        }

#else
# error "port me."
#endif
    }

    /*
     * Do the read.
     */
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    {
    KSSIZE cbRead;

    cbRead = kHlpSys_read(pRdrFile->File, pvBuf, cb);
    if (cbRead != cb)
    {
        pRdrFile->off = -1;
        if (cbRead < 0)
            return -cbRead;
        return KERR_GENERAL_FAILURE;
    }
    }

#elif K_OS == K_OS_OS2
    {
    ULONG cbRead = 0;
    APIRET rc = DosRead(pRdrFile->File, pvBuf, cb, &cbRead);
    if (rc)
    {
        pRdrFile->off = -1;
        return rc;
    }
    if (cbRead != cb)
    {
        pRdrFile->off = -1;
        return KERR_GENERAL_FAILURE;
    }
    }

#elif K_OS == K_OS_WINDOWS
    {
    DWORD cbRead = 0;
    if (!ReadFile(pRdrFile->File, pvBuf, cb, &cbRead, NULL))
    {
        int rc = GetLastError();
        if (!rc)
            rc = KERR_GENERAL_FAILURE;
        pRdrFile->off = -1;
        return rc;
    }
    if (cbRead != cb)
    {
        pRdrFile->off = -1;
        return KERR_GENERAL_FAILURE;
    }
    }

#else
# error "port me."
#endif

    pRdrFile->off = off + cb;
    return 0;
}


/** @copydoc KRDROPS::pfnDestroy */
static int krdrFileDestroy(PKRDR pRdr)
{
    PKRDRFILE    pRdrFile = (PKRDRFILE)pRdr;
    int          rc;

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    rc = kHlpSys_close(pRdrFile->File);

#elif K_OS == K_OS_OS2
    rc = DosClose(pRdrFile->File);

#elif K_OS == K_OS_WINDOWS
    rc = 0;
    if (!CloseHandle(pRdrFile->File))
        rc = GetLastError();

#else
# error "port me"
#endif

    if (pRdrFile->pvMapping)
    {
        kHlpFree(pRdrFile->pvMapping);
        pRdrFile->pvMapping = NULL;
    }

    kHlpFree(pRdr);
    return rc;
}


/** @copydoc KRDROPS::pfnCreate */
static int krdrFileCreate(PPKRDR ppRdr, const char *pszFilename)
{
    KSIZE       cchFilename;
    PKRDRFILE   pRdrFile;

    /*
     * Open the file, determin its size and correct filename.
     */
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    int         File;
    KFOFF       cb;
    KFOFF       rc;
    char        szFilename[1024];

    cchFilename = kHlpStrLen(pszFilename);
    if (cchFilename >= sizeof(szFilename))
        return KERR_OUT_OF_RANGE;
    kHlpMemCopy(szFilename, pszFilename, cchFilename + 1);
    /** @todo normalize the filename. */

# ifdef O_BINARY
    File = kHlpSys_open(pszFilename, O_RDONLY | O_BINARY, 0);
# else
    File = kHlpSys_open(pszFilename, O_RDONLY, 0);
# endif
    if (File < 0)
        return -File;

    cb = kHlpSys_lseek(File, SEEK_END, 0);
    rc = kHlpSys_lseek(File, SEEK_SET, 0);
    if (    cb < 0
        ||  rc < 0)
    {
        kHlpSys_close(File);
        return cb < 0 ? -cb : -rc;
    }

#elif K_OS == K_OS_OS2
    ULONG       ulAction = 0;
    FILESTATUS3 Info;
    APIRET      rc;
    HFILE       File = 0;
    KFOFF       cb;
    char        szFilename[CCHMAXPATH];

    if ((uintptr_t)pszFilename >= 0x20000000)
    {
        char *psz;
        cchFilename = kHlpStrLen(szFilename);
        psz = (char *)kHlpAllocA(cchFilename + 1);
        kHlpMemCopy(psz, pszFilename, cchFilename + 1);
        pszFilename = psz;
    }
    rc = DosOpen((PCSZ)pszFilename, &File, &ulAction, 0, FILE_NORMAL,
                 OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                 OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYWRITE | OPEN_ACCESS_READONLY | OPEN_FLAGS_RANDOMSEQUENTIAL,
                 NULL);
    if (rc)
        return rc;

    rc = DosQueryPathInfo((PCSZ)pszFilename, FIL_QUERYFULLNAME, szFilename, sizeof(szFilename));
    if (rc)
    {
        DosClose(File);
        return rc;
    }

    rc = DosQueryFileInfo(File, FIL_STANDARD, &Info, sizeof(Info));
    if (rc)
    {
        DosClose(File);
        return rc;
    }
    cb = Info.cbFile;

#elif K_OS == K_OS_WINDOWS
    SECURITY_ATTRIBUTES SecAttr;
    DWORD               High;
    DWORD               Low;
    int                 rc;
    HANDLE              File;
    KFOFF            cb;
    char                szFilename[MAX_PATH];

    SecAttr.bInheritHandle = FALSE;
    SecAttr.lpSecurityDescriptor = NULL;
    SecAttr.nLength = 0;
    File = CreateFile(pszFilename, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, &SecAttr, OPEN_EXISTING, 0, NULL);
    if (File == INVALID_HANDLE_VALUE)
        return GetLastError();

    if (!GetFullPathName(pszFilename, sizeof(szFilename), szFilename, NULL))
    {
        rc = GetLastError();
        CloseHandle(File);
        return rc;
    }

    SetLastError(0);
    Low = GetFileSize(File, &High);
    rc = GetLastError();
    if (rc)
    {
        CloseHandle(File);
        return rc;
    }
    cb = ((KFOFF)High << 32) | Low;

#else
# error "port me"
#endif


    /*
     * Allocate the reader instance.
     */
    cchFilename = kHlpStrLen(szFilename);
    pRdrFile = (PKRDRFILE)kHlpAlloc(sizeof(*pRdrFile) + cchFilename);
    if (!pRdrFile)
    {
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
        kHlpSys_close(File);
#elif K_OS == K_OS_OS2
        DosClose(File);
#elif K_OS == K_OS_WINDOWS
        CloseHandle(File);
#else
# error "port me"
#endif
        return KERR_NO_MEMORY;
    }

    /*
     * Initialize it and return successfully.
     */
    pRdrFile->Core.u32Magic = KRDR_MAGIC;
    pRdrFile->Core.pOps = &g_kRdrFileOps;
    pRdrFile->File = File;
    pRdrFile->cb = cb;
    pRdrFile->off = 0;
    pRdrFile->cPreps = 0;
    pRdrFile->cMappings = 0;
    pRdrFile->pvMapping = NULL;
    kHlpMemCopy(&pRdrFile->szFilename[0], szFilename, cchFilename + 1);

    *ppRdr = &pRdrFile->Core;
    return 0;
}

