/* $Id: cr_dump.h $ */

/** @file
 * Debugging: Dump API
 */
/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___cr_dump_h__
#define ___cr_dump_h__

/* dump stuff */
//#define VBOX_WITH_CRDUMPER
#ifdef VBOX_WITH_CRDUMPER

#include <iprt/cdefs.h>
#include <iprt/string.h>
#include <cr_spu.h>
#include <cr_glstate.h>
#include <cr_blitter.h>

# define VBOXDUMPDECL(_type) _type RTCALL

RT_C_DECLS_BEGIN

#ifdef RT_OS_WINDOWS
DECLEXPORT(void) crDmpDumpImgDmlBreak(struct CR_DUMPER * pDumper, CR_BLITTER_IMG *pImg, const char*pszEntryDesc);

DECLEXPORT(void) crDmpDumpStrDbgPrint(struct CR_DUMPER * pDumper, const char*pszStr);
#endif

struct CR_DUMPER;

typedef DECLCALLBACKPTR(void, PFNCRDUMPIMG)(struct CR_DUMPER * pDumper, CR_BLITTER_IMG *pImg, const char*pszEntryDesc);
typedef DECLCALLBACKPTR(void, PFNCRDUMPSTR)(struct CR_DUMPER * pDumper, const char*pszStr);

typedef struct CR_DUMPER
{
    PFNCRDUMPIMG pfnDumpImg;
    PFNCRDUMPSTR pfnDumpStr;
} CR_DUMPER;

#define crDmpImg(_pDumper, _pImg, _pDesc) do { \
            (_pDumper)->pfnDumpImg((_pDumper), (_pImg), (_pDesc)); \
        } while (0)

#define crDmpStr(_pDumper, _pDesc) do { \
            (_pDumper)->pfnDumpStr((_pDumper), (_pDesc)); \
        } while (0)

#ifndef RT_OS_WINDOWS
# define vsprintf_s vsnprintf
# define sprintf_s snprintf
#endif

DECLINLINE(void) crDmpStrV(CR_DUMPER *pDumper, const char *pszStr, va_list pArgList)
{
    char szBuffer[4096] = {0};
    vsprintf_s(szBuffer, sizeof (szBuffer), pszStr, pArgList);
    crDmpStr(pDumper, szBuffer);
}

DECLINLINE(void) crDmpStrF(CR_DUMPER *pDumper, const char *pszStr, ...)
{
    va_list pArgList;
    va_start(pArgList, pszStr);
    crDmpStrV(pDumper, pszStr, pArgList);
    va_end(pArgList);
}

DECLINLINE(void) crDmpImgV(CR_DUMPER *pDumper, CR_BLITTER_IMG *pImg, const char *pszStr, va_list pArgList)
{
    char szBuffer[4096] = {0};
    vsprintf_s(szBuffer, sizeof (szBuffer), pszStr, pArgList);
    crDmpImg(pDumper, pImg, szBuffer);
}

DECLINLINE(void) crDmpImgF(CR_DUMPER *pDumper, CR_BLITTER_IMG *pImg, const char *pszStr, ...)
{
    va_list pArgList;
    va_start(pArgList, pszStr);
    crDmpImgV(pDumper, pImg, pszStr, pArgList);
    va_end(pArgList);
}

VBOXDUMPDECL(size_t) crDmpFormatArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pVal, uint32_t cVal);
VBOXDUMPDECL(size_t) crDmpFormatRawArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pVal, uint32_t cVal);
VBOXDUMPDECL(size_t) crDmpFormatMatrixArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pVal, uint32_t cX, uint32_t cY);

typedef struct CR_DBGPRINT_DUMPER
{
    CR_DUMPER Base;
} CR_DBGPRINT_DUMPER;

typedef struct CR_HTML_DUMPER
{
    CR_DUMPER Base;
    const char* pszFile;
    const char* pszDir;
    FILE *pFile;
    uint32_t cImg;
} CR_HTML_DUMPER;

DECLEXPORT(bool) crDmpHtmlIsInited(struct CR_HTML_DUMPER * pDumper);
DECLEXPORT(void) crDmpHtmlTerm(struct CR_HTML_DUMPER * pDumper);
DECLEXPORT(int) crDmpHtmlInit(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile);
DECLEXPORT(int) crDmpHtmlInitV(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile, va_list pArgList);
DECLEXPORT(int) crDmpHtmlInitF(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile, ...);

#ifdef RT_OS_WINDOWS
DECLINLINE(void) crDmpDbgPrintInit(CR_DBGPRINT_DUMPER *pDumper)
{
    pDumper->Base.pfnDumpImg = crDmpDumpImgDmlBreak;
    pDumper->Base.pfnDumpStr = crDmpDumpStrDbgPrint;
}
#endif

typedef struct CR_RECORDER
{
    CR_BLITTER *pBlitter;
    SPUDispatchTable *pDispatch;
    CR_DUMPER *pDumper;
} CR_RECORDER;

DECLINLINE(void) crRecInit(CR_RECORDER *pRec, CR_BLITTER *pBlitter, SPUDispatchTable *pDispatch, CR_DUMPER *pDumper)
{
    pRec->pBlitter = pBlitter;
    pRec->pDispatch = pDispatch;
    pRec->pDumper = pDumper;
}

VBOXDUMPDECL(void) crRecDumpBuffer(CR_RECORDER *pRec, CRContext *ctx, GLint idRedirFBO, VBOXVR_TEXTURE *pRedirTex);
VBOXDUMPDECL(void) crRecDumpTextures(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpTextureV(CR_RECORDER *pRec, const VBOXVR_TEXTURE *pTex, CR_BLITTER_CONTEXT *pCurCtx, CR_BLITTER_WINDOW *pCurWin, const char *pszStr, va_list pArgList);
VBOXDUMPDECL(void) crRecDumpTextureF(CR_RECORDER *pRec, const VBOXVR_TEXTURE *pTex, const char *pszStr, ...);
VBOXDUMPDECL(void) crRecDumpTextureByIdV(CR_RECORDER *pRec, CRContext *ctx, GLint id, CR_BLITTER_CONTEXT *pCurCtx, CR_BLITTER_WINDOW *pCurWin, const char *pszStr, va_list pArgList);
VBOXDUMPDECL(void) crRecDumpTextureByIdF(CR_RECORDER *pRec, CRContext *ctx, GLint id, CR_BLITTER_CONTEXT *pCurCtx, CR_BLITTER_WINDOW *pCurWin, const char *pszStr, ...);
VBOXDUMPDECL(void) crRecDumpShader(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecDumpProgram(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecRecompileShader(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecRecompileProgram(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecDumpCurrentProgram(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpProgramUniforms(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecDumpCurrentProgramUniforms(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecRecompileCurrentProgram(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpProgramAttribs(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid);
VBOXDUMPDECL(void) crRecDumpCurrentProgramAttribs(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpGlGetState(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpGlEnableState(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpVertAttrv(CR_RECORDER *pRec, CRContext *ctx, GLuint idx, const char*pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cVal);
VBOXDUMPDECL(void) crRecDumpVertAttrF(CR_RECORDER *pRec, CRContext *ctx, const char*pszFormat, ...);
VBOXDUMPDECL(void) crRecDumpVertAttrV(CR_RECORDER *pRec, CRContext *ctx, const char*pszFormat, va_list pArgList);
VBOXDUMPDECL(void) crRecDumpTexParam(CR_RECORDER *pRec, CRContext *ctx, GLenum enmTarget);
VBOXDUMPDECL(void) crRecDumpTexEnv(CR_RECORDER *pRec, CRContext *ctx);
VBOXDUMPDECL(void) crRecDumpTexGen(CR_RECORDER *pRec, CRContext *ctx);

VBOXDUMPDECL(int) crRecAlphaImgCreate(const CR_BLITTER_IMG *pImg, CR_BLITTER_IMG *pAlphaImg);
VBOXDUMPDECL(void) crRecAlphaImgDestroy(CR_BLITTER_IMG *pImg);


typedef DECLCALLBACKPTR(GLuint, PFNCRDUMPGETHWID)(void *pvObj);
void* crDmpHashtableSearchByHwid(CRHashTable *pHash, GLuint hwid, PFNCRDUMPGETHWID pfnGetHwid, unsigned long *pKey);

RT_C_DECLS_END

#endif /* VBOX_WITH_CRDUMPER */
/* */

#endif /* #ifndef ___cr_dump_h__ */
