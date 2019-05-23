/* $Id: fakedri_drv.c $ */
/** @file
 * VBox OpenGL DRI driver functions
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define _GNU_SOURCE 1

#include "cr_error.h"
#include "cr_gl.h"
#include "cr_mem.h"
#include "stub.h"
#include "fakedri_drv.h"
#include "dri_glx.h"
#include "iprt/mem.h"
#include "iprt/err.h"
#include <dlfcn.h>
#include <elf.h>
#include <unistd.h>

#if defined(RT_OS_FREEBSD)
#include <sys/param.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <string.h>
#endif

/** X server message type definitions. */
typedef enum {
    X_PROBED,                   /* Value was probed */
    X_CONFIG,                   /* Value was given in the config file */
    X_DEFAULT,                  /* Value is a default */
    X_CMDLINE,                  /* Value was given on the command line */
    X_NOTICE,                   /* Notice */
    X_ERROR,                    /* Error message */
    X_WARNING,                  /* Warning message */
    X_INFO,                     /* Informational message */
    X_NONE,                     /* No prefix */
    X_NOT_IMPLEMENTED,          /* Not implemented */
    X_UNKNOWN = -1              /* unknown -- this must always be last */
} MessageType;

#define VBOX_NO_MESA_PATCH_REPORTS

//#define DEBUG_DRI_CALLS

/// @todo this could be different...
#ifdef RT_ARCH_AMD64
# ifdef RT_OS_FREEBSD
#  define DRI_DEFAULT_DRIVER_DIR "/usr/local/lib/dri"
#  define DRI_XORG_DRV_DIR "/usr/local/lib/xorg/modules/drivers/"
# else
#  define DRI_DEFAULT_DRIVER_DIR "/usr/lib64/dri:/usr/lib/dri:/usr/lib/x86_64-linux-gnu/dri:/usr/lib/xorg/modules/dri"
#  define DRI_XORG_DRV_DIR "/usr/lib/xorg/modules/drivers/"
# endif
#else
# ifdef RT_OS_FREEBSD
#  define DRI_DEFAULT_DRIVER_DIR "/usr/local/lib/dri"
#  define DRI_XORG_DRV_DIR "/usr/local/lib/xorg/modules/drivers/"
# else
#  define DRI_DEFAULT_DRIVER_DIR "/usr/lib/dri:/usr/lib/i386-linux-gnu/dri:/usr/lib/xorg/modules/dri"
#  define DRI_XORG_DRV_DIR "/usr/lib/xorg/modules/drivers/"
# endif
#endif

#ifdef DEBUG_DRI_CALLS
 #define SWDRI_SHOWNAME(pext, func) \
   crDebug("SWDRI: sc %s->%s", #pext, #func)
#else
 #define SWDRI_SHOWNAME(pext, func)
#endif

#define SWDRI_SAFECALL(pext, func, ...)        \
    SWDRI_SHOWNAME(pext, func);                \
    if (pext && pext->func){                   \
        (*pext->func)(__VA_ARGS__);            \
    } else {                                   \
        crDebug("swcore_call NULL for "#func); \
    }

#define SWDRI_SAFERET(pext, func, ...)         \
    SWDRI_SHOWNAME(pext, func);                \
    if (pext && pext->func){                   \
        return (*pext->func)(__VA_ARGS__);     \
    } else {                                   \
        crDebug("swcore_call NULL for "#func); \
        return 0;                              \
    }

#define SWDRI_SAFERET_CORE(func, ...) SWDRI_SAFERET(gpSwDriCoreExternsion, func, __VA_ARGS__)
#define SWDRI_SAFECALL_CORE(func, ...) SWDRI_SAFECALL(gpSwDriCoreExternsion, func, __VA_ARGS__)
#define SWDRI_SAFERET_SWRAST(func, ...) SWDRI_SAFERET(gpSwDriSwrastExtension, func, __VA_ARGS__)
#define SWDRI_SAFECALL_SWRAST(func, ...) SWDRI_SAFECALL(gpSwDriSwrastExtension, func, __VA_ARGS__)

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#ifdef RT_ARCH_AMD64
# define DRI_ELFSYM Elf64_Sym
#else
# define DRI_ELFSYM Elf32_Sym
#endif

#ifdef RT_ARCH_AMD64
typedef struct _FAKEDRI_PatchNode
{
    const char* psFuncName;
    void *pDstStart, *pDstEnd;
    const void *pSrcStart, *pSrcEnd;

    struct _FAKEDRI_PatchNode *pNext;
} FAKEDRI_PatchNode;
static FAKEDRI_PatchNode *g_pFreeList=NULL, *g_pRepatchList=NULL;
#endif

static struct _glapi_table* vbox_glapi_table = NULL;
fakedri_glxapi_table glxim;

static const __DRIextension **gppSwDriExternsion = NULL;
static const __DRIcoreExtension *gpSwDriCoreExternsion = NULL;
static const __DRIswrastExtension *gpSwDriSwrastExtension = NULL;

extern const __DRIextension * __driDriverExtensions[];

#define VBOX_SET_MESA_FUNC(table, name, func) \
    if (_glapi_get_proc_offset(name)>=0) SET_by_offset(table, _glapi_get_proc_offset(name), func); \
    else crWarning("%s not found in mesa table", name)

#define GLAPI_ENTRY(Func) VBOX_SET_MESA_FUNC(vbox_glapi_table, "gl"#Func, cr_gl##Func);

static void
vboxPatchMesaExport(const char* psFuncName, const void *pStart, const void *pEnd);

static void
vboxPatchMesaGLAPITable()
{
    void *pGLTable;

    pGLTable = (void *)_glapi_get_dispatch();
    vbox_glapi_table = crAlloc(_glapi_get_dispatch_table_size() * sizeof (void *));
    if (!vbox_glapi_table)
    {
        crError("Not enough memory to allocate dispatch table");
    }
    crMemcpy(vbox_glapi_table, pGLTable, _glapi_get_dispatch_table_size() * sizeof (void *));

    #include "fakedri_glfuncsList.h"

    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glBlendEquationSeparateEXT", cr_glBlendEquationSeparate);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glSampleMaskSGIS", cr_glSampleMaskEXT);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glSamplePatternSGIS", cr_glSamplePatternEXT);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2dMESA", cr_glWindowPos2d);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2dvMESA", cr_glWindowPos2dv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2fMESA", cr_glWindowPos2f);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2fvMESA", cr_glWindowPos2fv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2iMESA", cr_glWindowPos2i);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2ivMESA", cr_glWindowPos2iv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2sMESA", cr_glWindowPos2s);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos2svMESA", cr_glWindowPos2sv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3dMESA", cr_glWindowPos3d);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3dvMESA", cr_glWindowPos3dv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3fMESA", cr_glWindowPos3f);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3fvMESA", cr_glWindowPos3fv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3iMESA", cr_glWindowPos3i);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3ivMESA", cr_glWindowPos3iv);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3sMESA", cr_glWindowPos3s);
    VBOX_SET_MESA_FUNC(vbox_glapi_table, "glWindowPos3svMESA", cr_glWindowPos3sv);

    _glapi_set_dispatch(vbox_glapi_table);
};
#undef GLAPI_ENTRY

#define GLXAPI_ENTRY(Func) pGLXTable->Func = VBOXGLXTAG(glX##Func);
static void
vboxFillGLXAPITable(fakedri_glxapi_table *pGLXTable)
{
    #include "fakedri_glxfuncsList.h"
}
#undef GLXAPI_ENTRY

static void
vboxApplyPatch(const char* psFuncName, void *pDst, const void *pSrc, unsigned long size)
{
    void *alPatch;
    int rv;

    /* Get aligned start address we're going to patch*/
    alPatch = (void*) ((uintptr_t)pDst & ~(uintptr_t)(PAGESIZE-1));

#ifndef VBOX_NO_MESA_PATCH_REPORTS
    crDebug("MProtecting: %p, %li", alPatch, pDst-alPatch+size);
#endif

    /* Get write access to mesa functions */
    rv = RTMemProtect(alPatch, pDst-alPatch+size, RTMEM_PROT_READ|RTMEM_PROT_WRITE|RTMEM_PROT_EXEC);
    if (RT_FAILURE(rv))
    {
        crError("mprotect failed with %x (%s)", rv, psFuncName);
    }

#ifndef VBOX_NO_MESA_PATCH_REPORTS
    crDebug("Writing %li bytes to %p from %p", size, pDst, pSrc);
#endif

    crMemcpy(pDst, pSrc, size);

    /** @todo Restore the protection, probably have to check what was it before us...*/
    rv = RTMemProtect(alPatch, pDst-alPatch+size, RTMEM_PROT_READ|RTMEM_PROT_EXEC);
    if (RT_FAILURE(rv))
    {
        crError("mprotect2 failed with %x (%s)", rv, psFuncName);
    }
}

#define FAKEDRI_JMP64_PATCH_SIZE 13

#if defined(RT_OS_FREEBSD)
/* Provide basic dladdr1 flags */
enum {
        RTLD_DL_SYMENT  = 1
};

/* Provide a minimal local version of dladdr1 */
static int
dladdr1(const void *address, Dl_info *dlip, void **info, int flags)
{
        static DRI_ELFSYM desym;
        GElf_Sym sym;
        GElf_Shdr shdr;
        Elf *elf;
        Elf_Scn *scn;
        Elf_Data *data;
        int ret, fd, count, i;

        /* Initialize variables */
        fd = -1;
        elf = NULL;

        /* Call dladdr first */
        ret = dladdr(address, dlip);
        if (ret == 0) goto err_exit;

        /* Check for supported flags */
        if (flags != RTLD_DL_SYMENT) return 1;

        /* Open shared library's ELF file */
        if (elf_version(EV_CURRENT) == EV_NONE) goto err_exit;
        fd = open(dlip->dli_fname, O_RDONLY);
        if (fd < 0) goto err_exit;
        elf = elf_begin(fd, ELF_C_READ, NULL);
        if (elf == NULL) goto err_exit;

        /* Find the '.dynsym' section */
        scn = elf_nextscn(elf, NULL);
        while (scn != NULL) {
                if (gelf_getshdr(scn, &shdr) == NULL) goto err_exit;
                if (shdr.sh_type == SHT_DYNSYM) break;
                scn = elf_nextscn(elf, scn);
        }
        if (scn == NULL) goto err_exit;

        /* Search for the requested symbol by name and offset */
        data = elf_getdata(scn, NULL);
        count = shdr.sh_size / shdr.sh_entsize;
        for (i = 0; i < count; i++) {
                gelf_getsym(data, i, &sym);
                if ((strcmp(dlip->dli_sname,
                        elf_strptr(elf, shdr.sh_link, sym.st_name)) == 0) &&
                    (sym.st_value == (dlip->dli_saddr - dlip->dli_fbase))) {
                        break;
                }
        }

        /* Close ELF file */
        elf_end(elf);
        close(fd);

        /* Return symbol entry in native format */
        desym.st_name = sym.st_name;
        desym.st_info = sym.st_info;
        desym.st_other = sym.st_other;
        desym.st_shndx = sym.st_shndx;
        desym.st_value = sym.st_value;
        desym.st_size = sym.st_size;
        *info = &desym;
        return 1;

        /* Error handler */
err_exit:
        if (elf != NULL) elf_end(elf);
        if (fd >= 0) close(fd);
        return 0;
}
#endif

static void
vboxPatchMesaExport(const char* psFuncName, const void *pStart, const void *pEnd)
{
    Dl_info dlip;
    DRI_ELFSYM* sym=0;
    int rv;
    void *alPatch;
    void *pMesaEntry;
    char patch[FAKEDRI_JMP64_PATCH_SIZE];
    void *shift;
    int ignore_size=false;

#ifndef VBOX_NO_MESA_PATCH_REPORTS
    crDebug("\nvboxPatchMesaExport: %s", psFuncName);
#endif

    pMesaEntry = dlsym(RTLD_DEFAULT, psFuncName);

    if (!pMesaEntry)
    {
        crDebug("%s not defined in current scope, are we being loaded by mesa's libGL.so?", psFuncName);
        return;
    }

    rv = dladdr1(pMesaEntry, &dlip, (void**)&sym, RTLD_DL_SYMENT);
    if (!rv || !sym)
    {
        crError("Failed to get size for %p(%s)", pMesaEntry, psFuncName);
        return;
    }

#if VBOX_OGL_GLX_USE_CSTUBS
    {
        Dl_info dlip1;
        DRI_ELFSYM* sym1=0;
        int rv;

        rv = dladdr1(pStart, &dlip1, (void**)&sym1, RTLD_DL_SYMENT);
        if (!rv || !sym1)
        {
            crError("Failed to get size for vbox %p", pStart);
            return;
        }

        pEnd = pStart + sym1->st_size;
# ifndef VBOX_NO_MESA_PATCH_REPORTS
        crDebug("VBox Entry: %p, start: %p(%s:%s), size: %li", pStart, dlip1.dli_saddr, dlip1.dli_fname, dlip1.dli_sname, sym1->st_size);
# endif
    }
#endif

#ifndef VBOX_NO_MESA_PATCH_REPORTS
    crDebug("Mesa Entry: %p, start: %p(%s:%s), size: %li", pMesaEntry, dlip.dli_saddr, dlip.dli_fname, dlip.dli_sname, sym->st_size);
    crDebug("VBox code: start: %p, end %p, size: %li", pStart, pEnd, pEnd-pStart);
#endif

#ifndef VBOX_OGL_GLX_USE_CSTUBS
    if (sym->st_size<(pEnd-pStart))
#endif
    {
#ifdef RT_ARCH_AMD64
        int64_t offset;
#endif
        /* Try to insert 5 bytes jmp/jmpq to our stub code */

        if (sym->st_size<5)
        {
            /** @todo we don't really know the size of targeted static function, but it's long enough in practice. We will also patch same place twice, but it's ok.*/
            if (!crStrcmp(psFuncName, "glXDestroyContext") || !crStrcmp(psFuncName, "glXFreeContextEXT"))
            {
                if (((unsigned char*)dlip.dli_saddr)[0]==0xEB)
                {
                    /*it's a rel8 jmp, so we're going to patch the place it targets instead of jmp itself*/
                    dlip.dli_saddr = (void*) ((intptr_t)dlip.dli_saddr + ((char*)dlip.dli_saddr)[1] + 2);
                    ignore_size = true;
                }
                else
                {
                    crError("Can't patch size is too small.(%s)", psFuncName);
                    return;
                }
            }
            else if (!crStrcmp(psFuncName, "glXCreateGLXPixmapMESA"))
            {
                /** @todo it's just a return 0, which we're fine with for now*/
                return;
            }
            else
            {
                crError("Can't patch size is too small.(%s)", psFuncName);
                return;
            }
        }

        shift = (void*)((intptr_t)pStart-((intptr_t)dlip.dli_saddr+5));
#ifdef RT_ARCH_AMD64
        offset = (intptr_t)shift;
        if (offset>INT32_MAX || offset<INT32_MIN)
        {
            /*try to insert 64bit abs jmp*/
            if (sym->st_size>=FAKEDRI_JMP64_PATCH_SIZE || ignore_size)
            {
# ifndef VBOX_NO_MESA_PATCH_REPORTS
                crDebug("Inserting movq/jmp instead");
# endif
                /*add 64bit abs jmp*/
                patch[0] = 0x49; /*movq %r11,imm64*/
                patch[1] = 0xBB;
                crMemcpy(&patch[2], &pStart, 8);
                patch[10] = 0x41; /*jmp *%r11*/
                patch[11] = 0xFF;
                patch[12] = 0xE3;
                pStart = &patch[0];
                pEnd = &patch[FAKEDRI_JMP64_PATCH_SIZE];
            }
            else
            {
                FAKEDRI_PatchNode *pNode;
# ifndef VBOX_NO_MESA_PATCH_REPORTS
                crDebug("Can't patch offset is too big. Pushing for 2nd pass(%s)", psFuncName);
# endif
                /*Add patch node to repatch with chain jmps in 2nd pass*/
                pNode = (FAKEDRI_PatchNode *)crAlloc(sizeof(FAKEDRI_PatchNode));
                if (!pNode)
                {
                    crError("Not enough memory.");
                    return;
                }
                pNode->psFuncName = psFuncName;
                pNode->pDstStart = dlip.dli_saddr;
                pNode->pDstEnd = dlip.dli_saddr+sym->st_size;
                pNode->pSrcStart = pStart;
                pNode->pSrcEnd = pEnd;
                pNode->pNext = g_pRepatchList;
                g_pRepatchList = pNode;
                return;
            }
        }
        else
#endif
        {
#ifndef VBOX_NO_MESA_PATCH_REPORTS
            crDebug("Inserting jmp[q] with shift %p instead", shift);
#endif
            patch[0] = 0xE9;
            crMemcpy(&patch[1], &shift, 4);
            pStart = &patch[0];
            pEnd = &patch[5];
        }
    }

    vboxApplyPatch(psFuncName, dlip.dli_saddr, pStart, pEnd-pStart);

#ifdef RT_ARCH_AMD64
    /*Add rest of mesa function body to free list*/
    if (sym->st_size-(pEnd-pStart)>=FAKEDRI_JMP64_PATCH_SIZE)
    {
        FAKEDRI_PatchNode *pNode = (FAKEDRI_PatchNode *)crAlloc(sizeof(FAKEDRI_PatchNode));
        if (pNode)
        {
                pNode->psFuncName = psFuncName;
                pNode->pDstStart = dlip.dli_saddr+(pEnd-pStart);
                pNode->pDstEnd = dlip.dli_saddr+sym->st_size;
                pNode->pSrcStart = dlip.dli_saddr;
                pNode->pSrcEnd = NULL;
                pNode->pNext = g_pFreeList;
                g_pFreeList = pNode;
# ifndef VBOX_NO_MESA_PATCH_REPORTS
                crDebug("Added free node %s, func start=%p, free start=%p, size=%#lx",
                        psFuncName, pNode->pSrcStart, pNode->pDstStart, pNode->pDstEnd-pNode->pDstStart);
# endif
        }
    }
#endif
}

#ifdef RT_ARCH_AMD64
static void
vboxRepatchMesaExports(void)
{
    FAKEDRI_PatchNode *pFreeNode, *pPatchNode;
    int64_t offset;
    char patch[FAKEDRI_JMP64_PATCH_SIZE];

    pPatchNode = g_pRepatchList;
    while (pPatchNode)
    {
# ifndef VBOX_NO_MESA_PATCH_REPORTS
        crDebug("\nvboxRepatchMesaExports %s", pPatchNode->psFuncName);
# endif
        /*find free place in mesa functions, to place 64bit jump to our stub code*/
        pFreeNode = g_pFreeList;
        while (pFreeNode)
        {
            if (pFreeNode->pDstEnd-pFreeNode->pDstStart>=FAKEDRI_JMP64_PATCH_SIZE)
            {
                offset = ((intptr_t)pFreeNode->pDstStart-((intptr_t)pPatchNode->pDstStart+5));
                if (offset<=INT32_MAX && offset>=INT32_MIN)
                {
                    break;
                }
            }
            pFreeNode=pFreeNode->pNext;
        }

        if (!pFreeNode)
        {
            crError("Failed to find free space, to place repatch for %s.", pPatchNode->psFuncName);
            return;
        }

        /*add 32bit rel jmp, from mesa orginal function to free space in other mesa function*/
        patch[0] = 0xE9;
        crMemcpy(&patch[1], &offset, 4);
# ifndef VBOX_NO_MESA_PATCH_REPORTS
        crDebug("Adding jmp from mesa %s to mesa %s+%#lx", pPatchNode->psFuncName, pFreeNode->psFuncName,
                pFreeNode->pDstStart-pFreeNode->pSrcStart);
# endif
        vboxApplyPatch(pPatchNode->psFuncName, pPatchNode->pDstStart, &patch[0], 5);

        /*add 64bit abs jmp, from free space to our stub code*/
        patch[0] = 0x49; /*movq %r11,imm64*/
        patch[1] = 0xBB;
        crMemcpy(&patch[2], &pPatchNode->pSrcStart, 8);
        patch[10] = 0x41; /*jmp *%r11*/
        patch[11] = 0xFF;
        patch[12] = 0xE3;
# ifndef VBOX_NO_MESA_PATCH_REPORTS
        crDebug("Adding jmp from mesa %s+%#lx to vbox %s", pFreeNode->psFuncName, pFreeNode->pDstStart-pFreeNode->pSrcStart,
                pPatchNode->psFuncName);
# endif
        vboxApplyPatch(pFreeNode->psFuncName, pFreeNode->pDstStart, &patch[0], FAKEDRI_JMP64_PATCH_SIZE);
        /*mark this space as used*/
        pFreeNode->pDstStart = pFreeNode->pDstStart+FAKEDRI_JMP64_PATCH_SIZE;

        pPatchNode = pPatchNode->pNext;
    }
}

static void
vboxFakeDriFreeList(FAKEDRI_PatchNode *pList)
{
    FAKEDRI_PatchNode *pNode;

    while (pList)
    {
        pNode=pList;
        pList=pNode->pNext;
        crFree(pNode);
    }
}
#endif

#ifdef VBOX_OGL_GLX_USE_CSTUBS
static void
# define GLXAPI_ENTRY(Func) vboxPatchMesaExport("glX"#Func, &vbox_glX##Func, NULL);
vboxPatchMesaExports()
#else
static void
# define GLXAPI_ENTRY(Func) vboxPatchMesaExport("glX"#Func, &vbox_glX##Func, &vbox_glX##Func##_EndProc);
vboxPatchMesaExports()
#endif
{
    crDebug("Patching mesa glx entries");
    #include "fakedri_glxfuncsList.h"

#ifdef RT_ARCH_AMD64
    vboxRepatchMesaExports();
    vboxFakeDriFreeList(g_pRepatchList);
    g_pRepatchList = NULL;
    vboxFakeDriFreeList(g_pFreeList);
    g_pFreeList = NULL;
#endif
}
#undef GLXAPI_ENTRY

bool vbox_load_sw_dri()
{
    const char *libPaths, *p, *next;;
    char realDriverName[200];
    void *handle;
    int len, i;

    /*code from Mesa-7.2/src/glx/x11/dri_common.c:driOpenDriver*/

    libPaths = NULL;
    if (geteuid() == getuid()) {
        /* don't allow setuid apps to use LIBGL_DRIVERS_PATH */
        libPaths = getenv("LIBGL_DRIVERS_PATH");
        if (!libPaths)
            libPaths = getenv("LIBGL_DRIVERS_DIR"); /* deprecated */
    }
    if (libPaths == NULL)
        libPaths = DRI_DEFAULT_DRIVER_DIR;

    handle = NULL;
    for (p = libPaths; *p; p = next)
    {
        next = strchr(p, ':');
        if (next == NULL)
        {
            len = strlen(p);
            next = p + len;
        }
        else
        {
            len = next - p;
            next++;
        }

        snprintf(realDriverName, sizeof realDriverName, "%.*s/%s_dri.so", len, p, "swrast");
        crDebug("trying %s", realDriverName);
        handle = dlopen(realDriverName, RTLD_NOW | RTLD_LOCAL);
        if (handle) break;
    }

    /*end code*/

    if (handle) gppSwDriExternsion = dlsym(handle, "__driDriverExtensions");

    if (!gppSwDriExternsion)
    {
        crDebug("%s doesn't export __driDriverExtensions", realDriverName);
        return false;
    }
    crDebug("loaded %s", realDriverName);

    for (i = 0; gppSwDriExternsion[i]; i++)
    {
        if (strcmp(gppSwDriExternsion[i]->name, __DRI_CORE) == 0)
            gpSwDriCoreExternsion = (__DRIcoreExtension *) gppSwDriExternsion[i];
        if (strcmp(gppSwDriExternsion[i]->name, __DRI_SWRAST) == 0)
            gpSwDriSwrastExtension = (__DRIswrastExtension *) gppSwDriExternsion[i];
    }

    return gpSwDriCoreExternsion && gpSwDriSwrastExtension;
}

void __attribute__ ((constructor)) vbox_install_into_mesa(void)
{
    {
#ifdef _X_ATTRIBUTE_PRINTF
        void (*pxf86Msg)(MessageType type, const char *format, ...) _X_ATTRIBUTE_PRINTF(2,3);
#else
        void (*pxf86Msg)(MessageType type, const char *format, ...) _printf_attribute(2,3);
#endif

        pxf86Msg = dlsym(RTLD_DEFAULT, "xf86Msg");
        if (pxf86Msg)
        {
            pxf86Msg(X_INFO, "Next line is added to allow vboxvideo_drv.so to appear as whitelisted driver\n");
            pxf86Msg(X_INFO, "The file referenced, is *NOT* loaded\n");
            pxf86Msg(X_INFO, "Loading %s/ati_drv.so\n", DRI_XORG_DRV_DIR);

            /* we're failing to proxy software dri driver calls for certain xservers, so just make sure we're unloaded for now */
            __driDriverExtensions[0] = NULL;
            return;
        }
    }

    if (!stubInit())
    {
        crDebug("vboxdriInitScreen: stubInit failed");
        return;
    }

    /* Load swrast_dri.so to proxy dri related calls there. */
    if (!vbox_load_sw_dri())
    {
        crDebug("vboxdriInitScreen: vbox_load_sw_dri failed...going to fail badly");
        return;
    }

    /* Handle gl api.
     * In the end application call would look like this:
     * app call glFoo->(mesa asm dispatch stub)->cr_glFoo(vbox asm dispatch stub)->SPU Foo function(packspuFoo or alike)
     * Note, we don't need to install extension functions via _glapi_add_dispatch, because we'd override glXGetProcAddress.
     */
    /* Mesa's dispatch table is different across library versions, have to modify mesa's table using offset info functions*/
    vboxPatchMesaGLAPITable();

    /* Handle glx api.
     * In the end application call would look like this:
     * app call glxFoo->(mesa asm dispatch stub patched with vbox_glXFoo:jmp glxim[Foo's index])->VBOXGLXTAG(glxFoo)
     */
    /* Fill structure used by our assembly stubs */
    vboxFillGLXAPITable(&glxim);
    /* Now patch functions exported by libGL.so */
    vboxPatchMesaExports();
}

/*
 * @todo we're missing first glx related call from the client application.
 * Luckily, this doesn't add much problems, except for some cases.
 */

/* __DRIcoreExtension */

static __DRIscreen *
vboxdriCreateNewScreen(int screen, int fd, unsigned int sarea_handle,
                       const __DRIextension **extensions, const __DRIconfig ***driverConfigs,
                       void *loaderPrivate)
{
    (void) fd;
    (void) sarea_handle;
    SWDRI_SAFERET_SWRAST(createNewScreen, screen, extensions, driverConfigs, loaderPrivate);
}

static void
vboxdriDestroyScreen(__DRIscreen *screen)
{
    SWDRI_SAFECALL_CORE(destroyScreen, screen);
}

static const __DRIextension **
vboxdriGetExtensions(__DRIscreen *screen)
{
    SWDRI_SAFERET_CORE(getExtensions, screen);
}

static int
vboxdriGetConfigAttrib(const __DRIconfig *config,
                       unsigned int attrib,
                       unsigned int *value)
{
    SWDRI_SAFERET_CORE(getConfigAttrib, config, attrib, value);
}

static int
vboxdriIndexConfigAttrib(const __DRIconfig *config, int index,
                         unsigned int *attrib, unsigned int *value)
{
    SWDRI_SAFERET_CORE(indexConfigAttrib, config, index, attrib, value);
}

static __DRIdrawable *
vboxdriCreateNewDrawable(__DRIscreen *screen,
                         const __DRIconfig *config,
                         unsigned int drawable_id,
                         unsigned int head,
                         void *loaderPrivate)
{
    (void) drawable_id;
    (void) head;
    SWDRI_SAFERET_SWRAST(createNewDrawable, screen, config, loaderPrivate);
}

static void
vboxdriDestroyDrawable(__DRIdrawable *drawable)
{
    SWDRI_SAFECALL_CORE(destroyDrawable, drawable);
}

static void
vboxdriSwapBuffers(__DRIdrawable *drawable)
{
    SWDRI_SAFECALL_CORE(swapBuffers, drawable);
}

static __DRIcontext *
vboxdriCreateNewContext(__DRIscreen *screen,
                        const __DRIconfig *config,
                        __DRIcontext *shared,
                        void *loaderPrivate)
{
    SWDRI_SAFERET_CORE(createNewContext, screen, config, shared, loaderPrivate);
}

static int
vboxdriCopyContext(__DRIcontext *dest,
                   __DRIcontext *src,
                   unsigned long mask)
{
    SWDRI_SAFERET_CORE(copyContext, dest, src, mask);
}

static void
vboxdriDestroyContext(__DRIcontext *context)
{
    SWDRI_SAFECALL_CORE(destroyContext, context);
}

static int
vboxdriBindContext(__DRIcontext *ctx,
                   __DRIdrawable *pdraw,
                   __DRIdrawable *pread)
{
    SWDRI_SAFERET_CORE(bindContext, ctx, pdraw, pread);
}

static int
vboxdriUnbindContext(__DRIcontext *ctx)
{
    SWDRI_SAFERET_CORE(unbindContext, ctx)
}

/* __DRIlegacyExtension */

static __DRIscreen *
vboxdriCreateNewScreen_Legacy(int scrn,
                              const __DRIversion *ddx_version,
                              const __DRIversion *dri_version,
                              const __DRIversion *drm_version,
                              const __DRIframebuffer *frame_buffer,
                              drmAddress pSAREA, int fd,
                              const __DRIextension **extensions,
                              const __DRIconfig ***driver_modes,
                              void *loaderPrivate)
{
    (void) ddx_version;
    (void) dri_version;
    (void) frame_buffer;
    (void) pSAREA;
    (void) fd;
    SWDRI_SAFERET_SWRAST(createNewScreen, scrn, extensions, driver_modes, loaderPrivate);
}

static __DRIdrawable *
vboxdriCreateNewDrawable_Legacy(__DRIscreen *psp, const __DRIconfig *config,
                                drm_drawable_t hwDrawable, int renderType,
                                const int *attrs, void *data)
{
    (void) hwDrawable;
    (void) renderType;
    (void) attrs;
    (void) data;
    SWDRI_SAFERET_SWRAST(createNewDrawable, psp, config, data);
}

static __DRIcontext *
vboxdriCreateNewContext_Legacy(__DRIscreen *psp, const __DRIconfig *config,
                               int render_type, __DRIcontext *shared,
                               drm_context_t hwContext, void *data)
{
    (void) render_type;
    (void) hwContext;
    return vboxdriCreateNewContext(psp, config, shared, data);
}


static const __DRIlegacyExtension vboxdriLegacyExtension = {
    { __DRI_LEGACY, __DRI_LEGACY_VERSION },
    vboxdriCreateNewScreen_Legacy,
    vboxdriCreateNewDrawable_Legacy,
    vboxdriCreateNewContext_Legacy
};

static const __DRIcoreExtension vboxdriCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    vboxdriCreateNewScreen, /* driCreateNewScreen */
    vboxdriDestroyScreen,
    vboxdriGetExtensions,
    vboxdriGetConfigAttrib,
    vboxdriIndexConfigAttrib,
    vboxdriCreateNewDrawable, /* driCreateNewDrawable */
    vboxdriDestroyDrawable,
    vboxdriSwapBuffers,
    vboxdriCreateNewContext,
    vboxdriCopyContext,
    vboxdriDestroyContext,
    vboxdriBindContext,
    vboxdriUnbindContext
};

/* This structure is used by dri_util from mesa, don't rename it! */
DECLEXPORT(const __DRIextension *) __driDriverExtensions[] = {
    &vboxdriLegacyExtension.base,
    &vboxdriCoreExtension.base,
    NULL
};
